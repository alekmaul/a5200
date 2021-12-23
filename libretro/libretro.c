#ifndef _MSC_VER
#include <stdbool.h>
#include <sched.h>
#endif
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include <string/stdstring.h>
#include <file/file_path.h>
#include <streams/file_stream.h>

#include <libretro.h>
#include "libretro_core_options.h"

#include "atari.h"
#include "global.h"
#include "cartridge.h"
#include "input.h"
#include "pia.h"
#include "screen.h"
#include "sound.h"
#include "pokeysnd.h"
#include "statesav.h"

#ifdef _3DS
extern void* linearMemAlign(size_t size, size_t alignment);
extern void linearFree(void* mem);
#endif

static retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;
static retro_environment_t environ_cb;
static retro_audio_sample_t audio_cb;
static retro_audio_sample_batch_t audio_batch_cb;

#ifndef PATH_MAX_LENGTH
#if defined(_XBOX1) || defined(_3DS) || defined(PSP) || defined(PS2) || defined(GEKKO)|| defined(WIIU) || defined(ORBIS) || defined(__PSL1GHT__) || defined(__PS3__)
#define PATH_MAX_LENGTH 512
#else
#define PATH_MAX_LENGTH 4096
#endif
#endif

#ifndef PI
#define PI 3.141592653589793238
#endif

#define A5200_BIOS_SIZE 0x800
#define A5200_SAVE_STATE_SIZE 131357

#define A5200_SCREEN_BUFFER_WIDTH 512
#define A5200_SCREEN_BUFFER_HEIGHT 512
/* Note: Maximum Atari 5200 resolution is 320x192,
 * but outputting at this resolution causes severe
 * vertical cropping in most games. A vertical
 * display resolution of 224 seems to be the best
 * value for 'safely' cropping overscan */
#define A5200_VIDEO_WIDTH 320
#define A5200_VIDEO_HEIGHT 224
#define A5200_VIDEO_OFFSET_X ((ATARI_WIDTH - A5200_VIDEO_WIDTH) >> 1)
#define A5200_VIDEO_OFFSET_Y ((ATARI_HEIGHT - A5200_VIDEO_HEIGHT) >> 1)

#define A5200_FPS 60
#define A5200_AUDIO_BUFFER_SIZE (SOUND_SAMPLE_RATE / A5200_FPS)

#define LIBRETRO_ANALOG_RANGE 0x8000
#define A5200_JOY_MIN 6
#define A5200_JOY_MAX 220
#define A5200_JOY_CENTER 114
#define A5200_VIRTUAL_NUMPAD_THRESHOLD 0.7

