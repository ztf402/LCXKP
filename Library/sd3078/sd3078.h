#ifndef __SD3078_H__
#define __SD3078_H__

#include "main.h"

// ================= 配置部分 =================
// SD3078 默认 I2C 地址 (7-bit = 0x32), HAL库需要左移一位
#define SD3078_I2C_ADDR    (0x32 << 1)

// 定义 I2C 句柄类型 (方便移植)
typedef I2C_HandleTypeDef* SD3078_I2C_Handle;

// ================= 寄存器定义 =================
#define SD3078_REG_SC      0x00 // 秒
#define SD3078_REG_MN      0x01 // 分
#define SD3078_REG_HR      0x02 // 时
#define SD3078_REG_WK      0x03 // 星期
#define SD3078_REG_DY      0x04 // 日
#define SD3078_REG_MO      0x05 // 月
#define SD3078_REG_YR      0x06 // 年
#define SD3078_REG_CTR1    0x0F // 控制寄存器 1
#define SD3078_REG_CTR2    0x10 // 控制寄存器 2
#define SD3078_REG_CTR3    0x11 // 控制寄存器 3

// ================= 数据结构 =================

// 时间结构体
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;   // 24小时制
} SD3078_Time_t;

// 日期结构体
typedef struct {
    uint8_t day;     // 1-31
    uint8_t month;   // 1-12
    uint8_t year;    // 0-99 (代表 2000-2099)
    uint8_t week;    // 0-6 (0=Sunday, 1=Monday...)
} SD3078_Date_t;

// SD3078 设备句柄
typedef struct {
    SD3078_I2C_Handle hi2c;
    uint8_t           initialized;
} SD3078_t;

// ================= 函数声明 =================

// 初始化
int8_t SD3078_Init(SD3078_t *dev, SD3078_I2C_Handle hi2c);

// 设置时间/日期
int8_t SD3078_SetTime(SD3078_t *dev, uint8_t hour, uint8_t min, uint8_t sec);
int8_t SD3078_SetDate(SD3078_t *dev, uint8_t year, uint8_t month, uint8_t day, uint8_t week);

// 读取时间/日期
int8_t SD3078_GetTime(SD3078_t *dev, SD3078_Time_t *time);
int8_t SD3078_GetDate(SD3078_t *dev, SD3078_Date_t *date);

// 读取全部 (一次 I2C 事务，保证时间一致性)
int8_t SD3078_GetFullTime(SD3078_t *dev, SD3078_Time_t *time, SD3078_Date_t *date);

#endif
