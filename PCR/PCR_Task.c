/***************************************************
*	FileName 	:	PCR_Task.c
*	Date	 	:	2012.08.11
*	Author		:	Yeon June
****************************************************/

/** Includes ***************************************/
#include "HardwareProfile.h"
#include "./CONFIG/Compiler.h"
#include "./DEFINE/GlobalTypeVars.h"
#include "./DEFINE/UserDefs.h"
#include "./PCR/PCR_Task.h"
#include "./PCR/Temp_Ctrl.h"
#include "./PCR/State.h"
#include <string.h>		

/** Variables **************************************/
// Extern Buffer
extern unsigned char ReceivedDataBuffer[RX_BUFSIZE];
extern unsigned char ToSendDataBuffer[TX_BUFSIZE];

// Counters
unsigned char Cur_Loop_Count = -1;		// Current Loop Count
int Loop_Count = -1;

// Infomation Structure
Action PCR_Task_Line[20];

// Action Info Param
unsigned char Task_Label = 0;
unsigned char Task_Temp = 0;
UINT Task_Time = 0;

// Action
int Total_ActLine = 0;
int Cur_Action_Number = -1;

// Time Variables
int Left_Time = 0;
int Left_SecTime = 0;

// TimeOut Value
float Time_Out = TIMEOUT_COUNT;

// Flags
BOOL IsTimeInfinite = FALSE;

// Pointer
unsigned char *fptr;

// Serial Const
const char Serial[] = "MyPCR050000";

/***************************************************
 * Function:        void PCR_Task(void)
 *
 * OverView:		All calling related to PCR_Task
 *
 * Note:			None
 ***************************************************/
void PCR_Task(void)
{
	// Set up the USB_Buffer for USB comm.
	T100MS_Flag = FALSE;

	// Buffer Copy
	Buffer_Copy();

	// Command Setting
	Pre_Process();

	//	Control the temperature.
	Temp_Control();

	// Check the 1s tick event.
	if( T1S_Flag )
	{
		T1S_Flag = FALSE;
		// Control the given task.
		// Read the next task, and save history
		Task_Control();
	}

	// Check Machine State & Change the State for after task control
	Check_State();

	//	Set Tx Message & Set Status for State
	Post_Process();
}

/***************************************************
 * Function:        void Buffer_Copy(void)
 *
 * OverView:		Rx Buffer Copy & Previous Rx Buffer Clear
 *
 * Note:			None
 ***************************************************/
void Buffer_Copy(void)
{
	int i;

	for(i=0; i<RX_BUFSIZE; i++)		//	Copy Buffer	only Rx
		Rx_Buffer[i] = ReceivedDataBuffer[i];

	// Set zero all buffer size
	memset(ReceivedDataBuffer, 0, RX_BUFSIZE);	
}

/***************************************************
 * Function:        void Pre_Process(void)
 *
 * OverView:		Check Command, Set Next Task.
 *
 * Note:			None
 ***************************************************/
void Pre_Process(void)
{
	int i;

	Cur_Command = Rx_Buffer[RX_CMD];

	Prev_State = Cur_State;

	switch(Cur_Command)
	{
		case CMD_NOP:		// None Action
			break;
		
		case CMD_TASK_WRITE:	// Saving Protocol Task
			Task_Write();
			break;

		case CMD_TASK_END:		// End Write Task
			Calc_Time();
			break;
		
		case CMD_GO:		// None Action
			IsRunning = TRUE;
			break;
		
		case CMD_STOP:		// Stop PWM
			Stop_PWM_MODE();
			break;
	}
}

/***************************************************
 * Function:        void Task_Write(void)
 *
 * OverView:		This function is Saving PCR Task Protocol
 *
 * Note:			None
 ***************************************************/
void Task_Write(void)
{
	if( IsRunning )
		return;

	Stop_PWM_MODE();

	// Save Rx_Buffer Data
	Task_Label = Rx_Buffer[RX_LABEL];
	Task_Temp = Rx_Buffer[RX_TEMP];
	Task_Time = Rx_Buffer[RX_TIMEH]*256 + Rx_Buffer[RX_TIMEL];
	Het_Target_Temp = Rx_Buffer[RX_LIDTEMP];
	Write_Count = Rx_Buffer[RX_INDEX];
/* MJH
	if( Task_Label == 1 && Task_Temp == 9 && Task_Time == 6 ){
		mLED_T_Toggle();
	}
*/
	if( Write_Count == -1 ){
		return;
	}

	// structure clear
	if( Write_Count == 0 ){
		memset(PCR_Task_Line, 0, sizeof(PCR_Task_Line[0]));
	}
	
	// Save Task per Line
	PCR_Task_Line[Write_Count].Label = Task_Label;
	PCR_Task_Line[Write_Count].Temp = Task_Temp;
	PCR_Task_Line[Write_Count].Time = Task_Time;
}

