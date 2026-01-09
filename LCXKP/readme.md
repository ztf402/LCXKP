### 🛠️ SPI2/I2S2 复用功能配置指南

本项目实现了 SPI2 与 I2S2 的底层复用切换,且不会影响CubeMX的后续生成。为了启用该功能，请按照以下步骤配置你的工程：

#### 1\. 替换配置文件

将本目录下的 `stm32f4xx_hal_conf.h` 复制并替换掉工程中由 CubeMX 生成的默认配置文件（通常位于 `Core/Inc` 或 `Inc` 目录下）。

> **注意**：此文件是保证你可以继续使用CubeMX的基础。

#### 2\. 添加编译路径

在 IDE（如 Keil MDK, IAR 等）的项目设置中，将**本文件夹的路径**添加到 C/C++ 编译器的 **Include Paths**（头文件查找范围）中。

#### 3\. 添加源文件

将本目录下的以下两个源文件添加到工程的 `src` 分组或 `Application/User` 分组中参与编译：

*   `spi2.c`
    
*   `i2s2.c`
    

#### 4\. 在 main.c 中调用

配置完成后，在 `main.c` 的用户代码区（如 `USER CODE BEGIN 2`）使用如下预处理指令来动态选择初始化逻辑：

C

    /* 根据宏定义自动选择初始化 SPI2 还是 I2S2 */
    #if SPI2_OR_I2S2 == MODE_SPI
        MX_SPI2_Init();
    #endif
    
    #if SPI2_OR_I2S2 == MODE_I2S
        MX_I2S2_Init();
    #endif

* * *

### 💡 如何切换模式？

修改 `lcxkp_cfg.h` (或相关头文件) 中的宏定义即可一键切换硬件逻辑，无需修改 `main.c` 代码。

---

