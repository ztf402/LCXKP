# LCXKP - 立创天空星外设驱动库

本项目汇集了 **立创天空星 (LCXKP)** 筑基学习板的外设驱动代码、Flash 算法文件及相关测试例程。
### 关于为啥是XKP呢,我一直记得它叫星空派;直到刚才我才意识到叫错了
## 🛠 项目特性

* **核心架构**：基于 STM32 HAL 库开发。
* **兼容性**：完美兼容 STM32CubeMX 生成的工程结构。
* **特色功能**：包含一套独特的 SPI 与 I2S 动态切换逻辑（针对板载资源复用设计）。
* **工具支持**：提供自定义的外挂 Flash 下载算法 (.FLM) 源码及预编译文件。

## 📂 目录结构

项目主要包含驱动库 (`Library`) 与 Flash 算法工程 (`LCXKP_FLASH_SPI`)。[驱动部分为闭门造车,只保证能用,不保证性能😜]

```text
.
├── LCXKP                  # 用于切换 SPI2 和 I2S2 
├── LCXKP_FLASH_SPI        # Flash 算法生成工程 (MDK)
│   ├── Drivers
│   │   ├── CMSIS
│   │   └── FLASH
│   └── MDK-ARM            # Keil 工程文件
│       ├── DebugConfig
│       ├── LCXKP_FLASH_SPI
│       └── RTE
└── Library                # 外设驱动库
    ├── aht20              # 温湿度传感器
    ├── at24c02            # EEPROM
    ├── icm42688           # 6轴惯性测量单元 (IMU)
    ├── ina226             # 电流电压功率监控
    ├── pca9555            # I/O 扩展芯片
    ├── sd3078             # 实时时钟 (RTC)
    └── w25qxx             # SPI Flash 支持自动识别容量


