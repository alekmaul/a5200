/*
 * cartridge.c - cartridge emulation
 *
 * Copyright (C) 2001-2003 Piotr Fusik
 * Copyright (C) 2001-2005 Atari800 development team (see DOC/CREDITS)
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <encodings/crc32.h>

#include "atari.h"
#include "binload.h" /* loading_basic */
#include "cartridge.h"
#include "memory.h"
#include "pia.h"
#include "rtime8.h"
#include "util.h"
#ifndef BASIC
#include "statesav.h"
#endif

/*
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

static int cart_find_16k_mapping(uint32_t crc)
{
	if (crc == 0x35484751 || /* AE                */
		 crc == 0x9bae58dc || /* Beamrider         */
		 crc == 0xbe3cd348 || /* Berzerk           */
		 crc == 0xc8f9c094 || /* Blaster           */
		 crc == 0x0624E6E7 || /* BluePrint         */
		 crc == 0x9ad53bbc || /* ChopLifter        */
		 crc == 0xf43e7cd0 || /* Decathlon         */
		 crc == 0xd3bd3221 || /* Final Legacy      */
		 crc == 0x18a73af3 || /* H.E.R.O           */
		 crc == 0x83517703 || /* Last StarFigtr    */
		 crc == 0xab8e035b || /* Meteorites        */
		 crc == 0x969cfe1a || /* Millipede         */
		 crc == 0x7df1adfb || /* Miner 2049er      */
		 crc == 0xb8b6a2fd || /* Missle Command+   */
		 crc == 0xd0b2f285 || /* Moon Patrol       */
		 crc == 0xe8b130c4 || /* PAM Diags2.0      */
		 crc == 0x4b910461 || /* Pitfall II        */
		 crc == 0x47dc1314 || /* Preppie (Conv)    */
		 crc == 0xF1E21530 || /* Preppie (Conv)    */
		 crc == 0xb5f3402b || /* Quest Quintana    */
		 crc == 0x4252abd9 || /* Robotron 2084     */
		 crc == 0x387365dc || /* Space Shuttle     */
		 crc == 0x82E2981F || /* Super Pacman      */
		 crc == 0xFD8F0CD4 || /* Super Pacman      */
		 crc == 0xa4ddb1e  || /* Super Pacman      */
		 crc == 0xe80dbb2  || /* Time Runner (Conv) */
		 crc == 0x0ba22ece || /* Track and Field   */
		 crc == 0xd6f7ddfd || /* Wizard of Wor     */
		 crc == 0x2959d827 || /* Zone Ranger       */
		 crc == 0xB8FAAEC3 || /* Xari arena        */
		 crc == 0x38F4A6A4   )/* Xmas (Demo)       */
		return 2;

	/* crc == 0x8d2aaab5 asteroid.bin */
	/* crc == 0x4019ecec Astro Chase # A5200.A52 */
	/* crc == 0xb3b8e314 Battlezone */
	/* crc == 0x04807705 Buck Rogers - Planet of Zoom # A5200.A52 */
	/* crc == 0x7a9d9f85 boogie.bin */
	/* crc == 0x536a70fe Centipede # A5200.A52 */
	/* crc == 0x536a70fe Centipede.bin */
	/* crc == 0x82b91800 Congo Bongo # A5200.A52 */
	/* crc == 0x82b91800 CongoBongo.bin */
	/* crc == 0xfd541c80 Countermeasure # A5200.A52 */
	/* crc == 0x1187342f Tempest  */
	return 1;
}

static int cart_wait_on_type(uint32_t crc)
{
	int bRet=1;
	unsigned int posdeb=2;

	posdeb = cart_find_16k_mapping(crc) == 1 ? 18 : 2;

	bRet = (posdeb == 2 ? 2 : 1);

	return bRet;
}