/***************************************************
 * Function:        void Task_Read(void)
 *
 * OverView:		if Host is request the line task, read task
 *
 * Note:			None
 ***************************************************/
void Task_Read(void)
{
	if( Write_Count < 0 ) return;

	Task_Label = PCR_Task_Line[Rx_Buffer[RX_REQLINE]].Label;
	Task_Temp  = PCR_Task_Line[Rx_Buffer[RX_REQLINE]].Temp;
	Task_Time  = PCR_Task_Line[Rx_Buffer[RX_REQLINE]].Time;
}

/***************************************************
 * Function:        void Calc_Time(void)
 *
 * OverView:		Calculation Total left time, line left time.
 *
 * Note:			None
 ***************************************************/
void Calc_Time(void)
{
	int i, j, k;
	unsigned char line = 0;

	if( IsRunning )
		return;
	else if( Total_ActLine != 0 )
		return;

	Total_ActLine = Write_Count + 1;

	for(i=0; i<Total_ActLine; i++)
	{
		// Total time calculation
		if( PCR_Task_Line[i].Label != AF_GOTO )		// Add Time
			Left_Time += (int)PCR_Task_Line[i].Time;
		if( PCR_Task_Line[i].Label == AF_GOTO )	// Goto Line Processing
		{		
			// YJ 121104
			//Cur_Loop_Count = PCR_Task_Line[i].Temp;
			while(TRUE)
			{
				UINT temp = (UINT)PCR_Task_Line[i].Temp;
				if( (UINT)PCR_Task_Line[line++].Label == temp )
					break;
			}

			// Number of GOTO
			for(j=0; j<PCR_Task_Line[i].Time; j++)
			{
				for(k=line-1; k<i; k++)
					Left_Time += (UINT)PCR_Task_Line[k].Time;		// ? (UINT) -> (int) ?
			}
		}
	}
}

/***************************************************
 * Function:        void Task_Control(void)
 *
 * OverView:		Read the stored task and execute task.
 *					Called at every 1sec
 *
 * Note:			None
 ***************************************************/
void Task_Control(void)
{
	// Count until LID is heated fully
	if( IsCoverHeated == FALSE )
	{
		if( Time_Out > 0 )
		{
			Time_Out--;
			return;
		}
		else	// LID is not heated fully within time
			Cur_State = STATE_STOP;
	}

	// Left time per line
	Left_Time_H = (BYTE)(Left_SecTime>>8);
	Left_Time_L = (BYTE)Left_SecTime;

	// Total lef time
	Left_TotalTime_H = (BYTE)(Left_Time>>8);
	Left_TotalTime_L = (BYTE)(Left_Time);

	// Current loop state
	Cur_Loop_Count = (BYTE)Loop_Count;

	// One action time is out, goto the next action
	if( Left_SecTime == 0 && !IsTimeInfinite )	// if SecLeft = 0, Load the next action
	{
		Cur_Action_Number++;
		if( Cur_Action_Number >= Total_ActLine )	// if there left no action, end the PCR
		{
			IsCompletePCR = TRUE;
			PCR_End();
			return;
		}

		if( PCR_Task_Line[Cur_Action_Number].Label != AF_GOTO ) // if the action is not GOTO
		{
			// Current task state
			Cur_Task_Line = PCR_Task_Line[Cur_Action_Number].Label;
			
			Pre_Chamber_Target = Chamber_Target;

			Chamber_Target = (float)PCR_Task_Line[Cur_Action_Number].Temp;

			//	Cooling state
			if( Chamber_Target < 10. ) IsCoolingState = TRUE;
			else IsCoolingState = FALSE;

			// Set "not yet arrive the target temp"
			IsTargetArrived = FALSE;
			
			// Left time (sec)
			Left_SecTime = (int)PCR_Task_Line[Cur_Action_Number].Time;

			// Infinite mode
			if( Left_SecTime == 0 ) IsTimeInfinite = TRUE;

			Time_Out = ARRIVAL_TIMEOUT;
//KJD171107
			Find_PID_Params();	// Find the Kp, Ki, Kd with the start temp and target temp
		}
		else	// if the action is GOTO
		{
			// Not visit the closest GOTO
			if( Loop_Count < 0 )
				Loop_Count = (int)(PCR_Task_Line[Cur_Action_Number].Time);

			// The closest GOTO is terminated
			if( Loop_Count == 0 ) Loop_Count = -1;
			else	// The closest GOTO is not terminated
			{
				Loop_Count--;
				Cur_Action_Number = Get_Action_Number(Cur_Action_Number);
				Cur_Action_Number--;
			}
		}
	}
	else	// While taking action
	{
		// if SecLeft is not 0, wait the transition complete
		// Check timeout err after LID preheating
		if( IsTargetArrived == FALSE )
		{
			if( IsCoverHeated ) Time_Out--;		// Counted after preheating
			if( Time_Out == 0 ) PCR_End();
		}
		else
		{
			if( IsTimeInfinite == FALSE )
			{
				Left_SecTime--;
				Left_Time--;
			}
		}
	}
}

