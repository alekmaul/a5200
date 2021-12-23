#ifndef _SCREEN_H_
#define _SCREEN_H_

#include "atari.h"  /* UBYTE */

#define ATARI_VISIBLE_WIDTH 336
#define ATARI_LEFT_MARGIN 24

#ifdef DIRTYRECT
#ifndef CLIENTUPDATE
extern UBYTE *screen_dirty;
#endif
#endif
void entire_screen_dirty(void);

extern UWORD *atari_screen;

void Screen_Initialise(unsigned char *buf);

#endif /* _SCREEN_H_ */
