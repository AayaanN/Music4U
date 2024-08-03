/*=========================================================================*/
/*  Includes                                                               */
/*=========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <system.h>
#include <sys/alt_alarm.h>
#include <io.h>

#include "fatfs.h"
#include "diskio.h"

#include "ff.h"
#include "monitor.h"
#include "uart.h"

#include "alt_types.h"

#include <altera_up_avalon_audio.h>
#include <altera_up_avalon_audio_and_video_config.h>

#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"


/*=========================================================================*/
/*  DEFINE: All Structures and Common Constants                            */
/*=========================================================================*/

/*=========================================================================*/
/*  DEFINE: Macros                                                         */
/*=========================================================================*/

#define PSTR(_a)  _a

/*=========================================================================*/
/*  DEFINE: Prototypes                                                     */
/*=========================================================================*/

/*=========================================================================*/
/*  DEFINE: Definition of all local Data                                   */
/*=========================================================================*/
static alt_alarm alarm;
static unsigned long Systick = 0;
static volatile unsigned short Timer;   /* 1000Hz increment timer */

/*=========================================================================*/
/*  DEFINE: Definition of all local Procedures                             */
/*=========================================================================*/

/***************************************************************************/
/*  TimerFunction                                                          */
/*                                                                         */
/*  This timer function will provide a 10ms timer and                      */
/*  call ffs_DiskIOTimerproc.                                              */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: none                                                           */
/***************************************************************************/
static alt_u32 TimerFunction (void *context)
{
   static unsigned short wTimer10ms = 0;

   (void)context;

   Systick++;
   wTimer10ms++;
   Timer++; /* Performance counter for this module */

   if (wTimer10ms == 10)
   {
      wTimer10ms = 0;
      ffs_DiskIOTimerproc();  /* Drive timer procedure of low level disk I/O module */
   }

   return(1);
} /* TimerFunction */

/***************************************************************************/
/*  IoInit                                                                 */
/*                                                                         */
/*  Init the hardware like GPIO, UART, and more...                         */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: none                                                           */
/***************************************************************************/
static void IoInit(void)
{
   uart0_init(115200);

   /* Init diskio interface */
   ffs_DiskIOInit();

   //SetHighSpeed();

   /* Init timer system */
   alt_alarm_start(&alarm, 1, &TimerFunction, NULL);

} /* IoInit */

/*=========================================================================*/
/*  DEFINE: All code exported                                              */
/*=========================================================================*/

uint32_t acc_size;                 /* Work register for fs command */
uint16_t acc_files, acc_dirs;
FILINFO Finfo;
#if _USE_LFN
char Lfname[512];
#endif

char Line[256];                 /* Console input buffer */

#define BUFF_SIZE 512

FATFS Fatfs[_VOLUMES];          /* File system object for each logical drive */
FIL File1, File2;               /* File objects */
DIR Dir;                        /* Directory object */
uint8_t Buff[BUFF_SIZE] __attribute__ ((aligned(4)));  /* Working buffer */


#define ESC 27
#define CLEAR_LCD_STRING "[2J"

