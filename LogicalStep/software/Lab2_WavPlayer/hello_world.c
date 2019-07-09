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
#include "altera_avalon_timer_regs.h"
#include "altera_avalon_timer.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"

char songList[20][20];
ulong songSize[20];
int songIndex[20];
volatile int action;
volatile int trackNumber;
volatile int playListSize;

char *playmode = "STOPPED";
char *display_name = "PLAYBACK-NORMAL SPEED";

uint32_t acc_size; /* Work register for fs command */
uint16_t acc_files, acc_dirs;
FILINFO Finfo;
#if _USE_LFN
char Lfname[512];
#endif
char Line[256]; /* Console input buffer */
FATFS Fatfs[_VOLUMES]; /* File system object for each logical drive */
FIL File1, File2; /* File objects */
DIR Dir; /* Directory object */
uint8_t Buff[512] __attribute__((aligned(4))); /* Working buffer */


static int count;
int mono = 0;
int adder = 4;

static void button_irq(void *context, alt_u32 id) {

	IOWR(BUTTON_PIO_BASE, 2, 0x0);

	IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_BASE, 0x0005);

	while (IORD_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE) != 1) {
	}

	IOWR(BUTTON_PIO_BASE, 2, 0xF);
	IOWR(BUTTON_PIO_BASE, 3, 0x0);
}

static void timer_irq(void *context, alt_u32 id) {

	while (IORD_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE) != 1) {
	}

	IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE, 0x0);

	int buttons = IORD(BUTTON_PIO_BASE, 0x0);

	if (buttons == 0xE) {
		action = 1;
	}
	if (buttons == 0xD) {
		action = 2;
	}
	if (buttons == 0xB) {
		action = 3;
	}
	if (buttons == 0x7) {
		action = 4;
	}
}

//Determine if the file is a .wav
int isWav(char *filename) {
	char *dot = strrchr(filename, '.');

	if (!strcmp(dot + 1, "WAV")) {
		return 1;
	}
	return 0;
}

//Printing Song Name and Play-mode to LCD
static void printLCD(char *filename, char *playmode) {
	FILE *lcd;
	lcd = fopen("/dev/lcd_display", "w");

	/* Write some simple text to the LCD. */
	if (lcd != NULL) {
		fprintf(lcd, "\n%d:%s \n", songIndex[trackNumber], filename);
		fprintf(lcd, "%s \n", playmode);
	}
	fclose(lcd);

	return;
}

//Play the song and Play Mode
static void play() {
	uint8_t res;
	alt_up_audio_dev *audio_dev;
	/* used for audio record/playback */
	unsigned int l_buf;
	unsigned int r_buf;
	uint32_t s2, cnt, blen = sizeof(Buff);
	// open the Audio port
	audio_dev = alt_up_audio_open_dev("/dev/Audio");

	int i;
	playmode = "PLAYING";
	int p1;

	p1 = songSize[trackNumber];
	f_open(&File1, &songList[trackNumber], 1);
	printf("%s : %d \n", &songList[trackNumber], p1);
	action = 0;

// Play Mode
	while (p1) {
		//Next Track
		if (action == 1) {
			trackNumber++;
			if (trackNumber > playListSize) {
				trackNumber = 0;
			}
			action = 0;
			printLCD(&songList[trackNumber], playmode);
			return;
			//Paused - enters Paused Mode state machine
		} else if (action == 2) {
			if (!strcmp(playmode, "PLAYING")) {
				action = 0;
				playmode = "PAUSED";
				printLCD(&songList[trackNumber], playmode);

				//The Paused Mode
				while (1) {
					// Next Track and Back to Stop Mode
					if (action == 1) {
						trackNumber++;
						if (trackNumber > playListSize) {
							trackNumber = 0;
						}
						action = 0;
						playmode = "STOPPED";

						printLCD(&songList[trackNumber], "STOPPED");
						return;
						//Resume
					} else if (action == 2) {
						playmode = "PLAYING";
						action = 0;
						printLCD(&songList[trackNumber], display_name);
						break;
						//Stopped and Back to Stop Mode
					} else if (action == 3) {
						playmode = "STOPPED";

						action = 0;
						printLCD(&songList[trackNumber], playmode);
						return;
						//Previous Track and Back to Stop Mode
					} else if (action == 4) {
						trackNumber--;
						if (trackNumber < 0) {
							trackNumber = playListSize;
						}
						action = 0;
						playmode = "STOPPED";
						printLCD(&songList[trackNumber], playmode);
						return;
					}
				};
				//End of Pause Mode
			}
			//Stopped
		} else if (action == 3) {
			action = 0;
			playmode = "STOPPED";
			printLCD(songList[trackNumber], playmode);
			return;
			//Previous Track
		} else if (action == 4) {
			trackNumber--;
			if (trackNumber < 0) {
				trackNumber = playListSize;
			}
			action = 0;
			printLCD(songList[trackNumber], playmode);
			return;
		}
//Phase 2 File Player
		if ((uint32_t) p1 >= blen) {
			cnt = blen;
			p1 -= blen;
		} else {
			cnt = p1;
			p1 = 0;
		}
		res = f_read(&File1, Buff, cnt, &s2);

		for (i = 0; i < cnt; i += adder) {
			l_buf = Buff[i + 1] << 8 | Buff[i];
			r_buf = Buff[i + 3] << 8 | Buff[i + 2];

			if (mono == 1) {
				r_buf = l_buf;
			}

			while (alt_up_audio_write_fifo_space(audio_dev,
			ALT_UP_AUDIO_RIGHT) < 1)
				;

			alt_up_audio_write_fifo(audio_dev, &(r_buf), 1,
			ALT_UP_AUDIO_RIGHT);
			alt_up_audio_write_fifo(audio_dev, &(l_buf), 1,
			ALT_UP_AUDIO_LEFT);
		}

		if (res != FR_OK) {
			break;
		}
	}

	trackNumber++;
	playmode = "STOPPED";
	printLCD(songList[trackNumber], playmode);
}