int cart_kb[CART_LAST_SUPPORTED + 1] = {
	    0,
	    0,//8,  /* CART_STD_8 */
	    0,//16, /* CART_STD_16 */
	    0,//16, /* CART_OSS_16 */
	32, /* CART_5200_32 */
	    0,//32, /* CART_DB_32 */
	16, /* CART_5200_EE_16 */
	40, /* CART_5200_40 */
	    0,//64, /* CART_WILL_64 */
	    0,//64, /* CART_EXP_64 */
	    0,//64, /* CART_DIAMOND_64 */
	    0,//64, /* CART_SDX */
	    0,//32, /* CART_XEGS_32 */
	    0,//64, /* CART_XEGS_64 */
	    0,//128,/* CART_XEGS_128 */
	    0,//16, /* CART_OSS2_16 */
	16, /* CART_5200_NS_16 */
	    0,//128,/* CART_ATRAX_128 */
	    0,//40, /* CART_BBSB_40 */
	8,  /* CART_5200_8 */
	4,  /* CART_5200_4 */
	    0,//8,  /* CART_RIGHT_8 */
	    0,//32, /* CART_WILL_32 */
	    0,//256,/* CART_XEGS_256 */
	    0,//512,/* CART_XEGS_512 */
	0,//1024,/* CART_XEGS_1024 */
	0,//16, /* CART_MEGA_16 */
	0,//32, /* CART_MEGA_32 */
	0,//64, /* CART_MEGA_64 */
	0,//128,/* CART_MEGA_128 */
	0,//256,/* CART_MEGA_256 */
	0,//512,/* CART_MEGA_512 */
	0,//1024,/* CART_MEGA_1024 */
	0,//32, /* CART_SWXEGS_32 */
	0,//64, /* CART_SWXEGS_64 */
	0,//128,/* CART_SWXEGS_128 */
	0,//256,/* CART_SWXEGS_256 */
	0,//512,/* CART_SWXEGS_512 */
	0,//1024,/* CART_SWXEGS_1024 */
	0,//8,  /* CART_PHOENIX_8 */
	0,//16, /* CART_BLIZZARD_16 */
	0,//128,/* CART_ATMAX_128 */
	0,//1024 /* CART_ATMAX_1024 */
};

int CART_IsFor5200(int type)
{
	switch (type) {
	case CART_5200_32:
	case CART_5200_EE_16:
	case CART_5200_40:
	case CART_5200_NS_16:
	case CART_5200_8:
	case CART_5200_4:
		return TRUE;
	default:
		break;
	}
	return FALSE;
}

/* For cartridge memory */
const UBYTE *cart_image = NULL;
/* Holds a 'backup' reference to the cartridge
 * memory provided by the platform host, to enable
 * reinsertion on load state */
static const uint8_t *platform_cart_image = NULL;
static size_t platform_cart_size = 0;
//LUDO: int cart_type = CART_NONE;
int cart_type = CART_5200_32;

static int bank;

/* DB_32, XEGS_32, XEGS_64, XEGS_128, XEGS_256, XEGS_512, XEGS_1024 */
/* SWXEGS_32, SWXEGS_64, SWXEGS_128, SWXEGS_256, SWXEGS_512, SWXEGS_1024 */
static void set_bank_809F(int b, int main)
{
	if (b != bank) {
		if (b & 0x80) {
			Cart809F_Disable();
			CartA0BF_Disable();
		}
		else {
			Cart809F_Enable();
			CartA0BF_Enable();
			CopyROM(0x8000, 0x9fff, cart_image + b * 0x2000);
			if (bank & 0x80)
				CopyROM(0xa000, 0xbfff, cart_image + main);
		}
		bank = b;
	}
}

/* OSS_16, OSS2_16 */
static void set_bank_A0AF(int b, int main)
{
	if (b != bank) {
		if (b < 0)
			CartA0BF_Disable();
		else {
			CartA0BF_Enable();
			CopyROM(0xa000, 0xafff, cart_image + b * 0x1000);
			if (bank < 0)
				CopyROM(0xb000, 0xbfff, cart_image + main);
		}
		bank = b;
	}
}

/* EXP_64, DIAMOND_64, SDX_64 */
static void set_bank_A0BF(int b)
{
	if (b != bank) {
		if (b & 0x0008)
			CartA0BF_Disable();
		else {
			CartA0BF_Enable();
			CopyROM(0xa000, 0xbfff, cart_image + (~b & 7) * 0x2000);
		}
		bank = b;
	}
}

static void set_bank_A0BF_WILL64(int b)
{
	if (b != bank) {
		if (b & 0x0008)
			CartA0BF_Disable();
		else {
			CartA0BF_Enable();
			CopyROM(0xa000, 0xbfff, cart_image + (b & 7) * 0x2000);
		}
		bank = b;
	}
}

static void set_bank_A0BF_WILL32(int b)
{
	if (b != bank) {
		if (b & 0x0008)
			CartA0BF_Disable();
		else {
			CartA0BF_Enable();
			CopyROM(0xa000, 0xbfff, cart_image + (b & 3) * 0x2000);
		}
		bank = b;
	}
}

