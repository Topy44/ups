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
#include "millis.h"		// Uses TIMER0

enum ledstatus
{
	OFF,
	RED,
	GREEN,
	FLASHRED,
	FLASHGREEN,
	FASTRED,
	FASTGREEN
};

volatile ledstatus ledStatusA = OFF;
volatile ledstatus ledStatusB = OFF;

volatile bool powerStatusChanged = false;
volatile millis_t powerStatusTime = 0;
volatile bool powerStatus = false;

volatile uint8_t switchStatus = false;	// Needs to be uint8_t because it gets compared to a get() result
volatile millis_t switchStatusTime = 0;

volatile millis_t fanTurnOnTime = 0;
volatile millis_t fanStatusTime = 0;
volatile bool fanStatus = false;
volatile bool fanOverride = false;

volatile bool chargeStatus = false;

volatile millis_t adcTimer = 0;
volatile millis_t statusTimer = 0;
volatile millis_t ledTimer = 0;

int main(void)
{
	_delay_ms(200);	// Wait a bit to escape reset loops

	millis_init();
	serial_init();

	printf("12V USV v0.2.0\r\n(c)2014 Thorin Hopkins\r\n");

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
	out(BUZ);

	off(BUZ);	// Piezo input high -> off
		
	EICRA |= (1<<ISC00);	// INT0 trigger on level change
	EIMSK |= (1<<INT0);		// Enable INT0
	
	ADMUX |= (1<<REFS0) | (1<<REFS1);	// Use external voltage reference
	ADCSRA |= (1<<ADEN) | (1<<ADPS0) | (1<<ADPS1) | (1<<ADPS2);	// Enable ADC, Prescaler F_CPU/128
	
	TCCR2A |= (1<<WGM21);	// CTC Mode
	TCCR2B |= (1<<CS21) | (1<<CS22);	// Prescaler F_CPU/256
	OCR2A = 0x0C;
	OCR2B = OCR2A - 1;
	
	sei();	// Enable interrupts. Use atomic blocks from here on

	switchStatus = !get(MECHSW);

	// Figure out initial state
	if (get(OPTO))
	{
		// External Power turned on
		powerStatusChanged = true;
		powerStatusTime = millis();
		#ifdef DEBUG
			printf("External power turned on.\r\n");
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
			off(CHARGESEL);
			_delay_ms(SWITCHDELAY);
			off(SOURCESEL1);
			_delay_ms(SWITCHDELAY);
			off(SOURCESEL2);
		}
		#ifdef DEBUG
			printf("External power turned off.\r\n");
		#endif
	}
	
	buz(true);	// BEEP!
	_delay_ms(200);
	buz(false);
	
	adcTimer = millis();
	statusTimer = millis();
	ledTimer = millis();

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
				#ifdef DEBUG
					printf("Mech.Sw. turned on.\r\n");
				#endif
			}
			else
			{
				// Mech. Switch turned off
				off(OUTCTRL);	// Turn off output
				fanrun(FANMECHSWOFF);
				#ifdef DEBUG
					printf("Mech.Sw. turned off.\r\n");
				#endif
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
				fanrun(FANEXTPOWERON);
			}
		}
		
		if (powerStatus && (!get(BAT1STAT) || !get(BAT2STAT))) chargeStatus = true;	// Ignore charge status inputs if ext. power is off
		else chargeStatus = false;

		if (!get(MECHSW) || chargeStatus) fanOverride = true;	// Force fan on if mech. switch is on or batteries are charging
		else fanOverride = false;
		
		if (fanOverride && !fanStatus) fanrun(100);
		
		fancheck();

		double bat1voltage;
		double bat2voltage;
		bat1voltage = ((double)adcread(BAT1V)/1024*VREF)*VDIV1;
		bat2voltage = ((double)adcread(BAT2V)/1024*VREF)*VDIV2;
		if (!chargeStatus) bat1voltage -= bat2voltage;
		
		#ifdef DEBUG
			if (millis() - statusTimer >= STATUSFREQ)
			{
				statusTimer = millis();
				printf("System status at %lu:\r\nMechSw: %u - Fan: %u - Charging: %u (%u, %u) - ExtPower: %u - LED Status: %u:%u\r\n", millis(), !get(MECHSW), fanStatus, chargeStatus, !(bool)get(BAT1STAT), !(bool)get(BAT2STAT), powerStatus, ledStatusA, ledStatusB);
				printf("Battery 1 Voltage: %fV (Raw %lu) - Battery 2 Voltage: %fV (Raw: %lu)\r\n", bat1voltage, adcread(BAT1V), bat2voltage, adcread(BAT2V));
				printf("TCNT2: %u\r\n", TCNT2);
			}
		#endif

		bool batLowVoltage = false;
		bool batVeryLowVoltage = false;
		
		if (bat1voltage < BATLOWV || bat2voltage < BATLOWV) batLowVoltage = true;
		else batLowVoltage = false;
		if (bat1voltage < BATVLOWV || bat2voltage < BATVLOWV) batVeryLowVoltage = true;
		else batVeryLowVoltage = false;
		
		// Ext. Power on
		if (powerStatus && !switchStatus)
		{
			// LEDs: Green/Green			On, ext. power
			ledStatusA = GREEN;
			ledStatusB = GREEN;
		}
		else if (powerStatus && !chargeStatus)
		{
			// LEDs: Flash Red/Off			Off, charging
			ledStatusA = FLASHRED;
			ledStatusB = OFF;
		}
		else if (powerStatus && fanStatus)
		{
			// LEDs: Off/Flash Green			Off, ext. power, fan running
			ledStatusA = OFF;
			ledStatusB = FLASHGREEN;
		}
	
		// Ext. power off
		else if (!switchStatus && batLowVoltage)
		{
			// LEDs: Flash Red/Off			On, bat. power, bat. low
			ledStatusA = FLASHRED;
			ledStatusB = OFF;
		}
		else if (!switchStatus && batVeryLowVoltage)
		{
			// LEDs: Flash Red (fast)/Off		On, bat. power, bat. low, sound alarm
			ledStatusA = FASTRED;
			ledStatusB = OFF;
		}
		else if (!switchStatus)
		{
			// LEDs: Red/Off				On, bat. power
			ledStatusA = RED;
			ledStatusB = OFF;
		}
		else if (fanStatus)
		{
			// LEDs: Off/Flash Red			Off, bat.power, fan running
			ledStatusA = FLASHRED;
			ledStatusB = OFF;
		}
		else
		{
			// LEDs: Off/Off				Undefined state
			ledStatusA = OFF;
			ledStatusB = OFF;
		}

		if (!switchStatus && !powerStatus && (bat1voltage < BATSHUTOFF || bat2voltage < BATSHUTOFF));
		{
			buz(true);
			_delay_ms(1000);
			buz(false);
			_delay_ms(500);
			buz(true);
			_delay_ms(1000);
			buz(false);
			_delay_ms(500);
			buz(true);
			_delay_ms(1000);
			buz(false);
			off(FANCTRL);
			_delay_ms(30000);	// Not an infinite loop to avoid hanging in case of error
		}

		// Handle LEDs and piezo buzzer
		static ledstatus ledStatusAOld = OFF;
		static ledstatus ledStatusBOld = OFF;
		
		if (ledStatusA != ledStatusAOld || ledStatusB != ledStatusBOld)
		{
			ledStatusAOld = ledStatusA;
			ledStatusBOld = ledStatusB;
			ledcheck();
			printf("Forcing led status change...\r\n");
		}
		
		if (millis() - ledTimer >= LEDFREQ)
		{
			ledTimer = millis();
			ledcheck();
		}
		
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
		off(CHARGESEL);
		_delay_ms(SWITCHDELAY);
		off(SOURCESEL1);
		_delay_ms(SWITCHDELAY);
		off(SOURCESEL2);
		_delay_ms(ONDELAY);
	}
}

