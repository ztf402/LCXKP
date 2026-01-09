#include "w25qxx.h"
#include <string.h> // for NULL

// ================= 内部静态辅助函数 =================

/**
 * @brief 片选拉低
 */
static inline void W25Q_CS_Low(W25Q_Handle_t *dev) {
    HAL_GPIO_WritePin(dev->CS_Port, dev->CS_Pin, GPIO_PIN_RESET);
}

/**
 * @brief 片选拉高
 */
static inline void W25Q_CS_High(W25Q_Handle_t *dev) {
    HAL_GPIO_WritePin(dev->CS_Port, dev->CS_Pin, GPIO_PIN_SET);
}

/**
 * @brief SPI底层发送接收函数 (支持DMA/Polling)
 */
static int8_t W25Q_SPI_TxRx(W25Q_Handle_t *dev, uint8_t *txData, uint8_t *rxData, uint32_t len) {
    HAL_StatusTypeDef status = HAL_ERROR;

#if W25Q_USE_DMA
    // DMA 模式
    if (txData && rxData) {
        status = HAL_SPI_TransmitReceive_DMA(dev->hspi, txData, rxData, len);
    } else if (txData) {
        status = HAL_SPI_Transmit_DMA(dev->hspi, txData, len);
    } else if (rxData) {
        status = HAL_SPI_Receive_DMA(dev->hspi, rxData, len);
    }
    
    // 等待传输完成 (为了保持驱动逻辑简单，这里使用了阻塞等待DMA完成)
    // 实际项目中，如果追求极致性能，可以将等待逻辑移出驱动层
    if (status == HAL_OK) {
        while (HAL_SPI_GetState(dev->hspi) != HAL_SPI_STATE_READY) {
            // 可以加入超时机制防止死锁
        }
    }
#else
    // 轮询模式
    if (txData && rxData) {
        status = HAL_SPI_TransmitReceive(dev->hspi, txData, rxData, len, W25Q_TIMEOUT);
    } else if (txData) {
        status = HAL_SPI_Transmit(dev->hspi, txData, len, W25Q_TIMEOUT);
    } else if (rxData) {
        status = HAL_SPI_Receive(dev->hspi, rxData, len, W25Q_TIMEOUT);
    }
#endif

    return (status == HAL_OK) ? 0 : -1;
}

/**
 * @brief 写使能
 */
static void W25Q_WriteEnable(W25Q_Handle_t *dev) {
    uint8_t cmd = W25Q_CMD_WRITE_ENABLE;
    W25Q_CS_Low(dev);
    W25Q_SPI_TxRx(dev, &cmd, NULL, 1);
    W25Q_CS_High(dev);
}

/**
 * @brief 等待芯片忙碌结束 (读取Status Register 1)
 */
static void W25Q_WaitBusy(W25Q_Handle_t *dev) {
    uint8_t cmd = W25Q_CMD_READ_STATUS_R1;
    uint8_t status;
    
    W25Q_CS_Low(dev);
    W25Q_SPI_TxRx(dev, &cmd, NULL, 1);
    
    do {
        W25Q_SPI_TxRx(dev, NULL, &status, 1);
    } while ((status & 0x01) == 0x01); // 检查BUSY位 (Bit 0)
    
    W25Q_CS_High(dev);
}

// ================= 外部接口实现 =================

/**
 * @brief 初始化W25Q驱动
 */
int8_t W25Q_Init(W25Q_Handle_t *dev, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin) {
    if (dev == NULL || hspi == NULL) return -1;

    dev->hspi = hspi;
    dev->CS_Port = cs_port;
    dev->CS_Pin = cs_pin;

    W25Q_CS_High(dev); // 默认不选中
    HAL_Delay(100);    // 上电等待

    // 读取ID
    uint8_t cmd = W25Q_CMD_JEDEC_ID;
    uint8_t id_data[3];
    
    W25Q_CS_Low(dev);
    W25Q_SPI_TxRx(dev, &cmd, NULL, 1);
    W25Q_SPI_TxRx(dev, NULL, id_data, 3);
    W25Q_CS_High(dev);

    dev->ID = (id_data[1] << 8) | id_data[2];
    
    // 根据ID识别容量
    switch (dev->ID) {
        case W25Q80:  dev->Capacity = 1 * 1024 * 1024; break;
        case W25Q16:  dev->Capacity = 2 * 1024 * 1024; break;
        case W25Q32:  dev->Capacity = 4 * 1024 * 1024; break;
        case W25Q64:  dev->Capacity = 8 * 1024 * 1024; break;
        case W25Q128: dev->Capacity = 16 * 1024 * 1024; break;
        case W25Q256: dev->Capacity = 32 * 1024 * 1024; break;
        default: return -2; // 未知ID
    }

    dev->SectorCount = dev->Capacity / 4096;
    dev->BlockCount  = dev->Capacity / 65536;
    dev->PageCount   = dev->Capacity / 256;

    return 0;
}

