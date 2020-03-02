/*
 * command_server.c
 *
 *  Created on: 07.12.2016.
 *      Author: milan
 */

#include <load_current_sense.h>
#include "command_server.h"
#include "stddef.h"
#include "nv.h"
#include "fuel_gauge_lc709203f.h"
#include "charger_bq2416x.h"
#include "analog.h"
#include "battery.h"
#include "power_source.h"
#include "button.h"
#include "led.h"
#include "battery.h"
#include "power_management.h"
#include "rtc_ds1339_emu.h"
#include "io_control.h"
#include "execution.h"

#define REGISTERS_NUM	((uint16_t)256)

#define SYS_MEM_ADDRESS		0x1FFFD800 // for STM32F030x8 0x1FFFEC00

static int8_t reg[REGISTERS_NUM]; // registers used for i2c master access
//static int8_t regWriteFn[REGISTERS_NUM];
extern RTC_HandleTypeDef hrtc;
extern I2C_HandleTypeDef hi2c2;
extern I2C_HandleTypeDef hi2c1;//extern SMBUS_HandleTypeDef hsmbus;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim15;
extern TIM_HandleTypeDef htim17;

extern uint16_t wakeupOnCharge;
extern uint8_t powerOffBtnEventFlag;

extern void Error_Handler(void);

const uint8_t firmwareVer = 0x14;
const uint8_t firmwareVariant = 0x00;

typedef  void (*pFunction)(void);
pFunction Jump_To_Bootloader;