void ledcheck()
{
	static uint8_t count = 0;
	count++;
	count = count & 0x03;
	
	switch (ledStatusA)
	{
		case OFF:
			off(PWRLEDA);
			off(PWRLEDB);
			buz(false);
			break;
		case RED:
			on(PWRLEDA);
			off(PWRLEDB);
			buz(false);
			break;
		case GREEN:
			off(PWRLEDA);
			on(PWRLEDB);
			buz(false);
			break;
		case FASTRED:
			off(PWRLEDB);
			if (get(PWRLEDA)) off(PWRLEDA);
			else on(PWRLEDA);
			if (count >= 2) buz(true);
			else buz(false);
			break;
		case FLASHRED:
			off(PWRLEDB);
			if (count >= 2) on(PWRLEDA);
			else off(PWRLEDA);
			buz(false);
			break;
		case FASTGREEN:
			off(PWRLEDA);
			if (get(PWRLEDB)) off(PWRLEDB);
			else on(PWRLEDB);
			if (count >= 2) buz(true);
			else buz(false);
			break;
		case FLASHGREEN:
			off(PWRLEDA);
			if (count >= 2) on(PWRLEDB);
			else off(PWRLEDB);
			buz(false);
			break;
	}

	switch (ledStatusB)
	{
		case OFF:
			off(STATLEDA);
			off(STATLEDB);
			buz(false);
			break;
		case RED:
			on(STATLEDA);
			off(STATLEDB);
			buz(false);
			break;
		case GREEN:
			off(STATLEDA);
			on(STATLEDB);
			buz(false);
			break;
		case FASTRED:
			off(STATLEDB);
			if (get(STATLEDA)) off(STATLEDA);
			else on(STATLEDA);
			if (count >= 2) buz(true);
			else buz(false);
			break;
		case FLASHRED:
			off(STATLEDB);
			if (count >= 2) on(STATLEDA);
			else off(STATLEDA);
			buz(false);
			break;
		case FASTGREEN:
			off(STATLEDA);
			if (get(STATLEDB)) off(STATLEDB);
			else on(STATLEDB);
			if (count >= 2) buz(true);
			else buz(false);
			break;
		case FLASHGREEN:
			off(STATLEDA);
			if (count >= 2) on(STATLEDB);
			else off(STATLEDB);
			buz(false);
			break;
	}
}

