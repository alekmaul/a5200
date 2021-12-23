/*
 * statesav.c - saving the emulator's state to a file
 *
 * Copyright (C) 1995-1998 David Firth
 * Copyright (C) 1998-2006 Atari800 development team (see DOC/CREDITS)
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

#include <string.h>
#include <stdlib.h>
#include <libretro.h>
#include <streams/memory_stream.h>
#include "config.h"
#include "atari.h"
#include "util.h"

#ifndef PATH_MAX_LENGTH
#if defined(_XBOX1) || defined(_3DS) || defined(PSP) || defined(PS2) || defined(GEKKO)|| defined(WIIU) || defined(ORBIS) || defined(__PSL1GHT__) || defined(__PS3__)
#define PATH_MAX_LENGTH 512
#else
#define PATH_MAX_LENGTH 4096
#endif
#endif

#define SAVE_VERSION_NUMBER 4

extern void a5200_log(enum retro_log_level level, const char *format, ...);

void AnticStateSave(void);
void MainStateSave(void);
void CpuStateSave(UBYTE SaveVerbose);
void GTIAStateSave(void);
void PIAStateSave(void);
void POKEYStateSave(void);
void CARTStateSave(void);
void SIOStateSave(void);

void AnticStateRead(void);
void MainStateRead(void);
void CpuStateRead(UBYTE SaveVerbose);
void GTIAStateRead(void);
void PIAStateRead(void);
void POKEYStateRead(void);
void CARTStateRead(void);
void SIOStateRead(void);

static memstream_t *state_stream = NULL;
static bool state_stream_error   = false;

/* Value is memory location of data, num is number of type to save */
void SaveUBYTE(const UBYTE *data, int num)
{
   if (!state_stream || state_stream_error)
      return;

   /* Assumption is that UBYTE = 8bits and the pointer passed in refers
    * directly to the active bits in a padded location. If not (unlikely)
    * you'll have to redefine this to save appropriately for cross-platform
    * compatibility */
   if (memstream_write(state_stream, data, num) != num)
      state_stream_error = true;
}

/* Value is memory location of data, num is number of type to save */
void ReadUBYTE(UBYTE *data, int num)
{
   if (!state_stream || state_stream_error)
      return;

   if (memstream_read(state_stream, data, num) != num)
      state_stream_error = true;
}

/* Value is memory location of data, num is number of type to save */
void SaveUWORD(const UWORD *data, int num)
{
   if (!state_stream || state_stream_error)
      return;

   /* UWORDS are saved as 16bits, regardless of the size on this particular
    * platform. Each byte of the UWORD will be pushed out individually in
    * LSB order. The shifts here and in the read routines will work for both
    * LSB and MSB architectures. */
   while (num > 0)
   {
      UWORD temp = *data++;
      UBYTE byte = temp & 0xff;

      if (memstream_write(state_stream, &byte, 1) != 1)
      {
         state_stream_error = true;
         break;
      }

      temp >>= 8;
      byte   = temp & 0xff;

      if (memstream_write(state_stream, &byte, 1) != 1)
      {
         state_stream_error = true;
         break;
      }

      num--;
   }
}

/* Value is memory location of data, num is number of type to save */
void ReadUWORD(UWORD *data, int num)
{
   if (!state_stream || state_stream_error)
      return;

   while (num > 0)
   {
      UBYTE byte1;
      UBYTE byte2;

      if (memstream_read(state_stream, &byte1, 1) != 1)
      {
         state_stream_error = true;
         break;
      }

      if (memstream_read(state_stream, &byte2, 1) != 1)
      {
         state_stream_error = true;
         break;
      }

      *data++ = (byte2 << 8) | byte1;
      num--;
   }
}

void SaveINT(const int *data, int num)
{
   if (!state_stream || state_stream_error)
      return;

   /* INTs are always saved as 32bits (4 bytes) in the file. They can be any size
    * on the platform however. The sign bit is clobbered into the fourth byte saved
    * for each int; on read it will be extended out to its proper position for the
    * native INT size */
   while (num > 0)
   {
      UBYTE signbit = 0;
      unsigned int temp;
      UBYTE byte;
      int temp0;

      temp0 = *data++;
      if (temp0 < 0)
      {
         temp0   = -temp0;
         signbit = 0x80;
      }
      temp = (unsigned int)temp0;

      byte = temp & 0xff;
      if (memstream_write(state_stream, &byte, 1) != 1)
      {
         state_stream_error = true;
         break;
      }

      temp >>= 8;
      byte   = temp & 0xff;
      if (memstream_write(state_stream, &byte, 1) != 1)
      {
         state_stream_error = true;
         break;
      }

      temp >>= 8;
      byte   = temp & 0xff;
      if (memstream_write(state_stream, &byte, 1) != 1)
      {
         state_stream_error = true;
         break;
      }

      temp >>= 8;
      byte   = (temp & 0x7f) | signbit;
      if (memstream_write(state_stream, &byte, 1) != 1)
      {
         state_stream_error = true;
         break;
      }

      num--;
   }
}

void ReadINT(int *data, int num)
{
   if (!state_stream || state_stream_error)
      return;

   while (num > 0)
   {
      UBYTE signbit = 0;
      int temp;
      UBYTE byte1;
      UBYTE byte2;
      UBYTE byte3;
      UBYTE byte4;

      if (memstream_read(state_stream, &byte1, 1) != 1)
      {
         state_stream_error = true;
         break;
      }

      if (memstream_read(state_stream, &byte2, 1) != 1)
      {
         state_stream_error = true;
         break;
      }

      if (memstream_read(state_stream, &byte3, 1) != 1)
      {
         state_stream_error = true;
         break;
      }

      if (memstream_read(state_stream, &byte4, 1) != 1)
      {
         state_stream_error = true;
         break;
      }

      signbit = byte4 & 0x80;
      byte4  &= 0x7f;

      temp = (byte4 << 24) | (byte3 << 16) | (byte2 << 8) | byte1;
      if (signbit)
         temp = -temp;
      *data++ = temp;

      num--;
   }
}

