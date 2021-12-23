/* $Id: sound_dos.c,v 1.2 2001/04/08 06:01:02 knik Exp $ */
//ALEK #include "main.h"
#include "config.h"

#ifdef SOUND

#include "sound.h"
#include "pokeysnd.h"

void Sound_Initialise(void) {
#ifdef STEREO_SOUND
  Pokey_sound_init(FREQ_17_EXACT, SOUND_SAMPLE_RATE, 2, 0);
#else
  Pokey_sound_init(FREQ_17_EXACT, SOUND_SAMPLE_RATE, 1, 0); //SND_BIT16);
#endif
}

void Sound_Pause(void) {
}

void Sound_Continue(void) {
}

void Sound_Update(void) {
}

void Sound_Exit(void) {
}

#endif /* SOUND */