/***************************************************
 * Function:        void PCR_End(void)
 *
 * OverView:		if PCR is finished, all value is clear & LED Control
 *
 * Note:			None
 ***************************************************/
void PCR_End(void)
{
	// if PCR is complete, send data to host
	if( IsCompletePCR )
	{
		Cur_State = STATE_PCREND;
		TempCtrl_State = READY_COMPLETE;
		mLED_B_On();		// Alarm emergency stop
	}
	else	// PCR incomplete
	{
		// if PCR is complete abnormally, send data to host
		if( TempCtrl_State == RUN_REFRIGERATOR )
		{
			Cur_State = STATE_PCREND;
			TempCtrl_State = READY_COMPLETE;
			mLED_B_On();
			mLED_R_Off();
		}
		else
		{
			Cur_State = STATE_STOP;
			TempCtrl_State = READY_INCOMPLETE;
			mLED_R_On();
		}
	}

	Reset_All_Values();
}

/***************************************************
 * Function:        void Reset_All_Values(void)
 *
 * OverView:		All value initialize.
 *
 * Note:			None
 ***************************************************/
void Reset_All_Values(void)
{
	// Stop PWM
	Stop_PWM_MODE();

	// Reset all variables
	IsCoverCtrl = FALSE;
	IsCoverHeated = FALSE;
	IsChamberCtrl = FALSE;
	IsCompletePCR = FALSE;
	IsTargetArrived = FALSE;
	IsTimeInfinite = FALSE;
	IsRunning = FALSE;

	Left_SecTime = 0;
	Cur_Action_Number = -1;
	Loop_Count = -1;
	Total_ActLine = 0;
	Left_Time = 0;
	Cur_Loop_Count = -1;
	Cur_Task_Line = 0;

	Kp = 0;
	Ki = 0;
	Kd = 0;

	Left_Time_H = 0;
	Left_Time_L = 0;
	Left_TotalTime_H = 0;
	Left_TotalTime_L = 0;

	Task_Label = 0;
	Task_Temp = 0;
	Task_Time = 0;
	
	Write_Count = -1;

	Heater = 0x00;

	Chamber_Target = TARGET_TEMP_INIT;
}

/***************************************************
 * Function:        void Find_PID_Params(void)
 *
 * OverView:		Get PID Params
 *
 * Note:			None
 ***************************************************/
void Find_PID_Params(void)
{
	float dist = 10000;
	int param_idx = 0;
	int i = 0;
	float target_temp = (float)Chamber_Target;
	float d_t = target_temp - Pre_Chamber_Target;

	if( (d_t > -1.0) && (d_t < 1.0) ) return;
	Chamber_LastIntegral = 0.0;

	if( d_t < 0 )
	{
		Kp = 80; Ki = 0; Kd = 0;
	}
	else
	{
		if( Chamber_Target > 93 )
		{
			Kp = 60; Ki = 0.6; Kd = 50;
		}
		else if( Chamber_Target > 71 )
		{
			Kp = 60; Kd = 50;
			Ki = (Chamber_Target - 72) / (94-72) * 0.2 + 0.42;
		}
		else if( Chamber_Target > 49 )
		{
			Kp = 60; Kd = 50;
			Ki = (Chamber_Target - 50) / (72-50) * 0.1 + 0.3;
		}
		else
		{
			Kp = 60; Kd = 50; Ki = 0.3;
		}
	}
}

