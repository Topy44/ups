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
#include <stdbool.h>

#include "serial.h"
#include "iomacros.h"
#include "millis.h"		// Uses TIMER2

volatile bool powerStatusChanged = false;
volatile millis_t powerStatusTime = 0;
volatile bool powerStatus = false;

volatile bool switchStatus = false;
volatile millis_t switchStatusTime = 0;

volatile millis_t fanTurnOnTime = 0;
volatile millis_t fanRunningTime = 0;
volatile bool fanRunning = false;
volatile bool fanOverride = false;

volatile bool chargeStatus = false;

volatile millis_t adcTimer = 0;

int main(void)
{
	millis_init();
	serial_init();

	printf("12V USV v0.2.0\r\n");

	#ifdef DEBUG
		printf("Debug build!\r\n");
	#endif

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
	
	sei();	// Enable interrupts. Use atomic blocks from here on

	switchStatus = !get(MECHSW);

	_delay_ms(200);	// Wait a bit to escape reset loops

	// Figure out initial state
	if (get(OPTO))
	{
		// External Power turned on
		powerStatusChanged = true;
		powerStatusTime = millis();
		#ifdef DEBUG
			printf("External power turned on.");
		#endif
	}
	else
	{
		// External Power turned off
		powerStatusChanged = false;
		powerStatusTime = 0;
		powerStatus = false;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
		statled(LRED);
		off(CHARGESEL);
		_delay_ms(SWITCHDELAY);
		off(SOURCESEL1);
		_delay_ms(SWITCHDELAY);
		off(SOURCESEL2);
		}
		#ifdef DEBUG
			printf("External power turned off.");
		#endif
	}
	
	_delay_ms(500);
	
	adcTimer = millis();

	printf("Init complete, entering main loop...\r\n");

	// Main loop
    while(1)
    {
		if (switchStatus != get(MECHSW))
		{
			switchStatus = get(MECHSW);
			if (!switchStatus)
			{
				// Mech. Switch turned on
				on(OUTCTRL);	// Turn on output
				pwrled(LGREEN);
			}
			else
			{
				// Mech. Switch turned off
				off(OUTCTRL);	// Turn off output
				pwrled(LOFF);
				fanrun(FANMECHSWOFF);
			}
		}
	
		if (powerStatusChanged && ((millis() - powerStatusTime) >= ONDELAY) && get(OPTO))
		{
			// Power was turned on ONDELAY ago, react to it
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				on(SOURCESEL1);
				_delay_ms(SWITCHDELAY);
				on(SOURCESEL2);
				_delay_ms(SWITCHDELAY);
				on(CHARGESEL);

				powerStatusChanged = false;
				powerStatus = true;
				statled(LGREEN);
				fanrun(FANEXTPOWERON);
			}
		}
		
		if (powerStatus && (!get(BAT1STAT) || !get(BAT2STAT))) chargeStatus = true;	// Ignore charge status inputs if ext. power is off
		else chargeStatus = false;

		if (!get(MECHSW) || chargeStatus) fanOverride = true;	// Force fan on if mech. switch is on or batteries are charging
		else fanOverride = false;
    }	// End of main loop
}

ISR(INT0_vect)
{
	if (get(OPTO))
	{
		// External Power turned on
		powerStatusChanged = true;
		powerStatusTime = millis();
	}
	else
	{
		// External Power turned off
		powerStatusChanged = false;
		powerStatusTime = 0;
		powerStatus = false;
		statled(LRED);
		off(CHARGESEL);
		_delay_ms(SWITCHDELAY);
		off(SOURCESEL1);
		_delay_ms(SWITCHDELAY);
		off(SOURCESEL2);
		_delay_ms(ONDELAY);
	}
}

void fanrun(unsigned long ms)
{
	// Turn fan on for a period of time
	on(FANCTRL);
	fanRunning = true;
	fanTurnOnTime = millis();
	
	#ifdef DEBUG
		printf("Running fan for %u ms.", ms);
	#endif
	
	if (fanRunningTime < ms)
	{
		fanRunningTime = ms;
	}
}

void fanon()
{
	// Turn fan on
	if (!fanRunning)
	{
		on(FANCTRL);
		fanRunning = true;
	}
}

void fancheck()
{
	// Check if its time to turn the fan off
	if (!fanOverride && fanRunning && (millis() - fanTurnOnTime >= fanRunningTime))
	{
		off(FANCTRL);
		fanRunning = false;
		#ifdef DEBUG
			printf("Turning fan off. Delay was %u ms.", fanRunningTime);
		#endif
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