static
FRESULT scan_files(char *path)
{
    DIR dirs;
    FRESULT res;
    uint8_t i;
    char *fn;


    if ((res = f_opendir(&dirs, path)) == FR_OK) {
        i = (uint8_t)strlen(path);
        while (((res = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0]) {
            if (_FS_RPATH && Finfo.fname[0] == '.')
                continue;
#if _USE_LFN
            fn = *Finfo.lfname ? Finfo.lfname : Finfo.fname;
#else
            fn = Finfo.fname;
#endif
            if (Finfo.fattrib & AM_DIR) {
                acc_dirs++;
                *(path + i) = '/';
                strcpy(path + i + 1, fn);
                res = scan_files(path);
                *(path + i) = '\0';
                if (res != FR_OK)
                    break;
            } else {
                //      xprintf("%s/%s\n", path, fn);
                acc_files++;
                acc_size += Finfo.fsize;
            }
        }
    }

    return res;
}


//                put_rc(f_mount((uint8_t) p1, &Fatfs[p1]));

static
void put_rc(FRESULT rc)
{
    const char *str =
        "OK\0" "DISK_ERR\0" "INT_ERR\0" "NOT_READY\0" "NO_FILE\0" "NO_PATH\0"
        "INVALID_NAME\0" "DENIED\0" "EXIST\0" "INVALID_OBJECT\0" "WRITE_PROTECTED\0"
        "INVALID_DRIVE\0" "NOT_ENABLED\0" "NO_FILE_SYSTEM\0" "MKFS_ABORTED\0" "TIMEOUT\0"
        "LOCKED\0" "NOT_ENOUGH_CORE\0" "TOO_MANY_OPEN_FILES\0";
    FRESULT i;

    for (i = 0; i != rc && *str; i++) {
        while (*str++);
    }
    xprintf("rc=%u FR_%s\n", (uint32_t) rc, str);
}





char fl_char_array[20][20];
unsigned long fl_long_array[20];
int count = 0;
int cur_song = 0;
char* mode_displayed;
alt_8 button_val;
int button_pressed =-1;
int reset = 0;
int paused_stopped_state = 0; //0 for stop 1 for pause -> records whether the device was last paused or stopped
int play_pause_state = 0; //0 for pause 1 for play ->records the state of play or pause

FILE *lcd;

int mode = 0;
/*
 * 0 = stopped
 * 1 = paused
 * 2 = normal
 * 3 = half speed
 * 4 = double speed
 * 5 = mono
 */

void mode_handler(){

	alt_8 switches = IORD(SWITCH_PIO_BASE,0)&0x03;

	if (switches == 3){
		mode_displayed = "PBACK-NORM SPD";
	}else if (switches == 2){
		mode_displayed = "PBACK-HALF SPD";
	}else if (switches == 1){
		mode_displayed = "PBACK-DBL SPD";
	}else if (switches == 0){
		mode_displayed = "PBACK-MONO";
	}

	if (paused_stopped_state == 0 && play_pause_state == 0){
		mode_displayed = "STOPPED";
	}

	else if (paused_stopped_state == 1 && play_pause_state == 0){
		mode_displayed = "PAUSED";
	}

	fprintf(lcd, "%c%s", ESC, CLEAR_LCD_STRING);
	fprintf(lcd,"%d. %s\n",cur_song+1, fl_char_array[cur_song]);
	fprintf(lcd, "%s\n", mode_displayed);
}

void di(){
	disk_initialize(0);
}

void fi(){
	f_mount(0, &Fatfs[0]);
}

void initialize(){
	di();
	fi();
}

void fo(char *filename){
	f_open(&File1, filename, 1);
}

int isWav(char *filename){
	f_open(&File1, filename, 1);

	int bytes_read;
	char buffer[12];
	f_read(&File1, buffer ,12, (unsigned int *)&bytes_read);

	int res = memcmp(&buffer[8], (char*)"WAVE", 4);

	return (res==0);
}


void stop(){
	reset = 1;
	play_pause_state = 0;
	paused_stopped_state = 0;
	mode_handler();
}

void next(){
	int next = cur_song + 1;

	if (next >= count){
		next = 0;
	}

	cur_song = next;

	if (play_pause_state == 0){
		stop();
	}
	mode_handler();

}



void prev(){
	int prev = cur_song - 1;
	if (prev < 0){
		prev = count -1;
	}

	cur_song = prev;

	if (play_pause_state == 0){
		stop();
	}
	mode_handler();
}


void pause_play(){
	printf("%d\n", play_pause_state);
	printf("%d\n", paused_stopped_state);
	if (play_pause_state == 0){
		play_pause_state = 1; //was paused, now playing
		if (paused_stopped_state == 0){ //if was stopped, replay the song
			reset=0;

		}


	}else{
		play_pause_state = 0; //was playing now paused
		paused_stopped_state = 1;


	}
	printf("%d\n", play_pause_state);
	printf("%d\n", paused_stopped_state);

	mode_handler();
}


void fp(long p1){
	//fo(fl_char_array[cur_song]);
	printf("ran\n");
	put_rc(f_open(&File1, fl_char_array[cur_song], 1));
	uint32_t ofs = File1.fptr;
	uint32_t cnt;
	uint8_t res;

	alt_up_audio_dev * audio_dev;
	audio_dev = alt_up_audio_open_dev ("/dev/Audio");

	alt_8 switches = IORD(SWITCH_PIO_BASE,0)&0x03;
	//alt_8 push_buttons = IORD(BUTTON_PIO_BASE,0)&0x0F;
	mode_handler();


	int temp_cur_song=cur_song;

	while (p1)
	{


		if ((uint32_t) p1 >= BUFF_SIZE)
		{
			cnt = BUFF_SIZE;
			p1 -= BUFF_SIZE;
		}
		else
		{
			cnt = p1;
			p1 = 0;
		}
		res = f_read(&File1, Buff, cnt, &cnt);

		//put_rc(res);

		if ((IORD(SWITCH_PIO_BASE,0)&0x03)==3){
			for (int i = 0; i < cnt; i+=4){

				while(play_pause_state == 0){
					if (reset==1){
						return;
					}
				}
				if (cur_song != temp_cur_song){
					return;
				}
				while (alt_up_audio_write_fifo_space (audio_dev, ALT_UP_AUDIO_RIGHT) < 2 && alt_up_audio_write_fifo_space (audio_dev, ALT_UP_AUDIO_LEFT) < 2){}
				unsigned int temp_buff_l = Buff[i+1]<<8 | Buff[i];
				unsigned int temp_buff_r = Buff[i+3]<<8 | Buff[i+2];
				alt_up_audio_write_fifo (audio_dev, (&temp_buff_l), 1, ALT_UP_AUDIO_LEFT);
				alt_up_audio_write_fifo (audio_dev, (&temp_buff_r), 1, ALT_UP_AUDIO_RIGHT);

			}
		}else if ((IORD(SWITCH_PIO_BASE,0)&0x03)==2){
			for (int i = 0; i < cnt; i+=4){
				while(play_pause_state == 0){
					if (reset==1){
						return;
					}
				}
				if (cur_song != temp_cur_song){
					return;
				}
				while (alt_up_audio_write_fifo_space (audio_dev, ALT_UP_AUDIO_RIGHT) < 2 && alt_up_audio_write_fifo_space (audio_dev, ALT_UP_AUDIO_LEFT) < 2){}
				unsigned int temp_buff_l = Buff[i+1]<<8 | Buff[i];
				unsigned int temp_buff_r = Buff[i+3]<<8 | Buff[i+2];
				alt_up_audio_write_fifo (audio_dev, (&temp_buff_l), 1, ALT_UP_AUDIO_LEFT);
				alt_up_audio_write_fifo (audio_dev, (&temp_buff_r), 1, ALT_UP_AUDIO_RIGHT);
				//for (int j = 0; j < 20; j++){}
				temp_buff_l = Buff[i+1]<<8 | Buff[i];
				temp_buff_r = Buff[i+3]<<8 | Buff[i+2];
				//while (alt_up_audio_write_fifo_space (audio_dev, ALT_UP_AUDIO_RIGHT) < 2 && alt_up_audio_write_fifo_space (audio_dev, ALT_UP_AUDIO_LEFT) < 2){}
				alt_up_audio_write_fifo (audio_dev, (&temp_buff_l), 1, ALT_UP_AUDIO_LEFT);
				alt_up_audio_write_fifo (audio_dev, (&temp_buff_r), 1, ALT_UP_AUDIO_RIGHT);
				//for (int j = 0; j < 20; j++){}

			}
		}else if ((IORD(SWITCH_PIO_BASE,0)&0x03)==1){
			for (int i = 0; i < cnt; i+=8){
				while(play_pause_state == 0){
					if (reset==1){
						return;
					}
				}
				if (cur_song != temp_cur_song){
					return;
				}
				while (alt_up_audio_write_fifo_space (audio_dev, ALT_UP_AUDIO_RIGHT) < 2 && alt_up_audio_write_fifo_space (audio_dev, ALT_UP_AUDIO_LEFT) < 2){}
				unsigned int temp_buff_l = Buff[i+1]<<8 | Buff[i];
				unsigned int temp_buff_r = Buff[i+3]<<8 | Buff[i+2];
				alt_up_audio_write_fifo (audio_dev, (&temp_buff_l), 1, ALT_UP_AUDIO_LEFT);
				alt_up_audio_write_fifo (audio_dev, (&temp_buff_r), 1, ALT_UP_AUDIO_RIGHT);



			}
		}else if ((IORD(SWITCH_PIO_BASE,0)&0x03)==0){
			for (int i = 0; i < cnt; i+=4){
				while(play_pause_state == 0){
					if (reset==1){
						return;
					}
				}
				if (cur_song != temp_cur_song){
					return;
				}
				while (alt_up_audio_write_fifo_space (audio_dev, ALT_UP_AUDIO_RIGHT) < 2 && alt_up_audio_write_fifo_space (audio_dev, ALT_UP_AUDIO_LEFT) < 2){}
				unsigned int temp_buff_l = Buff[i+1]<<8 | Buff[i];
				unsigned int temp_buff_r = Buff[i+3]<<8 | Buff[i+2];
				alt_up_audio_write_fifo (audio_dev, (&temp_buff_l), 1, ALT_UP_AUDIO_LEFT);
				alt_up_audio_write_fifo (audio_dev, (&temp_buff_l), 1, ALT_UP_AUDIO_RIGHT);


			}
		}



	}
	printf("got here\n");
	stop();

}




void fl_check(FATFS *fs){
	initialize();
	uint8_t res = f_opendir(&Dir, NULL);
	if (res) // if res in non-zero there is an error; print the error.
	{
		printf("error\n");
		put_rc(res);
		return;
	}
	long p1;
	uint32_t s1;
	uint32_t s2;
	count = 0;
	p1 = s1 = s2 = 0; // otherwise initialize the pointers and proceed.
	for (;;)
	{
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0]) {
			break;
		}

		if (Finfo.fattrib & AM_DIR)
		{
			s2++;
		}
		else
		{
			s1++;
			p1 += Finfo.fsize;
		}

		if (isWav((char*)&(Finfo.fname[0]))){
			strcpy(fl_char_array[count], (char*)&(Finfo.fname[0]));
			fl_long_array[count] = Finfo.fsize;

			count++;
		}


#if _USE_LFN
		for (int p2 = strlen(Finfo.fname); p2 < 14; p2++)
			xputc(' ');
		xprintf("%s\n", Lfname);
#else


#endif
	}



	res = f_getfree(0, (uint32_t *) & p1, &fs);
	if (res == FR_OK){}

	else{}
		//put_rc(res);
}