void SaveFNAME(const char *filename)
{
   UWORD namelen = strlen(filename);
   /* Save the length of the filename, followed by the filename */
   SaveUWORD(&namelen, 1);
   SaveUBYTE((const UBYTE *)filename, namelen);
}

void ReadFNAME(char *filename)
{
   UWORD namelen = 0;

   ReadUWORD(&namelen, 1);
   if (namelen >= PATH_MAX_LENGTH)
   {
      a5200_log(RETRO_LOG_ERROR,
            "Save State: Filenames of %d characters not supported on this platform.\n",
            (int)namelen);
      return;
   }

   ReadUBYTE((UBYTE *)filename, namelen);
   filename[namelen] = 0;
}

int SaveAtariState(uint8_t *data, size_t size, UBYTE SaveVerbose)
{
   UBYTE StateVersion = SAVE_VERSION_NUMBER;

   /* Clean up any existing memory stream */
   if (state_stream)
   {
      memstream_close(state_stream);
      memstream_set_buffer(NULL, 0);
      state_stream = NULL;
   }
   state_stream_error = false;

   if (!data || size < 1)
   {
      a5200_log(RETRO_LOG_ERROR, "Save State: Invalid data.\n");
      goto error;
   }

   /* Open memory stream */
   memstream_set_buffer(data, size);
   state_stream = memstream_open(1);
   if (!state_stream)
   {
      a5200_log(RETRO_LOG_ERROR, "Save State: Failed to open memory stream for writing.\n");
      goto error;
   }

   if (memstream_write(state_stream, "ATARI5200", 9) != 9)
   {
      a5200_log(RETRO_LOG_ERROR, "Save State: Failed to write save state header.\n");
      goto error;
   }

   SaveUBYTE(&StateVersion, 1);
   SaveUBYTE(&SaveVerbose, 1);
   /* The order here is important. Main must be first because it saves the machine type, and
      decisions on what to save/not save are made based off that later in the process */
   MainStateSave();
   CARTStateSave();
   SIOStateSave();
   AnticStateSave();
   CpuStateSave(SaveVerbose);
   GTIAStateSave();
   PIAStateSave();
   POKEYStateSave();
#ifdef DREAMCAST
   DCStateSave();
#endif

   /* Close memory stream */
   memstream_close(state_stream);
	memstream_set_buffer(NULL, 0);
   state_stream = NULL;

   if (state_stream_error)
   {
      a5200_log(RETRO_LOG_ERROR, "Save State: Failed to write save state data.\n");
      return FALSE;
   }

   return TRUE;

error:
   if (state_stream)
      memstream_close(state_stream);
   memstream_set_buffer(NULL, 0);
   state_stream       = NULL;
   state_stream_error = true;
   return FALSE;
}

int ReadAtariState(const uint8_t *data, size_t size)
{
   char header_string[9];
   UBYTE StateVersion = 0; /* The version of the save file */
   UBYTE SaveVerbose  = 0; /* Verbose mode means save basic, OS if patched */

   /* Clean up any existing memory stream */
   if (state_stream)
   {
      memstream_close(state_stream);
      memstream_set_buffer(NULL, 0);
      state_stream = NULL;
   }
   state_stream_error = false;

   /* Open memory stream */
   memstream_set_buffer((uint8_t*)data, size);
   state_stream = memstream_open(0);
   if (!state_stream)
   {
      a5200_log(RETRO_LOG_ERROR, "Save State: Failed to open memory stream for reading.\n");
      goto error;
   }

   if (memstream_read(state_stream, header_string, 9) != 9)
   {
      a5200_log(RETRO_LOG_ERROR, "Save State: Failed to read save state header.\n");
      goto error;
   }

   if (memcmp(header_string, "ATARI5200", 9) != 0)
   {
      a5200_log(RETRO_LOG_ERROR, "Save State: This is not a5200 save state data.\n");
      goto error;
   }

   if ((memstream_read(state_stream, &StateVersion, 1) != 1) ||
       (memstream_read(state_stream, &SaveVerbose, 1)  != 1))
   {
      a5200_log(RETRO_LOG_ERROR, "Save State: Failed to read save state data.\n");
      goto error;
   }

   if ((StateVersion != SAVE_VERSION_NUMBER) &&
       (StateVersion != 3))
   {
      a5200_log(RETRO_LOG_ERROR, "Save State: Cannot read save state data - incompatible version detected.\n");
      goto error;
   }

   MainStateRead();
   if (StateVersion != 3)
   {
      CARTStateRead();
      SIOStateRead();
   }
   AnticStateRead();
   CpuStateRead(SaveVerbose);
   GTIAStateRead();
   PIAStateRead();
   POKEYStateRead();
#ifdef DREAMCAST
   DCStateRead();
#endif

   /* Close memory stream */
   memstream_close(state_stream);
	memstream_set_buffer(NULL, 0);
   state_stream = NULL;

   if (state_stream_error)
   {
      a5200_log(RETRO_LOG_ERROR, "Save State: Failed to read save state data.\n");
      return FALSE;
   }

   return TRUE;

error:
   if (state_stream)
      memstream_close(state_stream);
   memstream_set_buffer(NULL, 0);
   state_stream       = NULL;
   state_stream_error = true;
   return FALSE;
}
