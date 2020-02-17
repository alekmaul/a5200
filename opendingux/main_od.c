#include  <sys/time.h>

#include "shared.h"

unsigned int m_Flag;
unsigned int interval;

unsigned int gameCRC;
gamecfg GameConf;
char gameName[512];
char current_conf_app[MAX__PATH];
char current_bios_app[MAX__PATH];
unsigned char vidBuf[512*512];

unsigned char *filebuffer;

unsigned long nextTick, lastTick = 0, newTick, currentTick, wait;
int FPS = 60; 
int pastFPS = 0; 

SDL_Surface *layer,*layerback,*layerbackgrey;//,*layericons;
SDL_Surface *actualScreen, *screen;
SDL_Event event;
SDL_Joystick *stick = NULL;
#define JOYSTICK_AXIS 8192

SDL_mutex *sndlock;

unsigned int atari_analog = 0;
unsigned short atari_pal16[256];

unsigned long SDL_UXTimerRead(void) {
	struct timeval tval; // timing
  
  	gettimeofday(&tval, 0);
	return (((tval.tv_sec*1000000) + (tval.tv_usec )));
}

void graphics_scanline(unsigned short y, unsigned short width, unsigned short *buf) {
	#define SCANLINE_INTENSITY 0x18E3
	unsigned int x;
	unsigned short *pScr=buf;
	
    if (y & 1) {
		for (x=0;x<width;x++)  {
			*pScr -= (*pScr>>3) & SCANLINE_INTENSITY; 
			pScr++;
		}
	}
    else {
		for (x=0;x<width;++x) {
			*pScr += (~*pScr>>3) & SCANLINE_INTENSITY; 
			pScr++;
		}
	}
}

void graphics_paint(void) {
	unsigned short *buffer_scr = (unsigned short *) actualScreen->pixels;
	unsigned char *buffer_flip = (unsigned char *) &vidBuf;
	unsigned int W,H,ix,iy,x,y;
	static char buffer[32];

	if(SDL_MUSTLOCK(actualScreen)) SDL_LockSurface(actualScreen);
	
	x=0;
	y=0; 
	W=320;
	H=240;
	ix=(SYSVID_WIDTH<<16)/W;
	iy=(SYSVID_HEIGHT<<16)/H;

	do   {
		unsigned char *buffer_mem=&vidBuf[(y>>16)*512]+32;
		W=320; x=0;
		do {
			*buffer_scr++=atari_pal16[buffer_mem[x>>16]];
			x+=ix;
		} while (--W);
		if (GameConf.m_Scanline) graphics_scanline(y>>16,320,buffer_scr-320);
		y+=iy;
	} while (--H);

	pastFPS++;
	newTick = SDL_UXTimerRead();
	if ((newTick-lastTick)>1000000) {
		FPS = pastFPS;
		pastFPS = 0;
		lastTick = newTick;
	}

	if (GameConf.m_DisplayFPS) {
		sprintf(buffer,"%02d",FPS);
		print_string_video(300,1,buffer);
	}
		
	if (SDL_MUSTLOCK(actualScreen)) SDL_UnlockSurface(actualScreen);
	SDL_Flip(actualScreen);
}


void audio_callback(void *userdata, Uint8 *stream, int len) {
	SDL_mutexP(sndlock);

	Pokey_process(stream,len);
	
	SDL_mutexV(sndlock);
}

