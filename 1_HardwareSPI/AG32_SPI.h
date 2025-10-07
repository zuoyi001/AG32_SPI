#ifndef AG32_SPI_H
#define AG32_SPI_H

#include "example.h"
#include <stdint.h>
#include <stdbool.h>

// SPI端口定义 - 使用SDK已定义的SPI0和SPI1

// SPI模式枚举 - 避免宏冲突
typedef enum {
    AG32_SPI_MODE0 = 0,  // CPOL=0, CPHA=0
    AG32_SPI_MODE1 = 1,  // CPOL=0, CPHA=1
    AG32_SPI_MODE2 = 2,  // CPOL=1, CPHA=0
    AG32_SPI_MODE3 = 3   // CPOL=1, CPHA=1
} AG32_SPIMode_t;

// SPI时钟分频枚举
typedef enum {
    AG32_SPI_CLOCK_DIV2   = SPI_CTRL_SCLK_DIV2,
    AG32_SPI_CLOCK_DIV4   = SPI_CTRL_SCLK_DIV4,
    AG32_SPI_CLOCK_DIV8   = SPI_CTRL_SCLK_DIV8,
    AG32_SPI_CLOCK_DIV16  = SPI_CTRL_SCLK_DIV16,
    AG32_SPI_CLOCK_DIV32  = SPI_CTRL_SCLK_DIV32,
    AG32_SPI_CLOCK_DIV64  = SPI_CTRL_SCLK_DIV64,
    AG32_SPI_CLOCK_DIV128 = SPI_CTRL_SCLK_DIV128
} AG32_SPIClockDiv_t;

// 位序枚举
typedef enum {
    AG32_LSBFIRST = 0,
    AG32_MSBFIRST = 1
} AG32_SPIBitOrder_t;

// SPI设置结构体
typedef struct {
    AG32_SPIMode_t dataMode;        // SPI模式
    AG32_SPIBitOrder_t bitOrder;    // 位序
    AG32_SPIClockDiv_t clockDiv;    // 时钟分频
} SPISettings;

// SPI类结构体 - Arduino风格接口
typedef struct {
    SPI_TypeDef *spiPort;     // 底层SPI端口
    bool initialized;         // 初始化状态
    SPISettings settings;     // SPI设置
    
    // Arduino风格方法
    void (*begin)(SPI_TypeDef *spi);
    void (*end)(void);
    void (*beginTransaction)(SPISettings settings);
    void (*endTransaction)(void);
    uint8_t (*transfer)(uint8_t data);
    uint16_t (*transfer16)(uint16_t data);
    void (*transferBytes)(uint8_t *txData, uint8_t *rxData, size_t count);
    void (*setBitOrder)(AG32_SPIBitOrder_t bitOrder);
    void (*setDataMode)(AG32_SPIMode_t dataMode);
    void (*setClockDivider)(AG32_SPIClockDiv_t clockDiv);
} SPIClass_t;

// Arduino风格函数声明
void spi_begin(SPI_TypeDef *spi);
void spi_end(void);
void spi_beginTransaction(SPISettings settings);
void spi_endTransaction(void);
uint8_t spi_transfer(uint8_t data);
uint16_t spi_transfer16(uint16_t data);
void spi_transferBytes(uint8_t *txData, uint8_t *rxData, size_t count);
void spi_setBitOrder(AG32_SPIBitOrder_t bitOrder);
void spi_setDataMode(AG32_SPIMode_t dataMode);
void spi_setClockDivider(AG32_SPIClockDiv_t clockDiv);

// 便利宏定义
// #define SPISettings_Create(dataMode, bitOrder, clockDiv) \
//     ((SPISettings){.dataMode = (dataMode), .bitOrder = (bitOrder), .clockDiv = (clockDiv)})
// #define SPISettings_Create(mode, bitOrder, clockDiv) \
//     ((SPISettings){.dataMode = (mode), .bitOrder = (bitOrder), .clockDiv = (clockDiv)})

// #define SPISettings_Create(mode, bitOrder, clockDiv) ((SPISettings){.dataMode = (mode), .bitOrder = (bitOrder), .clockDiv = (clockDiv)})


// 替代SPISettings_Create宏的安全函数
static inline SPISettings SPISettings_Create(AG32_SPIMode_t mode, 
                                                 AG32_SPIBitOrder_t bitOrder,
                                                 AG32_SPIClockDiv_t clockDiv)
{
    SPISettings settings;
    settings.dataMode = mode;
    settings.bitOrder = bitOrder;
    settings.clockDiv = clockDiv;
    return settings;
}

// 全局SPI实例 - Arduino风格
extern SPIClass_t AG32_SPI;

#endif // AG32_SPI_H