void fanrun(unsigned long ms)
{
	// Turn fan on for a period of time
	on(FANCTRL);
	fanStatus = true;
	fanTurnOnTime = millis();
	
	#ifdef DEBUG
		printf("Running fan for %lu ms.\r\n", ms);
	#endif
	
	if (fanStatusTime < ms)
	{
		fanStatusTime = ms;
	}
}

void fancheck()
{
	// Check if its time to turn the fan off
	if (fanStatus && (millis() - fanTurnOnTime >= fanStatusTime))
	{
		if (!fanOverride)
		{
			off(FANCTRL);
			fanStatus = false;
			#ifdef DEBUG
				printf("Turning fan off. Delay was %lu ms.\r\n", fanStatusTime);
			#endif
			fanStatusTime = 0;

			#ifdef DEBUG
				if (!powerStatus) printf("System shutting down...\r\n");
			#endif
		}
	}
}

unsigned long adcread(uint8_t ch)
{
	ADMUX = ch;
	ADCSRA |= (1 << ADSC);
	while (!(ADCSRA & (1 << ADIF)));
	ADCSRA |= (1 << ADIF);
	long value;
	value = ADCL;
	value += (ADCH<<8);

	return (value);
}

void buz(bool state)
{
	if (state) TCCR2A |= (1<<COM2B0);	// Toggle OC2B on Compare Match
	else TCCR2A &= ~(1<<COM2B0); // Normal operation
	#ifdef DEBUG
		if (state) printf("Buzzer turned on...\r\n");
		else printf("Buzzer turned off...\r\n");
	#endif
}