static void set_bank_A0BF_ATMAX128(int b)
{
	if (b != bank) {
		if (b >= 0x20)
			return;
		else if (b >= 0x10)
			CartA0BF_Disable();
		else {
			CartA0BF_Enable();
			CopyROM(0xa000, 0xbfff, cart_image + b * 0x2000);
		}
		bank = b;
	}
}

static void set_bank_A0BF_ATMAX1024(int b)
{
	if (b != bank) {
		if (b >= 0x80)
			CartA0BF_Disable();
		else {
			CartA0BF_Enable();
			CopyROM(0xa000, 0xbfff, cart_image + b * 0x2000);
		}
		bank = b;
	}
}

static void set_bank_80BF(int b)
{
	if (b != bank) {
		if (b & 0x80) {
			Cart809F_Disable();
			CartA0BF_Disable();
		}
		else {
			Cart809F_Enable();
			CartA0BF_Enable();
			CopyROM(0x8000, 0xbfff, cart_image + b * 0x4000);
		}
		bank = b;
	}
}

/* an access (read or write) to D500-D5FF area */
static void CART_Access(UWORD addr)
{
	int b = bank;
	switch (cart_type) {
	case CART_OSS_16:
		if (addr & 0x08)
			b = -1;
		else
			switch (addr & 0x07) {
			case 0x00:
			case 0x01:
				b = 0;
				break;
			case 0x03:
			case 0x07:
				b = 1;
				break;
			case 0x04:
			case 0x05:
				b = 2;
				break;
			/* case 0x02:
			case 0x06: */
			default:
				break;
			}
		set_bank_A0AF(b, 0x3000);
		break;
	case CART_DB_32:
		set_bank_809F(addr & 0x03, 0x6000);
		break;
	case CART_WILL_64:
		set_bank_A0BF_WILL64(addr);
		break;
	case CART_WILL_32:
		set_bank_A0BF_WILL32(addr);
		break;
	case CART_EXP_64:
		if ((addr & 0xf0) == 0x70)
			set_bank_A0BF(addr);
		break;
	case CART_DIAMOND_64:
		if ((addr & 0xf0) == 0xd0)
			set_bank_A0BF(addr);
		break;
	case CART_SDX_64:
		if ((addr & 0xf0) == 0xe0)
			set_bank_A0BF(addr);
		break;
	case CART_OSS2_16:
		switch (addr & 0x09) {
		case 0x00:
			b = 1;
			break;
		case 0x01:
			b = 3;
			break;
		case 0x08:
			b = -1;
			break;
		case 0x09:
			b = 2;
			break;
		}
		set_bank_A0AF(b, 0x0000);
		break;
	case CART_PHOENIX_8:
		CartA0BF_Disable();
		break;
	case CART_BLIZZARD_16:
		Cart809F_Disable();
		CartA0BF_Disable();
		break;
	case CART_ATMAX_128:
		set_bank_A0BF_ATMAX128(addr & 0xff);
		break;
	case CART_ATMAX_1024:
		set_bank_A0BF_ATMAX1024(addr & 0xff);
		break;
	default:
		break;
	}
}

/* a read from D500-D5FF area */
UBYTE CART_GetByte(UWORD addr)
{
	if (rtime8_enabled && (addr == 0xd5b8 || addr == 0xd5b9))
		return RTIME8_GetByte();
	CART_Access(addr);
	return 0xff;
}

