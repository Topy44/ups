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
//#define FANDELAY 5*60000	// 5 Minutes
#define FANDELAY 5000
#define SWITCHDELAY 5

#define CTC_MATCH_OVERFLOW ((F_CPU / 1000) / 8)

// Aliases
#define LOFF 0
#define LRED 1
#define LGREEN 2

// Prototypes
void pwrled(int col);
void statled(int col);
long adcread(uint8_t ch);

#endif /* USVFIRMWARE_H_ */