static const uint32_t a5200_palette_ntsc[256] = {
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
static uint16_t a5200_palette_rgb565[256] = {0};

static const unsigned input_analog_numpad_map[8] = {
   AKEY_5200_7,
   AKEY_5200_8,
   AKEY_5200_9,
   AKEY_5200_6,
   AKEY_5200_3,
   AKEY_5200_2,
   AKEY_5200_1,
   AKEY_5200_4,
};

static struct retro_input_descriptor input_descriptors[] = {
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,   "Joystick Left (Digital)" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,     "Joystick Up (Digital)" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,   "Joystick Down (Digital)" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT,  "Joystick Right (Digital)" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,      "Fire 1" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,      "Fire 2" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X,      "NumPad #" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y,      "NumPad *" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R,      "NumPad 0" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3,     "NumPad 5" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Pause" },
   { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START,  "Start" },
   { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_X, "Joystick X (Analog)" },
   { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT,  RETRO_DEVICE_ID_ANALOG_Y, "Joystick Y (Analog)" },
   { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, "NumPad [1-9]" },
   { 0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, "NumPad [1-9]" },

   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "Joystick Left (Digital)" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "Joystick Up (Digital)" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "Joystick Down (Digital)" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Joystick Right (Digital)" },
   { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "Fire 1" },
   { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,  "Joystick X (Analog)" },
   { 1, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,  "Joystick Y (Analog)" },

   { 0 },
};

static uint8_t *a5200_screen_buffer  = NULL;
static uint16_t *video_buffer        = NULL;
static uint8_t *audio_samples_buffer = NULL;
static int16_t *audio_out_buffer     = NULL;

static bool audio_low_pass_enabled  = false;
static int32_t audio_low_pass_range = (60 * 0x10000) / 100;
static int32_t audio_low_pass_prev  = 0;

static unsigned input_shift_ctrl      = 0;
static int input_analog_deadzone      = (int)(0.15f * (float)LIBRETRO_ANALOG_RANGE);
static bool input_dual_stick_enabled  = false;
static float input_analog_sensitivity = 1.0f;

static uint8_t *rom_buf        = NULL;
static const uint8_t *rom_data = NULL;
static size_t rom_size         = 0;

static bool libretro_supports_bitmasks = false;

extern UBYTE PCPOT_input[8];

/************************************
 * Auxiliary functions
 ************************************/

void a5200_log(enum retro_log_level level, const char *format, ...)
{
   char msg[512];
   va_list ap;

   msg[0] = '\0';

   if (string_is_empty(format))
      return;

   va_start(ap, format);
   vsprintf(msg, format, ap);
   va_end(ap);

   if (log_cb)
      log_cb(level, "[a5200] %s", msg);
   else
      fprintf((level == RETRO_LOG_ERROR) ? stderr : stdout,
            "[a5200] %s", msg);
}

static bool load_bios(void)
{
   const char *system_dir = NULL;
   RFILE *bios_file       = NULL;
   int64_t bytes_read     = 0;
   char bios_path[PATH_MAX_LENGTH];

   bios_path[0] = '\0';

   /* Get system directory */
   if (!environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) ||
       !system_dir)
   {
      a5200_log(RETRO_LOG_ERROR,
            "No system directory defined, unable to look for bios.\n");
      return false;
   }

   /* Get BIOS path */
   fill_pathname_join(bios_path, system_dir,
         "5200.rom", sizeof(bios_path));

   /* Read BIOS file */
   bios_file = filestream_open(bios_path,
         RETRO_VFS_FILE_ACCESS_READ,
         RETRO_VFS_FILE_ACCESS_HINT_NONE);

   if (!bios_file)
   {
      a5200_log(RETRO_LOG_ERROR,
            "Failed to open bios file: %s\n", bios_path);
      return false;
   }

   bytes_read = filestream_read(bios_file,
         atari_os, A5200_BIOS_SIZE);
   filestream_close(bios_file);

   if (bytes_read != A5200_BIOS_SIZE)
   {
      a5200_log(RETRO_LOG_ERROR,
            "Failed to read bios file: %s\n", bios_path);
      return false;
   }

   a5200_log(RETRO_LOG_INFO, "Read bios: %s\n", bios_path);
   return true;
}

static void initialise_palette(void)
{
   uint8_t r, g, b;
   size_t i;

   for (i = 0; i < 256; i++)
   {
      r = (uint8_t)((a5200_palette_ntsc[i] & 0xFF0000) >> 16);
      g = (uint8_t)((a5200_palette_ntsc[i] & 0x00FF00) >>  8);
      b = (uint8_t)((a5200_palette_ntsc[i] & 0x0000FF)      );

      a5200_palette_rgb565[i] = (r >> 3) << 11 | (g >> 3) << 6 | (b >> 3);
   }
}

