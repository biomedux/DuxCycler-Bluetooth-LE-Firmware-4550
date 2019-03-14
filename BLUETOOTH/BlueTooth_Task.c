/***************************************************
*	FileName 	:	BlueTooth_Task.c
*	Date	 	:	2012.08.25
*	Author		:	Yeon June
****************************************************/

/** Duplicate Check ********************************/
#ifndef __BLUETOOTH_TASK_C__

/** Includes ***************************************/
#include "./CONFIG/Compiler.h"
#include "./DEFINE/UserDefs.h"
#include "./DEFINE/GenericTypeDefs.h"
#include "./DEFINE/GlobalTypeVars.h"
#include "HardwareProfile.h"
#include <usart.h>
#include <string.h>

/** Prototypes *************************************/
void USART_Rx_Interrupt_Routine(void);
void BlueTooth_RxTask(void);
void BlueTooth_TxTask(void);
void Jump_Bootloader(BYTE code);

/** Variables **************************************/
// Buffer
unsigned char ReceivedDataBuffer[RX_BUFSIZE];
unsigned char ToSendDataBuffer[TX_BUFSIZE];
unsigned char buffer = 0x00;


void USART_Rx_Interrupt_Routine(void)
{
	char rxData;

	if ( DataRdyUSART() )
	{
		rxData = ReadUSART();

		if ( Rx_Buf_Count < RX_BUFSIZE )
		{
			ReceivedDataBuffer[ Rx_Buf_Count++ ] = rxData;
		}
	}
}

/***************************************************
 * Function:        void BlueTooth_RxTask(void)
 *
 * OverView:		Receive Buffer by bluetooth
 *
 * Note:			#include <usart.h>
 ***************************************************/
void BlueTooth_RxTask(void)
{
	if( Rx_Buf_Count >= RX_BUFSIZE )	// When Rx Buffer is full or overflooding
	{
		Rx_Buf_Count = 0;
		Tx_Flag = 1;
	
		Jump_Bootloader(ReceivedDataBuffer[9]);
	}
}

/***************************************************
 * Function:        void BlueTooth_TxTask(void)
 *
 * OverView:		Send Buffer by Bluetooth
 *
 * Note:			#include <usart.h>
 ***************************************************/
void BlueTooth_TxTask(void)
{
	int idx;

	if( Tx_Flag )	// It is Blocking Safe Transfer
	{
		idx = 0;	
		while( idx < TX_BUFSIZE ) 
		{
			if( !BusyUSART() ){
				WriteUSART( ToSendDataBuffer[idx++] );
			}
		}
		Tx_Flag = 0;
	}
}

/***************************************************
 * Function:        void Jump_Bootloader(BYTE code)
 *
 * OverView:		For Firmware reset.
 *
 * Note:			None
 ***************************************************/
void Jump_Bootloader(BYTE code)
{
	if(code == CMD_BOOTLOADER)
	{
		INTCONbits.GIEH = 0;
		_asm goto 0x001C _endasm
		return;
	}
}

#endif	/** End of __BLUETOOTH_TASK_C__ *************/

