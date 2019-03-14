/****************************************************
 FileName 	:   UserDefs.h
 Date		:	2012.07.27
 Author		:	Yeon June	 
****************************************************/

/** Duplicate check ********************************/
#ifndef __USERDEFS_H__
#define __USERDEFS_H__

/** Includes ***************************************/
#include "./DEFINE/GenericTypeDefs.h"

#define TESTER
// 120825 YJ Serial Number in PCR_Task.c

// YJ 130202
#define FIRMWARE_VERSION	0x01

#define RX_BUFSIZE		20		// Buffer Size
#define TX_BUFSIZE		20		// For BLE end byte character(1byte added)

typedef enum _RXINDEX 			//	Rx Buffer matching at index
{								//  Used in PCR_Task.c
	RX_CMD = 0x00,
	RX_LABEL,
	RX_TEMP,
	RX_TIMEH,
	RX_TIMEL,
	RX_LIDTEMP,
	RX_REQLINE,
	RX_INDEX,
	RX_CHECKSUM,
} RXINDEX;

typedef enum _TXINDEX			//	Tx Buffer matching at index
{
	TX_STATE = 0x00,			//	0	
	TX_CURRENT_LABEL,
	TX_GOTO_COUNT,
	TX_LABEL_COUNT,
	TX_LINE_TIME_H,
	TX_LINE_TIME_L,
	TX_TOTAL_TIME_H,
	TX_TOTAL_TIME_L,
	TX_LIDTEMP_H,
	TX_LIDTEMP_L,
	TX_CHAMBER_TEMP_H,
	TX_CHAMBER_TEMP_L,
	TX_HEAT_TEMP,
	TX_CURRENT_OPER,
	TX_ERROR_REQLINE,	// use both error and reqline
	TX_REQ_LABEL,
	TX_REQ_TEMP,
	TX_REQ_TIME_H,
	TX_REQ_TIME_L,
	TX_CHECKSUM,
	//TX_END_OF_DATA,
} TXINDEX;

typedef enum _STATE				//	Device State.
{								//  Used in PCR_Task.c
	STATE_READY = 0x01,
	STATE_RUN,
	STATE_PCREND,
	STATE_STOP,
	STATE_TASK_WRITE,
} STATE;

typedef enum _COMMAND			//	Rx_Buffer[0] = Set Command
{								//  Used in PCR_Task.c
	CMD_NOP = 0x00,
	CMD_TASK_WRITE,
	CMD_TASK_END,
	CMD_GO,
	CMD_STOP,
	CMD_BOOTLOADER,
} COMMAND;

// Constants about action format.
// Action format : 4B
// [1B : Action Number] [1B : Set Temperature in cel. deg. -128~127] [ 2B : Hold time in sec.]
// AF : Used PCR_Task.c

#define AF_GOTO		250

//	Line Info Structure
typedef struct _ACTION
{
	unsigned char Label;
	unsigned char Temp;
	UINT Time;
} Action;

//	Differential between Target and CurrentTemp ? 
#define HEATERTEMP_DELTA			5.
#define TARGETTEMP_DELTA			0.5

// Heater & Fan duration
#define LID_HEATER_DURATION			10
#define CHM_FAN_DURATION			10
#define SYS_FAN_DURATION			10

//	Chamber & System Temperature Duration
#define CHMFAN_TEMP_DURATION_0		20.
#define CHMFAN_TEMP_DURATION_50		30.
#define CHMFAN_TEMP_DURATION_100	40.

#define SYSFAN_TEMP_DURATION_0		30.
#define SYSFAN_TEMP_DURATION_50		40.
#define SYSFAN_TEMP_DURATION_100	50.

#define HEATEROFF_TEMP				10.

//	Temperature Limitness
#define	LID_TEMP_LIMIT				120.
#define CHM_TEMP_LIMIT				100.
#define SINK_TEMP_LIMIT				80.

// TimeOut Value
#define ARRIVAL_TIMEOUT			20000	//	YJ

// Refrigerator Temperature
#define REFRIGERATOR_TEMP		10.

// Temperature Sampling Count
#define SAMPLING_COUNT			0x05

// Select Temperature Menu
#define HEATER_TEMP				0x01
#define CHAMBER_TEMP			0x02
#define HEATSINK_TEMP			0x03

// Select Sensor Menu ( Refer HardwareProfile - PICDEM FSUSB.h File in ADC Section )
#define SENSOR_HEATER			(0b00000000)
#define SENSOR_CHAMBER			(0b00000100)
#define SENSOR_HEATSINK			(0b00010000)

#define TARGET_TEMP_INIT		25.

typedef enum _OPRSTATE
{
	READY_INIT = 0x00,
	READY_COMPLETE,
	READY_INCOMPLETE,
	RUN_REFRIGERATOR
} OPR_STATE;

// Chamber Duration PWM_MAX
#define CHAMBER_PWM_MAX			1023.0

// Initalize TimeOut Count
#define TIMEOUT_COUNT			5000

#endif