#ifndef _sound_h
#define _sound_h

#define SOUND_SAMPLE_RATE 44100

void Sound_Initialise(void);
void Sound_Exit(void);
void Sound_Update(void);
void Sound_Pause(void);
void Sound_Continue(void);

#endif /* _sound_h */
