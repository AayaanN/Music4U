/*
 * "Hello World" example.
 *
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example
 * designs. It runs with or without the MicroC/OS-II RTOS and requires a STDOUT
 * device in your system's hardware.
 * The memory footprint of this hosted application is ~69 kbytes by default
 * using the standard reference design.
 *
 * For a reduced footprint version of this template, and an explanation of how
 * to reduce the memory footprint for a given application, see the
 * "small_hello_world" template.
 *
 */

#include "system.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include <stdio.h>
#include "alt_types.h"

alt_8 button_val;
int button_pressed =-1;

static void time_out_isr(void* context, alt_u32 id){
	alt_8 cur = IORD(BUTTON_PIO_BASE,0)&0x0F;
	//printf("button was pressed4\n");
	if (cur == 0x0F){
//		if (button_val == 0x0F){
//		}
//		else if (button_val != 0xF){
//			alt_8 leds = IORD(LED_PIO_BASE, 0)&0x0F;
//			IOWR(LED_PIO_BASE,0,leds&0x0B);
//		}
		alt_8 leds = IORD(LED_PIO_BASE, 0)&0x0F;
		IOWR(LED_PIO_BASE,0,leds&0x0B);

		if (button_pressed==0){
			printf("button 0 was pressed\n");
		}
		if (button_pressed==1){
			printf("button 1 was pressed\n");
		}
		if (button_pressed==2){
			printf("button 2 was pressed\n");
		}
		if (button_pressed==3){
			printf("button 3 was pressed\n");
		}
		button_pressed = 0;
		IOWR(BUTTON_PIO_BASE,3,0);
		IOWR(BUTTON_PIO_BASE, 2, 0xFF);
		IOWR(TIMER_0_BASE, 0, 0);
		IOWR(TIMER_0_BASE, 1, 9);
	}
	else{
		//printf("setting another timer\n");
		IOWR(TIMER_0_BASE,1,0x5);

	}



}

static void button_isr(void* context, alt_u32 id){
	alt_8 leds = IORD(LED_PIO_BASE, 0)&0x0F;
	IOWR(LED_PIO_BASE,0,leds|0x04);

	//printf("button was pressed6\n");
	IOWR(BUTTON_PIO_BASE, 2, 0x0);
	button_val = IORD(BUTTON_PIO_BASE,0)&0x0F;
	if (button_val == 14){
		button_pressed = 0;
	}else if (button_val == 13){
		button_pressed = 1;
	}else if (button_val == 11){
		button_pressed = 2;
	}else if (button_val == 7){
		button_pressed = 3;
	}
	IOWR(TIMER_0_BASE,1,0x5);
	//IOWR(BUTTON_PIO_BASE,3,0);


}


int main()
{
	alt_irq_register(BUTTON_PIO_IRQ, (void*)0,button_isr);
	alt_irq_register(TIMER_0_IRQ, (void*)0,time_out_isr);

	//SETTING TIMER VAL
	IOWR(TIMER_0_BASE,2,0XFFFF);
	IOWR(TIMER_0_BASE,3,1);
	IOWR(TIMER_0_BASE,1,0x8);
	IOWR(TIMER_0_BASE,0,0);



	//ENABLE AND SET IRQ
	IOWR(BUTTON_PIO_BASE,3,0);
	IOWR(BUTTON_PIO_BASE, 2, 0xFF);


  return 0;
}