static void time_out_isr(void* context, alt_u32 id){
//	alt_8 cur = IORD(BUTTON_PIO_BASE,0)&0x0F;
//	if (cur == 0x0F){
//		alt_8 leds = IORD(LED_PIO_BASE, 0)&0x0F;
//		IOWR(LED_PIO_BASE,0,leds&0x0B);
//
//		if (button_pressed==0){
//			next();
//		}
//		if (button_pressed==1){
//			pause_play();
//		}
//		if (button_pressed==2){
//			stop();
//		}
//		if (button_pressed==3){
//			prev();
//		}
//		button_pressed = -1;
//		IOWR(BUTTON_PIO_BASE,3,0);
//		IOWR(BUTTON_PIO_BASE, 2, 0xFF);
//		IOWR(TIMER_0_BASE, 0, 0);
//		IOWR(TIMER_0_BASE, 1, 9);
//	}
//	else{
//		IOWR(TIMER_0_BASE,1,0x5);
//
//	}



	if ((IORD(BUTTON_PIO_BASE,0)&0x0F)!= 0x0F){
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
		IOWR(BUTTON_PIO_BASE,3,0);
		IOWR(BUTTON_PIO_BASE, 2, 0xFF);
		IOWR(TIMER_0_BASE, 0, 0);
		IOWR(TIMER_0_BASE, 1, 9);
	}else{
		alt_8 leds = IORD(LED_PIO_BASE, 0)&0x0F;
		IOWR(LED_PIO_BASE,0,leds&0x0B);

		if (button_pressed==0){
			next();
		}
		if (button_pressed==1){
			printf("playing\n");
			pause_play();
		}
		if (button_pressed==2){
			stop();
		}
		if (button_pressed==3){
			prev();
		}
		button_pressed = -1;
		IOWR(BUTTON_PIO_BASE,3,0);
		IOWR(BUTTON_PIO_BASE, 2, 0xFF);
		IOWR(TIMER_0_BASE, 0, 0);
		IOWR(TIMER_0_BASE, 1, 9);

	}




}