static void check_variables(void)
{
   struct retro_variable var = {0};

   /* Audio Filter */
   var.key                = "a5200_low_pass_filter";
   var.value              = NULL;
   audio_low_pass_enabled = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) &&
       !string_is_empty(var.value) &&
       string_is_equal(var.value, "enabled"))
         audio_low_pass_enabled = true;

   /* Audio Filter Level */
   var.key              = "a5200_low_pass_range";
   var.value            = NULL;
   audio_low_pass_range = (60 * 0x10000) / 100;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) &&
       !string_is_empty(var.value))
   {
      uint32_t filter_level = string_to_unsigned(var.value);
      audio_low_pass_range  = (filter_level * 0x10000) / 100;
   }

   /* Dual Stick Controller */
   var.key                  = "a5200_gamepad_dual_stick_hack";
   var.value                = NULL;
   input_dual_stick_enabled = false;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) &&
       !string_is_empty(var.value) &&
       string_is_equal(var.value, "enabled"))
         input_dual_stick_enabled = true;

   /* Analog Sensitivity */
   var.key                  = "a5200_analog_sensitivity";
   var.value                = NULL;
   input_analog_sensitivity = 1.0f;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) &&
       !string_is_empty(var.value))
   {
      unsigned sensitivity     = string_to_unsigned(var.value);
      input_analog_sensitivity = (float)sensitivity / 100.0f;
   }

   /* Analog Deadzone */
   var.key               = "a5200_analog_deadzone";
   var.value             = NULL;
   input_analog_deadzone = (int)(0.15f * (float)LIBRETRO_ANALOG_RANGE);

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) &&
       !string_is_empty(var.value))
   {
      unsigned deadzone     = string_to_unsigned(var.value);
      input_analog_deadzone = (int)((float)deadzone * 0.01f *
            (float)LIBRETRO_ANALOG_RANGE);
   }
}

static INLINE unsigned int a5200_get_analog_pot(int input)
{
   /* Convert to amplitude */
   float amplitude = (float)((input > input_analog_deadzone) ?
         (input - input_analog_deadzone) :
               (input + input_analog_deadzone)) /
                     (float)(LIBRETRO_ANALOG_RANGE - input_analog_deadzone);

   /* Apply sensitivity correction */
   amplitude *= input_analog_sensitivity;

   /* Map to Atari 5200 values */
   if (amplitude >= 0.0f)
      return JOY_5200_CENTER +
            (unsigned int)(((float)(JOY_5200_MAX - JOY_5200_CENTER) * amplitude) + 0.5f);

   return JOY_5200_CENTER +
         (unsigned int)(((float)(JOY_5200_CENTER - JOY_5200_MIN) * amplitude) - 0.5f);
}

static unsigned a5200_get_analog_numpad_key(int input_x, int input_y)
{
   /* Right analog stick maps to the following
    * number pad keys:
    *    1 2 3
    *    4   6
    *    7 8 9 */
   double input_radius;
   double input_angle;
   unsigned numpad_index;

   /* Convert to amplitude */
   float amplitude_x = (float)((input_x > input_analog_deadzone) ?
         (input_x - input_analog_deadzone) :
               (input_x + input_analog_deadzone)) /
                     (float)(LIBRETRO_ANALOG_RANGE - input_analog_deadzone);
   float amplitude_y = (float)((input_y > input_analog_deadzone) ?
         (input_y - input_analog_deadzone) :
               (input_y + input_analog_deadzone)) /
                     (float)(LIBRETRO_ANALOG_RANGE - input_analog_deadzone);

   /* Convert to polar coordinates: part 1 */
   input_radius = sqrt((double)(amplitude_x * amplitude_x) +
         (double)(amplitude_y * amplitude_y));
   
   /* Check if radius is above threshold */
   if (input_radius > A5200_VIRTUAL_NUMPAD_THRESHOLD)
   {
      /* Convert to polar coordinates: part 2 */
      input_angle = atan2((double)amplitude_y, (double)amplitude_x);
      
      /* Adjust rotation offset... */
      input_angle = (2.0 * PI) - (input_angle + PI);
      input_angle = fmod(input_angle - (0.125 * PI), 2.0 * PI);
      if (input_angle < 0)
         input_angle += 2.0 * PI;

      /* Convert angle into numpad key index */
      numpad_index = (unsigned)((input_angle / (2.0 * PI)) * 8.0);
      /* Unnecessary safety check... */
      numpad_index = (numpad_index > 7) ? 7 : numpad_index;

      return input_analog_numpad_map[numpad_index];
   }

   return 0;
}