void initSDL(void) {
    SDL_AudioSpec spec, retSpec;
	
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());
		exit(1);
	}
	atexit(SDL_Quit);

	actualScreen = SDL_SetVideoMode(320, 240, 16, SDL_DOUBLEBUF | SDL_HWSURFACE );
	if(actualScreen == NULL) {
		fprintf(stderr, "Couldn't set video mode: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_ShowCursor(SDL_DISABLE);

    screen = SDL_CreateRGBSurface (actualScreen->flags,
                                actualScreen->w,
                                actualScreen->h,
                                actualScreen->format->BitsPerPixel,
                                actualScreen->format->Rmask,
                                actualScreen->format->Gmask,
                                actualScreen->format->Bmask,
                                actualScreen->format->Amask);
								
	if(screen == NULL) {
		fprintf(stderr, "Couldn't create surface: %s\n", SDL_GetError());
		exit(1);
	}

	// Init new layer to add background and text
	layer = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0,0,0,0);
	if(layer == NULL) {
		fprintf(stderr, "Couldn't create surface: %s\n", SDL_GetError());
		exit(1);
	}
	layerback = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0,0,0,0);
	if(layerback == NULL) {
		fprintf(stderr, "Couldn't create surface: %s\n", SDL_GetError());
		exit(1);
	}
	layerbackgrey = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 240, 16, 0,0,0,0);
	if(layerbackgrey == NULL) {
		fprintf(stderr, "Couldn't create surface: %s\n", SDL_GetError());
		exit(1);
	}
	
	// Init sound
	spec.freq = 44100;
	spec.format = AUDIO_U8;
	spec.channels = 1;
	spec.samples = 1024;
	spec.callback = audio_callback;
	spec.userdata = NULL;

    // Open the audio device and start playing sound! 
    if ( SDL_OpenAudio(&spec, &retSpec) < 0 ) {
        fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
        exit(1);
    }

	sndlock = SDL_CreateMutex();
	if (sndlock == NULL) {
		fprintf(stderr, "Unable to create lock: %s\n", SDL_GetError());
		SDL_CloseAudio();
        exit(1);
	}
	
	//nit joystick
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	SDL_JoystickEventState(SDL_ENABLE);
	stick = SDL_JoystickOpen(0);
}

