#ifndef __ICM42688_H__
#define __ICM42688_H__

#include "main.h" // 包含 HAL 库定义

// ================= 配置部分 =================
// 如果需要移植，只需修改这里的底层接口类型
typedef SPI_HandleTypeDef* ICM_SPI_Handle;
typedef GPIO_TypeDef* ICM_GPIO_Port;
typedef uint16_t           ICM_GPIO_Pin;

// ================= 寄存器定义 (Bank 0) =================
#define ICM42688_REG_BANK_SEL      0x76
#define ICM42688_WHO_AM_I          0x75
#define ICM42688_PWR_MGMT0         0x4E
#define ICM42688_GYRO_CONFIG0      0x4F
#define ICM42688_ACCEL_CONFIG0     0x50
#define ICM42688_TEMP_DATA1        0x1D
#define ICM42688_ACCEL_DATA_X1     0x1F
#define ICM42688_GYRO_DATA_X1      0x25

#define ICM42688_WHO_AM_I_VAL      0x47 // ICM-42688-P ID

// ================= 枚举定义 =================

// 加速度量程
typedef enum {
    ICM_ACCEL_16G = 0x00,
    ICM_ACCEL_8G  = 0x01,
    ICM_ACCEL_4G  = 0x02,
    ICM_ACCEL_2G  = 0x03
} ICM_AccelRange_t;

// 角速度量程
typedef enum {
    ICM_GYRO_2000DPS = 0x00,
    ICM_GYRO_1000DPS = 0x01,
    ICM_GYRO_500DPS  = 0x02,
    ICM_GYRO_250DPS  = 0x03,
    ICM_GYRO_125DPS  = 0x04
} ICM_GyroRange_t;

// 输出数据速率 (ODR)
typedef enum {
    ICM_ODR_32kHz  = 0x01, // 仅 LN 模式
    ICM_ODR_16kHz  = 0x02, // 仅 LN 模式
    ICM_ODR_8kHz   = 0x03,
    ICM_ODR_4kHz   = 0x04,
    ICM_ODR_2kHz   = 0x05,
    ICM_ODR_1kHz   = 0x06,
    ICM_ODR_200Hz  = 0x07,
    ICM_ODR_50Hz   = 0x08
} ICM_ODR_t;

// ================= 数据结构 =================

// 传感器原始数据
typedef struct {
    int16_t accel_x_raw;
    int16_t accel_y_raw;
    int16_t accel_z_raw;
    int16_t gyro_x_raw;
    int16_t gyro_y_raw;
    int16_t gyro_z_raw;
    int16_t temp_raw;
} ICM_RawData_t;

// 传感器物理数据 (浮点)
typedef struct {
    float accel_x_g;
    float accel_y_g;
    float accel_z_g;
    float gyro_x_dps;
    float gyro_y_dps;
    float gyro_z_dps;
    float temp_c;
} ICM_PhysData_t;

// 设备句柄对象
typedef struct {
    // 硬件接口
    ICM_SPI_Handle  hspi;
    ICM_GPIO_Port   cs_port;
    ICM_GPIO_Pin    cs_pin;
    
    // 配置状态 (用于换算)
    float           accel_scale; // g/LSB
    float           gyro_scale;  // dps/LSB
    
    // 数据
    ICM_RawData_t   raw_data;
    ICM_PhysData_t  data;
    
    // 标志位
    uint8_t         initialized;
} ICM42688_t;

// ================= 函数声明 =================

// 初始化
int8_t ICM42688_Init(ICM42688_t *dev, ICM_SPI_Handle hspi, ICM_GPIO_Port cs_port, ICM_GPIO_Pin cs_pin);

// 配置
int8_t ICM42688_SetAccelConfig(ICM42688_t *dev, ICM_AccelRange_t range, ICM_ODR_t odr);
int8_t ICM42688_SetGyroConfig(ICM42688_t *dev, ICM_GyroRange_t range, ICM_ODR_t odr);

// 数据读取
int8_t ICM42688_ReadData(ICM42688_t *dev);

#endif