static void update_input(void)
{
   size_t pad;

   input_poll_cb();

   key_code   = 0;
   key_shift  = 0;
   key_consol = CONSOL_NONE;

   for (pad = 0; pad < 2; pad++)
   {
      unsigned joypad_bits = 0;
      int left_analog_x;
      int left_analog_y;
      int right_analog_x;
      int right_analog_y;
      size_t i;

      /* Set a well-defined initial state */
      joy_5200_stick[pad]          = STICK_CENTRE;
      joy_5200_trig[pad]           = 1;
      atari_analog[pad]            = 0;
      joy_5200_pot[(pad << 1) + 0] = JOY_5200_CENTER;
      joy_5200_pot[(pad << 1) + 1] = JOY_5200_CENTER;

      /* Read input */
      if (libretro_supports_bitmasks)
         joypad_bits = input_state_cb(pad, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
      else
         for (i = 0; i < (RETRO_DEVICE_ID_JOYPAD_R3+1); i++)
            joypad_bits |= input_state_cb(pad, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;

      /* D-Pad */
      if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_UP))
      {
         if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
            joy_5200_stick[pad] = STICK_UL;
         else if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
            joy_5200_stick[pad] = STICK_UR;
         else
            joy_5200_stick[pad] = STICK_FORWARD;
      }
      else if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_DOWN))
      {
         if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
            joy_5200_stick[pad] = STICK_LL;
         else if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
            joy_5200_stick[pad] = STICK_LR;
         else
            joy_5200_stick[pad] = STICK_BACK;
      }
      else if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_LEFT))
         joy_5200_stick[pad] = STICK_LEFT;
      else if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_RIGHT))
         joy_5200_stick[pad] = STICK_RIGHT;

      /* Analog joystick */
      left_analog_x = input_state_cb(pad, RETRO_DEVICE_ANALOG,
            RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
      left_analog_y = input_state_cb(pad, RETRO_DEVICE_ANALOG,
            RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);

      if ((left_analog_x < -input_analog_deadzone) ||
          (left_analog_x > input_analog_deadzone))
      {
         joy_5200_pot[(pad << 1) + 0] = a5200_get_analog_pot(left_analog_x);
         atari_analog[pad]            = 1;
      }

      if ((left_analog_y < -input_analog_deadzone) ||
          (left_analog_y > input_analog_deadzone))
      {
         joy_5200_pot[(pad << 1) + 1] = a5200_get_analog_pot(left_analog_y);
         atari_analog[pad]            = 1;
      }

      /* 1st trigger button */
      joy_5200_trig[pad] = joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_A) ? 0 : 1;

      /* Internal emulation code only supports
       * 2nd trigger/start/pause/number pad input
       * from player 1 */
      if (pad > 0)
         break;

      /* 2st trigger button */
      if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_B))
      {
         input_shift_ctrl ^= AKEY_SHFT;
         key_shift         = 1;
      }
      key_code += input_shift_ctrl ? 0x40 : 0x00;

      /* Start/pause */
      if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_SELECT))
         key_code += AKEY_5200_PAUSE;
      else if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_START))
         key_code += AKEY_5200_START;
      /* Number pad */
      else if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_Y))
         key_code += AKEY_5200_ASTERISK;
      else if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_X))
         key_code += AKEY_5200_HASH;
      else if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_R))
         key_code += AKEY_5200_0;
      else if (joypad_bits & (1 << RETRO_DEVICE_ID_JOYPAD_R3))
         key_code += AKEY_5200_5;
      else if (!input_dual_stick_enabled)
      {
         /* If the dual stick hack is disabled,
          * right analog stick is used as a virtual
          * number pad, with directions mapping to
          * the physical key layout */
         right_analog_x = input_state_cb(pad, RETRO_DEVICE_ANALOG,
               RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
         right_analog_y = input_state_cb(pad, RETRO_DEVICE_ANALOG,
               RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);

         if ((right_analog_x < -input_analog_deadzone) ||
             (right_analog_x > input_analog_deadzone)  ||
             (right_analog_y < -input_analog_deadzone) ||
             (right_analog_y > input_analog_deadzone))
            key_code += a5200_get_analog_numpad_key(right_analog_x, right_analog_y);
      }

      /* Dual stick hack
       * > Maps player 2's joystick to the right
       *   analog stick of player 1's RetroPad */
      if (input_dual_stick_enabled)
      {
         right_analog_x = input_state_cb(pad, RETRO_DEVICE_ANALOG,
               RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
         right_analog_y = input_state_cb(pad, RETRO_DEVICE_ANALOG,
               RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);

         if ((right_analog_x < -input_analog_deadzone) ||
             (right_analog_x > input_analog_deadzone))
         {
            joy_5200_pot[((pad + 1) << 1) + 0] = a5200_get_analog_pot(right_analog_x);
            atari_analog[pad + 1]              = 1;
         }

         if ((right_analog_y < -input_analog_deadzone) ||
             (right_analog_y > input_analog_deadzone))
         {
            joy_5200_pot[((pad + 1) << 1) + 1] = a5200_get_analog_pot(right_analog_y);
            atari_analog[pad + 1]              = 1;
         }

         /* When dual stick hack is enabled,
          * player 2 input is disabled */
         joy_5200_stick[pad + 1] = STICK_CENTRE;
         joy_5200_trig[pad + 1]  = 1;
         break;
      }
   }
}