static void button_isr(void* context, alt_u32 id){
	alt_8 leds = IORD(LED_PIO_BASE, 0)&0x0F;
	IOWR(LED_PIO_BASE,0,leds|0x04);

	IOWR(BUTTON_PIO_BASE, 2, 0x0);
//	button_val = IORD(BUTTON_PIO_BASE,0)&0x0F;
//	if (button_val == 14){
//		button_pressed = 0;
//	}else if (button_val == 13){
//		button_pressed = 1;
//	}else if (button_val == 11){
//		button_pressed = 2;
//	}else if (button_val == 7){
//		button_pressed = 3;
//	}
	IOWR(TIMER_0_BASE,1,0x5);


}

/***************************************************************************/
/*  main                                                                   */
/***************************************************************************/
int main(void)
{

	alt_irq_register(BUTTON_PIO_IRQ, (void*)0,button_isr);
	alt_irq_register(TIMER_0_IRQ, (void*)0,time_out_isr);

	//SETTING TIMER VAL
	IOWR(TIMER_0_BASE,2,0XFFFF);
	IOWR(TIMER_0_BASE,3,0x3);
	IOWR(TIMER_0_BASE,1,0x8);
	IOWR(TIMER_0_BASE,0,0);

	//ENABLE AND SET IRQ
	IOWR(BUTTON_PIO_BASE,3,0);
	IOWR(BUTTON_PIO_BASE, 2, 0xFF);

	int fifospace;
    char *ptr, *ptr2;
    long p1, p2, p3;
    uint8_t res, b1, drv = 0;
    uint16_t w1;
    uint32_t s1, s2, cnt, blen = sizeof(Buff);
    static const uint8_t ft[] = { 0, 12, 16, 32 };
    uint32_t ofs = 0, sect = 0, blk[2];
    FATFS *fs;                  /* Pointer to file system object */

    alt_up_audio_dev * audio_dev;

    // open the Audio port
    audio_dev = alt_up_audio_open_dev ("/dev/Audio");
    if ( audio_dev == NULL)
    alt_printf ("Error: could not open audio device \n");
    else
    alt_printf ("Opened audio device \n");

    IoInit();

    IOWR(SEVEN_SEG_PIO_BASE,1,0x0007);



#if _USE_LFN
    Finfo.lfname = Lfname;
    Finfo.lfsize = sizeof(Lfname);
#endif

    fl_check(fs);
	lcd =fopen("/dev/lcd_display", "w");

	if (lcd != NULL){
		fprintf(lcd,"%d. %s\n",cur_song+1, fl_char_array[cur_song]);
		fprintf(lcd, "STOPPED\n");
	}

	while(1){
		if (play_pause_state == 1){
			fp(fl_long_array[cur_song]);
		}
	}

    return (0);
	}
