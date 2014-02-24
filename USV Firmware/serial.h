#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <stdio.h>

#define BAUD 9600UL

#define RECBUF_SIZE 256

// No idea why this is needed
#ifdef __cplusplus
extern "C" {
#endif
	
extern volatile int recBuffer[RECBUF_SIZE];
extern volatile uint16_t recReadIndex;
extern volatile uint16_t recWriteIndex;

//static FILE uart_stdio;

void serial_init(void);
int s_putchr(char c, FILE *stream);
int s_getchr(FILE *stream);
int s_hasdata(void);

#ifdef __cplusplus
}
#endif

#endif
