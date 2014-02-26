/*
Serial library by Lukas Schauer
Licensed under GPLv3
*/

#include "bitset.h"
#include "serial.h"
#include <avr/interrupt.h>

#ifndef whateveridontevencare
#define whateveridontevencare 0
#endif

volatile int recBuffer[RECBUF_SIZE];      // Empfangsbuffer
volatile uint16_t recReadIndex;            // Leseindex
volatile uint16_t recWriteIndex;          // Schreibindex

#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1);

static FILE uart_stdio = FDEV_SETUP_STREAM(s_putchr, s_getchr, _FDEV_SETUP_RW);

#if defined(__AVR_ATmega8__)
ISR(USART_RXC_vect)
#elif defined(__AVR_ATmega328P__)
ISR(USART_RX_vect)
#endif
{
#if defined(__AVR_ATmega8__)
    recBuffer[recWriteIndex] = UDR;
#elif defined(__AVR_ATmega328P__)
    recBuffer[recWriteIndex] = UDR0;
#endif
    if(recWriteIndex == RECBUF_SIZE-1) {
        recWriteIndex = 0;
    } else {
        recWriteIndex++;
    }
}

void serial_init(void)
{
    recReadIndex = recWriteIndex = 0;
#if defined(__AVR_ATmega8__)
    UCSRB = _UV(TXEN) | _UV(RXEN) | _UV(RXCIE); // tx/rx enabled, rx interrupt
    UCSRC = _UV(URSEL) | _UV(UCSZ1) | _UV(UCSZ0); // 8 bit, no parity, 1 stop
    UBRRL = UBRR_VAL;
    do{UDR;}while (UCSRA & (1 << RXC));
    UCSRA = (1 << TXC) | (1 << RXC);
#elif defined(__AVR_ATmega328P__)
    UCSR0B = _UV(TXEN0) | _UV(RXEN0) | _UV(RXCIE0); // tx/rx enabled, rx interrupt
    UCSR0C = _UV(UCSZ01) | _UV(UCSZ00); // 8 bit, no parity, 1 stop
    UBRR0 = UBRR_VAL;
    do{UDR0;}while (UCSR0A & (1 << RXC0));
    UCSR0A = (1 << TXC0) | (1 << RXC0);
#endif
    stdout = &uart_stdio;
    stdin = &uart_stdio;
}

int s_putchr(char c, FILE *stream) {
#if defined(__AVR_ATmega8__)
	loop_until_bit_is_set(UCSRA, UDRE);
        UDR = c;
#elif defined(__AVR_ATmega328P__)
	loop_until_bit_is_set(UCSR0A, UDRE0);
        UDR0 = c;
#endif
	return whateveridontevencare;
}

int s_hasdata(void) {
    return (recReadIndex != recWriteIndex);
}

int s_getchr(FILE *stream) {
    while(!s_hasdata()){}
    int data = recBuffer[recReadIndex];
    if(recReadIndex == RECBUF_SIZE-1) {
        recReadIndex = 0;
    } else {
        recReadIndex++;
    }
    return data;
}