void CmdServerDefaultReadWrite(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadStatus(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteEventFaultStatus(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadRsoc(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadRsocHigherResolution(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadButtonStatus(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadBatTemp(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadBatVoltage(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadBatCurrent(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadMainVoltage(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadMainCurrent(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteButtonConfigurationSw1(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteButtonConfigurationSw2(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteButtonConfigurationSw3(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteLedState1(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteLedState2(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteLedBlink1(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteLedBlink2(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteLedConfigurationLED1(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteLedConfigurationLED2(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteRunPinConfiguration(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteWDGConfiguration(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWritePowerRegulatorConfiguration(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
//void CmdServerReadWriteChargeCurrent(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
//void CmdServerReadWriteTerminationCurrent(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
//void CmdServerReadWriteBatRegVoltage(uint8_t dir, uint8_t *pData, uint16_t *dataLen) ;
void CmdServerReadWriteChargingConfig(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteBatProfile(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteBatExtendedProfile(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteBatteryProfileId(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteFuelGaugeConfig(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteDateTime(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteTime(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteDayLightSavingConfig(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteAlarm(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteRtcAlarmCtrlStatus(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteInputsConfig(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteScheduledPowerOff(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteVSysSwitchState(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteWakeupOnCharge(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteOwnAddress1(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteOwnAddress2(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteEEPROM_WriteProtect(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteEEPROM_WriteAddress(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteTestAndCalibration(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerRunBootloader(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteDefaultConfiguration(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadFirmwareVersion(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadBoardFaultStatus(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteIoConfig1(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteIoConfig2(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteIoValue1(uint8_t dir, uint8_t *pData, uint16_t *dataLen);
void CmdServerReadWriteIoValue2(uint8_t dir, uint8_t *pData, uint16_t *dataLen);

MasterCommand_T masterCommands[REGISTERS_NUM] =
{
/*0*/	NULL,
/*1*/	NULL,
/*2*/	NULL,
/*3*/	NULL,
/*4*/	NULL,
/*5*/	NULL,
/*6*/	NULL,
/*7*/	NULL,
/*8*/	NULL,
/*9*/	NULL,
/*10*/	NULL,
/*11*/	NULL,
/*12*/	NULL,
/*13*/	NULL,
/*14*/	NULL,
/*15*/	NULL,
/*16*/	NULL,
/*17*/	NULL,
/*18*/	NULL,
/*19*/	NULL,
/*20*/	NULL,
/*21*/	NULL,
/*22*/	NULL,
/*23*/	NULL,
/*24*/	NULL,
/*25*/	NULL,
/*26*/	NULL,
/*27*/	NULL,
/*28*/	NULL,
/*29*/	NULL,
/*30*/	NULL,
/*31*/	NULL,
/*32*/	NULL,
/*33*/	NULL,
/*34*/	NULL,
/*35*/	NULL,
/*36*/	NULL,
/*37*/	NULL,
/*38*/	NULL,
/*39*/	NULL,
/*40*/	NULL,
/*41*/	NULL,
/*42*/	NULL,
/*43*/	NULL,
/*44*/	NULL,
/*45*/	NULL,
/*46*/	NULL,
/*47*/	NULL,
/*48*/	NULL,
/*49*/	NULL,
/*50*/	NULL,
/*51*/	NULL,
/*52*/	NULL,
/*53*/	NULL,
/*54*/	NULL,
/*55*/	NULL,
/*56*/	NULL,
/*57*/	NULL,
/*58*/	NULL,
/*59*/	NULL,
/*60*/	NULL,
/*61*/	NULL,
/*62*/	NULL,
/*63*/	NULL,
/*64*/	CmdServerReadStatus, // status, bit0-fault, bit1-button event,bit2&3-bat status,bit4&5-IN stat,bit6&7-Pi 5V pow stat
/*65*/	CmdServerReadRsoc, // state of charge %
/*66*/	CmdServerReadRsocHigherResolution, // state of charge % 0.1 resolution, two bytes
/*67*/	NULL, // reserved for high byte of state of charge
/*68*/	CmdServerReadWriteEventFaultStatus, // fault/event codes, cleared after read, bit0-reserved, bit1-sys undervoltage event,bit2-5V shutdown event,bit3-wdg reset,bit4-reserved,bit5 invalid bat profile,bit6-7 bat temp Fault
/*69*/  CmdServerReadButtonStatus,// sw1 0-3, sw2 4-7
/*70*/  NULL,// reserved for sw3 0-3
/*71*/	CmdServerReadBatTemp,// battery temperature celsius
/*72*/	NULL,// reserved
/*73*/	CmdServerReadBatVoltage,// battery voltage low byte
/*74*/	NULL,// battery voltage high byte
/*75*/	CmdServerReadBatCurrent,// battery current low byte
/*76*/	NULL,// battery current high byte
/*77*/	CmdServerReadMainVoltage,// RPI voltage low byte
/*78*/	NULL,// RPI voltage high byte
/*79*/	CmdServerReadMainCurrent,// RPI current low byte
/*80*/	NULL,// RPI current high byte

		// --charger parameters-- //
/*81*/	CmdServerReadWriteChargingConfig,// charging enable config
/*82*/	CmdServerReadWriteBatteryProfileId, // charge profile
/*83*/	CmdServerReadWriteBatProfile,//CmdServerReadWriteChargeCurrent, // charge current
/*84*/	CmdServerReadWriteBatExtendedProfile,
		// Termination current, unit 10mA
/*85*/	NULL,//CmdServerReadWriteBatRegVoltage,// Battery Regulation Voltage
/*86*/	NULL,// TS function enable/disable
/*87*/	NULL,// tCold
/*88*/	NULL,// tCool
/*89*/	NULL,// tWarm
/*90*/	NULL,// tHot
/*91*/	NULL,// NTC B low byte
/*92*/	NULL,// NTC B high byte
/*93*/	CmdServerReadWriteFuelGaugeConfig,

	// -- power management
/*94*/	CmdServerReadWriteInputsConfig,
/*95*/	CmdServerReadWriteRunPinConfiguration,
/*96*/  CmdServerReadWritePowerRegulatorConfiguration,
/*97*/  CmdServerReadWriteWDGConfiguration,
/*98*/	CmdServerReadWriteScheduledPowerOff, // 0 - 250 seconds, 0xFF means no power off, 251 - 254 reserved
/*99*/  CmdServerReadWriteWakeupOnCharge,
/*100*/	CmdServerReadWriteVSysSwitchState, // --Vsys output switch control--
/*101*/	NULL,

	// --on board led--
/*102*/	CmdServerReadWriteLedState1,	//
/*103*/	CmdServerReadWriteLedState2,	//
/*104*/	CmdServerReadWriteLedBlink1,	//
/*105*/	CmdServerReadWriteLedBlink2,	//
/*106*/	CmdServerReadWriteLedConfigurationLED1,
/*107*/	CmdServerReadWriteLedConfigurationLED2,
/*108*/	NULL,
/*109*/	NULL,
/*110*/	CmdServerReadWriteButtonConfigurationSw1,
/*111*/	CmdServerReadWriteButtonConfigurationSw2, //
/*112*/	CmdServerReadWriteButtonConfigurationSw3,
/*113*/	NULL,

// --IO1 control--
/*114*/	CmdServerReadWriteIoConfig1, // IO1 type, DI_PD, DI_PU, OD_NOPULL, OD_PU, DO, AI, PWM
/*115*/	NULL, // IO1 param byte 1
/*116*/	NULL, // IO1 param byte 2
/*117*/	CmdServerReadWriteIoValue1, // IO1 param byte 3
/*118*/	NULL, // IO1 param byte 4

// --IO2 control--
/*119*/	CmdServerReadWriteIoConfig2,// IO2 type, DI_PD, DI_PU, OD_NOPULL, OD_PU, DO, PWM
/*120*/	NULL,// IO2 param byte 1
/*121*/	NULL,// IO2 param byte 2
/*122*/	CmdServerReadWriteIoValue2,// IO2 param byte 3
/*123*/	NULL,// IO2 param byte 4

// --I2C Address--
/*124*/	CmdServerReadWriteOwnAddress1,	// OwnAddress1
/*125*/	CmdServerReadWriteOwnAddress2,	// OwnAddress2

// --ID EEPROM--
/*126*/	CmdServerReadWriteEEPROM_WriteProtect,	// write protect
/*127*/	CmdServerReadWriteEEPROM_WriteAddress,	// address

// reserved for rtc ds1337 registers
/*128*/	NULL,
/*129*/	NULL,
/*130*/	NULL,
/*131*/	NULL,
/*132*/	NULL,
/*133*/	NULL,
/*134*/	NULL,
/*135*/	NULL,
/*136*/	NULL,
/*137*/	NULL,
/*138*/	NULL,
/*139*/	NULL,
/*140*/	NULL,
/*141*/	NULL,
/*142*/	NULL,
/*143*/	NULL,

// reserved
/*144*/	NULL,
/*145*/	NULL,
/*146*/	NULL,
/*147*/	NULL,
/*148*/	NULL,
/*149*/	NULL,
/*150*/	NULL,
/*151*/	NULL,
/*152*/	NULL,
/*153*/	NULL,
/*154*/	NULL,
/*155*/	NULL,
/*156*/	NULL,
/*157*/	NULL,
/*158*/	NULL,
/*159*/	NULL,

// reserved
/*160*/	NULL,
/*161*/	NULL,
/*162*/	NULL,
/*163*/	NULL,
/*164*/	NULL,
/*165*/	NULL,
/*166*/	NULL,
/*167*/	NULL,
/*168*/	NULL,
/*169*/	NULL,
/*170*/	NULL,
/*171*/	NULL,
/*172*/	NULL,
/*173*/	NULL,
/*174*/	NULL,
/*175*/	NULL,

      	// --Date Settings//
/*176*/	CmdServerReadWriteDateTime,// Year
/*177*/	NULL,// Month
/*178*/	NULL,// Date
/*179*/	NULL,// WeekDay

		// --Time settings
/*180*/	NULL,//CmdServerReadWriteTime, // Hours
/*181*/	NULL,// Minutes
/*182*/	NULL,// Seconds
/*183*/	NULL,// SubSeconds
/*184*/	NULL, //CmdServerReadWriteDayLightSavingConfig,// DayLightSaving, 0x01 - ADD1H, 0x02 - SUB1H, bit2 store operation set/reset

		// --Alarm settings-- //
/*185*/	CmdServerReadWriteAlarm,// Hours
/*186*/	NULL,// Minutes
/*187*/	NULL,// Seconds
/*188*/	NULL,// SubSeconds
/*189*/	NULL,// AlarmDateWeekDaySel, first 3bits weekday, 7-3 bit month
/*190*/	NULL,// AlarmMask
/*191*/	NULL,// Alarm Repeat Selection 4 bytes
/*192*/	NULL,
/*193*/	NULL,
/*194*/	CmdServerReadWriteRtcAlarmCtrlStatus,

// not used
/*195*/	NULL,
/*196*/	NULL,
/*197*/	NULL,
/*198*/	NULL,
/*199*/	NULL,
/*200*/	NULL,
/*201*/	NULL,
/*202*/	NULL,
/*203*/	NULL,
/*204*/	NULL,
/*205*/	NULL,
/*206*/	NULL,
/*207*/	NULL,

// not used
/*208*/	NULL,
/*209*/	NULL,
/*210*/	NULL,
/*211*/	NULL,
/*212*/	NULL,
/*213*/	NULL,
/*214*/	NULL,
/*215*/	NULL,
/*216*/	NULL,
/*217*/	NULL,
/*218*/	NULL,
/*219*/	NULL,
/*220*/	NULL,
/*221*/	NULL,
/*222*/	NULL,
/*223*/	NULL,

// not used
/*224*/	NULL,
/*225*/	NULL,
/*226*/	NULL,
/*227*/	NULL,
/*228*/	NULL,
/*229*/	NULL,
/*230*/	NULL,
/*231*/	NULL,
/*232*/	NULL,
/*233*/	NULL,
/*234*/	NULL,
/*235*/	NULL,
/*236*/	NULL,
/*237*/	NULL,
/*238*/	NULL,
/*239*/	NULL,

// sys use
/*240*/	CmdServerReadWriteDefaultConfiguration,
/*241*/	NULL,
/*242*/	NULL,
/*243*/	NULL,
/*244*/	NULL,
/*245*/	NULL,
/*246*/	NULL,
/*247*/	NULL,
/*248*/	CmdServerReadWriteTestAndCalibration,
/*249*/	NULL,
/*250*/	CmdServerReadBoardFaultStatus,
/*251*/	NULL,
/*252*/	NULL,
/*253*/	CmdServerReadFirmwareVersion,
/*254*/	CmdServerRunBootloader,
/*255*/	NULL,

};

#define REGISTER_MAX		(sizeof(masterCommands) / sizeof (MasterCommand_T))

uint8_t CalcFcs(uint8_t *msg, int size)
{
	uint8_t result = 0xFF;

	while (size) result ^= msg[--size];

	return result;
}

void CommandServerInit(void) {

	// init memory map
	uint8_t size = REGISTERS_NUM;
	while((--size) > 0) reg[size] = 0;
}

int8_t CmdServerProcessRequest(uint8_t dir, uint8_t pData[], uint16_t *dataLen) {
	if (pData[0] <= REGISTER_MAX ) {
		if (masterCommands[pData[0]] != NULL)
			if (dir == MASTER_CMD_DIR_WRITE) {
				if (CalcFcs(pData+1, *dataLen-2) == pData[*dataLen-1]) {
					(masterCommands[pData[0]])(dir, pData, dataLen);
				} else {
					return 1;
				}
			} else {
				(masterCommands[pData[0]])(dir, pData, dataLen);
				pData[*dataLen] = CalcFcs(pData, *dataLen);
				(*dataLen) ++;
			}
		else
			CmdServerDefaultReadWrite(dir, pData, dataLen);
	}
	return 0;
}

void CmdServerDefaultReadWrite(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {

	if (dir == MASTER_CMD_DIR_WRITE) {
		if (pData[0] >= 186) {
			//reg[pData[0]] = pData[1];
			//int16_t size = *dataLen-2;
			//while((--size) >= 0) reg[size+pData[0]] = pData[size+1];
		}
	} else {
		//if (pData[0] < 186) {
			pData[0] = reg[pData[0]];
			*dataLen = 1;
		/*} else {
			int16_t i = pData[0];
			for(i=pData[0]; i < REGISTER_MAX; i++) pData[i-pData[0]] = reg[i];
			*dataLen = REGISTER_MAX - pData[0];
		}*/
	}
}

static uint8_t IsEventFault(void) {
	uint8_t ev = 0;
	ev = ev || powerOffBtnEventFlag;
	ev = ev || forcedPowerOffFlag;
	ev = ev || forcedVSysOutputOffFlag;
	ev = ev || watchdogExpiredFlag;
	ev = ev || ((currentBatProfile == NULL) ? 0x20 : 0);
	ev = ev || CHRGER_TS_FAULT_STATUS();
	return ev;
}

void CmdServerReadStatus(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_READ) {
		pData[0] = IsEventFault();
		pData[0] |= IsButtonEvent() << 1;
		pData[0] |= (batteryStatus << 2);
		pData[0] |= (powerInStatus << 4);
		pData[0] |= (power5vIoStatus << 6);
		*dataLen = 1;
	}
}

void CmdServerReadWriteEventFaultStatus(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_READ) {
		uint8_t ev = 0;
		ev |= powerOffBtnEventFlag;
		ev |= forcedPowerOffFlag << 1;
		//forcedPowerOffFlag = 0;
		ev |= forcedVSysOutputOffFlag << 2;
		//forcedVSysOutputOffFlag = 0;
		ev |= watchdogExpiredFlag << 3;
		//watchdogExpiredFlag = 0;
		ev |= (currentBatProfile == NULL) ? 0x20 : 0;
		ev |= CHRGER_TS_FAULT_STATUS() << 6;
		pData[0] = ev;
		*dataLen = 1;
	} else {
		powerOffBtnEventFlag = powerOffBtnEventFlag && (pData[1] & 0x01);
		forcedPowerOffFlag = forcedPowerOffFlag && (pData[1] & 0x02);
		forcedVSysOutputOffFlag = forcedVSysOutputOffFlag && (pData[1] & 0x04);
		watchdogExpiredFlag = watchdogExpiredFlag && (pData[1] & 0x08);
	}
}

void CmdServerReadRsoc(uint8_t dir, uint8_t *pData, uint16_t *dataLen){
	if (dir == MASTER_CMD_DIR_READ) {
		pData[0] = batteryRsoc<1000 ? ((uint32_t)batteryRsoc * 819) >> 13 : 100;
		*dataLen = 1;
	}
}

void CmdServerReadRsocHigherResolution(uint8_t dir, uint8_t *pData, uint16_t *dataLen){
	if (dir == MASTER_CMD_DIR_READ) {
		pData[0] = *((uint8_t*)&batteryRsoc);
		pData[1] = *((uint8_t*)(&batteryRsoc) + 1);
		*dataLen = 2;
	}
}

void CmdServerReadButtonStatus(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_READ) {
		ButtonEvent_T evSw = GetButtonEvent(0);
		pData[0] = evSw & 0x0f;
		evSw = GetButtonEvent(1);
		pData[0] |= evSw << 4;
		evSw = GetButtonEvent(2);
		pData[1] = evSw & 0x0f;
		*dataLen = 2;
	} else {
		if (!(pData[1] & 0x0F)) ButtonRemoveEvent(0);
		if (!(pData[1] & 0xF0)) ButtonRemoveEvent(1);
		if (!(pData[2] & 0x0F)) ButtonRemoveEvent(2);
	}
}

void CmdServerReadBatTemp(uint8_t dir, uint8_t *pData, uint16_t *dataLen){
	if (dir == MASTER_CMD_DIR_READ) {
		uint8_t adr = pData[0];
		reg[adr] = batteryTemp;
		//reg[adr+1] = batteryTemp >> 8;
		pData[0] = reg[adr];
		pData[1] = reg[adr+1];
		*dataLen = 2;
	}
}

void CmdServerReadBatVoltage(uint8_t dir, uint8_t *pData, uint16_t *dataLen){
	if (dir == MASTER_CMD_DIR_READ) {
		uint8_t adr = pData[0];
		reg[adr] = batteryVoltage;
		reg[adr+1] = batteryVoltage >> 8;
		pData[0] = reg[adr];
		pData[1] = reg[adr+1];
		*dataLen = 2;
	}
}

void CmdServerReadBatCurrent(uint8_t dir, uint8_t *pData, uint16_t *dataLen){
	if (dir == MASTER_CMD_DIR_READ) {
		volatile uint16_t cur = batteryCurrent;
		uint8_t adr = pData[0];
		reg[adr] = cur;
		reg[adr+1] = cur >> 8;
		pData[0] = reg[adr];
		pData[1] = reg[adr+1];
		*dataLen = 2;
	}
}

void CmdServerReadMainVoltage(uint8_t dir, uint8_t *pData, uint16_t *dataLen){
	if (dir == MASTER_CMD_DIR_READ) {
		uint8_t adr = pData[0];
		volatile uint16_t ioVolt = Get5vIoVoltage();
		reg[adr] = ioVolt;
		reg[adr+1] = ioVolt >> 8;
		pData[0] = reg[adr];
		pData[1] = reg[adr+1];
		*dataLen = 2;
	}
}

void CmdServerReadMainCurrent(uint8_t dir, uint8_t *pData, uint16_t *dataLen){
	if (dir == MASTER_CMD_DIR_READ) {
		uint16_t cur = GetLoadCurrent();
		uint8_t adr = pData[0];
		reg[adr] = cur;
		reg[adr+1] = cur >> 8;
		pData[0] = reg[adr];
		pData[1] = reg[adr+1];
		*dataLen = 2;
	}
}
/*
void CmdServerReadWriteChargeCurrent(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		if (batProfileId == BATTERY_CUSTOM_PROFILE_ID) {
			SetChargeCurrentReq(pData[1]);
			currentBatProfile.chargeCurrent = pData[1];
			NvSaveParameterReq(CHARGE_CURRENT_NV_ADDR, pData[1]);
		}
	} else {
		pData[0] = BatteryGetProfile()->chargeCurrent;//CHARGE_CURRENT;
		*dataLen = 1;
	}
}

void CmdServerReadWriteTerminationCurrent(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		if (batProfileId == BATTERY_CUSTOM_PROFILE_ID) {
			SetChargeTerminationCurrentReq(pData[1]);
			currentBatProfile.terminationCurr = pData[1];
			NvSaveParameterReq(CHARGE_TERM_CURRENT_NV_ADDR, pData[1]);
		}
	} else {
		pData[0] = BatteryGetProfile()->terminationCurr;//CHARGE_TERMINATION_CURRENT;
		*dataLen = 1;
	}
}

void CmdServerReadWriteBatRegVoltage(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		if (batProfileId == BATTERY_CUSTOM_PROFILE_ID) {
			SetBatRegulationVoltageReq(pData[1]);
			currentBatProfile.regulationVoltage = pData[1];
			NvSaveParameterReq(BAT_REG_VOLTAGE_NV_ADDR, pData[1]);
		}
	} else {
		pData[0] = BatteryGetProfile()->regulationVoltage;//BAT_REG_VOLTAGE;
		*dataLen = 1;
	}
}*/

void CmdServerReadWriteChargingConfig(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		ChargerWriteChargingConfig(pData[1]);
	} else {
		pData[0] = ChargerReadChargingConfig();
		*dataLen = 1;
	}
}

void CmdServerReadWriteBatProfile(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		BatteryWriteCustomProfileReq(&pData[1], *dataLen-1);
	} else {
		BatteryReadCurrentProfile(pData, dataLen);
	}
}

void CmdServerReadWriteBatExtendedProfile(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		BatteryWriteCustomExtendedProfileReq(&pData[1], *dataLen-1);
	} else {
		BatteryReadCurrentExtendedProfile(pData, dataLen);
	}
}

void CmdServerReadWriteBatteryProfileId(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		if (BatterySetProfileReq(pData[1]) == 0) {
		}
	} else {
		BatteryReadProfileStatus(pData, dataLen);
	}
}

void CmdServerReadWriteFuelGaugeConfig(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		if (FuelGaugeSetConfig(pData+1, *dataLen - 1) == 0) {
		}
	} else {
		FuelGaugeGetConfig(pData, dataLen);
	}
}

void CmdServerReadWriteDateTime(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		RtcWriteTime(pData+1, 1);
//		RTC_DateTypeDef dateConf;
//		dateConf.Year = pData[1];
//		dateConf.Month = pData[2];
//		dateConf.Date = pData[3];
//		dateConf.WeekDay = pData[4];
//		HAL_RTC_SetDate(&hrtc, &dateConf, RTC_FORMAT_BIN);
	} else {
		RtcReadTime(pData, 1);
		*dataLen = 9;
//		RTC_DateTypeDef dateConf;
//		HAL_RTC_GetDate(&hrtc, &dateConf, RTC_FORMAT_BIN);
//		uint8_t adr = pData[0];
//		pData[0] = dateConf.Year;
//		reg[adr] = pData[0];
//		pData[1] = dateConf.Month;
//		reg[adr+1] = pData[1];
//		pData[2] = dateConf.Date;
//		reg[adr+2] = pData[2];
//		pData[3] = dateConf.WeekDay;
//		reg[adr+3] = pData[3];
//		*dataLen = 4;
	}
}

void CmdServerReadWriteTime(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		RTC_TimeTypeDef sTime;
		sTime.Hours = pData[1];
		sTime.Minutes = pData[2];
		sTime.Seconds = pData[3];
		sTime.SubSeconds = pData[4];
		sTime.SecondFraction = 99;
		sTime.DayLightSaving = pData[5] & 0x01 ? RTC_DAYLIGHTSAVING_ADD1H : (pData[5] & 0x02 ? RTC_DAYLIGHTSAVING_SUB1H : RTC_DAYLIGHTSAVING_NONE);
		sTime.StoreOperation = pData[5] & 0x04 ? RTC_STOREOPERATION_SET : RTC_STOREOPERATION_RESET;
		HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	} else {
		RTC_TimeTypeDef sTime;
		HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
		uint8_t adr = pData[0];
		pData[0] = sTime.Hours;
		reg[adr] = pData[0];
		pData[1] = sTime.Minutes;
		reg[adr+1] = pData[1];
		pData[2] = sTime.Seconds;
		reg[adr+2] = pData[2];
		pData[3] = sTime.SubSeconds;
		reg[adr+3] = pData[3];
		pData[4] = sTime.DayLightSaving == RTC_DAYLIGHTSAVING_ADD1H ? 0x01 : (sTime.DayLightSaving == RTC_DAYLIGHTSAVING_SUB1H ? 0x02 : 0x00);
		pData[4] |= sTime.StoreOperation == RTC_STOREOPERATION_SET ? 0x04 : 0x00;
		reg[adr+4] = pData[4];
		*dataLen = 5;
	}
}

void CmdServerReadWriteDayLightSavingConfig(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {

	} else {

	}
}

void CmdServerReadWriteAlarm(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		RtcWriteAlarm1(pData+1, 1);
	}else {
		RtcReadAlarm1(pData, 1);
		*dataLen = 9;
	}
	/*RTC_AlarmTypeDef sAlarm;
	if (dir == MASTER_CMD_DIR_WRITE) {
		if (!(pData[5]&0x80)) { // bit 7 alarm enabled when 0
			sAlarm.AlarmTime.TimeFormat = RTC_HOURFORMAT12_AM;
			sAlarm.AlarmTime.Hours = pData[1];
			sAlarm.AlarmTime.Minutes = pData[2];
			sAlarm.AlarmTime.Seconds = pData[3];
			sAlarm.AlarmTime.SubSeconds = pData[4];
			sAlarm.Alarm = RTC_ALARM_A;
			sAlarm.AlarmDateWeekDaySel = pData[5] & 0x40 ? RTC_ALARMDATEWEEKDAYSEL_DATE : RTC_ALARMDATEWEEKDAYSEL_WEEKDAY; // bit 6 day if 1 weekday if 0
			sAlarm.AlarmDateWeekDay = pData[5] & 0x40 ? pData[5] & 0x1F : pData[5] & 0x07;
			sAlarm.AlarmMask = pData[6] ? (pData[6] < 32 ? 0x1U << pData[6] : RTC_ALARMMASK_ALL) : RTC_ALARMMASK_NONE;
			sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_NONE;
			HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN);
		} else {
			HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
		}
	} else {
		HAL_RTC_GetAlarm(&hrtc, &sAlarm, RTC_ALARM_A, RTC_FORMAT_BIN);
		uint8_t adr = pData[0];
		pData[0] = sAlarm.AlarmTime.Hours;
		reg[adr] = pData[0];
		pData[1] = sAlarm.AlarmTime.Minutes;
		reg[adr+1] = pData[1];
		pData[2] = sAlarm.AlarmTime.Seconds;
		reg[adr+2] = pData[2];
		pData[3] = sAlarm.AlarmTime.SubSeconds;
		reg[adr+3] = pData[3];
		pData[4] = (hrtc.Instance->CR & (RTC_CR_ALRAE)) ? 0x00 : 0x80;
		pData[4] |= (sAlarm.AlarmDateWeekDaySel == RTC_ALARMDATEWEEKDAYSEL_WEEKDAY) ? sAlarm.AlarmDateWeekDay : 0x40 | sAlarm.AlarmDateWeekDay;
		reg[adr+4] = pData[4];
		if (sAlarm.AlarmMask == RTC_ALARMMASK_ALL)
			pData[5] = 0xFF;
		else if (sAlarm.AlarmMask == RTC_ALARMMASK_SECONDS)
			pData[5] = 7;
		else if (sAlarm.AlarmMask == RTC_ALARMMASK_MINUTES)
			pData[5] = 15;
		else if (sAlarm.AlarmMask == RTC_ALARMMASK_HOURS)
			pData[5] = 23;
		else if (sAlarm.AlarmMask == RTC_ALARMMASK_DATEWEEKDAY)
			pData[5] = 31;
		else
			pData[5] = 0;
		reg[adr+5] = pData[5];
		*dataLen = 6;
	}*/
}

void CmdServerReadWriteRtcAlarmCtrlStatus(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		RtcWriteControlStatus(pData+1, *dataLen);
	} else {
		RtcReadControlStatus(pData, dataLen);
	}
}

void CmdServerReadWriteInputsConfig(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		ChargerWriteInputsConfig(pData[1]);
	} else {
		pData[0] = ChargerReadInputsConfig();
		*dataLen = 1;
	}
}

void CmdServerReadWriteScheduledPowerOff(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		//if (pData[1] == ~pData[2]) {
		PowerMngmtSchedulePowerOff(pData[1]);
		//}
	} else {
		pData[0] = PowerMngmtGetPowerOffCounter();
		*dataLen = 1;
	}
}

void CmdServerReadWriteVSysSwitchState(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		PowerSourceSetVSysSwitchState(pData[1]);
	} else {
		pData[0] = PowerSourceGetVSysSwitchState();
		*dataLen = 1;
	}
}

void CmdServerReadWriteWakeupOnCharge(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		//wakeupOnCharge = (pData[1]&0x7F) <= 100 ? (pData[1]&0x7F) * 10 : 0xFFFF;
		PowerMngmtSetWakeupOnChargeCmd(pData+1, *dataLen - 1);
	} else {
		//pData[0] = wakeupOnCharge <= 1000 ? wakeupOnCharge / 10 : 0xFF;
		//*dataLen = 1;
		PowerMngmtGetWakeupOnChargeCmd(pData, dataLen);
	}
}

void CmdServerReadWriteButtonConfigurationSw1(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		ButtonSetConfiguarion(0, pData+1, *dataLen - 1);
	} else {
		ButtonGetConfiguarion(0, pData, dataLen);
	}
}

void CmdServerReadWriteButtonConfigurationSw2(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		ButtonSetConfiguarion(1, pData+1, *dataLen - 1);
	} else {
		ButtonGetConfiguarion(1, pData, dataLen);
	}
}

void CmdServerReadWriteButtonConfigurationSw3(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		ButtonSetConfiguarion(2, pData+1, *dataLen - 1);
	} else {
		ButtonGetConfiguarion(2, pData, dataLen);
	}
}

void CmdServerReadWriteLedState1(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		LedCmdSetState(0, pData+1, *dataLen - 1);
	} else {
		LedCmdGetState(0, pData, dataLen);
	}
}

void CmdServerReadWriteLedState2(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		LedCmdSetState(1, pData+1, *dataLen - 1);
	} else {
		LedCmdGetState(1, pData, dataLen);
	}
}

void CmdServerReadWriteLedBlink1(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		LedCmdSetBlink(0, pData+1, *dataLen - 1);
	} else {
		LedCmdGetBlink(0, pData, dataLen);
	}
}

void CmdServerReadWriteLedBlink2(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		LedCmdSetBlink(1, pData+1, *dataLen - 1);
	} else {
		LedCmdGetBlink(1, pData, dataLen);
	}
}

void CmdServerReadWriteLedConfigurationLED1(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		LedSetConfiguarion(0, pData+1, *dataLen - 1);
	} else {
		LedGetConfiguarion(0, pData, dataLen);
	}
}

void CmdServerReadWriteLedConfigurationLED2(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		LedSetConfiguarion(1, pData+1, *dataLen - 1);
	} else {
		LedGetConfiguarion(1, pData, dataLen);
	}
}

void CmdServerReadWriteRunPinConfiguration(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		RunPinInstallationStatusSetConfigCmd(pData+1, *dataLen - 1);
	} else {
		RunPinInstallationStatusGetConfigCmd(pData, dataLen);
	}
}

void CmdServerReadWriteWDGConfiguration(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		PowerMngmtConfigureWatchdogCmd(pData+1, *dataLen - 1);
	} else {
		PowerMngmtGetWatchdogConfigurationCmd(pData, dataLen);
	}
}

void CmdServerReadWritePowerRegulatorConfiguration(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		SetPowerRegulatorConfigCmd(pData+1, *dataLen - 1);
	} else {
		GetPowerRegulatorConfigCmd(pData, dataLen);
	}
}

void CmdServerReadWriteOwnAddress1(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		uint8_t adr = pData[1]*2;
		if (pData[1] > 0 && pData[1] < 128 && hi2c1.Init.OwnAddress1 != adr ){
			EE_WriteVariable(OWN_ADDRESS1_NV_ADDR, adr | ((uint16_t)~adr<<8));
			uint16_t var = 0;
			EE_ReadVariable(OWN_ADDRESS1_NV_ADDR, &var);
			if ( (var&0xFF) == adr && (((~var)&0xFF) == (var>>8)) ) {
				// if successfully saved reinitialize I2C with new address
				hi2c1.Init.OwnAddress1 = adr;
				if (HAL_I2C_DeInit(&hi2c1) != HAL_OK)
				{
					//Error_Handler();
				}
				if (HAL_I2C_Init(&hi2c1) != HAL_OK)
				{
					//Error_Handler();
				}
			}
		}
	} else {
		pData[0] = hi2c1.Init.OwnAddress1 >> 1;
		*dataLen = 1;
	}
}

void CmdServerReadWriteOwnAddress2(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		uint8_t adr = pData[1]*2;
		if (pData[1] > 0 && pData[1] < 128 && hi2c1.Init.OwnAddress2 != adr ){
			EE_WriteVariable(OWN_ADDRESS2_NV_ADDR, adr | ((uint16_t)~adr<<8));
			uint16_t var = 0;
			EE_ReadVariable(OWN_ADDRESS2_NV_ADDR, &var);
			if ( (var&0xFF) == adr && (((~var)&0xFF) == (var>>8)) ) {
				// if successfully saved reinitialize I2C with new address
				hi2c1.Init.OwnAddress2 = adr;
				if (HAL_I2C_DeInit(&hi2c1) != HAL_OK)
				{
					//Error_Handler();
				}
				if (HAL_I2C_Init(&hi2c1) != HAL_OK)
				{
					//Error_Handler();
				}
			}
		}
	} else {
		pData[0] = hi2c1.Init.OwnAddress2 >> 1;
		*dataLen = 1;
	}
}

void CmdServerReadWriteEEPROM_WriteProtect(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, (pData[1]&0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
	} else {
		pData[0] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_8) == GPIO_PIN_SET ? 1 : 0;
		*dataLen = 1;
	}
}

void CmdServerReadWriteEEPROM_WriteAddress(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		uint8_t adrState = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_SET ? 0x52 : 0x50;
		if ( (pData[1] == 0x50 || pData[1] == 0x52) && adrState != pData[1] ){
			EE_WriteVariable(ID_EEPROM_ADR_NV_ADDR, pData[1] | ((uint16_t)~(pData[1])<<8));
			uint16_t var = 0;
			EE_ReadVariable(ID_EEPROM_ADR_NV_ADDR, &var);
			if ( (var&0xFF) == pData[1] && (((~var)&0xFF) == (var>>8)) ) {
				HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, (pData[1]&0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
			}
		}
	} else {
		pData[0] = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_SET ? 0x52 : 0x50;
		*dataLen = 1;
	}
}

void CmdServerReadWriteTestAndCalibration(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		if (pData[1] == 0x55 && pData[2] == 0x26 && pData[3] == 0xa0 && pData[4] == 0x2b ) {
			CalibrateLoadCurrent();
		}
	} else {
		//pData[0] = 0;
		*dataLen = 0;
	}
}

void CmdServerRunBootloader(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
  // Execute bootloader by jumping to system memory

	if ( pData[1] != 0x01 || dir == MASTER_CMD_DIR_READ ) return;

	executionState = EXECUTION_STATE_UPDATE;

	HAL_ADC_MspDeInit(&hadc);
	HAL_I2C_DeInit(&hi2c1);//HAL_SMBUS_MspDeInit(&hsmbus);
	HAL_I2C_MspDeInit(&hi2c2);
	HAL_RTC_MspDeInit(&hrtc);
	HAL_TIM_PWM_MspDeInit(&htim3);
	HAL_TIM_PWM_MspDeInit(&htim15);
	HAL_TIM_Base_MspDeInit(&htim17);
  // Disable all peripheral clocks
  __HAL_RCC_GPIOC_CLK_DISABLE();
  __HAL_RCC_GPIOF_CLK_DISABLE();
  __HAL_RCC_GPIOA_CLK_DISABLE();
  __HAL_RCC_GPIOB_CLK_DISABLE();
  __HAL_RCC_ADC1_CLK_DISABLE();
  __HAL_RCC_DMA1_CLK_DISABLE();
  __HAL_RCC_I2C1_CLK_DISABLE();
  __HAL_RCC_I2C2_CLK_DISABLE();
  __HAL_RCC_TIM1_CLK_DISABLE();
  __HAL_RCC_TIM3_CLK_DISABLE();
  __HAL_RCC_TIM14_CLK_DISABLE();
  __HAL_RCC_TIM15_CLK_DISABLE();
  __HAL_RCC_TIM17_CLK_DISABLE();
  __HAL_RCC_PWR_CLK_DISABLE();
  __HAL_RCC_SYSCFG_CLK_DISABLE();

  // Disable used PLL

  // Disable and clear interrupts
  // disable global interrupt
  __disable_irq();
  int i;
  for (i = 0; i <= 29; i++) {
	  HAL_NVIC_DisableIRQ(i);
	  HAL_NVIC_ClearPendingIRQ(i);
  }

  __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();

  // jump to bootloader address
  uint32_t JumpAddress = *(__IO uint32_t*) (SYS_MEM_ADDRESS + 4);
  Jump_To_Bootloader = (pFunction) JumpAddress;
   __set_MSP(*(__IO uint32_t*)SYS_MEM_ADDRESS);
  Jump_To_Bootloader();
}

extern void ButtonDualLongPressEventCb(void);

void CmdServerReadWriteDefaultConfiguration(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		if (pData[1] == 0xaa && pData[2] == 0x55 && pData[3] == 0x0a && pData[4] == 0xa3 ) {
			ButtonDualLongPressEventCb();
		}
	}
}

void CmdServerReadFirmwareVersion(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_READ) {
		pData[0] = firmwareVer;
		pData[1] = firmwareVariant;
		*dataLen = 2;
	}
}

void CmdServerReadBoardFaultStatus(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_READ) {
		// bit 0 charger i2c fault
		pData[0] = (hi2c2.ErrorCode || chargerI2cErrorCounter) & 0x01;
		// bit 1-3 charger fault status
		pData[0] |= ((CHARGER_FAULT_STATUS()) << 1) & 0xE0;
		// bit 4 fuel gauge i2c fault
		pData[0] |= FUEL_GAUGE_IC_FAULT_STATUS() << 4;
		// bit 5 fuel gauge temp sense fault (bad sensor connection)
		pData[0] |= FUEL_GAUGE_TEMP_SENSE_FAULT_STATUS() << 5;
		*dataLen = 1;
	}
}

void CmdServerReadWriteIoConfig1(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		IoSetConfiguarion(1, pData+1, *dataLen - 1);
	} else {
		IoGetConfiguarion(1, pData, dataLen);
	}
}

void CmdServerReadWriteIoConfig2(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		IoSetConfiguarion(2, pData+1, *dataLen - 1);
	} else {
		IoGetConfiguarion(2, pData, dataLen);
	}
}

void CmdServerReadWriteIoValue1(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		IoWrite(1, pData+1, *dataLen - 1);
	} else {
		IoRead(1, pData, dataLen);
	}
}

void CmdServerReadWriteIoValue2(uint8_t dir, uint8_t *pData, uint16_t *dataLen) {
	if (dir == MASTER_CMD_DIR_WRITE) {
		IoWrite(2, pData+1, *dataLen - 1);
	} else {
		IoRead(2, pData, dataLen);
	}
}

