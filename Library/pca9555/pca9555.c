#include "pca9555.h"

/* 内部辅助函数：写寄存器 */
static HAL_StatusTypeDef PCA9555_WriteReg(PCA9555_HandleTypeDef *hdev, uint8_t reg, uint8_t value) {
    uint8_t data[2];
    data[0] = reg;
    data[1] = value;
    return HAL_I2C_Master_Transmit(hdev->hi2c, hdev->Addr, data, 2, 100);
}

/* 内部辅助函数：读寄存器 */
static HAL_StatusTypeDef PCA9555_ReadReg(PCA9555_HandleTypeDef *hdev, uint8_t reg, uint8_t *pVal) {
    return HAL_I2C_Mem_Read(hdev->hi2c, hdev->Addr, reg, I2C_MEMADD_SIZE_8BIT, pVal, 1, 100);
}

/**
 * @brief  初始化 PCA9555 句柄
 * @param  hdev: PCA9555 对象指针
 * @param  hi2c: STM32 I2C 句柄指针
 * @param  addr: 设备的 I2C 地址 (8位地址，例如 0x40)
 * @retval HAL Status
 */
HAL_StatusTypeDef PCA9555_Init(PCA9555_HandleTypeDef *hdev, I2C_HandleTypeDef *hi2c, uint16_t addr) {
    hdev->hi2c = hi2c;
    hdev->Addr = addr;
    
    // 检查设备是否在线
    if (HAL_I2C_IsDeviceReady(hdev->hi2c, hdev->Addr, 2, 100) != HAL_OK) {
        return HAL_ERROR;
    }

    // 初始化影子寄存器：从设备读取当前的配置
    // 读取 Output Port 0 & 1
    if (PCA9555_ReadReg(hdev, PCA9555_REG_OUTPUT_PORT0, &hdev->Output_Reg[0]) != HAL_OK) return HAL_ERROR;
    if (PCA9555_ReadReg(hdev, PCA9555_REG_OUTPUT_PORT1, &hdev->Output_Reg[1]) != HAL_OK) return HAL_ERROR;

    // 读取 Config Port 0 & 1
    if (PCA9555_ReadReg(hdev, PCA9555_REG_CONFIG_0, &hdev->Config_Reg[0]) != HAL_OK) return HAL_ERROR;
    if (PCA9555_ReadReg(hdev, PCA9555_REG_CONFIG_1, &hdev->Config_Reg[1]) != HAL_OK) return HAL_ERROR;

    hdev->Initialized = 1;
    return HAL_OK;
}

/**
 * @brief  设置引脚模式 (输入/输出)
 * @param  hdev: PCA9555 对象
 * @param  port: Port 0 或 Port 1
 * @param  pin_mask: 引脚掩码 (如 PCA9555_PIN_0 | PCA9555_PIN_1)
 * @param  mode: PCA9555_MODE_OUTPUT 或 PCA9555_MODE_INPUT
 */
HAL_StatusTypeDef PCA9555_SetMode(PCA9555_HandleTypeDef *hdev, PCA9555_Port_t port, uint16_t pin_mask, PCA9555_Mode_t mode) {
    uint8_t port_idx = (port == PCA9555_PORT_0) ? 0 : 1;
    uint8_t reg_addr = (port == PCA9555_PORT_0) ? PCA9555_REG_CONFIG_0 : PCA9555_REG_CONFIG_1;
    
    if (mode == PCA9555_MODE_INPUT) {
        hdev->Config_Reg[port_idx] |= (uint8_t)pin_mask; // 置1为输入
    } else {
        hdev->Config_Reg[port_idx] &= ~((uint8_t)pin_mask); // 置0为输出
    }
    
    return PCA9555_WriteReg(hdev, reg_addr, hdev->Config_Reg[port_idx]);
}

/**
 * @brief  写引脚电平
 */
HAL_StatusTypeDef PCA9555_WritePin(PCA9555_HandleTypeDef *hdev, PCA9555_Port_t port, uint16_t pin_mask, PCA9555_PinState_t state) {
    uint8_t port_idx = (port == PCA9555_PORT_0) ? 0 : 1;
    uint8_t reg_addr = (port == PCA9555_PORT_0) ? PCA9555_REG_OUTPUT_PORT0 : PCA9555_REG_OUTPUT_PORT1;
    
    if (state == PCA9555_PIN_SET) {
        hdev->Output_Reg[port_idx] |= (uint8_t)pin_mask;
    } else {
        hdev->Output_Reg[port_idx] &= ~((uint8_t)pin_mask);
    }
    
    return PCA9555_WriteReg(hdev, reg_addr, hdev->Output_Reg[port_idx]);
}

/**
 * @brief  翻转引脚电平
 */
HAL_StatusTypeDef PCA9555_TogglePin(PCA9555_HandleTypeDef *hdev, PCA9555_Port_t port, uint16_t pin_mask) {
    uint8_t port_idx = (port == PCA9555_PORT_0) ? 0 : 1;
    uint8_t reg_addr = (port == PCA9555_PORT_0) ? PCA9555_REG_OUTPUT_PORT0 : PCA9555_REG_OUTPUT_PORT1;
    
    // 异或操作翻转位
    hdev->Output_Reg[port_idx] ^= (uint8_t)pin_mask;
    
    return PCA9555_WriteReg(hdev, reg_addr, hdev->Output_Reg[port_idx]);
}

/**
 * @brief  读取引脚电平
 * @note   读取输入寄存器（真实电平），而不是输出寄存器（设定值）
 */
PCA9555_PinState_t PCA9555_ReadPin(PCA9555_HandleTypeDef *hdev, PCA9555_Port_t port, uint16_t pin_mask) {
    uint8_t reg_addr = (port == PCA9555_PORT_0) ? PCA9555_REG_INPUT_PORT0 : PCA9555_REG_INPUT_PORT1;
    uint8_t val = 0;
    
    if (PCA9555_ReadReg(hdev, reg_addr, &val) != HAL_OK) {
        return PCA9555_PIN_RESET; // 错误处理：默认返回低电平
    }
    
    if (val & (uint8_t)pin_mask) {
        return PCA9555_PIN_SET;
    } else {
        return PCA9555_PIN_RESET;
    }
}

/**
 * @brief  写整个端口 (8-bit)
 */
HAL_StatusTypeDef PCA9555_WritePort(PCA9555_HandleTypeDef *hdev, PCA9555_Port_t port, uint8_t value) {
    uint8_t port_idx = (port == PCA9555_PORT_0) ? 0 : 1;
    uint8_t reg_addr = (port == PCA9555_PORT_0) ? PCA9555_REG_OUTPUT_PORT0 : PCA9555_REG_OUTPUT_PORT1;
    
    hdev->Output_Reg[port_idx] = value;
    return PCA9555_WriteReg(hdev, reg_addr, value);
}

/**
 * @brief  读整个端口 (8-bit)
 */
HAL_StatusTypeDef PCA9555_ReadPort(PCA9555_HandleTypeDef *hdev, PCA9555_Port_t port, uint8_t *pData) {
    uint8_t reg_addr = (port == PCA9555_PORT_0) ? PCA9555_REG_INPUT_PORT0 : PCA9555_REG_INPUT_PORT1;
    return PCA9555_ReadReg(hdev, reg_addr, pData);
}
