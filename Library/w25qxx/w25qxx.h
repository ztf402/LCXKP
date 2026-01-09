#ifndef __W25QXX_H
#define __W25QXX_H

#ifdef __cplusplus
extern "C" {
#endif

/* 根据你的MCU型号包含对应的HAL库头文件 */
#include "main.h" 
// #include "stm32f1xx_hal.h"
// #include "stm32f4xx_hal.h"

// ================= 配置区域 =================

/* 是否使用DMA传输： 1=开启, 0=关闭(使用轮询) */
#define W25Q_USE_DMA     1

/* 默认超时时间 */
#define W25Q_TIMEOUT     1000

// ================= 指令定义 =================
#define W25Q_CMD_WRITE_ENABLE       0x06
#define W25Q_CMD_WRITE_DISABLE      0x04
#define W25Q_CMD_READ_STATUS_R1     0x05
#define W25Q_CMD_READ_DATA          0x03
#define W25Q_CMD_PAGE_PROGRAM       0x02
#define W25Q_CMD_SECTOR_ERASE       0x20  // 4KB Erase
#define W25Q_CMD_BLOCK_ERASE_32K    0x52
#define W25Q_CMD_BLOCK_ERASE_64K    0xD8
#define W25Q_CMD_CHIP_ERASE         0xC7
#define W25Q_CMD_JEDEC_ID           0x9F
#define W25Q_CMD_RESET_ENABLE       0x66
#define W25Q_CMD_RESET_DEVICE       0x99

// ================= 数据结构 =================

/* W25Q ID 枚举 (常用型号) */
typedef enum {
    W25Q80  = 0xEF13,
    W25Q16  = 0xEF14,
    W25Q32  = 0xEF15,
    W25Q64  = 0xEF16,
    W25Q128 = 0xEF17,
    W25Q256 = 0xEF18
} W25Q_ID_t;

/* 驱动句柄结构体 */
typedef struct {
    SPI_HandleTypeDef *hspi;       // HAL SPI 句柄
    GPIO_TypeDef      *CS_Port;    // 片选端口
    uint16_t          CS_Pin;      // 片选引脚
    uint16_t          ID;          // 芯片ID
    uint32_t          SectorCount; // 扇区数量
    uint32_t          PageCount;   // 页数量
    uint32_t          BlockCount;  // 块数量
    uint32_t          Capacity;    // 总容量(Bytes)
} W25Q_Handle_t;

// ================= 函数声明 =================

/* 初始化 */
int8_t W25Q_Init(W25Q_Handle_t *dev, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin);

/* 基础操作 */
void W25Q_Read(W25Q_Handle_t *dev, uint32_t addr, uint8_t *pData, uint32_t len);
void W25Q_Write(W25Q_Handle_t *dev, uint32_t addr, uint8_t *pData, uint32_t len); // 智能写，自动处理分页

/* 擦除操作 */
void W25Q_EraseSector(W25Q_Handle_t *dev, uint32_t sector_addr); // 擦除4KB
void W25Q_EraseBlock(W25Q_Handle_t *dev, uint32_t block_addr);   // 擦除64KB
void W25Q_EraseChip(W25Q_Handle_t *dev);

/* 休眠与唤醒 (可选) */
void W25Q_PowerDown(W25Q_Handle_t *dev);
void W25Q_WakeUp(W25Q_Handle_t *dev);

#ifdef __cplusplus
}
#endif

#endif