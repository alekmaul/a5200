/* $Id: atari_vga.c,v 1.1 2001/10/26 05:36:48 fox Exp $ */
/* -------------------------------------------------------------------------- */

/*
 * DJGPP - VGA Backend for David Firth's Atari 800 emulator
 *
 * by Ivo van Poorten (C)1996  See file COPYRIGHT for policy.
 *
 */

/* -------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

//ALEK #include "main.h"

#include "config.h"
#include "cpu.h"
#include "input.h"
#include "sound.h"
#include "pcjoy.h"
#include "rt-config.h"  /* for refresh_rate */
#include "screen.h"

unsigned int joy_5200_trig[4]  = {0};
unsigned int joy_5200_stick[4] = {0};
unsigned int joy_5200_pot[8]   = {0};

/* -------------------------------------------------------------------------- */
/* CONFIG & INITIALISATION                                                    */
/* -------------------------------------------------------------------------- */
void Atari_Initialise(void)
{
   unsigned i;

#ifdef SOUND
   /* initialise sound routines */
   Sound_Initialise();
#endif

   for (i = 0; i < 4; i++)
   {
      joy_5200_trig[i]  = 1;
      joy_5200_stick[i] = STICK_CENTRE;
      atari_analog[i]   = 0;
   }

   for (i = 0; i < 8; i++)
      joy_5200_pot[i] = JOY_5200_CENTER;

   key_consol = CONSOL_NONE;
}

/* -------------------------------------------------------------------------- */
/* ATARI EXIT                                                                 */
/* -------------------------------------------------------------------------- */
int Atari_Exit(int run_monitor)
{
#ifdef SOUND
   Sound_Exit();
#endif
   return 0;
}

/* -------------------------------------------------------------------------- */
unsigned int Atari_PORT(unsigned int num)
{
   /* num is port index
    * > port 0: controller 0, 1
    * > port 1: controller 2, 3 */
   switch (num)
   {
      case 0:
         return (joy_5200_stick[1] << 4) | joy_5200_stick[0];
      case 1:
         return (joy_5200_stick[3] << 4) | joy_5200_stick[2];
      default:
         break;
   }

   return (STICK_CENTRE << 4) | STICK_CENTRE;
}

/* -------------------------------------------------------------------------- */
unsigned int Atari_TRIG(unsigned int num)
{
   /* num is controller index */
   if (num < 4)
      return joy_5200_trig[num];

   return 1;
}

/* -------------------------------------------------------------------------- */
unsigned int Atari_POT(unsigned int num)
{
   /* num is analog axis index
    * > 0, 1 - controller 1: x, y
    * > 2, 3 - controller 2: x, y
    * > 4, 5 - controller 3: x, y
    * > 6, 7 - controller 4: x, y */
   if (num < 8)
      return joy_5200_pot[num];

   return JOY_5200_CENTER;
}
