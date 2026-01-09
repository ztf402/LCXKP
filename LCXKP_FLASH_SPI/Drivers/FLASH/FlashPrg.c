/**************************************************************************//**
 * @file     FlashPrg.c
 * @brief    Flash Programming Functions for W25Q128 (STM32F4 SPI2) - Pure CMSIS
 ******************************************************************************/

#include "FlashOS.H"        // FlashOS Structures
#include "stm32f4xx.h"      

// ======================= 1.  () =======================

// --- CS : PE4 ---
#define CS_GPIO_PORT         GPIOE
#define CS_PIN_POS           4
#define RCC_AHB1_CS          RCC_AHB1ENR_GPIOEEN

// --- SPI SCK : PB10 ---
#define SPI_SCK_PORT         GPIOB
#define SPI_SCK_PIN_POS      10
#define RCC_AHB1_SCK         RCC_AHB1ENR_GPIOBEN

// --- SPI MISO/MOSI : PC2, PC3 ---
#define SPI_MISO_MOSI_PORT   GPIOC
#define SPI_MISO_PIN_POS     2
#define SPI_MOSI_PIN_POS     3
#define RCC_AHB1_MISO_MOSI   RCC_AHB1ENR_GPIOCEN

// --- SPI  ---
#define RCC_APB1_SPI         RCC_APB1ENR_SPI2EN

// =========================================================================

/* W25Q128  */
#define W25Q_WriteEnable      0x06
#define W25Q_ReadStatusReg1   0x05
#define W25Q_PageProgram      0x02
#define W25Q_SectorErase_4K   0x20
#define W25Q_ChipErase        0xC7
#define W25Q_ReadData         0x03  
#define WIP_Flag              0x01  
#define DUMMY_BYTE            0xFF

// ---------------------  (: CMSIS) ---------------------

//  (CS High):  BSRR 16 (Bit Set)
//  (1UL << 4) , HAL  GPIO_PIN_4
#define SPI_CS_HIGH()   CS_GPIO_PORT->BSRR = (1UL << CS_PIN_POS)

//  (CS Low):   BSRR 16 (Bit Reset)
//  (1UL << (4 + 16)) , BR4
#define SPI_CS_LOW()    CS_GPIO_PORT->BSRR = (1UL << (CS_PIN_POS + 16))

// SPI 
static uint8_t SPI_ReadWriteByte(uint8_t data)
{
    while (!(SPI2->SR & SPI_SR_TXE));
    *(__IO uint8_t *)&SPI2->DR = data;

    while (!(SPI2->SR & SPI_SR_RXNE));
    return *(__IO uint8_t *)&SPI2->DR;
}


//  Flash 
static void SPI_Wait_Busy(void) {
    uint8_t status = 0;
    SPI_CS_LOW();
    SPI_ReadWriteByte(W25Q_ReadStatusReg1);
    do {
        status = SPI_ReadWriteByte(DUMMY_BYTE);
    } while ((status & WIP_Flag) == 0x01);
    SPI_CS_HIGH();
}

static void SPI_WaitTxDone(void)
{
    while (SPI2->SR & SPI_SR_BSY);   // ??????
    (void)SPI2->DR;                  // ??:? DR/SR ???????
    (void)SPI2->SR;
}


// 
static void SPI_Write_Enable(void)
{
    SPI_CS_LOW();
    SPI_ReadWriteByte(W25Q_WriteEnable);
    SPI_WaitTxDone();
    SPI_CS_HIGH();
}

int ProgramPage (unsigned long adr, unsigned long sz, unsigned char *buf)
{
    uint32_t flash_addr = adr & 0x00FFFFFF;
    SPI_Wait_Busy();
    SPI_Write_Enable();

    SPI_CS_LOW();
    SPI_ReadWriteByte(W25Q_PageProgram);
    SPI_ReadWriteByte((uint8_t)(flash_addr >> 16));
    SPI_ReadWriteByte((uint8_t)(flash_addr >> 8));
    SPI_ReadWriteByte((uint8_t)(flash_addr));

    while (sz--) {
        SPI_ReadWriteByte(*buf++);
    }
    SPI_WaitTxDone();
    SPI_CS_HIGH();

    SPI_Wait_Busy();
    return 0;
}


/*
 * Initialize Flash Programming Functions
 */
