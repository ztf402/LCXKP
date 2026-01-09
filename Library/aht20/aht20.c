#include "aht20.h"

/* --- 内部辅助函数：CRC8 校验 --- */
// AHT20 使用 CRC-8-MAXIM: x^8 + x^5 + x^4 + 1
static uint8_t AHT20_CalcCRC8(uint8_t *data, uint8_t len) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/**
 * @brief  初始化 AHT20
 * @note   检查校准状态，如果未校准则发送初始化命令
 */
HAL_StatusTypeDef AHT20_Init(AHT20_HandleTypeDef *hdev, I2C_HandleTypeDef *hi2c) {
    hdev->hi2c = hi2c;
    hdev->Temperature = 0.0f;
    hdev->Humidity = 0.0f;
    uint8_t status = 0;

    // 上电后需等待 40ms (通常由主程序延时，这里加一个小延时确保安全)
    HAL_Delay(40);

    // 读取状态字
    if (HAL_I2C_Master_Receive(hdev->hi2c, AHT20_ADDRESS, &status, 1, 100) != HAL_OK) {
        return HAL_ERROR;
    }

    // 检查 Bit[3] 是否为 1 (已校准)
    if ((status & AHT20_STATUS_CALI) == 0) {
        // 发送初始化命令 0xBE, 0x08, 0x00
        uint8_t init_cmd[3] = {AHT20_CMD_INIT, 0x08, 0x00};
        if (HAL_I2C_Master_Transmit(hdev->hi2c, AHT20_ADDRESS, init_cmd, 3, 100) != HAL_OK) {
            return HAL_ERROR;
        }
        HAL_Delay(10); // 等待初始化完成
    }

    hdev->Initialized = 1;
    return HAL_OK;
}

/**
 * @brief  触发一次测量
 * @note   发送触发命令后，需等待至少 80ms 才能读取数据
 */
HAL_StatusTypeDef AHT20_TriggerMeasurement(AHT20_HandleTypeDef *hdev) {
    // 触发命令: 0xAC, 0x33, 0x00
    uint8_t cmd[3] = {AHT20_CMD_TRIGGER, 0x33, 0x00};
    return HAL_I2C_Master_Transmit(hdev->hi2c, AHT20_ADDRESS, cmd, 3, 100);
}

/**
 * @brief  获取测量数据 (解析与计算)
 * @note   应在 Trigger 后 >80ms 调用
 */
HAL_StatusTypeDef AHT20_GetData(AHT20_HandleTypeDef *hdev) {
    uint8_t buffer[7]; // Status + 5 Data + CRC
    
    // 读取 7 字节
    if (HAL_I2C_Master_Receive(hdev->hi2c, AHT20_ADDRESS, buffer, 7, 100) != HAL_OK) {
        return HAL_ERROR;
    }

    // 1. 检查 Busy 位 (Bit 7)
    if ((buffer[0] & AHT20_STATUS_BUSY) != 0) {
        return HAL_BUSY; // 还在转换中
    }

    // 2. CRC 校验 (计算前6个字节的CRC，对比第7个字节)
    if (AHT20_CalcCRC8(buffer, 6) != buffer[6]) {
        return HAL_ERROR; // 数据校验失败
    }

    // 3. 数据解析
    // 湿度: 20bit = Byte1 << 12 | Byte2 << 4 | Byte3 >> 4
    uint32_t raw_hum = ((uint32_t)buffer[1] << 12) | ((uint32_t)buffer[2] << 4) | ((uint32_t)buffer[3] >> 4);
    
    // 温度: 20bit = (Byte3 & 0x0F) << 16 | Byte4 << 8 | Byte5
    uint32_t raw_temp = (((uint32_t)buffer[3] & 0x0F) << 16) | ((uint32_t)buffer[4] << 8) | (uint32_t)buffer[5];

    // 4. 转换公式
    // RH(%) = (raw / 2^20) * 100
    hdev->Humidity = (float)raw_hum * 100.0f / 1048576.0f;
    
    // T(C) = (raw / 2^20) * 200 - 50
    hdev->Temperature = (float)raw_temp * 200.0f / 1048576.0f - 50.0f;

    return HAL_OK;
}

/**
 * @brief  阻塞式读取一次数据 (集成了触发和延时)
 */
HAL_StatusTypeDef AHT20_ReadNow(AHT20_HandleTypeDef *hdev) {
    HAL_StatusTypeDef status;
    
    status = AHT20_TriggerMeasurement(hdev);
    if (status != HAL_OK) return status;

    // 官方建议等待 80ms，这里给 85ms 留余量
    HAL_Delay(85);

    return AHT20_GetData(hdev);
}

/**
 * @brief  软件复位
 */
HAL_StatusTypeDef AHT20_SoftReset(AHT20_HandleTypeDef *hdev) {
    uint8_t cmd = AHT20_CMD_SOFT_RESET;
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(hdev->hi2c, AHT20_ADDRESS, &cmd, 1, 100);
    HAL_Delay(20); // 复位后等待
    return status;
}
