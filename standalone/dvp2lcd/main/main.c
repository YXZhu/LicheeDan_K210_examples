#include <stdio.h>
#include <stdlib.h>
#include "sysctl.h"
#include "plic.h"
#include "dmac.h"
#include "lcd.h"
#include "dvp.h"
#include "ov2640.h"
#include "ov5640.h"
#include "uarths.h"
#include "fpioa.h"
#include "gpiohs.h"
#include "st7789.h"
#include "sleep.h"

#define OV2640

uint8_t buf_sel;
volatile uint8_t buf_used[2];
uint32_t *lcd_buf[2];

static int dvp_irq(void *ctx)
{
	if (dvp_get_interrupt(DVP_STS_FRAME_FINISH)) {
		dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
		buf_used[buf_sel] = 1;
		buf_sel ^= 0x01;
		dvp_set_display_addr((uint32_t)lcd_buf[buf_sel]);
	} else {
		dvp_clear_interrupt(DVP_STS_FRAME_START);
		if (buf_used[buf_sel] == 0)
		{
			dvp_start_convert();
		}
	}
	return 0;
}

int main(void)
{
	uint64_t core_id = current_coreid();
	void *ptr;
	uint16_t manuf_id, device_id;

    if (core_id == 0)
    {
    	sysctl_pll_set_freq(SYSCTL_CLOCK_PLL0,400000000);
   	 	uarths_init();
   	 	printf("pll0 freq:%d\r\n",sysctl_clock_get_freq(SYSCTL_CLOCK_PLL0));

		sysctl->power_sel.power_mode_sel6 = 1;
		sysctl->power_sel.power_mode_sel7 = 1;
		sysctl->misc.spi_dvp_data_enable = 1;

		plic_init();
		//需要重新配置DMA,LCD才能使用DMA刷屏
		dmac->reset = 0x01;
		while (dmac->reset)
			;
		dmac->cfg = 0x03;
		// LCD init
		printf("LCD init\r\n");
		lcd_init();
		lcd_clear(BLUE);
		// DVP init
		printf("DVP init\r\n");

#ifdef OV2640
		do {
			printf("init ov2640\r\n");
			ov2640_init();
			ov2640_read_id(&manuf_id, &device_id);
			printf("manuf_id:0x%04x,device_id:0x%04x\r\n", manuf_id, device_id);
		} while (manuf_id != 0x7FA2 || device_id != 0x2642);
#else
		do{
			myov5640_init();
			device_id = ov5640_read_id();
			printf("ov5640 id:%d\r\n",device_id);
		}
		while (device_id!=0x5640);
#endif

		ptr = malloc(sizeof(uint8_t) * 320 * 240 * (2 * 2) + 127);
		if (ptr == NULL)
			return;

		lcd_buf[0] = (uint32_t *)(((uint32_t)ptr + 127) & 0xFFFFFF80);
		lcd_buf[1] = (uint32_t *)((uint32_t)lcd_buf[0] + 320 * 240 * 2);
		buf_used[0] = 0;
		buf_used[1] = 0;
		buf_sel = 0;

		dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
		dvp_set_display_addr((uint32_t)lcd_buf[buf_sel]);
		plic_set_priority(IRQN_DVP_INTERRUPT, 1);
		plic_irq_register(IRQN_DVP_INTERRUPT, dvp_irq, NULL);
		plic_irq_enable(IRQN_DVP_INTERRUPT);
		// system start
		printf("system start\n");
		set_csr(mstatus, MSTATUS_MIE);
		dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
		dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);

		while(1)
		{
				while(buf_used[buf_sel]==0);
				lcd_draw_picture(0, 0, 320, 240, lcd_buf[buf_sel]);
				while (tft_busy());
				buf_used[buf_sel] = 0;
		}
	}
	while(1);
}
