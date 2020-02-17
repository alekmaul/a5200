#ifndef SHARED_H
#define SHARED_H

//#include <stdbool.h>
//#include <malloc.h>
//#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

#include <SDL/SDL.h>

// defines and macros
#define MAX__PATH 1024
#define FILE_LIST_ROWS 19

#define SYSVID_WIDTH	320
#define SYSVID_HEIGHT	240

#define GF_GAMEINIT    1
#define GF_MAINUI      2
#define GF_GAMEQUIT    3
#define GF_GAMERUNNING 4

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define PIX_TO_RGB(fmt, r, g, b) (((r>>fmt->Rloss)<<fmt->Rshift)| ((g>>fmt->Gloss)<<fmt->Gshift)|((b>>fmt->Bloss)<<fmt->Bshift))

// a5200 dependencies
#include "atari.h"
#include "global.h"
#include "cartridge.h"
#include "input.h"
#include "pia.h"

extern UBYTE *cart_image ;		/* For cartridge memory */

#define cartridge_IsLoaded() (cart_image != NULL)

typedef struct {
  unsigned int sndLevel;
  unsigned int m_ScreenRatio; // 0 = original show, 1 = full screen
  unsigned int OD_Joy[12]; // each key mapping
  unsigned int m_DisplayFPS;
  unsigned int m_Analog;
  char current_dir_rom[MAX__PATH];
  unsigned int m_Scanline;
} gamecfg;

#define true 1
#define false 0

extern SDL_Surface* screen;						// Main program screen
extern SDL_Surface* actualScreen;						// Main program screen

extern SDL_Surface *layer,*layerback,*layerbackgrey;//,*layericons;

extern SDL_Event event;

extern SDL_mutex *sndlock;

extern gamecfg GameConf;

extern char gameName[512];
extern char current_conf_app[MAX__PATH];

extern unsigned char vidBuf[512*512];

extern unsigned char *filebuffer;

extern unsigned int gameCRC;

extern void system_loadcfg(char *cfg_name);
extern void system_savecfg(char *cfg_name);
extern void gethomedir(char *dir, char* name);

extern unsigned int atari_analog;

extern unsigned long crc32 (unsigned int crc, const unsigned char *buf, unsigned int len);

extern int atari_waitoncardtype(unsigned long crcfile);

// menu
extern void screen_showtopmenu(void);
extern void print_string_video(int x, int y, const char *s);

#endif