static void update_video(void)
{
   uint16_t *video_buffer_ptr = video_buffer;
   size_t x, y;

   for (y = 0; y < A5200_VIDEO_HEIGHT; y++)
   {
      uint8_t *screen_buffer_ptr = a5200_screen_buffer +
            ((y + A5200_VIDEO_OFFSET_Y) * A5200_SCREEN_BUFFER_WIDTH) +
            A5200_VIDEO_OFFSET_X;

      for (x = 0; x < A5200_VIDEO_WIDTH; x++)
         *(video_buffer_ptr++) = a5200_palette_rgb565[*(screen_buffer_ptr + x)];
   }

   video_cb(video_buffer, A5200_VIDEO_WIDTH, A5200_VIDEO_HEIGHT,
         A5200_VIDEO_WIDTH << 1);
}

static void update_audio(void)
{
   uint8_t *samples_ptr = audio_samples_buffer;
   int16_t *out_ptr   = audio_out_buffer;
   size_t i;

   Pokey_process(samples_ptr, A5200_AUDIO_BUFFER_SIZE);

   if (audio_low_pass_enabled)
   {
      /* Restore previous sample */
      int32_t low_pass = audio_low_pass_prev;
      /* Single-pole low-pass filter (6 dB/octave) */
      int32_t factor_a = audio_low_pass_range;
      int32_t factor_b = 0x10000 - factor_a;

      for(i = 0; i < A5200_AUDIO_BUFFER_SIZE; i++)
      {
         /* Convert 8u mono to 16s stereo */
         int16_t sample_16 = (int16_t)((*(samples_ptr++) - 128) << 8);

         /* Apply low-pass filter */
         low_pass = (low_pass * factor_a) + (sample_16 * factor_b);

         /* 16.16 fixed point */
         low_pass >>= 16;

         /* Update output buffer */
         *(out_ptr++) = (int16_t)low_pass;
         *(out_ptr++) = (int16_t)low_pass;
      }

      /* Save last sample for next frame */
      audio_low_pass_prev = low_pass;
   }
   else
   {
      for(i = 0; i < A5200_AUDIO_BUFFER_SIZE; i++)
      {
         /* Convert 8u mono to 16s stereo */
         int16_t sample_16 = (int16_t)((*(samples_ptr++) - 128) << 8);

         /* Update output buffer */
         *(out_ptr++) = sample_16;
         *(out_ptr++) = sample_16;
      }
   }

   audio_batch_cb(audio_out_buffer, A5200_AUDIO_BUFFER_SIZE);
}

