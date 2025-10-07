#include "example.h"

// #include "i2c_lcd1602.h"

// #include "Wire.h"

#include "LCD1602_I2C.h"

#include "AG32_SPI.h"

extern int TestSpiAdvanced();


extern int TestSpiComm();


const unsigned short mexhat_tl[256] = {
    1221,1219,1217,1215,1213,1211,1208,1205,1202,1199,1195,1191,1187,1182,
    1177,1172,1166,1160,1153,1145,1137,1129,1119,1109,1099,1088,1076,1063,
    1049,1035,1020,1003,986,968,950,930,909,887,865,841,817,791,765,738,
    710,681,651,621,590,559,527,495,463,430,398,365,333,302,270,240,210,
    182,155,129,105,83,63,46,31,18,9,3,0,1,5,14,27,44,65,91,122,158,198,
    244,294,350,411,476,547,622,703,787,877,970,1068,1169,1274,1382,1493,
    1606,1722,1839,1958,2077,2197,2316,2436,2554,2670,2785,2897,3006,3111,
    3213,3310,3402,3489,3571,3646,3715,3777,3831,3879,3919,3951,3976,3992,
    4000,4000,3992,3976,3951,3919,3879,3831,3777,3715,3646,3571,3489,3402,
    3310,3213,3111,3006,2897,2785,2670,2554,2436,2316,2197,2077,1958,1839,
    1722,1606,1493,1382,1274,1169,1068,970,877,787,703,622,547,476,411,350,
    294,244,198,158,122,91,65,44,27,14,5,1,0,3,9,18,31,46,63,83,105,129,155,
    182,210,240,270,302,333,365,398,430,463,495,527,559,590,621,651,681,710,
    738,765,791,817,841,865,887,909,930,950,968,986,1003,1020,1035,1049,1063,
    1076,1088,1099,1109,1119,1129,1137,1145,1153,1160,1166,1172,1177,1182,1187,
    1191,1195,1199,1202,1205,1208,1211,1213,1215,1217,1219,1221
};


void Button_isr(void)
{
  if (button_isr_cb) {
    button_isr_cb();
  }
  UTIL_IdleMs(400); // to debounce
  GPIO_ClearInt(BUT_GPIO, BUT_GPIO_BITS);
}

void MTIMER_isr(void)
{
  GPIO_Toggle(EXT_GPIO, EXT_GPIO_BITS);
  INT_SetMtime(0);
}

void TestMtimer(int ms)
{
  clint_isr[IRQ_M_TIMER] = MTIMER_isr;
  INT_SetMtime(0);
  INT_SetMtimeCmp(SYS_GetSysClkFreq() / 1000 * ms);
  INT_EnableIntTimer();
  while (1);
}



void DAC8830(void) {
    printf("Arduino SPI Example Started\n");
    
    AG32_SPI.begin(SPI0);
    
  
    printf("\n=== SPI ===\n");

    SPISettings settings = SPISettings_Create( AG32_SPI_MODE0, AG32_MSBFIRST, AG32_SPI_CLOCK_DIV64 );
    
    AG32_SPI.beginTransaction(settings); 


    while(1)
    {
      for (int i = 1; i < 256; i++)
      {
          AG32_SPI.transfer16((int)(mexhat_tl[i]*16));

      }
    }

    
    AG32_SPI.endTransaction();

    AG32_SPI.end();
}



// void DAC8830_writeDAC(SPI_TypeDef *spi, uint16_t value)
// {
//   SPI_Send(spi, 2, (value >> 8) | (value << 8));
// }



int main(void)
{
  // This will init clock and uart on the board
  board_init();
  
  // The default isr table is plic_isr. The default entries in the table are peripheral name based like CAN0_isr() or
  // GPIO0_isr(), and can be re-assigned.
  plic_isr[BUT_GPIO_IRQ] = Button_isr;
  // Any interrupt priority needs to be greater than MIN_IRQ_PRIORITY to be effective
  INT_SetIRQThreshold(MIN_IRQ_PRIORITY);
  // Enable interrupt from BUT_GPIO
  INT_EnableIRQ(BUT_GPIO_IRQ, PLIC_MAX_PRIORITY);



  DAC8830(); 
  

}