const unsigned int palette_ntsc[256] = {
  0x000000, 0x1c1c1c, 0x393939, 0x595959, 
  0x797979, 0x929292, 0xababab, 0xbcbcbc, 
  0xcdcdcd, 0xd9d9d9, 0xe6e6e6, 0xececec, 
  0xf2f2f2, 0xf8f8f8, 0xffffff, 0xffffff, 
  0x391701, 0x5e2304, 0x833008, 0xa54716, 
  0xc85f24, 0xe37820, 0xff911d, 0xffab1d, 
  0xffc51d, 0xffce34, 0xffd84c, 0xffe651, 
  0xfff456, 0xfff977, 0xffff98, 0xffff98, 
  0x451904, 0x721e11, 0x9f241e, 0xb33a20, 
  0xc85122, 0xe36920, 0xff811e, 0xff8c25, 
  0xff982c, 0xffae38, 0xffc545, 0xffc559, 
  0xffc66d, 0xffd587, 0xffe4a1, 0xffe4a1, 
  0x4a1704, 0x7e1a0d, 0xb21d17, 0xc82119, 
  0xdf251c, 0xec3b38, 0xfa5255, 0xfc6161, 
  0xff706e, 0xff7f7e, 0xff8f8f, 0xff9d9e, 
  0xffabad, 0xffb9bd, 0xffc7ce, 0xffc7ce, 
  0x050568, 0x3b136d, 0x712272, 0x8b2a8c, 
  0xa532a6, 0xb938ba, 0xcd3ecf, 0xdb47dd, 
  0xea51eb, 0xf45ff5, 0xfe6dff, 0xfe7afd, 
  0xff87fb, 0xff95fd, 0xffa4ff, 0xffa4ff, 
  0x280479, 0x400984, 0x590f90, 0x70249d, 
  0x8839aa, 0xa441c3, 0xc04adc, 0xd054ed, 
  0xe05eff, 0xe96dff, 0xf27cff, 0xf88aff, 
  0xff98ff, 0xfea1ff, 0xfeabff, 0xfeabff, 
  0x35088a, 0x420aad, 0x500cd0, 0x6428d0, 
  0x7945d0, 0x8d4bd4, 0xa251d9, 0xb058ec, 
  0xbe60ff, 0xc56bff, 0xcc77ff, 0xd183ff, 
  0xd790ff, 0xdb9dff, 0xdfaaff, 0xdfaaff, 
  0x051e81, 0x0626a5, 0x082fca, 0x263dd4, 
  0x444cde, 0x4f5aee, 0x5a68ff, 0x6575ff, 
  0x7183ff, 0x8091ff, 0x90a0ff, 0x97a9ff, 
  0x9fb2ff, 0xafbeff, 0xc0cbff, 0xc0cbff, 
  0x0c048b, 0x2218a0, 0x382db5, 0x483ec7, 
  0x584fda, 0x6159ec, 0x6b64ff, 0x7a74ff, 
  0x8a84ff, 0x918eff, 0x9998ff, 0xa5a3ff, 
  0xb1aeff, 0xb8b8ff, 0xc0c2ff, 0xc0c2ff, 
  0x1d295a, 0x1d3876, 0x1d4892, 0x1c5cac, 
  0x1c71c6, 0x3286cf, 0x489bd9, 0x4ea8ec, 
  0x55b6ff, 0x70c7ff, 0x8cd8ff, 0x93dbff, 
  0x9bdfff, 0xafe4ff, 0xc3e9ff, 0xc3e9ff, 
  0x2f4302, 0x395202, 0x446103, 0x417a12, 
  0x3e9421, 0x4a9f2e, 0x57ab3b, 0x5cbd55, 
  0x61d070, 0x69e27a, 0x72f584, 0x7cfa8d, 
  0x87ff97, 0x9affa6, 0xadffb6, 0xadffb6, 
  0x0a4108, 0x0d540a, 0x10680d, 0x137d0f, 
  0x169212, 0x19a514, 0x1cb917, 0x1ec919, 
  0x21d91b, 0x47e42d, 0x6ef040, 0x78f74d, 
  0x83ff5b, 0x9aff7a, 0xb2ff9a, 0xb2ff9a, 
  0x04410b, 0x05530e, 0x066611, 0x077714, 
  0x088817, 0x099b1a, 0x0baf1d, 0x48c41f, 
  0x86d922, 0x8fe924, 0x99f927, 0xa8fc41, 
  0xb7ff5b, 0xc9ff6e, 0xdcff81, 0xdcff81, 
  0x02350f, 0x073f15, 0x0c4a1c, 0x2d5f1e, 
  0x4f7420, 0x598324, 0x649228, 0x82a12e, 
  0xa1b034, 0xa9c13a, 0xb2d241, 0xc4d945, 
  0xd6e149, 0xe4f04e, 0xf2ff53, 0xf2ff53, 
  0x263001, 0x243803, 0x234005, 0x51541b, 
  0x806931, 0x978135, 0xaf993a, 0xc2a73e, 
  0xd5b543, 0xdbc03d, 0xe1cb38, 0xe2d836, 
  0xe3e534, 0xeff258, 0xfbff7d, 0xfbff7d, 
  0x401a02, 0x581f05, 0x702408, 0x8d3a13, 
  0xab511f, 0xb56427, 0xbf7730, 0xd0853a, 
  0xe19344, 0xeda04e, 0xf9ad58, 0xfcb75c, 
  0xffc160, 0xffc671, 0xffcb83, 0xffcb83
};

