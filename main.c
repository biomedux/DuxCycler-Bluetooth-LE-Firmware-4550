/****************************************************
 *	FileName 	:   main.c
 *	Date		:	1. 	2012.08.25	Start
 *	Author		:	Yeon June 
****************************************************/

/** Duplicate check ********************************/
#ifndef __MAIN_C__
#define __MAIN_C__

/** Includes ***************************************/
#include "./CONFIG/BootLoader.h"	//	for use interrupt & bootloader
#include "./CONFIG/Init.h"			//	for user Initialize 


/****************************************************
 * Function:        void main(void)
 *
 * Overview:        Main program entry point.
 *
 * Note:            None
 ***************************************************/
void main(void)
{
	Init();		// all initialize in .CONFIG/init.c

    while(1)
    {
		MainLooper();	// usb routine & PCR routine in .CONFIG/Init.h
    }
}

#endif	/** End __MAIN_C__ *************************/




