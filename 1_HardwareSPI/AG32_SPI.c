#include "AG32_SPI.h"
#include <stdlib.h>
#include <string.h>

// 从AG32_SPI_Example.c复制的寄存器定义和函数
#define SPI0_CTRL (MMIO_BASE + 0)
#define SPI0_DATA (MMIO_BASE + 4)
#define SPI1_CTRL SPI0_CTRL
#define SPI1_DATA SPI0_DATA

#define SPI_CTRL_CPOL  (1 << 24)
#define SPI_CTRL_CPHA  (1 << 25)

// 静态全局变量保存当前SPI状态
static SPI_TypeDef *current_spi = NULL;
static bool spi_initialized = false;
static SPISettings current_settings = {AG32_SPI_MODE0, AG32_MSBFIRST, AG32_SPI_CLOCK_DIV4};

// 获取寄存器位置
static uint32_t SPI_getRegisterId(SPI_TypeDef *spi, int type)
{
    if (spi == SPIx(0)) {
        if (type == 0) return SPI0_CTRL;
        else if (type == 1) return SPI0_DATA;
    } else {
        if (type == 0) return SPI1_CTRL;
        else if (type == 1) return SPI1_DATA;
    }
    return 0;
}

// 设置SPI控制参数
static int SPI_SetCtrlParam(SPI_TypeDef *spi, int cpol, int cpha)
{
    uint32_t ctrlValue = 0;
    if (cpol == 1) ctrlValue |= SPI_CTRL_CPOL;
    if (cpha == 1) ctrlValue |= SPI_CTRL_CPHA;
    ctrlValue |= SPI_CTRL_ENDIAN;

    uint32_t spi_ctrl = SPI_getRegisterId(spi, 0);
    WR_REG(spi_ctrl, ctrlValue);
    return 1;
}

// 短数据传输 (<=4字节)
static int SPI_SendWithRecv_short(SPI_TypeDef *spi, uint8_t *txBuff, uint8_t *rxBuff, int transLen)
{
    if (transLen > 4) return 0;

    uint32_t txData = 0, rxData = 0;
    for (int i = 0; i < transLen && i < 4; i++) {
        txData |= (txBuff[i] << (i * 8));
    }

    SPI_SetPhaseCtrl(spi, SPI_PHASE_0, SPI_PHASE_ACTION_TX, SPI_PHASE_MODE_SINGLE, transLen);
    SPI_SetPhaseData(spi, SPI_PHASE_0, txData);

    SPI_Start(spi, SPI_CTRL_PHASE_CNT1, SPI_CTRL_DMA_OFF, SPI_INTERRUPT_OFF);
    SPI_WaitForDone(spi);

    uint32_t spi_data = SPI_getRegisterId(spi, 1);
    rxData = RD_REG(spi_data) & 0xffffffff;
    
    for (int i = 0; i < transLen; i++) {
        rxBuff[i] = (rxData >> (i * 8)) & 0xFF;
    }
    return 1;
}

// 长数据传输 (>4字节) - 使用DMA
static int SPI_SendWithRecv_long(SPI_TypeDef *spi, uint8_t *txBuff, uint8_t *rxBuff, int transLen)
{
    // 对于大于4字节的数据，分块传输
    size_t remaining = transLen;
    size_t offset = 0;
    
    while (remaining > 0) {
        size_t chunkSize = (remaining > 4) ? 4 : remaining;
        uint8_t txChunk[4] = {0xFF, 0xFF, 0xFF, 0xFF};
        uint8_t rxChunk[4] = {0};
        
        if (txBuff) {
            memcpy(txChunk, txBuff + offset, chunkSize);
        }
        
        SPI_SendWithRecv_short(spi, txChunk, rxChunk, chunkSize);
        
        if (rxBuff) {
            memcpy(rxBuff + offset, rxChunk, chunkSize);
        }
        
        offset += chunkSize;
        remaining -= chunkSize;
    }
    return 1;
}

// Arduino风格函数实现
void spi_begin(SPI_TypeDef *spi)
{
    current_spi = spi;
    
    // 启用SPI外设 - 避免宏冲突
    if (spi == SPIx(0)) {
        PERIPHERAL_ENABLE_ALL(SPI, 0);
    } else {
        PERIPHERAL_ENABLE_ALL(SPI, 1);
    }
    
    // 默认设置
    uint32_t pclk_freq = SYS_GetPclkFreq();
    SPI_SclkDivTypeDef spi_sclk_div = pclk_freq > 240 ? SPI_CTRL_SCLK_DIV16 : 
                                      pclk_freq > 120 ? SPI_CTRL_SCLK_DIV8 : 
                                      SPI_CTRL_SCLK_DIV4;
    
    SPI_Init(spi, spi_sclk_div);
    
    // 默认SPI设置 (MODE0, MSBFIRST, DIV4)
    current_settings.dataMode = AG32_SPI_MODE0;
    current_settings.bitOrder = AG32_MSBFIRST;
    current_settings.clockDiv = AG32_SPI_CLOCK_DIV4;
    
    // 应用默认设置
    SPI_SetCtrlParam(spi, 0, 0); // MODE0: CPOL=0, CPHA=0
    
    spi_initialized = true;
}

