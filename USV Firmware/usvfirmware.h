/*
 * usvfirmware.h
 *
 * Created: 13.02.2014 03:27:04
 *  Author: Thorin Hopkins
 */ 

#ifndef USVFIRMWARE_H_
#define USVFIRMWARE_H_

// -- Constants
#define STATUSFREQ 5000
#define LEDFREQ 500

#define VREF 3	// Reference voltage for ADC
#define VDIV1 (25.5+4.9)/4.9	// Battery 1 voltage divider
#define VDIV2 (25.5+12.4)/12.4	// Battery 2 voltage divider

// Switching delays
#define ONDELAY 2000
#define SWITCHDELAY 5

// Fan delays
#ifdef DEBUG
	#define FANEXTPOWERON 5000
	#define FANMECHSWOFF 1500
#else
	#define FANEXTPOWERON 180*60000	// Run fan for 3 hours when ext. power turned on
	#define FANMECHSWOFF 3*60000	// Run fan for 3 minutes when mech. switch turned off
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