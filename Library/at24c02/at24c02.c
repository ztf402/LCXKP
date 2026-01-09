/**
 * @file at24c02.c
 * @author ztf402
 * @brief at24c02 eeprom 驱动 注意,一定要对齐8bit
 * @version 0.1
 * @date 2026-01-06
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "at24c02.h"
#include <string.h> // for memset

/**
 * @brief  初始化 AT24C02
 * @param  addr: 通常是 0xA0 (如果 A0/A1/A2 接地)
 */
HAL_StatusTypeDef AT24C02_Init(AT24C02_HandleTypeDef *hdev, I2C_HandleTypeDef *hi2c, uint16_t addr) {
    hdev->hi2c = hi2c;
    hdev->Addr = addr;
    hdev->PageSize = AT24C02_PAGE_SIZE;
    
    // 检查设备是否存在
    return HAL_I2C_IsDeviceReady(hdev->hi2c, hdev->Addr, 3, 100);
}

/**
 * @brief  等待 EEPROM 内部写入周期完成 (Acknowledge Polling)
 * @note   EEPROM 写入一个字节或一页后，需要约 5ms 的内部处理时间。
 * 在此期间它不会响应 I2C 命令。
 */
HAL_StatusTypeDef AT24C02_CheckReady(AT24C02_HandleTypeDef *hdev) {
    // 尝试最多等待 10ms
    return HAL_I2C_IsDeviceReady(hdev->hi2c, hdev->Addr, 10, 10);
}

/**
 * @brief  写入单个字节
 */
HAL_StatusTypeDef AT24C02_WriteByte(AT24C02_HandleTypeDef *hdev, uint16_t memAddr, uint8_t data) {
    HAL_StatusTypeDef status;
    
    status = HAL_I2C_Mem_Write(hdev->hi2c, hdev->Addr, memAddr, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    if (status != HAL_OK) return status;
    
    // 写入后必须等待内部周期完成
    return AT24C02_CheckReady(hdev);
}

/**
 * @brief  读取单个字节
 */
uint8_t AT24C02_ReadByte(AT24C02_HandleTypeDef *hdev, uint16_t memAddr) {
    uint8_t data = 0;
    HAL_I2C_Mem_Read(hdev->hi2c, hdev->Addr, memAddr, I2C_MEMADD_SIZE_8BIT, &data, 1, 100);
    return data;
}

/**
 * @brief  任意长度写入 (核心算法)
 * @note   自动处理页边界 (Page Boundary) 和写入周期延时
 */
HAL_StatusTypeDef AT24C02_WriteBuffer(AT24C02_HandleTypeDef *hdev, uint16_t memAddr, uint8_t *pData, uint16_t len) {
    uint16_t wAddr = memAddr;
    uint16_t wLen = len;
    uint8_t *wData = pData;
    uint8_t numBytesToWrite = 0;
    uint8_t gap = 0;
    
    // 计算当前页剩余空间: PageSize - (Addr % PageSize)
    // 例如: 地址0x06, PageSize 8. 0x06 % 8 = 6. 剩余 8-6=2 字节可写
    gap = hdev->PageSize - (wAddr % hdev->PageSize);
    
    // 如果剩余空间大于等于要写的总长度，说明不用跨页
    if (gap >= wLen) {
        numBytesToWrite = wLen;
    } else {
        numBytesToWrite = gap; // 先填满当前页
    }
    
    while (wLen > 0) {
        // 1. 写入一小段数据 (最大不超过一页)
        if (HAL_I2C_Mem_Write(hdev->hi2c, hdev->Addr, wAddr, I2C_MEMADD_SIZE_8BIT, wData, numBytesToWrite, 100) != HAL_OK) {
            return HAL_ERROR;
        }
        
        // 2. 关键：等待 EEPROM 内部烧录完成 (约5ms)
        if (AT24C02_CheckReady(hdev) != HAL_OK) {
            return HAL_TIMEOUT;
        }
        
        // 3. 更新指针和计数器
        wAddr += numBytesToWrite;
        wData += numBytesToWrite;
        wLen  -= numBytesToWrite;
        
        if (wLen == 0) break;
        
        // 4. 计算下一次要写入的长度
        // 之后每次写入都是从新的一页开头开始，所以最大可以写 PageSize
        if (wLen >= hdev->PageSize) {
            numBytesToWrite = hdev->PageSize;
        } else {
            numBytesToWrite = wLen;
        }
    }
    
    return HAL_OK;
}

/**
 * @brief  读取任意长度
 * @note   读取不需要考虑页边界，地址会自动递增
 */
HAL_StatusTypeDef AT24C02_ReadBuffer(AT24C02_HandleTypeDef *hdev, uint16_t memAddr, uint8_t *pData, uint16_t len) {
    return HAL_I2C_Mem_Read(hdev->hi2c, hdev->Addr, memAddr, I2C_MEMADD_SIZE_8BIT, pData, len, 1000);
}

/**
 * @brief  清空整片芯片 (填充 0xFF 或 0x00)
 */
HAL_StatusTypeDef AT24C02_EraseChip(AT24C02_HandleTypeDef *hdev) {
    uint8_t empty_page[8];
    memset(empty_page, 0xFF, 8); // 通常擦除是置为 0xFF
    
    // 按页循环擦除
    for (uint16_t i = 0; i < AT24C02_TOTAL_SIZE; i += AT24C02_PAGE_SIZE) {
        if (AT24C02_WriteBuffer(hdev, i, empty_page, AT24C02_PAGE_SIZE) != HAL_OK) {
            return HAL_ERROR;
        }
    }
    return HAL_OK;
}
