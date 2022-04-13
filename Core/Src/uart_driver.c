/*
 * uart_driver.c
 *
 *  Created on: Feb 23, 2022
 *      Author: mihailozar
 */

#include "uart_driver.h"

#include <string.h>
#include "usart.h"
#include "slaveConfig.h"
#include "stdlib.h"


extern TIM_HandleTypeDef htim4;
static volatile uint8_t flag = 0;
extern volatile uint8_t voltageBuff[10][28];
uint8_t uartData5;
uint8_t uartData1;
volatile uint8_t procitano=0;
extern int counter;
extern TIM_HandleTypeDef htim5;
extern volatile int numOfMsg;

volatile uint8_t recBuf1[64];
uint8_t recBuf5[32];
uint8_t tempBuff[10][14];

volatile int receiveCnt=0;
int cntRec=1;
int numOfBytes = 0;
int flag1 = 0;
volatile int rxComplete = 0;
int firstEntry = 1;
volatile int done = 0;
volatile int shutDownSlaves = 0;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart == &huart5) {
		if (recBuf5[0] == '1')
		{
			shutDownSlaves = 1;
		}
		else
		{
			shutDownSlaves = 0;
		}
		UART_Receive(5, 1);
	}
	else
	{
		if(firstEntry) {
			numOfBytes = recBuf1[0] + 1;
			if(numOfBytes != 0) {
				HAL_UART_Receive_IT(huart, recBuf1+1, numOfBytes);
				firstEntry = 0;
			}
		} else {
			numOfBytes = 0;
			firstEntry = 1;
			for(int i = 1; i <= 28; i++) {
					voltageBuff[receiveCnt][i-1] = recBuf1[i];
				}
			for(int i = 1; i <= 14; i++){
				tempBuff[receiveCnt][i-1] = recBuf1[i + 28];
			}
			if(receiveCnt == 9)
						done = 1;
			receiveCnt=(receiveCnt+1)%10;
		}
	}

}

uartMsg tmp ;
void UART_AsyncTransmitString(int id, char* pFrame, int bPktLen) {
	if (pFrame != NULL) {
		if(id==1){
			HAL_UART_Transmit(&huart1, pFrame, bPktLen, 100);
		}else{
			HAL_UART_Transmit(&huart5, pFrame, bPktLen, 100);
		}
	}
}

// RECEIVE UTIL
// -----------------------------------------------------------------------------


int UART_Receive(int id, int len){
	if(id==1){
		HAL_UART_Receive_IT(&huart1, recBuf1, len);
	}else{
		HAL_UART_Receive_IT(&huart5, recBuf5, len);
	}
	return 1;
}

void clearUartBuffer5(){
	for(int i=0; i<32;i++){
			recBuf5[i]='\0';
		}
}
void clearUartBuffer51(){
	for(int i=0; i<32;i++){
			recBuf1[i]='\0';
		}
}