/*
#  
bd52623b  Defender # A5200.A52
ce07d9ad  Diagnostic Cart.bin
6a687f9c  Dig Dug # A5200.A52
d3bd3221  Final Legacy (Prototype) # A5200.A52
d3bd3221  FinalLegacy.bin
04b299a4  Frisky Tom (Prototype) # A5200.A52
0af19345  Frogger 2 - Threedeep! # A5200.A52
1062ef6a  Frogger.bin
0fe438b3  frogger2.bin
04b299a4  frskytom.bin
97b15243  Gorf.bin
cfd4a7f9  Gyruss # A5200.A52
cfd4a7f9  Gyruss.bin
18a73af3  H.E.R.O. # A5200.A52
18a73af3  H.E.R.O. (1984) (Activision).a52
18a73af3  Hero.a52
d9ae4518  James Bond 007 # A5200.A52
d9ae4518  JamesBond007.bin
bfd30c01  Joust # A5200.A52
7c30592c  Jr Pac-Man # A5200.A52
2c676662  Jungle Hunt # A5200.A52
ecfa624f  Kangaroo # A5200.A52
02cdfc70  KrazyShootOut.bin
83517703  Last Starfighter # A5200.A52
84df4925  Looney Tunes Hotel (Prototype # A5200.A52
ab8e035b  Meteorites # A5200.A52
931a454a  MICRGAMN.BIN
931a454a  Microgammon SB (Prototype) # A5200.A52
969cfe1a  Millipede # A5200.A52
7df1adfb  Miner 2049 # A5200.A52
c597c087  Miniature Golf # A5200.A52
c597c087  MINIGOLF.BIN
2a640143  Montezuma's Revenge # A5200.A52
d0b2f285  Moon Patrol # A5200.A52
457fb9b3  Mr. Do's Castle.bin
752f5efd  Ms. Pac-Man # A5200.A52
59983c40  pacjr52.bin
8873ef51  Pac-Man # A5200.A52
e9f826bd  pete.bin
4b910461  Pitfall 2 - the Lost Caverns # A5200.A52
4b910461  Pitfall II - The Lost Caverns (1984) (Activision).a52
abc2d1e4  Pole Position # A5200.A52
abc2d1e4  PolePosition.bin
a18a9a40  Popeye # A5200.A52
3c33f26e  Qbert.bin
aea6d2c2  QIX # A5200.A52
b5f3402b  Quest for Quintana Roo # A5200.A52
4336c2cc  Realsports Football # A5200.A52
ecbd1853  Realsports Soccer # A5200.A52
10f33c90  Realsports Tennis # A5200.A52
0f996184  RealsportsBasketballRev1.bin
4336c2cc  RealsportsFootball.bin
10f33c90  RealsportsTennis.bin
a97606ab  Roadrunner # A5200.A52
4252abd9  Robotron 2084 # A5200.A52
b68d61e8  Space Dungeon # A5200.A52
387365dc  Space Shuttle - a Journey Into Space # A5200.A52
b68d61e8  SpaceDungeon.bin
73b5b6fb  Sport Goofy (Prototype) # A5200.A52
7d819a9f  Star Raiders # A5200.A52
69f23548  Star Trek - Strategic Operations Simulator # A5200.A52
75f566df  Star Wars - the Arcade Game # A5200.A52
1d1cee27  Stargate # A5200.A52
75f566df  StarWarsArcade.bin
fd8f0cd4  Super Pac Man Final (5200).bin
0a4ddb1e  Super Pac-Man (Prototype) # A5200.A52
1187342f  Tempest (Prototype) # A5200.A52
1187342f  Tempst52.bin
0ba22ece  Track and Field # A5200.A52
d6f7ddfd  Wizard of Wor# A5200.A52
b8faaec3  Xari Arena (Prototype) # A5200.A52
b8faaec3  xari52.bin
12cc298f  yellwsub.bin
2959d827  Zone Ranger # A5200.A52
*/