//Locate the files on the SD Card and storing values to songIndex, songList, songSize
static void findFiles() {
	char *ptr;
	long p1;
	uint32_t s1, s2, cnt, blen = sizeof(Buff);

	count = 0;
	uint8_t res;

	p1 = s1 = s2 = 0; // otherwise initialize the pointers and proceed.
	int counter = 0;
	for (;;) {
		res = f_readdir(&Dir, &Finfo);
		if ((res != FR_OK) || !Finfo.fname[0])
			break;
		if (Finfo.fattrib & AM_DIR) {
			s2++;
		} else {
			s1++;
			p1 += Finfo.fsize;
		}

		counter++;
		if (isWav(&(Finfo.fname[0])) == 1) {
			strcpy(songList[count], &Finfo.fname[0]);
			songSize[count] = Finfo.fsize;
			songIndex[count] = counter;
			count++;
		}
	}
}

// Determine the states of the switches - DBL | HALF | MONO | NORM
static void checkState() {
	int switch_state = IORD(SWITCH_PIO_BASE, 0) & 3;

	switch (switch_state) {
	//Normal Speed
	case 0:
		adder = 4;
		mono = 0;
		display_name = "PLAYBACK-NORMAL SPEED";
		break;
		//Half Speed
	case 1:
		adder = 2;
		mono = 0;
		display_name = "PLAYBACK - HALF SPD";
		break;
		//Double Speed
	case 2:
		adder = 8;
		mono = 0;
		display_name = "PLAYBACK - DBL SPD";
		break;
		//Mono
	case 3:
		mono = 1;
		adder = 4;
		display_name = "PLAYBACK - MONO-LEFT_Audio";
		break;
	default:
		break;
	}

}

int main() {
	trackNumber = 0;
	char *ptr;
	long p1;
	uint8_t res = 0;
	uint32_t s1, s2, cnt, blen = sizeof(Buff);
	FATFS *fs; /* Pointer to file system object */
	alt_up_audio_dev *audio_dev;

	// open the Audio port
	audio_dev = alt_up_audio_open_dev("/dev/Audio");

	alt_irq_register(BUTTON_PIO_IRQ, (void *) 0, button_irq);
	alt_irq_register(TIMER_0_IRQ, (void *) 0, timer_irq);

	//code starts here
	disk_initialize((uint8_t )0);
	f_mount((uint8_t) 0, &Fatfs[0]);
	f_opendir(&Dir, 0);

	//Find files on SD Card
	findFiles();

	playListSize = count - 1;
	action = 0;

	//Initialized the timer settings
	IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_0_BASE, 0x0003);
	IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_0_BASE, 0x0000);
	IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_0_BASE, 6000);
	IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_0_BASE, 0);

	IOWR(BUTTON_PIO_BASE, 2, 0x0F);
	IOWR(BUTTON_PIO_BASE, 3, 0x0);

	printLCD(songList[trackNumber], playmode);

	while (1) { //STOP MODE

		checkState();

		switch (action) {
		//NEXT
		case 1:
			trackNumber++;
			if (trackNumber > playListSize) {
				trackNumber = 0;
			}
			action = 0;
			printLCD(songList[trackNumber], playmode);
			break;

			//PLAY
		case 2:
			playmode = "PLAYING";
			printLCD(songList[trackNumber], playmode);
			action = 0;
			break;

			//STOP
		case 3:
			action = 0;
			playmode = "STOPPED";
			printLCD(songList[trackNumber], playmode);
			break;

			//PREVIOUS
		case 4:
			trackNumber--;
			if (trackNumber < 0) {
				trackNumber = playListSize;
			}
			printLCD(songList[trackNumber], playmode);
			action = 0;
			break;

		default:
			if (playmode == "PLAYING") {
				printLCD(songList[trackNumber], display_name);
				play();
				if (trackNumber < 0) {
					trackNumber = playListSize;
				} else if (trackNumber > playListSize) {
					trackNumber = 0;
				}
				break;
			}
			break;
		}
	}
	return 0;
}
