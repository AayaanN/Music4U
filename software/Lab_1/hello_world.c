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


static void egm_isr(void* context, alt_u32 id){
	//IOWR(EGM_BASE,0,0);

	//alt_u16 busy = IORD(EGM_BASE, 1);
	alt_8 leds = IORD(LED_PIO_BASE, 0)&0x0F;
	IOWR(LED_PIO_BASE,0,leds|0x04);
	leds = IORD(LED_PIO_BASE, 0)&0x0F;
	IOWR(LED_PIO_BASE,0,leds&0x0B);

	IOWR(RESPONSE_OUT_BASE, 0,1);
	IOWR(RESPONSE_OUT_BASE, 0, 0);

	IOWR(STIMULUS_IN_BASE,3,1);



}

int background()
{


	int j;
	int x = 0;
	int grainsize = 4;
	int g_taskProcessed = 0;

	for(j = 0; j < grainsize; j++)
	{
	g_taskProcessed++;
	}
	return x;
}

int interrupt(){
	//set the period and pulse width of the egm
	int period = 2;
	int width = 1;
	int count = 0;
	IOWR(EGM_BASE,2,period);
	IOWR(EGM_BASE,3,width);


	//register the irq
	alt_irq_register(STIMULUS_IN_IRQ, (void*)0,egm_isr);
	IOWR(STIMULUS_IN_BASE, 2, 0xFF);


	//IOWR_ALTERA_AVALON_PIO_EDGE_CAP(STIMULUS_IN_BASE,1);
	while(period <= 5000){

		alt_8 leds = IORD(LED_PIO_BASE, 0)&0x0F;
		IOWR(LED_PIO_BASE,0,leds|0x02);
		leds = IORD(LED_PIO_BASE, 0)&0x0F;
		IOWR(LED_PIO_BASE,0,leds&0x0D);
		IOWR(EGM_BASE,0,1);

		while(IORD(EGM_BASE,1)!=0){
			alt_8 leds = IORD(LED_PIO_BASE, 0)&0x0F;
			IOWR(LED_PIO_BASE,0,leds|0x01);
			background();
			leds = IORD(LED_PIO_BASE, 0)&0x0F;
			IOWR(LED_PIO_BASE,0,leds&0x0E);

			count+=1;

		}

		int latency = IORD(EGM_BASE,4);
		int missed_pulses = IORD(EGM_BASE,5);
		int multi_pulses =IORD(EGM_BASE,6);
		printf("%d,%d,%d,%d,%d,%d\n",period, width,count, latency, missed_pulses, multi_pulses);

		period += 2;
		width = period/2;

		IOWR(EGM_BASE,0,0);
		IOWR(EGM_BASE,2,period);
		IOWR(EGM_BASE,3,width);

		count=0;




	}
	return 0;
}

