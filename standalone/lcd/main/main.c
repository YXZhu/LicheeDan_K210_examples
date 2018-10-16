#include <stdio.h>
#include <string.h>
// #include "common.h"
#include "sysctl.h"
#include "fpioa.h"
#include "dmac.h"
#include "plic.h"
#include "lcd.h"
#include "img.h"
#include "st7789.h"
#include "sleep.h"
#include "uarths.h"

int main(void)
{
    uint64_t core_id = current_coreid();
	uint16_t cnt=0;

    if (core_id == 0)
    {
    	sysctl_pll_set_freq(SYSCTL_CLOCK_PLL0,400000000);
   	 	uarths_init();
   	 	printf("pll0 freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL0));
		//需要重新配置DMA,LCD才能使用DMA刷屏
		dmac->reset = 0x01;
		while (dmac->reset)
			;
		dmac->cfg = 0x03;
		printf("lcd test\n");
		lcd_init();
		lcd_clear(BLUE);
		while(1)
		{
			lcd_draw_picture(0,0,320,240,(uint32_t*)&gImage_1);
			while (tft_busy())
			{
				printf("lcd busy\n");
			}
			sleep(2);
			lcd_draw_picture(0,0,320,240,(uint32_t*)&gImage_2);
			while (tft_busy())
			{
				printf("lcd busy\n");
			}
			sleep(2);
			printf("draw pic over\n");
		}
	}
	else
	{
		while(1)
		{

		}
	}
	return 0;
}
