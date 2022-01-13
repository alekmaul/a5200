#ifndef A5200_OSK_H__
#define A5200_OSK_H__

#include <stdint.h>
#include <stddef.h>

void a5200_osk_init(void);
void a5200_osk_deinit(void);

void a5200_osk_draw(uint16_t *buffer, size_t width, size_t height);
void a5200_osk_move_cursor(int delta);
unsigned a5200_osk_get_key(void);

#endif
