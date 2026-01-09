#include "icm42688.h"
#include <string.h> // for memset

// ================= 内部静态辅助函数 (底层接口) =================

static void ICM_CS_Low(ICM42688_t *dev) {
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_RESET);
}

static void ICM_CS_High(ICM42688_t *dev) {
    HAL_GPIO_WritePin(dev->cs_port, dev->cs_pin, GPIO_PIN_SET);
}

// SPI 写寄存器
static int8_t ICM_WriteReg(ICM42688_t *dev, uint8_t reg, uint8_t value) {
    uint8_t data[2];
    data[0] = reg & 0x7F; // 写操作，最高位为0
    data[1] = value;

    ICM_CS_Low(dev);
    HAL_StatusTypeDef status = HAL_SPI_Transmit(dev->hspi, data, 2, 10);
    ICM_CS_High(dev);

    return (status == HAL_OK) ? 0 : -1;
}

// SPI 读寄存器
static int8_t ICM_ReadRegs(ICM42688_t *dev, uint8_t reg, uint8_t *buffer, uint16_t len) {
    uint8_t tx_data = reg | 0x80; // 读操作，最高位为1
    HAL_StatusTypeDef status;

    ICM_CS_Low(dev);
    // 发送寄存器地址
    status = HAL_SPI_Transmit(dev->hspi, &tx_data, 1, 10);
    if (status == HAL_OK) {
        // 接收数据
        status = HAL_SPI_Receive(dev->hspi, buffer, len, 10);
    }
    ICM_CS_High(dev);

    return (status == HAL_OK) ? 0 : -1;
}

// 切换寄存器 Bank
static int8_t ICM_SetBank(ICM42688_t *dev, uint8_t bank) {
    return ICM_WriteReg(dev, ICM42688_REG_BANK_SEL, bank);
}

// ================= 外部接口实现 =================

/**
 * @brief 初始化 ICM42688 设备
 */
int8_t ICM42688_Init(ICM42688_t *dev, ICM_SPI_Handle hspi, ICM_GPIO_Port cs_port, ICM_GPIO_Pin cs_pin) {
    if (dev == NULL || hspi == NULL) return -1;

    // 1. 绑定句柄
    dev->hspi = hspi;
    dev->cs_port = cs_port;
    dev->cs_pin = cs_pin;
    dev->initialized = 0;
    
    // 初始化 CS 引脚状态
    ICM_CS_High(dev);
    HAL_Delay(10); // 上电等待

    // 2. 软件复位 (Optional, but recommended)
    // 切换到 Bank 0
    ICM_SetBank(dev, 0);
    // 复位设备 (Device Config 寄存器，位 0) - 具体地址视手册，通常在 Bank 0 0x11
    ICM_WriteReg(dev, 0x11, 0x01); 
    HAL_Delay(100); // 等待复位完成

    // 3. 检查 Device ID
    uint8_t who_am_i = 0;
    if (ICM_ReadRegs(dev, ICM42688_WHO_AM_I, &who_am_i, 1) != 0) return -2; // 通信失败

    if (who_am_i != ICM42688_WHO_AM_I_VAL) {
        return -3; // ID 不匹配
    }

    // 4. 电源管理: 开启 Accel 和 Gyro 的 Low Noise Mode
    // PWR_MGMT0: Bit 5: Gyro Mode (11=LN), Bit 4: Accel Mode (11=LN)
    // 0x0F = 0000 1111 => Temp Enabled, RC Osc on, Gyro LN, Accel LN
    // 注意：ICM42688 的 PWR_MGMT0 定义：
    // Bit 3:2 = GYRO_MODE (11: Low Noise)
    // Bit 1:0 = ACCEL_MODE (11: Low Noise)
    // Bit 5 = TEMP_DIS (0: Enable)
    uint8_t pwr_mgmt = 0x0F; // Gyro LN (11) | Accel LN (11)
    ICM_WriteReg(dev, ICM42688_PWR_MGMT0, pwr_mgmt);
    HAL_Delay(45); // 等待电源稳定

    // 5. 设置默认配置
    ICM42688_SetAccelConfig(dev, ICM_ACCEL_16G, ICM_ODR_1kHz);
    ICM42688_SetGyroConfig(dev, ICM_GYRO_2000DPS, ICM_ODR_1kHz);

    dev->initialized = 1;
    return 0;
}

/**
 * @brief 设置加速度计配置
 */