/* a write to D500-D5FF area */
void CART_PutByte(UWORD addr, UBYTE byte)
{
	if (rtime8_enabled && (addr == 0xd5b8 || addr == 0xd5b9)) {
		RTIME8_PutByte(byte);
		return;
	}
	switch (cart_type) {
	case CART_XEGS_32:
		set_bank_809F(byte & 0x03, 0x6000);
		break;
	case CART_XEGS_64:
		set_bank_809F(byte & 0x07, 0xe000);
		break;
	case CART_XEGS_128:
		set_bank_809F(byte & 0x0f, 0x1e000);
		break;
	case CART_XEGS_256:
		set_bank_809F(byte & 0x1f, 0x3e000);
		break;
	case CART_XEGS_512:
		set_bank_809F(byte & 0x3f, 0x7e000);
		break;
	case CART_XEGS_1024:
		set_bank_809F(byte & 0x7f, 0xfe000);
		break;
	case CART_ATRAX_128:
		if (byte & 0x80) {
			if (bank >= 0) {
				CartA0BF_Disable();
				bank = -1;
			}
		}
		else {
			int b = byte & 0xf;
			if (b != bank) {
				CartA0BF_Enable();
				CopyROM(0xa000, 0xbfff, cart_image + b * 0x2000);
				bank = b;
			}
		}
		break;
	case CART_MEGA_16:
		set_bank_80BF(byte & 0x80);
		break;
	case CART_MEGA_32:
		set_bank_80BF(byte & 0x81);
		break;
	case CART_MEGA_64:
		set_bank_80BF(byte & 0x83);
		break;
	case CART_MEGA_128:
		set_bank_80BF(byte & 0x87);
		break;
	case CART_MEGA_256:
		set_bank_80BF(byte & 0x8f);
		break;
	case CART_MEGA_512:
		set_bank_80BF(byte & 0x9f);
		break;
	case CART_MEGA_1024:
		set_bank_80BF(byte & 0xbf);
		break;
	case CART_SWXEGS_32:
		set_bank_809F(byte & 0x83, 0x6000);
		break;
	case CART_SWXEGS_64:
		set_bank_809F(byte & 0x87, 0xe000);
		break;
	case CART_SWXEGS_128:
		set_bank_809F(byte & 0x8f, 0x1e000);
		break;
	case CART_SWXEGS_256:
		set_bank_809F(byte & 0x9f, 0x3e000);
		break;
	case CART_SWXEGS_512:
		set_bank_809F(byte & 0xbf, 0x7e000);
		break;
	case CART_SWXEGS_1024:
		set_bank_809F(byte, 0xfe000);
		break;
	default:
		CART_Access(addr);
		break;
	}
}

/* special support of Bounty Bob on Atari5200 */
void CART_BountyBob1(UWORD addr)
{
	if (machine_type == MACHINE_5200) {
		if (addr >= 0x4ff6 && addr <= 0x4ff9) {
			addr -= 0x4ff6;
			CopyROM(0x4000, 0x4fff, cart_image + addr * 0x1000);
		}
	} else {
		if (addr >= 0x8ff6 && addr <= 0x8ff9) {
			addr -= 0x8ff6;
			CopyROM(0x8000, 0x8fff, cart_image + addr * 0x1000);
		}
	}
}

void CART_BountyBob2(UWORD addr)
{
	if (machine_type == MACHINE_5200) {
		if (addr >= 0x5ff6 && addr <= 0x5ff9) {
			addr -= 0x5ff6;
			CopyROM(0x5000, 0x5fff, cart_image + 0x4000 + addr * 0x1000);
		}
	}
	else {
		if (addr >= 0x9ff6 && addr <= 0x9ff9) {
			addr -= 0x9ff6;
			CopyROM(0x9000, 0x9fff, cart_image + 0x4000 + addr * 0x1000);
		}
	}
}

#ifdef PAGED_ATTRIB
UBYTE BountyBob1_GetByte(UWORD addr)
{
	if (machine_type == MACHINE_5200) {
		if (addr >= 0x4ff6 && addr <= 0x4ff9) {
			CART_BountyBob1(addr);
			return 0;
		}
	} else {
		if (addr >= 0x8ff6 && addr <= 0x8ff9) {
			CART_BountyBob1(addr);
			return 0;
		}
	}
	return dGetByte(addr);
}

UBYTE BountyBob2_GetByte(UWORD addr)
{
	if (machine_type == MACHINE_5200) {
		if (addr >= 0x5ff6 && addr <= 0x5ff9) {
			CART_BountyBob2(addr);
			return 0;
		}
	} else {
		if (addr >= 0x9ff6 && addr <= 0x9ff9) {
			CART_BountyBob2(addr);
			return 0;
		}
	}
	return dGetByte(addr);
}

void BountyBob1_PutByte(UWORD addr, UBYTE value)
{
	if (machine_type == MACHINE_5200) {
		if (addr >= 0x4ff6 && addr <= 0x4ff9) {
			CART_BountyBob1(addr);
		}
	} else {
		if (addr >= 0x8ff6 && addr <= 0x8ff9) {
			CART_BountyBob1(addr);
		}
	}
}

