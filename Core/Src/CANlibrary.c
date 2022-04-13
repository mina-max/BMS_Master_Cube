/*
 * CANlibrary.c
 *
 *  Created on: Feb 24, 2022
 *      Author: mihailozar
 */

#include "CANlibrary.h"
#include "master.h"

uint32_t TxMailbox;
CANMsg *msg;

extern int prechargeFlag;
extern int ecuSHDReqFlag;
extern uint8_t tempBuff[10][14];

static const double A = 0.003354016;
static const double B = 0.000256524;
static const double C = 0.00000260597;
static const double D = 0.0000000632926;
static const double R25 = 10000.0;             //ohm
static const double Rref = 1000.0;               //ohm
static const double VADC = 5.2;                //V
static const double KelvinToCelzius = -272.15;

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
	CAN_RxHeaderTypeDef pHeader;
	uint8_t rxData[8];
	HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &pHeader, rxData);
//	canSend(pHeader.StdId, rxData);
	CANMsg msg = { &pHeader, rxData };
//	xQueueSendToBackFromISR(CAN_Rx_Queue,&(msg), portMAX_DELAY );
	//From ECATU
	if (pHeader.StdId == 0x097) {
		if (rxData[0] == 1) {
			//start PrechargeProcess
			if (prechargeFlag != 1)
				prechargeFlag = 1;

		}
		UART_AsyncTransmitString(5, "Start precharge!\n",
				strlen("Start precharge!\n"));
	}
	//From ECATU
	if (pHeader.StdId == 0x300) {
		if (rxData[2] & 0x40) {
			//open SHUT DOWN Circuit
			if (ecuSHDReqFlag != 1)
				ecuSHDReqFlag = 1;

		}

		UART_AsyncTransmitString(5, "Open SHUTDOWN!\n",
				strlen("Open SHUTDOWN!\n"));
	}


}

void Can_Init() {

	CAN_FilterTypeDef CanFilter;
	CanFilter.FilterIdHigh = 0x0000;
	CanFilter.FilterIdLow = 0;
	CanFilter.FilterMaskIdHigh = 0x0000;
	CanFilter.FilterMaskIdLow = 0;
	CanFilter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	CanFilter.FilterBank = 0;
	CanFilter.FilterMode = CAN_FILTERMODE_IDMASK;
	CanFilter.FilterScale = CAN_FILTERSCALE_32BIT;
	CanFilter.FilterActivation = CAN_FILTER_ENABLE;

	HAL_CAN_ConfigFilter(&hcan1, &CanFilter);
	HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
	HAL_CAN_Start(&hcan1);

}

void canSend(uint16_t id, uint8_t *canMsg) {

	CAN_TxHeaderTypeDef pHeader;
	pHeader.DLC = 8;
	pHeader.RTR = CAN_RTR_DATA;
	pHeader.IDE = CAN_ID_STD;
	pHeader.StdId = id;
	HAL_CAN_AddTxMessage(&hcan1, &pHeader, canMsg, &TxMailbox);
	while (HAL_CAN_IsTxMessagePending(&hcan1, TxMailbox))
		;
}

//*** Pack 2B data into CAN messages
//Pakuje unit16_t u dva podatka od po 1B, na poziciji pos u CAN poruci
void pack_data_2B(CANMsg *canMsg, uint16_t data, uint8_t position) {
	canMsg->data[position << 1] = data & 0xFF;
	canMsg->data[(position << 1) + 1] = data >> 8;
}
//*** Pack 1B data into CAN message
//Pakuje unit16_t na poziciji pos u CAN poruci
void pack_data_1B(CANMsg *canMsg, uint16_t data, uint8_t position) {
	canMsg->data[position] = data & 0xFF;
}

extern volatile uint8_t voltageBuff[10][28];
//*** CAN send Voltages
void canSendVoltagesAndTemps(int nDev_ID) {
	//We pack 14 voltages into 2 messages on CAN nework, more info-> CAN PROJEKTOVANJE on Google Drive
	//First Message, lowest 8 voltages is packed in msgL, ID: LOW
	//Second Message, higest 6 voltages is packed in msgH, ID: HIGH

	//*** DATA:
	//-------------------------------------------------------------------------------------------------
	// Voltages
	CANMsg msgLVol;        //Lowset message
	CANMsg msgHVol;        //Highest message
	//Temperatures
	CANMsg msgTemp;        //Lowset message

	//*** Voltage CAN Send
	//-------------------------------------------------------------------------------------------------
	//
	int j = 0;
	for (int i = 0; i < 14; i++) {
		if (i < 8) {
			msgLVol.data[i] = (uint8_t) voltageBuff[nDev_ID][j]; //Value in Volts
		} else {
			msgHVol.data[i - 8] = (uint8_t) voltageBuff[nDev_ID][j]; //Value in Volts
		}
		j += 2;
	}
	canSend(Id_Can_VOLTAGES[nDev_ID][LOW], msgLVol.data); //Low part of messages
	canSend(Id_Can_VOLTAGES[nDev_ID][HIGH], msgHVol.data); //High part of messages

	for (int i = 0; i < 7; i++) {
		uint16_t voltage = tempBuff[i][j + 1];
		voltage |= (tempBuff[i][j]) << 8;
		float vol = ((float) voltage) / 65536.0 * 5.0;
		float Rntc = vol * Rref / (VADC - vol);
		float temp = ntcFunction(Rntc) + KelvinToCelzius;
		int tempertaure = (int) temp;
#ifdef CAN_TEMEPERATURE_SERIAL_PRINT
            pc1.printf("Temp: %.6f, CAN temp[%d]: %d\n", temp, i, tempertaure);
            #endif
		msgTemp.data[i] = (uint8_t) (tempertaure);           //value in Celzijus
	}
	canSend(Id_Can_Temp[nDev_ID], msgTemp.data);
}