/************************************
 * libretro implementation
 ************************************/

void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { audio_cb = cb; }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { input_poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_state_cb = cb; }

void retro_set_environment(retro_environment_t cb)
{
   bool option_cats_supported;
   struct retro_vfs_interface_info vfs_iface_info;
   static const struct retro_system_content_info_override content_overrides[] = {
      {
         "a52|bin", /* extensions */
         false,     /* need_fullpath */
         true       /* persistent_data */
      },
      { NULL, false, false }
   };

   environ_cb = cb;

   /* Initialise core options */
   libretro_set_core_options(environ_cb, &option_cats_supported);

   /* Initialise VFS */
   vfs_iface_info.required_interface_version = 1;
   vfs_iface_info.iface                      = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
      filestream_vfs_init(&vfs_iface_info);

   /* Request a persistent content data buffer */
   environ_cb(RETRO_ENVIRONMENT_SET_CONTENT_INFO_OVERRIDE,
         (void*)content_overrides);
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name = "a5200";
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif
   info->library_version = "2.0.2 " GIT_VERSION;
   info->need_fullpath = false;
   info->valid_extensions = "a52|bin";
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));
   info->timing.fps            = (double)A5200_FPS;
   info->timing.sample_rate    = (double)SOUND_SAMPLE_RATE;
   info->geometry.base_width   = A5200_VIDEO_WIDTH;
   info->geometry.base_height  = A5200_VIDEO_HEIGHT;
   info->geometry.max_width    = A5200_VIDEO_WIDTH;
   info->geometry.max_height   = A5200_VIDEO_HEIGHT;
   info->geometry.aspect_ratio = 4.0f / 3.0f;
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
   (void)port;
   (void)device;
}

size_t retro_serialize_size(void) 
{
   return A5200_SAVE_STATE_SIZE;
}

bool retro_serialize(void *data, size_t size)
{
   if (SaveAtariState(data, size, 0))
      return true;

   return false;
}

bool retro_unserialize(const void *data, size_t size)
{
   if (ReadAtariState(data, size))
      return true;

   return false;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned index, bool enabled, const char *code)
{
   (void)index;
   (void)enabled;
   (void)code;
}

bool retro_load_game(const struct retro_game_info *info)
{
   const struct retro_game_info_ext *info_ext = NULL;
   enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;

   /* a5200 requires a persistent ROM data buffer */
   rom_buf  = NULL;
   rom_data = NULL;
   rom_size = 0;

   if (environ_cb(RETRO_ENVIRONMENT_GET_GAME_INFO_EXT, &info_ext) &&
       info_ext->persistent_data)
   {
      rom_data = (const uint8_t*)info_ext->data;
      rom_size = info_ext->size;
   }

   /* If frontend does not support persistent
    * content data, must create a copy */
   if (!rom_data)
   {
      if (!info)
         goto error;

      rom_size = info->size;
      rom_buf  = (uint8_t*)malloc(rom_size);

      if (!rom_buf)
      {
         a5200_log(RETRO_LOG_INFO, "Failed to allocate ROM buffer.\n");
         goto error;
      }

      memcpy(rom_buf, (const uint8_t*)info->data, rom_size);
      rom_data = (const uint8_t*)rom_buf;
   }

   /* Set colour depth */
   if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt))
   {
      a5200_log(RETRO_LOG_INFO, "RGB565 is not supported.\n");
      goto error;
   }

   /* Set input descriptors */
   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS,
         input_descriptors);

   /* Load bios */
   if (!load_bios())
      goto error;

   /* Load game */
   if (!Atari800_OpenFile(rom_data, rom_size, 1, 1, 1))
   {
      a5200_log(RETRO_LOG_INFO, "Failed to load content: %s\n", info->path);
      goto error;
   }

   Atari800_Initialise();

   /* Apply initial core options */
   check_variables();

   return true;