void BountyBob2_PutByte(UWORD addr, UBYTE value)
{
	if (machine_type == MACHINE_5200) {
		if (addr >= 0x5ff6 && addr <= 0x5ff9) {
			CART_BountyBob2(addr);
		}
	} else {
		if (addr >= 0x9ff6 && addr <= 0x9ff9) {
			CART_BountyBob2(addr);
		}
	}
}
#endif

int CART_Checksum(const UBYTE *image, int nbytes)
{
	int checksum = 0;
	while (nbytes > 0) {
		checksum += *image++;
		nbytes--;
	}
	return checksum;
}

int CART_Insert(const uint8_t *data, size_t size)
{
	int type;
	UBYTE header[16];

	/* remove currently inserted cart */
	CART_Remove();

	if (data == NULL || size < 16)
		return CART_CANT_OPEN;

	/* if full kilobytes, assume it is raw image */
	if ((size & 0x3ff) == 0) {
#ifdef NOCASH
		nocashMessage("raw image detected\n");
#endif  
		/* Assign pointers */
		platform_cart_image = data;
		platform_cart_size  = size;
		cart_image          = (UBYTE *)data;
		/* find cart type */
		uint32_t crccomputed = encoding_crc32(0, cart_image, size);
		cart_type = CART_NONE;
		size >>= 10;	/* number of kilobytes */
		for (type = 1; type <= CART_LAST_SUPPORTED; type++)
			if (cart_kb[type] == size) {
				if (cart_type == CART_NONE) {
					cart_type = type;
				}
				else {
#ifdef NOCASH      
					nocashMessage("multiple cart type");
#endif      
					cart_type = cart_wait_on_type(crccomputed);
					cart_type = (cart_type == 1 ? CART_5200_EE_16 : CART_5200_NS_16);
					break;
				}
			}
		if (cart_type != CART_NONE) {
			CART_Start();
#ifdef NOCASH      
			nocashMessage("found cart type");
#endif      
			return 0;	
		}
		cart_image = NULL;
		return CART_BAD_FORMAT;
	}
	/* if not full kilobytes, assume it is CART file */
	memcpy(header, data, 16);
	if ((header[0] == 'C') &&
		 (header[1] == 'A') &&
		 (header[2] == 'R') &&
		 (header[3] == 'T')) {
		type = (header[4] << 24) |
				 (header[5] << 16) |
				 (header[6] << 8) |
				  header[7];
		if (CART_IsFor5200(type)) { return CART_BAD_FORMAT; }
		if (type >= 1 && type <= CART_LAST_SUPPORTED) {
			int checksum;
			int size_corrected = cart_kb[type] << 10;
			size_corrected = (size_corrected > size - 16) ? size - 16 : size_corrected;
			/* Assign pointers */
			platform_cart_image = data;
			platform_cart_size  = size;
			cart_image          = (UBYTE *)(data + 16);
			checksum = (header[8] << 24) |
						  (header[9] << 16) |
						  (header[10] << 8) |
							header[11];
			cart_type = type;
			CART_Start();
			{
				int checksum2 = CART_Checksum(cart_image, size_corrected);
				int error = (checksum == checksum2) ? 0 : CART_BAD_CHECKSUM;
				return error;
			}
		}
	}
	return CART_BAD_FORMAT;
}

void CART_Remove(void) {
#ifdef NOCASH
	char sz[64];sprintf(sz,"CART_Remove %08x\n",cart_image);nocashMessage(sz);
#endif
	cart_type = CART_NONE;
	cart_image = NULL;
	CART_Start();
}

