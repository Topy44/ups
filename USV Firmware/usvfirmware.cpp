/*
 * USV_Firmware.cpp
 *
 * Created: 28.01.2014 03:37:14
 *  Author: Thorin Hopkins
 */ 
#include <stdlib.h>
#include <avr/io.h>

#include "global.h"
#include "usvfirmware.h"
#include "pins.h"

#include <avr/interrupt.h>
#include <util/delay.h>
#include <util/atomic.h>

#include "serial.h"
#include "iomacros.h"
#include "millis.h"		// Uses TIMER2

volatile bool powerStatusChanged = FALSE;
volatile millis_t powerStatusTime = 0;
volatile bool powerStatus = FALSE;

volatile bool switchStatus = FALSE;
volatile millis_t switchStatusTime = 0;

volatile bool fanStatus = FALSE;
volatile millis_t fanStatusTime = 0;
volatile bool fanDelayRunning = FALSE;

volatile millis_t adcTimer = 0;

int main(void)
{
	char buf[20];
	in(OPTO);
	in(MECHSW);
	in(BAT1STAT);
	in(BAT2STAT);
	in(AUX);
	
	pullup(MECHSW);
	pullup(BAT1STAT);
	pullup(BAT2STAT);
	
	out(FANCTRL);
	out(SOURCESEL1);
	out(SOURCESEL2);
	out(STATLEDA);
	out(STATLEDB);
	out(PWRLEDA);
	out(PWRLEDB);
	out(OUTCTRL);
	out(CHARGESEL);
		
	EICRA |= (1<<ISC00);	// INT0 trigger on level change
	EIMSK |= (1<<INT0);	// Enable INT0
	
	//ADMUX &= ~(1<<REFS0) | (1<<REFS1);
	ADMUX |= (1<<REFS0) | (1<<REFS1);
	ADCSRA |= (1<<ADEN) | (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2);	// Enable ADC, Prescaler F_CPU/128
	
	millis_init();
	serial_init();

	sei();	// Enable interrupts. Use atomic blocks from here on

	switchStatus = !get(MECHSW);

	_delay_ms(200);	// Wait a bit to escape reset loops

	// Figure out initial state
	if (get(OPTO))
	{
		// External Power turned on
		powerStatusChanged = TRUE;
		powerStatusTime = millis();
	}
	else
	{
		// External Power turned off
		powerStatusChanged = FALSE;
		powerStatusTime = 0;
		powerStatus = FALSE;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
		statled(LRED);
		off(CHARGESEL);
		_delay_ms(SWITCHDELAY);
		off(SOURCESEL1);
		_delay_ms(SWITCHDELAY);
		off(SOURCESEL2);
		}
	}
	
	_delay_ms(500);
	
	adcTimer = millis();

	// Main loop
    while(1)
    {
		if (switchStatus != get(MECHSW))
		{
			switchStatus = get(MECHSW);
			if (!switchStatus)
			{
				// Power Switch turned on
				pwrled(LGREEN);
				on(OUTCTRL);	// Turn on output
			}
			else
			{
				// Power Switch turned off
				pwrled(LOFF);
				off(OUTCTRL);	// Turn off output
			}
		}
	
		if (powerStatusChanged && ((millis() - powerStatusTime) >= ONDELAY) && get(OPTO))
		{
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				powerStatusChanged = FALSE;
				powerStatus = TRUE;
				statled(LGREEN);
				on(SOURCESEL1);
				_delay_ms(SWITCHDELAY);
				on(SOURCESEL2);
				_delay_ms(SWITCHDELAY);
				on(CHARGESEL);
			}
		}
		
		if (!get(MECHSW) || !get(BAT1STAT) || !get(BAT2STAT))
		{
			on(FANCTRL);	// Turn fan on
			fanStatus = TRUE;
			fanDelayRunning = FALSE;
		}
		else if (fanStatus && !fanDelayRunning)
		{
			fanStatusTime = millis();
			fanDelayRunning = TRUE;
		}
		
		if (fanDelayRunning && (millis() - fanStatusTime >= FANDELAY))
		{
			fanStatus = FALSE;
			fanDelayRunning = FALSE;
			off(FANCTRL);	// Turn fan off
		}
		
		if (get(MECHSW))
		{
			if (!get(BAT1STAT) || !get(BAT2STAT))
			{
				pwrled(LRED);
			}
			else
			{
				pwrled(LOFF);
			}
		}
    }
}

ISR(INT0_vect)
{
	if (get(OPTO))
	{
		// External Power turned on
		powerStatusChanged = TRUE;
		powerStatusTime = millis();
	}
	else
	{
		// External Power turned off
		powerStatusChanged = FALSE;
		powerStatusTime = 0;
		powerStatus = FALSE;
		statled(LRED);
		off(CHARGESEL);
		_delay_ms(SWITCHDELAY);
		off(SOURCESEL1);
		_delay_ms(SWITCHDELAY);
		off(SOURCESEL2);
		_delay_ms(ONDELAY);
	}
}

void pwrled(int col)
{
	switch(col)
	{
		case LOFF:
			off(PWRLEDB);
			off(PWRLEDA);
			break;
		
		case LRED:
			off(PWRLEDA);
			on(PWRLEDB);
			break;
		
		case LGREEN:
			off(PWRLEDB);
			on(PWRLEDA);
			break;
	}
}

void statled(int col)
{
	switch(col)
	{
		case LOFF:
		off(STATLEDB);
		off(STATLEDA);
		break;
		
		case LRED:
		off(STATLEDA);
		on(STATLEDB);
		break;
		
		case LGREEN:
		off(STATLEDB);
		on(STATLEDA);
		break;
	}
}