void spi_end(void)
{
    if (!spi_initialized) return;
    
    // 这里可以添加SPI外设禁用代码
    spi_initialized = false;
    current_spi = NULL;
}

void spi_beginTransaction(SPISettings settings)
{
    if (!spi_initialized) return;
    
    current_settings = settings;
    
    // 根据dataMode设置CPOL和CPHA
    int cpol = (settings.dataMode == AG32_SPI_MODE2 || settings.dataMode == AG32_SPI_MODE3) ? 1 : 0;
    int cpha = (settings.dataMode == AG32_SPI_MODE1 || settings.dataMode == AG32_SPI_MODE3) ? 1 : 0;
    
    //SPI_SetCtrlParam(SPI_TypeDef *spi, int cpol, int cpha)
    SPI_SetCtrlParam(current_spi, cpol, cpha); // 先设置为MODE0
    
    // 重新初始化时钟分频
    SPI_Init(current_spi, settings.clockDiv);
}

void spi_endTransaction(void)
{
    // Arduino中endTransaction通常不做特殊处理
    // 这里保持空实现，保持兼容性
}

uint8_t spi_transfer(uint8_t data)
{
    if (!spi_initialized) return 0;
    
    uint8_t txBuff[1] = {data};
    uint8_t rxBuff[1] = {0};
    
    SPI_SendWithRecv_short(current_spi, txBuff, rxBuff, 1);
    
    return rxBuff[0];
}

uint16_t spi_transfer16(uint16_t data)
{
    if (!spi_initialized) return 0;
    
    uint8_t txBuff[2], rxBuff[2];
    
    if (current_settings.bitOrder == AG32_MSBFIRST) {
        txBuff[0] = (data >> 8) & 0xFF;  // 高字节
        txBuff[1] = data & 0xFF;         // 低字节
    } else {
        txBuff[0] = data & 0xFF;         // 低字节
        txBuff[1] = (data >> 8) & 0xFF;  // 高字节
    }
    
    SPI_SendWithRecv_short(current_spi, txBuff, rxBuff, 2);
    
    uint16_t result;
    if (current_settings.bitOrder == AG32_MSBFIRST) {
        result = (rxBuff[0] << 8) | rxBuff[1];
    } else {
        result = (rxBuff[1] << 8) | rxBuff[0];
    }
    
    return result;
}

void spi_transferBytes(uint8_t *txData, uint8_t *rxData, size_t count)
{
    if (!spi_initialized) return;
    
    if (count <= 4) {
        uint8_t txBuff[4] = {0xFF, 0xFF, 0xFF, 0xFF};
        uint8_t rxBuff[4] = {0};
        
        if (txData) {
            memcpy(txBuff, txData, count);
        }
        
        SPI_SendWithRecv_short(current_spi, txBuff, rxBuff, count);
        
        if (rxData) {
            memcpy(rxData, rxBuff, count);
        }
    } else {
        SPI_SendWithRecv_long(current_spi, txData, rxData, count);
    }
}

void spi_setBitOrder(AG32_SPIBitOrder_t bitOrder)
{
    current_settings.bitOrder = bitOrder;
}

void spi_setDataMode(AG32_SPIMode_t dataMode)
{
    if (!spi_initialized) return;
    
    current_settings.dataMode = dataMode;
    
    int cpol = (dataMode == AG32_SPI_MODE2 || dataMode == AG32_SPI_MODE3) ? 1 : 0;
    int cpha = (dataMode == AG32_SPI_MODE1 || dataMode == AG32_SPI_MODE3) ? 1 : 0;
    
    SPI_SetCtrlParam(current_spi, cpol, cpha);
}

void spi_setClockDivider(AG32_SPIClockDiv_t clockDiv)
{
    if (!spi_initialized) return;
    
    current_settings.clockDiv = clockDiv;
    SPI_Init(current_spi, clockDiv);
}

// 全局SPI实例 - Arduino风格
SPIClass_t AG32_SPI = {
    .spiPort = NULL,
    .initialized = false,
    .settings = {AG32_SPI_MODE0, AG32_MSBFIRST, AG32_SPI_CLOCK_DIV4},
    .begin = spi_begin,
    .end = spi_end,
    .beginTransaction = spi_beginTransaction,
    .endTransaction = spi_endTransaction,
    .transfer = spi_transfer,
    .transfer16 = spi_transfer16,
    .transferBytes = spi_transferBytes,
    .setBitOrder = spi_setBitOrder,
    .setDataMode = spi_setDataMode,
    .setClockDivider = spi_setClockDivider
};