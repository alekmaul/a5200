/*
 * rtime8.c - Emulate ICD R-Time 8 cartridge
 *
 * Copyright (C) 2000 Jason Duerstock
 * Copyright (C) 2000-2005 Atari800 development team (see DOC/CREDITS)
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
#include <stdlib.h>	/* for NULL */
#include <string.h>	/* for strcmp() */

#include <time/rtime.h>

#include "atari.h"
//#include "log.h"

int rtime8_enabled = 1;

static int rtime8_state = 0;
				/* 0 = waiting for register # */
				/* 1 = got register #, waiting for hi nybble */
				/* 2 = got hi nybble, waiting for lo nybble */
static int rtime8_tmp = 0;
static int rtime8_tmp2 = 0;

static UBYTE regset[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void RTIME8_Initialise(void)
{
	rtime8_state = 0;
	rtime8_tmp = 0;
	rtime8_tmp2 = 0;
	memset(regset, 0, sizeof(regset));
}

static int hex2bcd(int h)
{
	return ((h / 10) << 4) | (h % 10);
}

static int gettime(int p)
{
	time_t tt;
	struct tm lt;

	tt = time(NULL);
	rtime_localtime(&tt, &lt);

	switch (p) {
	case 0:
		return hex2bcd(lt.tm_sec);
	case 1:
		return hex2bcd(lt.tm_min);
	case 2:
		return hex2bcd(lt.tm_hour);
	case 3:
		return hex2bcd(lt.tm_mday);
	case 4:
		return hex2bcd(lt.tm_mon + 1);
	case 5:
		return hex2bcd(lt.tm_year % 100);
	case 6:
		return hex2bcd(((lt.tm_wday + 2) % 7) + 1);
	}

	return 0;
}

#define HAVE_GETTIME

UBYTE RTIME8_GetByte(void)
{
	switch (rtime8_state) {
	case 0:
		/* iprintf("pretending rtime not busy, returning 0"); */
		return 0;
	case 1:
		rtime8_state = 2;
		return (
#ifdef HAVE_GETTIME
			rtime8_tmp <= 6 ?
			gettime(rtime8_tmp) :
#endif
			regset[rtime8_tmp]) >> 4;
	case 2:
		rtime8_state = 0;
		return (
#ifdef HAVE_GETTIME
			rtime8_tmp <= 6 ?
			gettime(rtime8_tmp) :
#endif
			regset[rtime8_tmp]) & 0x0f;
	}
	return 0;
}

void RTIME8_PutByte(UBYTE byte)
{
	switch (rtime8_state) {
	case 0:
		rtime8_tmp = byte & 0x0f;
		rtime8_state = 1;
		break;
	case 1:
		rtime8_tmp2 = byte << 4;
		rtime8_state = 2;
		break;
	case 2:
		regset[rtime8_tmp] = rtime8_tmp2 | (byte & 0x0f);
		rtime8_state = 0;
		break;
	}
}