void CART_Start(void) {
#ifdef NOCASH
  char sz[64];sprintf(sz,"CART_Start M%d %d\n",machine_type,cart_type);nocashMessage(sz);
#endif
	if (machine_type == MACHINE_5200) {
		SetROM(0x4ff6, 0x4ff9);		/* disable Bounty Bob bank switching */
		SetROM(0x5ff6, 0x5ff9);
		switch (cart_type) {
		case CART_5200_32:
#ifdef NOCASH
      nocashMessage("patch CART_5200_32");
#endif      
			CopyROM(0x4000, 0xbfff, cart_image);
			break;
		case CART_5200_EE_16:
#ifdef NOCASH
      nocashMessage("patch CART_5200_EE_16");
#endif      
			CopyROM(0x4000, 0x5fff, cart_image);
			CopyROM(0x6000, 0x9fff, cart_image);
			CopyROM(0xa000, 0xbfff, cart_image + 0x2000);
			break;
		case CART_5200_40:
#ifdef NOCASH
      nocashMessage("patch CART_5200_40");
#endif      
			CopyROM(0x4000, 0x4fff, cart_image);
			CopyROM(0x5000, 0x5fff, cart_image + 0x4000);
			CopyROM(0x8000, 0x9fff, cart_image + 0x8000);
			CopyROM(0xa000, 0xbfff, cart_image + 0x8000);
#ifndef PAGED_ATTRIB
			SetHARDWARE(0x4ff6, 0x4ff9);
			SetHARDWARE(0x5ff6, 0x5ff9);
#else
			readmap[0x4f] = BountyBob1_GetByte;
			readmap[0x5f] = BountyBob2_GetByte;
			writemap[0x4f] = BountyBob1_PutByte;
			writemap[0x5f] = BountyBob2_PutByte;
#endif
			break;
		case CART_5200_NS_16:
#ifdef NOCASH
      nocashMessage("patch CART_5200_NS_16");
#endif      
			CopyROM(0x8000, 0xbfff, cart_image);
			break;
		case CART_5200_8:
#ifdef NOCASH
      nocashMessage("patch CART_5200_8");
#endif      
			CopyROM(0x8000, 0x9fff, cart_image);
			CopyROM(0xa000, 0xbfff, cart_image);
			break;
		case CART_5200_4:
#ifdef NOCASH
      nocashMessage("patch CART_5200_4");
#endif      
			CopyROM(0x8000, 0x8fff, cart_image);
			CopyROM(0x9000, 0x9fff, cart_image);
			CopyROM(0xa000, 0xafff, cart_image);
			CopyROM(0xb000, 0xbfff, cart_image);
			break;
		default:
#ifdef NOCASH
      nocashMessage("patch default");
#endif      
			/* clear cartridge area so the 5200 will crash */
			dFillMem(0x4000, 0, 0x8000);
			break;
		}
	}
	else {
		switch (cart_type) {
		case CART_STD_8:
		case CART_PHOENIX_8:
			Cart809F_Disable();
			CartA0BF_Enable();
			CopyROM(0xa000, 0xbfff, cart_image);
			break;
		case CART_STD_16:
		case CART_BLIZZARD_16:
			Cart809F_Enable();
			CartA0BF_Enable();
			CopyROM(0x8000, 0xbfff, cart_image);
			break;
		case CART_OSS_16:
			Cart809F_Disable();
			CartA0BF_Enable();
			CopyROM(0xa000, 0xafff, cart_image);
			CopyROM(0xb000, 0xbfff, cart_image + 0x3000);
			bank = 0;
			break;
		case CART_DB_32:
			Cart809F_Enable();
			CartA0BF_Enable();
			CopyROM(0x8000, 0x9fff, cart_image);
			CopyROM(0xa000, 0xbfff, cart_image + 0x6000);
			bank = 0;
			break;
		case CART_WILL_64:
		case CART_WILL_32:
		case CART_EXP_64:
		case CART_DIAMOND_64:
		case CART_SDX_64:
			Cart809F_Disable();
			CartA0BF_Enable();
			CopyROM(0xa000, 0xbfff, cart_image);
			bank = 0;
			break;
		case CART_XEGS_32:
		case CART_SWXEGS_32:
			Cart809F_Enable();
			CartA0BF_Enable();
			CopyROM(0x8000, 0x9fff, cart_image);
			CopyROM(0xa000, 0xbfff, cart_image + 0x6000);
			bank = 0;
			break;
		case CART_XEGS_64:
		case CART_SWXEGS_64:
			Cart809F_Enable();
			CartA0BF_Enable();
			CopyROM(0x8000, 0x9fff, cart_image);
			CopyROM(0xa000, 0xbfff, cart_image + 0xe000);
			bank = 0;
			break;
		case CART_XEGS_128:
		case CART_SWXEGS_128:
			Cart809F_Enable();
			CartA0BF_Enable();
			CopyROM(0x8000, 0x9fff, cart_image);
			CopyROM(0xa000, 0xbfff, cart_image + 0x1e000);
			bank = 0;
			break;
		case CART_XEGS_256:
		case CART_SWXEGS_256:
			Cart809F_Enable();
			CartA0BF_Enable();
			CopyROM(0x8000, 0x9fff, cart_image);
			CopyROM(0xa000, 0xbfff, cart_image + 0x3e000);
			bank = 0;
			break;
		case CART_XEGS_512:
		case CART_SWXEGS_512:
			Cart809F_Enable();
			CartA0BF_Enable();
			CopyROM(0x8000, 0x9fff, cart_image);
			CopyROM(0xa000, 0xbfff, cart_image + 0x7e000);
			bank = 0;
			break;
		case CART_XEGS_1024:
		case CART_SWXEGS_1024:
			Cart809F_Enable();
			CartA0BF_Enable();
			CopyROM(0x8000, 0x9fff, cart_image);
			CopyROM(0xa000, 0xbfff, cart_image + 0xfe000);
			bank = 0;
			break;
		case CART_OSS2_16:
			Cart809F_Disable();
			CartA0BF_Enable();
			CopyROM(0xa000, 0xafff, cart_image + 0x1000);
			CopyROM(0xb000, 0xbfff, cart_image);
			bank = 0;
			break;
		case CART_ATRAX_128:
			Cart809F_Disable();
			CartA0BF_Enable();
			CopyROM(0xa000, 0xbfff, cart_image);
			bank = 0;
			break;
		case CART_BBSB_40:
			Cart809F_Enable();
			CartA0BF_Enable();
			CopyROM(0x8000, 0x8fff, cart_image);
			CopyROM(0x9000, 0x9fff, cart_image + 0x4000);
			CopyROM(0xa000, 0xbfff, cart_image + 0x8000);
#ifndef PAGED_ATTRIB
			SetHARDWARE(0x8ff6, 0x8ff9);
			SetHARDWARE(0x9ff6, 0x9ff9);
#else
			readmap[0x8f] = BountyBob1_GetByte;
			readmap[0x9f] = BountyBob2_GetByte;
			writemap[0x8f] = BountyBob1_PutByte;
			writemap[0x9f] = BountyBob2_PutByte;
#endif
			break;
		case CART_RIGHT_8:
			if (machine_type == MACHINE_OSA || machine_type == MACHINE_OSB) {
				Cart809F_Enable();
				CopyROM(0x8000, 0x9fff, cart_image);
				if ((!disable_basic || loading_basic) && have_basic) {
					CartA0BF_Enable();
					CopyROM(0xa000, 0xbfff, atari_basic);
					break;
				}
				CartA0BF_Disable();
				break;
			}
			/* there's no right slot in XL/XE */
			Cart809F_Disable();
			CartA0BF_Disable();
			break;
		case CART_MEGA_16:
		case CART_MEGA_32:
		case CART_MEGA_64:
		case CART_MEGA_128:
		case CART_MEGA_256:
		case CART_MEGA_512:
		case CART_MEGA_1024:
			Cart809F_Enable();
			CartA0BF_Enable();
			CopyROM(0x8000, 0xbfff, cart_image);
			bank = 0;
			break;
		case CART_ATMAX_128:
			Cart809F_Disable();
			CartA0BF_Enable();
			CopyROM(0xa000, 0xbfff, cart_image);
			bank = 0;
			break;
		case CART_ATMAX_1024:
			Cart809F_Disable();
			CartA0BF_Enable();
			CopyROM(0xa000, 0xbfff, cart_image + 0xfe000);
			bank = 0x7f;
			break;
		default:
			Cart809F_Disable();
			if ((machine_type == MACHINE_OSA || machine_type == MACHINE_OSB)
			 && (!disable_basic || loading_basic) && have_basic) {
				CartA0BF_Enable();
				CopyROM(0xa000, 0xbfff, atari_basic);
				break;
			}
			CartA0BF_Disable();
			break;
		}
	}
}

#ifndef BASIC

void CARTStateRead(void)
{
	int savedCartType = CART_NONE;

	/* Read the cart type from the file.  If there is no cart type, becaused we have
	   reached the end of the file, this will just default to CART_NONE */
	ReadINT(&savedCartType, 1);
	if (savedCartType != CART_NONE) {
		/* Insert the cartridge... */
		if (CART_Insert(platform_cart_image, platform_cart_size) >= 0) {
			/* And set the type to the saved type, in case it was a raw cartridge image */
			cart_type = savedCartType;
			CART_Start();
		}
	}
}

void CARTStateSave(void)
{
	/* Save the cartridge type, or CART_NONE if there isn't one...*/
	SaveINT(&cart_type, 1);
}

#endif