int8_t ICM42688_SetAccelConfig(ICM42688_t *dev, ICM_AccelRange_t range, ICM_ODR_t odr) {
    // ACCEL_CONFIG0:
    // Bit 7:5 = ACCEL_FS_SEL
    // Bit 3:0 = ACCEL_ODR
    uint8_t config = (range << 5) | (odr & 0x0F);
    
    if (ICM_WriteReg(dev, ICM42688_ACCEL_CONFIG0, config) != 0) return -1;

    // 更新换算系数
    switch (range) {
        case ICM_ACCEL_16G: dev->accel_scale = 16.0f / 32768.0f; break;
        case ICM_ACCEL_8G:  dev->accel_scale = 8.0f / 32768.0f; break;
        case ICM_ACCEL_4G:  dev->accel_scale = 4.0f / 32768.0f; break;
        case ICM_ACCEL_2G:  dev->accel_scale = 2.0f / 32768.0f; break;
        default: break;
    }
    return 0;
}

/**
 * @brief 设置陀螺仪配置
 */
int8_t ICM42688_SetGyroConfig(ICM42688_t *dev, ICM_GyroRange_t range, ICM_ODR_t odr) {
    // GYRO_CONFIG0:
    // Bit 7:5 = GYRO_FS_SEL
    // Bit 3:0 = GYRO_ODR
    uint8_t config = (range << 5) | (odr & 0x0F);
    
    if (ICM_WriteReg(dev, ICM42688_GYRO_CONFIG0, config) != 0) return -1;

    // 更新换算系数
    switch (range) {
        case ICM_GYRO_2000DPS: dev->gyro_scale = 2000.0f / 32768.0f; break;
        case ICM_GYRO_1000DPS: dev->gyro_scale = 1000.0f / 32768.0f; break;
        case ICM_GYRO_500DPS:  dev->gyro_scale = 500.0f / 32768.0f; break;
        case ICM_GYRO_250DPS:  dev->gyro_scale = 250.0f / 32768.0f; break;
        case ICM_GYRO_125DPS:  dev->gyro_scale = 125.0f / 32768.0f; break;
        default: break;
    }
    return 0;
}

/**
 * @brief 读取传感器数据 (Burst Read)
 */
int8_t ICM42688_ReadData(ICM42688_t *dev) {
    ICM_SetBank(dev, 0);
    uint8_t buffer[14]; // Temp(2) + Accel(6) + Gyro(6)
    
    // 从 TEMP_DATA1 (0x1D) 开始读取 14 个字节
    // 顺序: TempH, TempL, AccelX_H, AccelX_L ... GyroZ_L
    if (ICM_ReadRegs(dev, ICM42688_TEMP_DATA1, buffer, 14) != 0) return -1;

    // 1. 解析原始数据 (Big Endian)
    dev->raw_data.temp_raw    = (int16_t)((buffer[0] << 8) | buffer[1]);
    dev->raw_data.accel_x_raw = (int16_t)((buffer[2] << 8) | buffer[3]);
    dev->raw_data.accel_y_raw = (int16_t)((buffer[4] << 8) | buffer[5]);
    dev->raw_data.accel_z_raw = (int16_t)((buffer[6] << 8) | buffer[7]);
    dev->raw_data.gyro_x_raw  = (int16_t)((buffer[8] << 8) | buffer[9]);
    dev->raw_data.gyro_y_raw  = (int16_t)((buffer[10] << 8) | buffer[11]);
    dev->raw_data.gyro_z_raw  = (int16_t)((buffer[12] << 8) | buffer[13]);

    // 2. 转换为物理量
    dev->data.accel_x_g = dev->raw_data.accel_x_raw * dev->accel_scale;
    dev->data.accel_y_g = dev->raw_data.accel_y_raw * dev->accel_scale;
    dev->data.accel_z_g = dev->raw_data.accel_z_raw * dev->accel_scale;

    dev->data.gyro_x_dps = dev->raw_data.gyro_x_raw * dev->gyro_scale;
    dev->data.gyro_y_dps = dev->raw_data.gyro_y_raw * dev->gyro_scale;
    dev->data.gyro_z_dps = dev->raw_data.gyro_z_raw * dev->gyro_scale;

    // 温度转换 (公式来自数据手册: Temp_degC = (Temp_raw / 132.48) + 25)
    dev->data.temp_c = (dev->raw_data.temp_raw / 132.48f) + 25.0f;

    return 0;
}