int cartfind16kmapping ( unsigned int crc ) {
	if ( crc == 0x35484751 ||     /* AE                */
       crc == 0x9bae58dc ||     /* Beamrider         */
	     crc == 0xbe3cd348 ||     /* Berzerk           */
       crc == 0xc8f9c094 ||     /* Blaster           */
	     crc == 0x0624E6E7 ||     /* BluePrint         */
       
       
	     crc == 0x9ad53bbc ||     /* ChopLifter        */
	     crc == 0xf43e7cd0 ||     /* Decathlon         */
	     crc == 0xd3bd3221 ||     /* Final Legacy      */
	     crc == 0x18a73af3 ||     /* H.E.R.O           */
	     crc == 0x83517703 ||     /* Last StarFigtr    */
	     crc == 0xab8e035b ||     /* Meteorites        */
	     crc == 0x969cfe1a ||     /* Millipede         */
	     crc == 0x7df1adfb ||     /* Miner 2049er      */
	     crc == 0xb8b6a2fd ||     /* Missle Command+   */
	     crc == 0xd0b2f285 ||     /* Moon Patrol       */
	     crc == 0xe8b130c4 ||     /* PAM Diags2.0      */
	     crc == 0x4b910461 ||     /* Pitfall II        */
	     crc == 0x47dc1314 ||     /* Preppie (Conv)    */
	     crc == 0xF1E21530 ||     /* Preppie (Conv)    */
	     crc == 0xb5f3402b ||     /* Quest Quintana    */
	     crc == 0x4252abd9 ||     /* Robotron 2084     */
	     crc == 0x387365dc ||     /* Space Shuttle     */
	     crc == 0x82E2981F ||     /* Super Pacman      */
	     crc == 0xFD8F0CD4 ||     /* Super Pacman      */
	     crc == 0xa4ddb1e  ||     /* Super Pacman      */
	     crc == 0xe80dbb2  ||     /* Time Runner (Conv) */
	     crc == 0x0ba22ece ||     /* Track and Field   */
	     crc == 0xd6f7ddfd ||     /* Wizard of Wor     */
	     crc == 0x2959d827 ||     /* Zone Ranger       */
	     crc == 0xB8FAAEC3 ||     /* Xari arena        */
	     crc == 0x38F4A6A4    ) { /* Xmas (Demo)       */
		return 2;
	}
	else {
//         crc == 0x8d2aaab5 || // asteroid.bin
//         crc == 0x4019ecec || // Astro Chase # A5200.A52
//         crc == 0xb3b8e314 || // Battlezone
//	       crc == 0x04807705 || // Buck Rogers - Planet of Zoom # A5200.A52
//	       crc == 7a9d9f85  boogie.bin
//	       crc == 536a70fe  Centipede # A5200.A52
//	       crc == 536a70fe  Centipede.bin
//	       crc == 82b91800  Congo Bongo # A5200.A52
//	       crc == 82b91800  CongoBongo.bin
//	       crc == fd541c80  Countermeasure # A5200.A52

//         crc == 0x1187342f || // Tempest           
		return 1;
  }
} // cart_find_16k_mapping 

int load_os(char *filename ) {
  FILE *romfile = fopen(filename, "rb");
  if (romfile == NULL) return 1;
  
  fread(atari_os, 0x800, 1, romfile);
  fclose(romfile);
  
 	return 0;
} /* end load_os */

int atari_waitoncardtype(unsigned long crcfile) {
	int bRet=1;
	unsigned int posdeb=2;

	posdeb = cartfind16kmapping(crcfile) == 1 ? 18 : 2;

	bRet = (posdeb==2 ? 2 : 1);

	return bRet;
}

int atari_init(char *filename) {
	unsigned int buffer_size=0, index;
  
	// Free buffer if needed
	if (filebuffer != 0)
		free(filebuffer);

    // load card game if ok
    if (Atari800_OpenFile(filename, true, 1, true) != AFILE_ERROR) {	
		// Init palette
		for(index = 0; index < 256; index++) {
			unsigned char r, g, b;
			r = (unsigned char) ((palette_ntsc[index]& 0x00ff0000) >> 16);
			g = (unsigned char)  ((palette_ntsc[index]& 0x0000ff00) >> 8);
			b = (unsigned char)  ((palette_ntsc[index]& 0x000000ff) );
			atari_pal16[index] = PIX_TO_RGB(actualScreen->format,r,g,b);
		}
		return 1;
	}
	
	return 0;
}