int Init (unsigned long adr, unsigned long clk, unsigned long fnc) {
    // 1) RCC
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOEEN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    (void)RCC->AHB1ENR;
    (void)RCC->APB1ENR;

    // 2) CS: PE4 PE7 output push-pull high
    GPIOE->MODER   = (GPIOE->MODER   & ~(3UL << (4*2))) | (1UL << (4*2));
    GPIOE->OTYPER &= ~(1UL << 4);
    GPIOE->OSPEEDR = (GPIOE->OSPEEDR & ~(3UL << (4*2))) | (2UL << (4*2));
    GPIOE->PUPDR   = (GPIOE->PUPDR   & ~(3UL << (4*2))) | (0UL << (4*2));
    GPIOE->BSRR = (1UL << 4);
    
    GPIOE->MODER &= ~(3UL << (7 * 2));
    GPIOE->MODER |=  (1UL << (7 * 2));
    GPIOE->OTYPER &= ~(1UL << 7);
    GPIOE->OSPEEDR &= ~(3UL << (7 * 2));
    GPIOE->OSPEEDR |=  (2UL << (7 * 2));
    GPIOE->PUPDR &= ~(3UL << (7 * 2));
    GPIOE->BSRR = (1UL << 7);

    // 3) SCK: PB10 AF5 (SPI2_SCK)
    GPIOB->MODER   = (GPIOB->MODER   & ~(3UL << (10*2))) | (2UL << (10*2)); // AF
    GPIOB->OTYPER &= ~(1UL << 10);                                         // PP
    GPIOB->OSPEEDR = (GPIOB->OSPEEDR & ~(3UL << (10*2))) | (2UL << (10*2)); // High speed
    GPIOB->PUPDR   = (GPIOB->PUPDR   & ~(3UL << (10*2))) | (0UL << (10*2)); // NOPULL
    GPIOB->AFR[1]  = (GPIOB->AFR[1]  & ~(0xFUL << ((10-8)*4))) | (5UL << ((10-8)*4)); // AF5


    // 4) MISO PC2 AF5, MOSI PC3 AF5
    GPIOC->MODER   = (GPIOC->MODER   & ~((3UL<<(2*2))|(3UL<<(3*2)))) | ((2UL<<(2*2))|(2UL<<(3*2)));
    GPIOC->OSPEEDR = (GPIOC->OSPEEDR & ~((3UL<<(2*2))|(3UL<<(3*2)))) | ((2UL<<(2*2))|(2UL<<(3*2)));
    GPIOC->PUPDR   = (GPIOC->PUPDR   & ~((3UL<<(2*2))|(3UL<<(3*2)))) | ((0UL<<(2*2))|(0UL<<(3*2)));
    GPIOC->AFR[0]  = (GPIOC->AFR[0]  & ~((0xFUL<<(2*4))|(0xFUL<<(3*4)))) | ((5UL<<(2*4))|(5UL<<(3*4)));

    // 5) SPI2 config
    SPI2->CR1 = 0;
    SPI2->CR1 |= SPI_CR1_MSTR;                 // master
    SPI2->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;    // software NSS
    SPI2->CR1 |= (SPI_CR1_BR_1);               // prescaler = fPCLK/8 (???)
    // Mode0: CPOL=0 CPHA=0 -> do nothing
    SPI2->CR1 &= ~SPI_CR1_LSBFIRST;            // MSB first
    SPI2->CR1 &= ~SPI_CR1_DFF;                 // 8-bit
    SPI2->CR1 |= SPI_CR1_SPE;


    return (0);
}

/*
 * De-Initialize
 */
int UnInit (unsigned long fnc) {
    SPI2->CR1 &= ~SPI_CR1_SPE;
    RCC->APB1ENR &= ~RCC_APB1ENR_SPI2EN;  
    return (0);
}

/*
 * Erase Chip
 */
int EraseChip (void) {
    SPI_Wait_Busy();
    SPI_Write_Enable();
    SPI_CS_LOW();
    SPI_ReadWriteByte(W25Q_ChipErase);
    SPI_CS_HIGH();
    SPI_Wait_Busy();
    return (0);
}

/*
 * Erase Sector
 */
int EraseSector (unsigned long adr) {
    uint32_t flash_addr = adr & 0x00FFFFFF; 
    SPI_Wait_Busy();
    SPI_Write_Enable();
    SPI_CS_LOW();
    SPI_ReadWriteByte(W25Q_SectorErase_4K);
    SPI_ReadWriteByte((uint8_t)((flash_addr) >> 16));
    SPI_ReadWriteByte((uint8_t)((flash_addr) >> 8));
    SPI_ReadWriteByte((uint8_t)(flash_addr));
    SPI_CS_HIGH();
    SPI_Wait_Busy();
    return (0);
}


/*
 * Verify (,Keil)
 */
unsigned long Verify (unsigned long adr, unsigned long sz, unsigned char *buf) {
    uint32_t flash_addr = adr & 0x00FFFFFF;
    uint8_t t;
    unsigned long i;

    SPI_Wait_Busy();
    SPI_CS_LOW();
    
    // 
    SPI_ReadWriteByte(W25Q_ReadData);
    SPI_ReadWriteByte((uint8_t)((flash_addr) >> 16));
    SPI_ReadWriteByte((uint8_t)((flash_addr) >> 8));
    SPI_ReadWriteByte((uint8_t)(flash_addr));

    for (i = 0; i < sz; i++) {
        t = SPI_ReadWriteByte(DUMMY_BYTE);
        if (t != buf[i]) {
            SPI_CS_HIGH();
            return (adr + i); // 
        }
    }

    SPI_CS_HIGH();
    return (adr + sz); // 
}
