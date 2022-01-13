#include <stdlib.h>
#include <string.h>

#include "input.h"
#include "a5200_osk.h"

/* Colours taken from the Atari 5200 controller:
 * > Yellow: #FFF133 */
#define OSK_TEXT_COLOUR   ((0xFF >> 3) << 11 | (0xF1 >> 3) << 6 | (0x33 >> 3))
/* > Button black: #494949 */
#define OSK_BUTTON_COLOUR ((0x49 >> 3) << 11 | (0x49 >> 3) << 6 | (0x49 >> 3))
/* > Casing black:  #2D2D2B */
#define OSK_FRAME_COLOUR  ((0x2D >> 3) << 11 | (0x2D >> 3) << 6 | (0x2B >> 3))
/* > Logo silver: #F3F3F3 */
#define OSK_CURSOR_COLOUR ((0xF3 >> 3) << 11 | (0xF3 >> 3) << 6 | (0xF3 >> 3))
/* > Background is pure black */
#define OSK_BG_COLOUR     0

#define OSK_NUM_KEYS 12
#define OSK_SYMBOL_WIDTH 5
#define OSK_SYMBOL_HEIGHT 7
#define OSK_KEY_WIDTH 11
#define OSK_KEY_HEIGHT 13
#define OSK_BORDER_WIDTH 2

/* <bg_border><key_border><key><key_border><bg_border>...<bg_border><key_border><key><key_border><bg_border>*/
#define OSK_BMP_WIDTH ((OSK_NUM_KEYS * (OSK_KEY_WIDTH + (3 * OSK_BORDER_WIDTH))) + OSK_BORDER_WIDTH)
/* <bg_border>
 * <key_border>
 * <key>
 * <key_border>
 * <bg_border> */
#define OSK_BMP_HEIGHT (OSK_KEY_HEIGHT + (4 * OSK_BORDER_WIDTH))

static const uint8_t osk_symbol_asterisk[OSK_SYMBOL_WIDTH * OSK_SYMBOL_HEIGHT] = {
   0, 0, 0, 0, 0,
   0, 0, 1, 0, 0,
   1, 0, 1, 0, 1,
   0, 1, 1, 1, 0,
   1, 0, 1, 0, 1,
   0, 0, 1, 0, 0,
   0, 0, 0, 0, 0
};

static const uint8_t osk_symbol_hash[OSK_SYMBOL_WIDTH * OSK_SYMBOL_HEIGHT] = {
   0, 1, 0, 1, 0,
   1, 1, 1, 1, 1,
   0, 1, 0, 1, 0,
   0, 1, 0, 1, 0,
   1, 1, 1, 1, 1,
   0, 1, 0, 1, 0,
   0, 0, 0, 0, 0
};

static const uint8_t osk_symbol_0[OSK_SYMBOL_WIDTH * OSK_SYMBOL_HEIGHT] = {
   0, 1, 1, 1, 0,
   1, 0, 0, 0, 1,
   1, 0, 0, 0, 1,
   1, 0, 1, 0, 1,
   1, 0, 0, 0, 1,
   1, 0, 0, 0, 1,
   0, 1, 1, 1, 0
};

static const uint8_t osk_symbol_1[OSK_SYMBOL_WIDTH * OSK_SYMBOL_HEIGHT] = {
   0, 0, 1, 0, 0,
   0, 1, 1, 0, 0,
   0, 0, 1, 0, 0,
   0, 0, 1, 0, 0,
   0, 0, 1, 0, 0,
   0, 0, 1, 0, 0,
   0, 1, 1, 1, 0
};

static const uint8_t osk_symbol_2[OSK_SYMBOL_WIDTH * OSK_SYMBOL_HEIGHT] = {
   0, 1, 1, 1, 0,
   1, 0, 0, 0, 1,
   0, 0, 0, 0, 1,
   0, 0, 1, 1, 0,
   0, 1, 0, 0, 0,
   1, 0, 0, 0, 0,
   1, 1, 1, 1, 1
};