int tight_polling(){

	//set the period and pulse width of the egm
		int period = 2;
		int width = 1;
		int count = 0;
		IOWR(EGM_BASE,2,period);
		IOWR(EGM_BASE,3,width);

		//enable the egm
		IOWR(EGM_BASE, 0, 1);

		int prev = 0;
		int cur = 0;
		int first_cycle=1;

		//IOWR_ALTERA_AVALON_PIO_EDGE_CAP(STIMULUS_IN_BASE,1);
		while(period <= 5000){
				alt_8 leds = IORD(LED_PIO_BASE, 0)&0x0F;
				IOWR(LED_PIO_BASE,0,leds|0x02);
				leds = IORD(LED_PIO_BASE, 0)&0x0F;
				IOWR(LED_PIO_BASE,0,leds&0x0D);
				IOWR(EGM_BASE,0,1);
				int char_count = 0;
				while(IORD(EGM_BASE,1)!=0){
					if (IORD(STIMULUS_IN_BASE,0)==1){

						IOWR(RESPONSE_OUT_BASE, 0,1);
						IOWR(RESPONSE_OUT_BASE, 0, 0);

						if (first_cycle==1){

							while(first_cycle==1){
								alt_8 leds = IORD(LED_PIO_BASE, 0)&0x0F;
								IOWR(LED_PIO_BASE,0,leds|0x01);
								background();
								leds = IORD(LED_PIO_BASE, 0)&0x0F;
								IOWR(LED_PIO_BASE,0,leds&0x0E);

								char_count++;
								count++;
								prev = cur;
								cur = IORD(STIMULUS_IN_BASE,0);

								if (cur > prev){
									first_cycle=0;
								}
							}
							//printf("%d\n", char_count);
						}else{
							for (int i = 0; i < char_count-1; i++){
								alt_8 leds = IORD(LED_PIO_BASE, 0)&0x0F;
								IOWR(LED_PIO_BASE,0,leds|0x01);
								background();
								leds = IORD(LED_PIO_BASE, 0)&0x0F;
								IOWR(LED_PIO_BASE,0,leds&0x0E);
								count++;
							}

							if (IORD(STIMULUS_IN_BASE,0)==1){
								while(IORD(STIMULUS_IN_BASE,0)==1){

								}
							}
						}


					}




				}

				int latency = IORD(EGM_BASE,4);
				int missed_pulses = IORD(EGM_BASE,5);
				int multi_pulses =IORD(EGM_BASE,6);
				printf("%d,%d,%d,%d,%d,%d\n",period, width,count, latency, missed_pulses, multi_pulses);

				period += 2;
				width = period/2;

				IOWR(EGM_BASE,0,0);
				IOWR(EGM_BASE,2,period);
				IOWR(EGM_BASE,3,width);

				count=0;
				first_cycle=1;


		}
		return 0;

}

int main()
{
//	while(1){
//		alt_8 switches = IORD(SWITCH_PIO_BASE,0)&0x0F;
//		alt_8 push_buttons = IORD(BUTTON_PIO_BASE,0)&0x0F;
//
//		alt_8 result = (switches | ~(push_buttons));
//
//		IOWR(LED_PIO_BASE,0,result);
//	}

//  	alt_8 switches;
//    alt_8 push_buttons;
//    int prev = IORD(SWITCH_PIO_BASE,0)&0x0F;
//    int cur = prev;
//    switches = prev;
//    if ((switches&1)==0){
//		printf("Interrupt mode selected.\n");
//	}else{
//		printf("Tight polling mode selected.\n");
//	}
//    printf("Period, Pulse_Width, BG_Tasks Run, Latency, Missed, Multiple \n\n");
//    printf("Please, press PB0 to continue.\n\n");
//  	while(1){
//
//  		if (cur!=prev){
//			prev = cur;
//			if ((switches&1)==0){
//				printf("Interrupt mode selected.\n");
//
//			}else{
//				printf("Tight polling mode selected.\n");
//			}
//			printf("Period, Pulse_Width, BG_Tasks Run, Latency, Missed, Multiple \n\n");
//			printf("Please, press PB0 to continue.\n\n");
//		}
//
//
//  		switches = IORD(SWITCH_PIO_BASE,0)&0x0F;
//  		cur = switches;
//  		push_buttons = IORD(BUTTON_PIO_BASE,0)&0x0F;
//
//  		alt_8 result = ~(push_buttons);
//
//
//
//  		if (result&1==1){
//  			break;
//  		}
//  	}
//
//  	if ((switches&1)==0){
//		interrupt();
//		printf("Interrupt mode selected.\n\n");
//	}else{
//		tight_polling();
//		printf("Tight polling mode selected.\n\n");
//
//	}

  	alt_8 switches = IORD(SWITCH_PIO_BASE,0)&0x0F;
  	alt_8 push_buttons = IORD(BUTTON_PIO_BASE,0)&0x0F;

  	if ((switches&1)==0){
  		printf("Interrupt method selected.\n");
  	}else if ((switches&1)==1){
  		printf("Tight polling method selected.\n");
  	}

  	printf("Period, Pulse_Width, BG_Tasks Run, Latency, Missed, Multiple \n\n");
  	printf("Please, press PB0 to continue.\n");


  	while(1){
  		push_buttons = IORD(BUTTON_PIO_BASE,0)&0x0F;
  		alt_8 result = ~(push_buttons);
		if (result&1==1){
			break;
		}
  	}

  	if ((switches&1)==0){
  		interrupt();
  	}else{
  		tight_polling();

  	}



	return 0;
}