/**
 * @brief 读取数据
 */
void W25Q_Read(W25Q_Handle_t *dev, uint32_t addr, uint8_t *pData, uint32_t len) {
    uint8_t cmd[4];
    cmd[0] = W25Q_CMD_READ_DATA;
    cmd[1] = (uint8_t)((addr >> 16) & 0xFF);
    cmd[2] = (uint8_t)((addr >> 8) & 0xFF);
    cmd[3] = (uint8_t)(addr & 0xFF);

    W25Q_CS_Low(dev);
    W25Q_SPI_TxRx(dev, cmd, NULL, 4);   // 发送指令+地址
    W25Q_SPI_TxRx(dev, NULL, pData, len); // 读取数据
    W25Q_CS_High(dev);
}

/**
 * @brief 内部函数：写入一页 (不进行边界检查)
 */
static void W25Q_WritePage(W25Q_Handle_t *dev, uint32_t addr, uint8_t *pData, uint32_t len) {
    W25Q_WriteEnable(dev);

    uint8_t cmd[4];
    cmd[0] = W25Q_CMD_PAGE_PROGRAM;
    cmd[1] = (uint8_t)((addr >> 16) & 0xFF);
    cmd[2] = (uint8_t)((addr >> 8) & 0xFF);
    cmd[3] = (uint8_t)(addr & 0xFF);

    W25Q_CS_Low(dev);
    W25Q_SPI_TxRx(dev, cmd, NULL, 4);
    W25Q_SPI_TxRx(dev, pData, NULL, len);
    W25Q_CS_High(dev);

    W25Q_WaitBusy(dev);
}

/**
 * @brief 写入任意长度数据 (自动处理页对齐和翻页)
 */
void W25Q_Write(W25Q_Handle_t *dev, uint32_t addr, uint8_t *pData, uint32_t len) {
    uint32_t pageremain;
    pageremain = 256 - addr % 256; // 单页剩余空间

    if (len <= pageremain) pageremain = len; // 如果数据量小于剩余空间

    while (1) {
        W25Q_WritePage(dev, addr, pData, pageremain);
        
        if (len == pageremain) break; // 写入完成

        pData += pageremain;
        addr  += pageremain;
        len   -= pageremain;

        // 下一次写入长度判断
        if (len > 256) pageremain = 256;
        else pageremain = len;
    }
}

/**
 * @brief 扇区擦除 (4KB)
 */
void W25Q_EraseSector(W25Q_Handle_t *dev, uint32_t sector_addr) {
    // 确保地址对齐到扇区首地址 (虽然W25Q通常忽略低位，但最好处理一下)
    // sector_addr *= 4096; // 如果传入的是扇区号而非地址，取消此注释

    W25Q_WriteEnable(dev);
    W25Q_WaitBusy(dev);

    uint8_t cmd[4];
    cmd[0] = W25Q_CMD_SECTOR_ERASE;
    cmd[1] = (uint8_t)((sector_addr >> 16) & 0xFF);
    cmd[2] = (uint8_t)((sector_addr >> 8) & 0xFF);
    cmd[3] = (uint8_t)(sector_addr & 0xFF);

    W25Q_CS_Low(dev);
    W25Q_SPI_TxRx(dev, cmd, NULL, 4);
    W25Q_CS_High(dev);

    W25Q_WaitBusy(dev);
}

/**
 * @brief 块擦除 (64KB)
 */
void W25Q_EraseBlock(W25Q_Handle_t *dev, uint32_t block_addr) {
    W25Q_WriteEnable(dev);
    W25Q_WaitBusy(dev);

    uint8_t cmd[4];
    cmd[0] = W25Q_CMD_BLOCK_ERASE_64K;
    cmd[1] = (uint8_t)((block_addr >> 16) & 0xFF);
    cmd[2] = (uint8_t)((block_addr >> 8) & 0xFF);
    cmd[3] = (uint8_t)(block_addr & 0xFF);

    W25Q_CS_Low(dev);
    W25Q_SPI_TxRx(dev, cmd, NULL, 4);
    W25Q_CS_High(dev);

    W25Q_WaitBusy(dev);
}

/**
 * @brief 整片擦除 (耗时很长!)
 */
void W25Q_EraseChip(W25Q_Handle_t *dev) {
    W25Q_WriteEnable(dev);
    W25Q_WaitBusy(dev);

    uint8_t cmd = W25Q_CMD_CHIP_ERASE;

    W25Q_CS_Low(dev);
    W25Q_SPI_TxRx(dev, &cmd, NULL, 1);
    W25Q_CS_High(dev);

    W25Q_WaitBusy(dev);
}