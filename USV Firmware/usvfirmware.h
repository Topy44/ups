/*
 * usvfirmware.h
 *
 * Created: 13.02.2014 03:27:04
 *  Author: Thorin Hopkins
 */ 

#ifndef USVFIRMWARE_H_
#define USVFIRMWARE_H_

// Constants
#define ONDELAY 2000
#define SWITCHDELAY 10

#define FANEXTPOWERON 180*60000	// Run fan for 3 hours when ext. power turned on

#define CTC_MATCH_OVERFLOW ((F_CPU / 1000) / 8)

// Aliases
#define LOFF 0
#define LRED 1
#define LGREEN 2

// Prototypes
void pwrled(int col);
void statled(int col);
void fanrun(unsigned long ms);
void fanon();
void fancheck();
//unsigned long adcread(uint8_t ch);

#endif /* USVFIRMWARE_H_ */