error:
   if (rom_buf)
      free(rom_buf);
   rom_buf = NULL;
   return false;
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
   (void)game_type;
   (void)info;
   (void)num_info;
   return false;
}

void retro_unload_game(void) 
{
   CART_Remove();
   Atari800_Exit(0);

   if (rom_buf)
      free(rom_buf);

   rom_buf               = NULL;
   rom_data              = NULL;
   rom_size              = 0;
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC;
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void *retro_get_memory_data(unsigned id)
{
   return NULL;
}

size_t retro_get_memory_size(unsigned id)
{
   return 0;
}

void retro_init(void)
{
   struct retro_log_callback log;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

   a5200_screen_buffer = (uint8_t*)malloc(A5200_SCREEN_BUFFER_WIDTH *
         A5200_SCREEN_BUFFER_HEIGHT * sizeof(uint8_t));
#ifdef _3DS
   video_buffer = (uint16_t*)linearMemAlign(A5200_VIDEO_WIDTH *
         A5200_VIDEO_HEIGHT * sizeof(uint16_t), 128);
#else
   video_buffer = (uint16_t*)malloc(A5200_VIDEO_WIDTH *
         A5200_VIDEO_HEIGHT * sizeof(uint16_t));
#endif

   memset(a5200_screen_buffer, 0, A5200_SCREEN_BUFFER_WIDTH *
         A5200_SCREEN_BUFFER_HEIGHT * sizeof(uint8_t));
   memset(video_buffer, 0, A5200_VIDEO_WIDTH *
         A5200_VIDEO_HEIGHT * sizeof(uint16_t));

   /* Mono */
   audio_samples_buffer = (uint8_t*)malloc(A5200_AUDIO_BUFFER_SIZE *
         sizeof(uint8_t));
   /* Stereo */
   audio_out_buffer     = (int16_t*)malloc((A5200_AUDIO_BUFFER_SIZE << 1) *
         sizeof(int16_t));

   input_shift_ctrl         = 0;
   input_dual_stick_enabled = false;
   input_analog_sensitivity = 1.0f;
   audio_low_pass_prev      = 0;

   initialise_palette();
   Screen_Initialise(a5200_screen_buffer);
}

void retro_deinit(void)
{
   libretro_supports_bitmasks = false;
   input_shift_ctrl           = 0;
   input_dual_stick_enabled   = false;
   input_analog_sensitivity   = 1.0f;
   audio_low_pass_prev        = 0;

   if (a5200_screen_buffer)
   {
      free(a5200_screen_buffer);
      a5200_screen_buffer = NULL;
   }

   if (video_buffer)
   {
#ifdef _3DS
      linearFree(video_buffer);
#else
      free(video_buffer);
#endif
      video_buffer = NULL;
   }

   if (audio_samples_buffer)
   {
      free(audio_samples_buffer);
      audio_samples_buffer = NULL;
   }

   if (audio_out_buffer)
   {
      free(audio_out_buffer);
      audio_out_buffer = NULL;
   }
}

void retro_reset(void)
{
   Warmstart();
}

void retro_run(void)
{
   bool options_updated = false;

   /* Core options */
   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &options_updated) &&
       options_updated)
      check_variables();

   /* Update input */
   update_input();

   /* Run emulator */
   Atari800_Frame(0);

   /* Output video */
   update_video();

   /* Output audio */
   update_audio();
}