/***************************************************
 * Function:        int Get_Action_Number(int number)
 *
 * OverView:		Get Current Action Number.
 *
 * Note:			None
 ***************************************************/
int Get_Action_Number(int number)
{
	int i = 0;
	for(i=0; i<number; i++)
	{
		if( PCR_Task_Line[number].Temp == PCR_Task_Line[i].Label )
			break;
	}
	return i;
}

/***************************************************
 * Function:        void Check_State(void)
 *
 * OverView:		Check State & Called Related Functions.
 *
 * Note:			Functions defined in State.h
 ***************************************************/
void Check_State(void)
{
	switch(Cur_State)
	{
		case STATE_READY:
			Process_Ready();
			break;
		case STATE_RUN:
			Process_Run();
			break;
		case STATE_PCREND:
			Process_PCREnd();	
			break;
		case STATE_STOP:
			Process_Stop();
			PCR_End();
			Cur_State = STATE_READY;
			break;
		case STATE_TASK_WRITE:
			Process_TaskWrite();
			break;
	}
}

// 160101 YJ for checksum
// only using on tx buffer
int calc_checksum(BYTE *buffer){
	int i, sum = 0;
	for(i=0; i<20; ++i){	// not include last packet(20 index)
		sum += buffer[i];
	}

	return (~sum&0xff);
}

/***************************************************
 * Function:        void Post_Process(void)
 *
 * OverView:		Set Tx Buffers.
 *
 * Note:			None
 ***************************************************/
void Post_Process(void)
{
	char mypcr_serial[7];
	int uint_serial;

	// Get Current Device Status
	ToSendDataBuffer[TX_STATE] = Cur_State;
	ToSendDataBuffer[TX_CURRENT_LABEL] = Cur_Task_Line;
	ToSendDataBuffer[TX_GOTO_COUNT] = Cur_Loop_Count;
	ToSendDataBuffer[TX_LABEL_COUNT] = Total_ActLine;
	ToSendDataBuffer[TX_LINE_TIME_H] = Left_Time_H;
	ToSendDataBuffer[TX_LINE_TIME_L] = Left_Time_L;
	ToSendDataBuffer[TX_TOTAL_TIME_H] = Left_TotalTime_H;
	ToSendDataBuffer[TX_TOTAL_TIME_L] = Left_TotalTime_L;

	ToSendDataBuffer[TX_LIDTEMP_H] = LID_Temp_H;
	ToSendDataBuffer[TX_LIDTEMP_L] = LID_Temp_L;

	ToSendDataBuffer[TX_CHAMBER_TEMP_H] = Chamber_Temp_H;
	ToSendDataBuffer[TX_CHAMBER_TEMP_L] = Chamber_Temp_L;

	// Heatsink Temperature
	ToSendDataBuffer[TX_HEAT_TEMP] = 0;

	// Current Operation
	ToSendDataBuffer[TX_CURRENT_OPER] = TempCtrl_State;

	// first 4bit is error, last 4bit is reqline
	ToSendDataBuffer[TX_ERROR_REQLINE] = Fatal_Error&0xf0|Rx_Buffer[RX_REQLINE]&0x0f;

	// Set Tx Action Line
	Task_Read();
	ToSendDataBuffer[TX_REQ_LABEL] = Task_Label;
	ToSendDataBuffer[TX_REQ_TEMP] = Task_Temp;
	ToSendDataBuffer[TX_REQ_TIME_H] = (BYTE)(Task_Time/256);
	ToSendDataBuffer[TX_REQ_TIME_L] = (BYTE)(Task_Time);

	// Checksum 
	ToSendDataBuffer[TX_CHECKSUM] = 0;
	ToSendDataBuffer[TX_CHECKSUM] = calc_checksum(ToSendDataBuffer);

	// End Point
	// ToSendDataBuffer[TX_END_OF_DATA] = 0x0d;

	// Serial Number YJ 120825
	/*	Removed 160101 YJ 
	strncpy(mypcr_serial, Serial+5, 6);
	mypcr_serial[6] = 0;
	uint_serial = atoi(mypcr_serial);
	ToSendDataBuffer[TX_SERIAL_H1] = (BYTE)(uint_serial>>16);
	ToSendDataBuffer[TX_SERIAL_H2] = (BYTE)(uint_serial>>8);
	ToSendDataBuffer[TX_SERIAL_L] = (BYTE)uint_serial;
	ToSendDataBuffer[TX_VERSION] = FIRMWARE_VERSION;
	*/
}
