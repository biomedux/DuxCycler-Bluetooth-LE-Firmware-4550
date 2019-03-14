/***************************************************
 *	FileName 	:   Init.c
 *	Date		:	2012.07.27
 *				:	Modify 2012.08.25 : Add Flags
 *	Author		:	Yeon June	 
****************************************************/

/** Includes ***************************************/
#include "HardwareProfile.h"
#include "./CONFIG/Compiler.h"
#include "./CONFIG/Init.h"
#include "./DEFINE/GlobalTypeVars.h"
#include "./DEFINE/UserDefs.h"
#include "./PCR/Timer.h"
#include "./PCR/PCR_Task.h"
#include <usart.h>

/** Extern Variables & Functions *******************/
extern void BlueTooth_RxTask(void);
extern void BlueTooth_TxTask(void);
extern unsigned char ReceivedDataBuffer[RX_BUFSIZE];
extern unsigned char ToSendDataBuffer[TX_BUFSIZE];

/***************************************************
 * Function:        void Init(void)
 *
 * OverView:		All calling related to Initialized functions.
 *
 * Note:			None
 ***************************************************/
void Init(void)
{
	InitializeUser();		//	Initialize for User Variables.
	InitializeDevice();		//	Initialize PICF4550 LED, PWM, ADC Ports
}

/***************************************************
 * Function:        InitializeDevice(void)
 *
 * OverView:		Initialize Device Port, LED, Switches, ADC, PWM, TIMER, ETC..
 *
 * Note:			None
 ***************************************************/
void InitializeDevice(void)
{

	//Initialize all of the LED pins
	mInitAllLEDs();	

    //Initialize all of the push buttons
    mInitAllSwitches();

	//initialize adc for ntc sensor
	InitADC();

	//initialize lid heater
	InitHeater();

	//initialize fan
	Init_ChamberFan();
	Init_SystemFan();

	//initialize pwm 	
	Stop_PWM_MODE();

	// Change the position by TIMER init routine.
	// YJ 130204
	// Initialize USART for Bluetooth
	OpenUSART( USART_TX_INT_OFF &
			   USART_RX_INT_ON &	/* USE PIR1.RCIF interrupt. Set INTCON.GIE and INTCON.PEIE bits	*/
			   USART_ASYNCH_MODE &
			   USART_EIGHT_BIT &
			   USART_CONT_RX &
			   USART_BRGH_HIGH,
			   25				);

	RCONbits.IPEN = 1;	// YJ 120825
	IPR1bits.RCIP = 1;	// Set Flag for USART
	INTCONbits.GIEH = 1;

	//initialize fan & heater control with timer0
	TIMR0_init();

	//pcr task tick
	TIMR1_init();
}

/***************************************************
 * Function:        InitializeUser(void)
 *
 * OverView:		Now only buffer Init.
 *
 * Note:			None
 ***************************************************/
void InitializeUser(void)
{
	//	Buffer init
	memset(ReceivedDataBuffer, 0, RX_BUFSIZE);
	memset(ToSendDataBuffer, 0, TX_BUFSIZE);
}

/***************************************************
 * Function:        void MainLooper(void)
 *
 * OverView:		At 0.2 ticks, Called USB_Task(), PCR_Task() functions.
 *
 * Note:			None
 ***************************************************/
void MainLooper(void)
{
	if(T100MS_Flag)		//	At 0.1 ticks. but now 0.2 ms? maybe
	{
		BlueTooth_RxTask();
		
		PCR_Task();		//	PCR Service Routine.

		BlueTooth_TxTask();
	}
}