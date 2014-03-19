/*
 * Project: 12V DC Uninterruptable Power Supply
 * File: usvfirmware.h
 * Author: Thorin Hopkins (topy at untergrund dot net)
 * Copyright: (C) 2014 by Thorin Hopkins
 * License: GNU GPL v3 (see LICENSE.txt)
 * Web: https://github.com/Topy44/ups
 */ 


#ifndef USVFIRMWARE_H_
#define USVFIRMWARE_H_

// -- Constants
#define STATUSFREQ 1000
#define LEDFREQ 500

#define UPDATEDELAY 200	// Do not update LEDs and alarm after switching for X

#define VREF 3	// Reference voltage for ADC
#define VDIV1 (25.5+4.9)/4.9	// Battery 1 voltage divider
#define VDIV2 (25.5+12.4)/12.4	// Battery 2 voltage divider

#define BATMAX 8.15
#define BATLOWV 7.0
#define BATVLOWV 6.8
#define BATSHUTOFF 6.4
#define BATCYCLE 7.8

// Switching delays
#define ONDELAY 2000
#define SWITCHDELAY 5

// Fan delays
#ifdef DEBUG
	#define FANEXTPOWERON 30000L	// Run for 30 seconds in debug builds
#else
	#define FANEXTPOWERON 180*60000L	// Run fan for 3 hours when ext. power turned on
#endif

// -- Aliases
#define LOFF 0
#define LRED 1
#define LGREEN 2

/*
#define SPOWER 0
#define SSWITCH 1
#define SCHARGE 2
#define SBATLOW 3
#define SBATVLOW 4
*/

// -- Prototypes
void pwrled(int col);
void statled(int col);
void fanrun(unsigned long ms);
void fancheck();
void ledcheck();
unsigned long adcread(uint8_t ch);
void buz(bool state);

#endif /* USVFIRMWARE_H_ */