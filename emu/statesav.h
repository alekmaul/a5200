#ifndef _STATESAV_H_
#define _STATESAV_H_

#include <stdint.h>
#include "atari.h"

int SaveAtariState(uint8_t *data, size_t size, UBYTE SaveVerbose);
int ReadAtariState(const uint8_t *data, size_t size);

void SaveUBYTE(const UBYTE *data, int num);
void SaveUWORD(const UWORD *data, int num);
void SaveINT(const int *data, int num);
void SaveFNAME(const char *filename);

void ReadUBYTE(UBYTE *data, int num);
void ReadUWORD(UWORD *data, int num);
void ReadINT(int *data, int num);
void ReadFNAME(char *filename);

#endif