static const uint8_t osk_symbol_3[OSK_SYMBOL_WIDTH * OSK_SYMBOL_HEIGHT] = {
   0, 1, 1, 1, 0,
   1, 0, 0, 0, 1,
   0, 0, 0, 0, 1,
   0, 0, 1, 1, 0,
   0, 0, 0, 0, 1,
   1, 0, 0, 0, 1,
   0, 1, 1, 1, 0
};

static const uint8_t osk_symbol_4[OSK_SYMBOL_WIDTH * OSK_SYMBOL_HEIGHT] = {
   0, 0, 0, 1, 0,
   0, 0, 1, 1, 0,
   0, 1, 0, 1, 0,
   1, 0, 0, 1, 0,
   1, 1, 1, 1, 1,
   0, 0, 0, 1, 0,
   0, 0, 0, 1, 0
};

static const uint8_t osk_symbol_5[OSK_SYMBOL_WIDTH * OSK_SYMBOL_HEIGHT] = {
   1, 1, 1, 1, 1,
   1, 0, 0, 0, 0,
   1, 0, 0, 0, 0,
   1, 1, 1, 1, 0,
   0, 0, 0, 0, 1,
   1, 0, 0, 0, 1,
   0, 1, 1, 1, 0
};

static const uint8_t osk_symbol_6[OSK_SYMBOL_WIDTH * OSK_SYMBOL_HEIGHT] = {
   0, 0, 1, 1, 0,
   0, 1, 0, 0, 0,
   1, 0, 0, 0, 0,
   1, 1, 1, 1, 0,
   1, 0, 0, 0, 1,
   1, 0, 0, 0, 1,
   0, 1, 1, 1, 0
};

static const uint8_t osk_symbol_7[OSK_SYMBOL_WIDTH * OSK_SYMBOL_HEIGHT] = {
   1, 1, 1, 1, 1,
   0, 0, 0, 0, 1,
   0, 0, 0, 1, 0,
   0, 0, 0, 1, 0,
   0, 0, 1, 0, 0,
   0, 0, 1, 0, 0,
   0, 1, 0, 0, 0
};

static const uint8_t osk_symbol_8[OSK_SYMBOL_WIDTH * OSK_SYMBOL_HEIGHT] = {
   0, 1, 1, 1, 0,
   1, 0, 0, 0, 1,
   1, 0, 0, 0, 1,
   0, 1, 1, 1, 0,
   1, 0, 0, 0, 1,
   1, 0, 0, 0, 1,
   0, 1, 1, 1, 0
};

static const uint8_t osk_symbol_9[OSK_SYMBOL_WIDTH * OSK_SYMBOL_HEIGHT] = {
   0, 1, 1, 1, 0,
   1, 0, 0, 0, 1,
   1, 0, 0, 0, 1,
   0, 1, 1, 1, 1,
   0, 0, 0, 0, 1,
   0, 0, 0, 1, 0,
   0, 1, 1, 0, 0
};

static const uint8_t *osk_symbols[OSK_NUM_KEYS] = {
   osk_symbol_asterisk,
   osk_symbol_hash,
   osk_symbol_0,
   osk_symbol_1,
   osk_symbol_2,
   osk_symbol_3,
   osk_symbol_4,
   osk_symbol_5,
   osk_symbol_6,
   osk_symbol_7,
   osk_symbol_8,
   osk_symbol_9
};

static const unsigned osk_key_map[OSK_NUM_KEYS] = {
   AKEY_5200_ASTERISK,
   AKEY_5200_HASH,
   AKEY_5200_0,
   AKEY_5200_1,
   AKEY_5200_2,
   AKEY_5200_3,
   AKEY_5200_4,
   AKEY_5200_5,
   AKEY_5200_6,
   AKEY_5200_7,
   AKEY_5200_8,
   AKEY_5200_9
};

static uint16_t *osk_bmp      = NULL;
static uint8_t osk_cursor_idx = 0;

