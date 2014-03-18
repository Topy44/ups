/*
 * Project: 12V DC Uninterruptable Power Supply
 * File: pins.h
 * Author: Thorin Hopkins (topy at untergrund dot net)
 * Copyright: (C) 2014 by Thorin Hopkins
 * License: GNU GPL v3 (see LICENSE.txt)
 * Web: https://github.com/Topy44/ups
 */ 

#ifndef PINS_H_
#define PINS_H_

#define RX			0,D
#define TX			1,D
#define OPTO		2,D
#define BUZ			3,D
#define CHARGESEL	4,D
#define FANCTRL		5,D
#define SOURCESEL2	6,D
#define SOURCESEL1	7,D
#define AUX			0,B
#define STATLEDA	1,C
#define STATLEDB	2,C
// #define STATLEDA	2,B
// #define STATLEDB	1,B
#define OUTCTRL		0,C
#define PWRLEDA		1,B
#define PWRLEDB		2,B
// #define PWRLEDA		2,C
// #define PWRLEDB		1,C
#define MECHSW		3,C
#define BAT1STAT	4,C
#define BAT2STAT	5,C

// ADC Channels
#define BAT1V 6
#define BAT2V 7


#endif /* PINS_H_ */