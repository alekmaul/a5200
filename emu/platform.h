#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include "config.h"
#include <stdio.h>

/* This include file defines prototypes for platform-specific functions. */

void Atari_Initialise(void);
int Atari_Exit(int run_monitor);
int Atari_Keyboard(void);
void Atari_DisplayScreen(void);

unsigned int Atari_PORT(unsigned int num);
unsigned int Atari_TRIG(unsigned int num);
unsigned int Atari_POT(unsigned int num);

#endif /* _PLATFORM_H_ */