static void osk_draw_symbol(uint16_t *buffer,
      size_t buffer_width, size_t buffer_height, 
      const uint8_t *symbol,
      size_t x, size_t y, uint16_t colour)
{
   size_t i, j;

   if ((x + OSK_SYMBOL_WIDTH  >= buffer_width) ||
       (y + OSK_SYMBOL_HEIGHT >= buffer_height))
      return;

   for (j = 0; j < OSK_SYMBOL_HEIGHT; j++)
   {
      size_t buff_offset = ((y + j) * buffer_width) + x;

      for (i = 0; i < OSK_SYMBOL_WIDTH; i++)
      {
         if (*symbol++ == 1)
            *(osk_bmp + buff_offset + i) = colour;
      }
   }
}

static void osk_draw_rect(uint16_t *buffer,
      size_t buffer_width, size_t buffer_height, 
      size_t x, size_t y,
      size_t width, size_t height,
      uint16_t color)
{
   size_t x_index, y_index;
   size_t x_start = x <= buffer_width  ? x : buffer_width;
   size_t y_start = y <= buffer_height ? y : buffer_height;
   size_t x_end   = x + width;
   size_t y_end   = y + height;

   x_end = x_end <= buffer_width  ? x_end : buffer_width;
   y_end = y_end <= buffer_height ? y_end : buffer_height;

   for (y_index = y_start; y_index < y_end; y_index++)
   {
      uint16_t *buffer_ptr = buffer + (y_index * buffer_width);
      for (x_index = x_start; x_index < x_end; x_index++)
         *(buffer_ptr + x_index) = color;
   }
}

void a5200_osk_init(void)
{
   size_t i;
   size_t key_x, key_y;
   size_t symbol_x, symbol_y;
   size_t border_x, border_y;

   osk_cursor_idx = 0;
   osk_bmp        = (uint16_t*)malloc(OSK_BMP_WIDTH *
         OSK_BMP_HEIGHT * sizeof(uint16_t));

   /* Draw OSK bitmap */

   /* > Fill with background colour */
   for (i = 0; i < OSK_BMP_WIDTH * OSK_BMP_HEIGHT; i++)
      osk_bmp[i] = OSK_BG_COLOUR;

   /* > Get initial draw locations */
   key_x = (2 * OSK_BORDER_WIDTH);
   key_y = (2 * OSK_BORDER_WIDTH);

   symbol_x = (2 * OSK_BORDER_WIDTH) +
         ((OSK_KEY_WIDTH - OSK_SYMBOL_WIDTH) >> 1);
   symbol_y = (2 * OSK_BORDER_WIDTH) +
         ((OSK_KEY_HEIGHT - OSK_SYMBOL_HEIGHT) >> 1);

   border_x = OSK_BORDER_WIDTH;
   border_y = OSK_BORDER_WIDTH;

   for (i = 0; i < OSK_NUM_KEYS; i++)
   {
      /* > Draw key */
      osk_draw_rect(osk_bmp, OSK_BMP_WIDTH, OSK_BMP_HEIGHT,
            key_x, key_y,
            OSK_KEY_WIDTH, OSK_KEY_HEIGHT,
            OSK_BUTTON_COLOUR);

      key_x += OSK_KEY_WIDTH + (3 * OSK_BORDER_WIDTH);

      /* > Draw symbol */
      osk_draw_symbol(osk_bmp, OSK_BMP_WIDTH, OSK_BMP_HEIGHT,
            osk_symbols[i], symbol_x, symbol_y, OSK_TEXT_COLOUR);

      symbol_x += OSK_KEY_WIDTH + (3 * OSK_BORDER_WIDTH);

      /* > Draw key frame */
      osk_draw_rect(osk_bmp, OSK_BMP_WIDTH, OSK_BMP_HEIGHT,
            border_x, border_y,
            OSK_KEY_WIDTH + (2 * OSK_BORDER_WIDTH), OSK_BORDER_WIDTH,
            OSK_FRAME_COLOUR);

      osk_draw_rect(osk_bmp, OSK_BMP_WIDTH, OSK_BMP_HEIGHT,
            border_x, border_y + OSK_KEY_HEIGHT + OSK_BORDER_WIDTH,
            OSK_KEY_WIDTH + (2 * OSK_BORDER_WIDTH), OSK_BORDER_WIDTH,
            OSK_FRAME_COLOUR);

      osk_draw_rect(osk_bmp, OSK_BMP_WIDTH, OSK_BMP_HEIGHT,
            border_x, border_y + OSK_BORDER_WIDTH,
            OSK_BORDER_WIDTH, OSK_KEY_HEIGHT,
            OSK_FRAME_COLOUR);

      osk_draw_rect(osk_bmp, OSK_BMP_WIDTH, OSK_BMP_HEIGHT,
            border_x + OSK_KEY_WIDTH + OSK_BORDER_WIDTH, border_y + OSK_BORDER_WIDTH,
            OSK_BORDER_WIDTH, OSK_KEY_HEIGHT,
            OSK_FRAME_COLOUR);

      border_x += OSK_KEY_WIDTH + (3 * OSK_BORDER_WIDTH);
   }
}