extern unsigned int trig0;
extern unsigned int stick0;

int main(int argc, char *argv[]) {
	unsigned int index;
	double period;
	unsigned int ksel = 0, kx = 0, ky = 0, key_x=0, key_y=0,key_l=0,key_se=0,key_r=0;
	int shiftctrl;
	
	// Get init file directory & name
	gethomedir(current_conf_app, "a5200");
	sprintf(current_conf_app,"%s//a5200-od.cfg",current_conf_app);
	
	// Init graphics & sound
	initSDL();

	atari_analog = 0;
	
	m_Flag = GF_MAINUI;
	system_loadcfg(current_conf_app);

	SDL_WM_SetCaption("a5200-od", NULL);

	gethomedir(current_bios_app, "");
	sprintf(current_bios_app,"%s//5200.rom",current_bios_app);
	if (load_os(current_bios_app)) {
		fprintf(stderr,"can't load atari 5200 bios\n");fflush(stderr);
		exit(0);
	}
	
    //load rom file via args if a rom path is supplied
	if(argc > 1) {
		strcpy(gameName,argv[1]);
		m_Flag = GF_GAMEINIT;
	}

	while (m_Flag != GF_GAMEQUIT) {
		SDL_PollEvent(&event);
		unsigned char *keys = SDL_GetKeyState(NULL);

		switch (m_Flag) {
			case GF_MAINUI:
				SDL_PauseAudio(1);
				screen_showtopmenu();
				if (cartridge_IsLoaded()) {
					// let's go for sound :)
					SDL_PauseAudio(0);
					nextTick = SDL_UXTimerRead() + interval;
				}
				break;

			case GF_GAMEINIT:
				if (atari_init(gameName)) {
					m_Flag = GF_GAMERUNNING;
					Atari800_Initialise();

					// Init timing
					period = 1.0 / 60;
					period = period * 1000000;
					interval = (int) period;
					nextTick = SDL_UXTimerRead() + interval;
					SDL_PauseAudio(0);
				}
				break;
		
			case GF_GAMERUNNING:
				currentTick = SDL_UXTimerRead(); 
				wait = (nextTick - currentTick);
				if (wait > 0) {
					if (wait < 1000000) 
						usleep(wait);
				}
				
				// Execute one frame
				Atari800_Frame(0);

				// Draw frame
				graphics_paint();

				// Wait for keys
				key_consol = CONSOL_NONE; //|= (CONSOL_OPTION | CONSOL_SELECT | CONSOL_START); /* OPTION/START/SELECT key OFF */
				shiftctrl = 0; key_shift = 0;
				stick0 = STICK_CENTRE;

				trig0 = (keys[SDLK_LCTRL] == SDL_PRESSED) ? 0 : 1; 	
				if (keys[SDLK_LALT] == SDL_PRESSED) { shiftctrl ^= AKEY_SHFT; key_shift = 1; } // B 
				key_code = shiftctrl ? 0x40 : 0x00;

				if (keys[SDLK_UP] == SDL_PRESSED) stick0 = STICK_FORWARD;
				if (keys[SDLK_LEFT] == SDL_PRESSED) stick0 = STICK_LEFT;
				if (keys[SDLK_RIGHT] == SDL_PRESSED) stick0 = STICK_RIGHT;
				if (keys[SDLK_DOWN] == SDL_PRESSED) stick0 = STICK_BACK;
				if ((keys[SDLK_UP] == SDL_PRESSED) && (keys[SDLK_LEFT] == SDL_PRESSED)) stick0 = STICK_UL;
				if ((keys[SDLK_UP] == SDL_PRESSED) && (keys[SDLK_RIGHT] == SDL_PRESSED)) stick0 = STICK_UR;
				if ((keys[SDLK_DOWN] == SDL_PRESSED) && (keys[SDLK_LEFT] == SDL_PRESSED)) stick0 = STICK_LL;
				if ((keys[SDLK_DOWN] == SDL_PRESSED) && (keys[SDLK_RIGHT] == SDL_PRESSED)) stick0 = STICK_LR;

				if (keys[SDLK_SPACE] == SDL_PRESSED) key_code = AKEY_5200_ASTERISK; // X
				if (keys[SDLK_LSHIFT] == SDL_PRESSED) key_code = AKEY_5200_HASH;     // Y
				if (keys[SDLK_BACKSPACE] == SDL_PRESSED) key_code = AKEY_5200_0 + key_code; // R
				if (keys[SDLK_TAB] == SDL_PRESSED) key_code = AKEY_5200_1 + key_code; // L
				
				if (stick != NULL) {
					short xaxis=SDL_JoystickGetAxis(stick, 0); short yaxis=SDL_JoystickGetAxis(stick, 1);
					if(xaxis < -JOYSTICK_AXIS) { // Left
						stick0 = STICK_LEFT;
					} 
					else if(xaxis > JOYSTICK_AXIS) { // Right 
						stick0 = STICK_RIGHT;
					}
					if(yaxis < -JOYSTICK_AXIS) { // Up 
						stick0 = STICK_FORWARD;
						if(xaxis < -JOYSTICK_AXIS) stick0 = STICK_UL;
						else if(xaxis > JOYSTICK_AXIS) stick0 = STICK_UR;
					} 
					else if(yaxis > JOYSTICK_AXIS) { // Down 
						stick0 = STICK_BACK;
						if(xaxis < -JOYSTICK_AXIS) stick0 = STICK_LL;
						else if(xaxis > JOYSTICK_AXIS) stick0 = STICK_LR;
					}
				}
				
				if ((keys[SDLK_ESCAPE] == SDL_PRESSED) && (keys[SDLK_RETURN] == SDL_PRESSED )) { 
					m_Flag = GF_MAINUI;
				}
				else if ( (keys[SDLK_ESCAPE] == SDL_PRESSED) )  { // SELECT
					key_code = AKEY_5200_PAUSE + key_code;
				} 
				else if ( (keys[SDLK_RETURN] == SDL_PRESSED) ) {  // START
					key_code =  AKEY_5200_START + key_code;
				} 
				else { key_se =0; }

				nextTick += interval;
				break;
		}
	}

	SDL_PauseAudio(1);
	SDL_DestroyMutex(sndlock);

	// Free memory
	//SDL_FreeSurface(layericons);
	SDL_FreeSurface(layerbackgrey);
	SDL_FreeSurface(layerback);
	SDL_FreeSurface(layer);
	SDL_FreeSurface(screen);
	SDL_FreeSurface(actualScreen);
	
	// Free memory
	SDL_QuitSubSystem(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_JOYSTICK);
	
	exit(0);
}


#define DO1(buf) crc = crc_table[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);
// Table of CRC-32's of all single-byte values (made by make_crc_table)
unsigned int crc_table[256] = {
  0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
  0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
  0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
  0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
  0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
  0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
  0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
  0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
  0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
  0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
  0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
  0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
  0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
  0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
  0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
  0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
  0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
  0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
  0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
  0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
  0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
  0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
  0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
  0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
  0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
  0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
  0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
  0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
  0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
  0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
  0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
  0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
  0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
  0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
  0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
  0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
  0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
  0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
  0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
  0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
  0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
  0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
  0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
  0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
  0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
  0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
  0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
  0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
  0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
  0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
  0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
  0x2d02ef8dL
};

unsigned long crc32 (unsigned int crc, const unsigned char *buf, unsigned int len) {
  if (buf == 0) return 0L;
  crc = crc ^ 0xffffffffL;
  while (len >= 8) {
    DO8(buf);
    len -= 8;
  }
  if (len) do {
    DO1(buf);
  } while (--len);
  return crc ^ 0xffffffffL;
}
