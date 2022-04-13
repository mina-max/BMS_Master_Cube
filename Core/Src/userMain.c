/*
 * userMain.c
 *
 *  Created on: Feb 23, 2022
 *      Author: mihailozar
 */

#include "userMian.h"
#include "PwmIn.h"
#include "uart_driver.h"
#include "can.h"
#include "slaveConfig.h"
#include "stdlib.h"


typedef struct CANMessage {
	CAN_RxHeaderTypeDef pHeader;
	uint8_t data[8];
} CANMsg;

extern void InitPL455();

volatile int numOfMsg = 0;

extern void PwmInInit();
extern void Can_Init();
extern void canSend(uint16_t id, uint8_t *canMsg);
extern void shutDownSlavesCommand();
extern void clearUartBuffer51();
int flagovi = 0;
extern volatile uint8_t recBuf1[64];
extern uint8_t recBuf5[32];
extern int prechargeFlag = 0;
int ecuSHDReqFlag = 0;
extern TIM_HandleTypeDef htim5;
extern int WriteReg(char bID, uint16_t wAddr, uint64_t dwData, char bLen,
		char bWriteType);
#define DEVICE_COMMAND          2       //Command Register
#define FRMWRT_SGL_R            0x00    // single device write with response
volatile uint8_t voltageBuff[10][28];
extern int receiveCnt;
extern void masterInit();
extern UART_HandleTypeDef huart1;
volatile int voltage = 0;
extern volatile int rxComplete;
volatile int vol = 0;
extern volatile int firstEntry;
extern void canSendVoltagesAndTemps();

void userMainInit() {
	Can_Init();
}

volatile int counter = 0;
extern volatile int done;
extern volatile int shutDownSlaves;
extern void shutDownSlavesCommand();

extern uint8_t tempBuff[10][14];

static const double A = 0.003354016;
static const double B = 0.000256524;
static const double C = 0.00000260597;
static const double D = 0.0000000632926;
static const double R25 = 10000.0;             //ohm
static const double Rref = 1000.0;               //ohm
static const double VADC = 5.2;                //V
static const double KelvinToCelzius = -272.15;

double ntcFunction(double R)
{
    double res = A + B * log(R/R25) + C *  pow(log(R/R25), 2) + D * pow(log(R/R25), 3);
    return 1.0/res;
}



void uartSendVoltages() {
	while(!done) {
		if (firstEntry) {
			UART_Receive(1, 1);
			//HAL_Delay(5);
			//WriteReg(0, DEVICE_COMMAND, 0x01, 1, FRMWRT_SGL_R);
			WriteReg(receiveCnt, DEVICE_COMMAND, 0x00, 1, 0x10); //Current Value
			WriteReg(receiveCnt, DEVICE_COMMAND, 0x20, 1, FRMWRT_SGL_R); //Volatge request
		}
		HAL_Delay(50);
	}

	done = 0;
	char volt[25];
	for (int i = 0; i < 10; i++) {
		snprintf(volt, 18, "BMS SLAVE %d\n", i + 1);
		UART_AsyncTransmitString(5, volt, 18);
		for (int j = 0; j < 28; j += 2) {
			voltage = voltageBuff[i][j + 1];
			voltage |= (voltageBuff[i][j]) << 8;
			float vol = ((double) voltage) / 65536.0 * 5.0;
			snprintf(volt, 19, "CELL[%d] = %4f V	", j / 2, vol);
			UART_AsyncTransmitString(5, volt, 19);
			UART_AsyncTransmitString(5, "\n", 1);
			HAL_Delay(30);
		}
		UART_AsyncTransmitString(5, "\n", 1);

		for(int j = 0; j < 14; j += 2){
			voltage = tempBuff[i][j + 1];
			voltage |= (tempBuff[i][j]) << 8;
			float vol = ((float) voltage) / 65536.0 * 5.0;
			float Rntc = vol * Rref/(VADC - vol);
			float temp = ntcFunction(Rntc) + KelvinToCelzius;
			snprintf(volt, 19, "TEMP[%d] = %4f V	", j / 2, temp);
			UART_AsyncTransmitString(5, volt, 19);
			UART_AsyncTransmitString(5, "\n", 1);
		}

		UART_AsyncTransmitString(5, "\n", 1);

	}
}

void canSendVoltages() {
	for(int i = 0; i < 10; i++) {
		canSendVoltagesAndTemps(i);
		HAL_Delay(20);
	}
}


int userMain(void) {

//	PwmInInit();

	masterInit();
	InitPL455();
	//HAL_TIM_Base_Start_IT(&htim5);
//	vTaskDelay(pdMS_TO_TICKS(1000));
//	shutDownSlavesCommand();
	int firstEntry = 1;
	rxComplete = 0;
	HAL_Delay(500);
	UART_Receive(5, 1);
	while (1) {

		HAL_Delay(500);

		uartSendVoltages();
		uint8_t data[8] = {0};
		canSendVoltages();

		if(shutDownSlaves) {
			shutDownSlavesCommand();
		}


	}

	return 0;
}