void a5200_osk_deinit(void)
{
   if (osk_bmp)
      free(osk_bmp);

   osk_bmp        = NULL;
   osk_cursor_idx = 0;
}

void a5200_osk_draw(uint16_t *buffer, size_t width, size_t height)
{
   uint16_t *src = NULL;
   uint16_t *dst = NULL;
   size_t bmp_offset_x;
   size_t bmp_offset_y;
   size_t bmp_y;
   size_t cursor_x;
   size_t cursor_y;

   if ((width < OSK_BMP_WIDTH) ||
       (height < OSK_BMP_HEIGHT))
      return;

   /* Get draw position */
   bmp_offset_x = (width - OSK_BMP_WIDTH) >> 1;
   bmp_offset_y = (height - OSK_BMP_HEIGHT);

   /* Copy OSK bitmap to buffer */
   for (bmp_y = 0; bmp_y < OSK_BMP_HEIGHT; bmp_y++)
   {
      src = osk_bmp + (bmp_y * OSK_BMP_WIDTH);
      dst = buffer + bmp_offset_x + ((bmp_y + bmp_offset_y) * width);
      
      memcpy(dst, src, OSK_BMP_WIDTH * sizeof(uint16_t));
   }

   /* Draw cursor */
   cursor_x = bmp_offset_x + OSK_BORDER_WIDTH + (osk_cursor_idx *
         (OSK_KEY_WIDTH + (3 * OSK_BORDER_WIDTH)));
   cursor_y = bmp_offset_y + OSK_BORDER_WIDTH;

   osk_draw_rect(buffer, width, height,
         cursor_x, cursor_y,
         OSK_KEY_WIDTH + (2 * OSK_BORDER_WIDTH), OSK_BORDER_WIDTH,
         OSK_CURSOR_COLOUR);

   osk_draw_rect(buffer, width, height,
         cursor_x, cursor_y + OSK_KEY_HEIGHT + OSK_BORDER_WIDTH,
         OSK_KEY_WIDTH + (2 * OSK_BORDER_WIDTH), OSK_BORDER_WIDTH,
         OSK_CURSOR_COLOUR);

   osk_draw_rect(buffer, width, height,
         cursor_x, cursor_y + OSK_BORDER_WIDTH,
         OSK_BORDER_WIDTH, OSK_KEY_HEIGHT,
         OSK_CURSOR_COLOUR);

   osk_draw_rect(buffer, width, height,
         cursor_x + OSK_KEY_WIDTH + OSK_BORDER_WIDTH, cursor_y + OSK_BORDER_WIDTH,
         OSK_BORDER_WIDTH, OSK_KEY_HEIGHT,
         OSK_CURSOR_COLOUR);
}

void a5200_osk_move_cursor(int delta)
{
   int cursor_idx = delta;

   cursor_idx %= OSK_NUM_KEYS;

   if (cursor_idx < 0)
      cursor_idx += OSK_NUM_KEYS;

   cursor_idx += (int)osk_cursor_idx;
   if (cursor_idx >= OSK_NUM_KEYS)
      cursor_idx -= OSK_NUM_KEYS;

   osk_cursor_idx = (uint8_t)cursor_idx;
}

unsigned a5200_osk_get_key(void)
{
   return osk_key_map[osk_cursor_idx];
}
