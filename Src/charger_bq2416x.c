#include "charger_bq2416x.h"
#include "stm32f0xx_hal.h"
#include "time_count.h"
#include "nv.h"
#include "eeprom.h"
#include "fuel_gauge_lc709203f.h"
#include "stddef.h"
#include "power_source.h"
#include "analog.h"

#define CHG_READ_PERIOD_MS 	90  // ms
#define WD_RESET_TRSH_MS 	(30000 / 3)

#define BQ2416X_OTG_LOCK_BIT	0X08
#define BQ2416X_NOBATOP_BIT		0X01

//#define CHARGER_VIN_DPM_IN		0X00
#define CHARGER_VIN_DPM_USB		0X07

extern uint8_t resetStatus;

uint8_t chargerNeedPoll = 0;

extern I2C_HandleTypeDef hi2c2;
static uint32_t readTimeCounter = 0;
static uint32_t wdTimeCounter = 0;
uint8_t regs[8] = {0x00, 0x00, 0x8C, 0x14, 0x40, 0x32, 0x00, 0x98};
static uint8_t regsw[8] = {0x08, 0x08, 0x1C, 0x02, 0x00, 0x00, 0x38, 0xC0};
static uint8_t regsStatusRW[8] = {0x00}; // 0 - no read write errors, bit0 1 - read error, bit1 1 - write error
static uint8_t regswMask[8] = {0x08, 0x0A, 0x7F, 0xFF, 0x00, 0xFF, 0x3F, 0xE9}; // write and read verify masks

uint8_t chargerI2cErrorCounter = 0;

ChargerUSBInLockoutStatus_T usbInLockoutStatus = CHG_USB_IN_UNKNOWN;

ChargerUsbInCurrentLimit_T chargerUsbInCurrentLimit = CHG_IUSB_LIMIT_150MA; // current limit code as defined in datasheet

uint8_t chargerInputsConfig __attribute__((section("no_init")));

uint8_t usbInEnabled __attribute__((section("no_init")));

uint8_t noBatteryTurnOn __attribute__((section("no_init")));

uint8_t chargerInputsPrecedence __attribute__((section("no_init")));

uint8_t chargerInLimit __attribute__((section("no_init"))); // 0 - 1.5A, 1 - 2.5A

uint8_t chargerInDpm __attribute__((section("no_init")));

uint8_t chargingConfig __attribute__((section("no_init")));

uint8_t chargingEnabled __attribute__((section("no_init")));

uint8_t noBatteryOperationEnabled = 0;

/*int32_t chargeCurrentReq = -1;
int16_t batRegVoltageReq = -1;
int16_t chargeTerminationCurrentReq = -1;*/
int16_t chargerSetProfileDataReq = 0;

ChargerStatus_T chargerStatus = CHG_NA;
//static volatile ChargerFaultStatus_T faultStatus = CHG_FAULT_UNKNOWN;

uint8_t chargerInterruptFlag = 0;

//#define CHARGER_I2C_REG_WRITE_TIMEOUT_US	300
//#define CHARGER_I2C_REG_READ_TIMEOUT_US	500

void ChargerSetInputsConfig(uint8_t config);

HAL_StatusTypeDef ChargerRegRead( uint8_t regAddress ) {
	HAL_StatusTypeDef rStatus;
	uint8_t regVal1, regVal2;
	regsStatusRW[regAddress] |= 0x01;
	rStatus = HAL_I2C_Mem_Read(&hi2c2, 0xD6, regAddress, 1, &regVal1, 1, 1);
	if (rStatus == HAL_OK) {
		// read once more to confirm
		regVal2 = ~regVal1;
		rStatus = HAL_I2C_Mem_Read(&hi2c2, 0xD6, regAddress, 1, &regVal2, 1, 1);
		if (rStatus == HAL_OK) {
			if (regVal2 == regVal1) {
				regs[regAddress] = regVal2;
				regsStatusRW[regAddress] &= ~0x01;
				rStatus = HAL_OK;
			} else
				rStatus = HAL_ERROR;
		}
	}

	if (rStatus != HAL_OK) {
		chargerI2cErrorCounter ++;
	} else {
		chargerI2cErrorCounter = 0;
	}

	return rStatus;
}

HAL_StatusTypeDef ChargerRegWrite( uint8_t regAddress ) {
	regsStatusRW[regAddress] |= 0x03;
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write(&hi2c2, 0xD6, regAddress, 1, &regsw[regAddress], 1, 1);
	if (status != HAL_OK) return status;
	// verify reading back from charger register
	status = ChargerRegRead(regAddress);
	if ( status == HAL_OK ) {
		regsStatusRW[regAddress] &= ~0x01; // read was successful
		if ((regs[regAddress]&regswMask[regAddress]) == (regsw[regAddress]&regswMask[regAddress])) {
			regsStatusRW[regAddress] &= ~0x02; // write was successful
			status = HAL_OK;
		} else {
			status = HAL_ERROR;
		}
	}

	if (status != HAL_OK) {
		chargerI2cErrorCounter ++;
	} else {
		chargerI2cErrorCounter = 0;
	}
	return status;
}

/*int8_t ChargerReadRegulationVoltage() {
	uint8_t reg = ~regsw[3];
	if (ChargerRegRead(0x03, &reg) != HAL_OK) {
		return 1;
	} else {
		regs[3] &= ~0xFC;
		regs[3] |= reg&0xFC;
	}
	return 0;
}*/

int8_t ChargerUpdateRegulationVoltage() {
	static int8_t status;
	//static int8_t *pStatus;
	//static uint32_t readTimeCnt;

	regsw[3] &= ~0xFE;
	regsw[3] |= chargerInLimit << 1;
	if (currentBatProfile!=NULL) {
		int16_t newRegVol;
		if (batteryTemp > currentBatProfile->tWarm && tempSensorConfig != BAT_TEMP_SENSE_CONFIG_NOT_USED) {
			newRegVol = (int16_t)(currentBatProfile->regulationVoltage) - (140/20);
			newRegVol = newRegVol < 0 ? 0 : newRegVol;
		} else {
			newRegVol = currentBatProfile->regulationVoltage;
		}

		regsw[3] |= newRegVol << 2;
	} else {
		if (!regsStatusRW[0x03]) return 0;
	}

	// if there were errors in previous transfers read state from register
	if (/*pStatus==0 || status*/ regsStatusRW[0x03] ) {
		/*if (pStatus==0) {
			pStatus = &status;
			MS_TIME_COUNTER_INIT(readTimeCnt);
		}*/

		if (ChargerRegRead(0x03) != HAL_OK) {
			status = 1;
			return 1;
		}
	}

	if ( (regsw[3]&0xFF) != (regs[3]&0xFF) ) {
		// set high impedance mode first, only if there is battery present, or it will disable vsys mosfet
		/*if (CHARGER_IS_BATTERY_PRESENT()) {
			regsw[2] |= 0x01;
			if (ChargerRegWrite(0x02) != HAL_OK) {
				status = 2;
				return 2;
			}
		}*/
		// write regulation voltage register
		if (ChargerRegWrite(0x03) != HAL_OK) {
			status = 3;
			return 2;
		}
		/*if (CHARGER_IS_BATTERY_PRESENT()) {
			regsw[2] &= ~0x01;
			// return from high impedance to normal mode
			if (ChargerRegWrite(0x02) != HAL_OK) {
				status = 4;
				return 2;
			}
		}*/

	} /*else if ( MS_TIME_COUNT(readTimeCnt) > 9425){
		MS_TIME_COUNTER_INIT(readTimeCnt);
		// Just periodically read-back
		if (ChargerRegRead(0x03) != HAL_OK) {
			status = 5;
			return 1;
		}
	}*/

	status = 0;
	return 0;
}

int8_t ChargerUpdateChgCurrentAndTermCurrent() {
	static int8_t status;

	if (currentBatProfile!=NULL) {
		regsw[5] = ((currentBatProfile->chargeCurrent>26?26:currentBatProfile->chargeCurrent&0x1F) << 3) | (currentBatProfile->terminationCurr&0x07);
	} else {
		regsw[5] = 0;
		if (!regsStatusRW[0x05]) return 0;
	}

	// if there were errors in previous transfers, or first time update, read state from register
	if (regsStatusRW[0x05]) {
		if (ChargerRegRead(0x05) != HAL_OK) {
			status = 1;
			return 1;
		}
	}

	if ( regsw[5] != regs[5] ) {
		// write new value to register
		if (ChargerRegWrite(0x05) != HAL_OK) {
			status = 2;
			return 2;
		}
	}

	status = 0;
	return 0;
}

int8_t ChargerUpdateVinDPM() {
	static int8_t status;

	// if there were errors in previous transfers read state from register
	if (regsStatusRW[0x06]) {
		if (ChargerRegRead(0x06) != HAL_OK) {
			status = 1;
			return 1;
		}
	}

	regsw[6] = (uint8_t)chargerInDpm | ((uint8_t)CHARGER_VIN_DPM_USB << 3);
	if ( regsw[6] != (regs[6]&0x3F) ) {
		// write new value to register
		if (ChargerRegWrite(0x06) != HAL_OK) {
			status = 2;
			return 2;
		}
	}

	status = 0;
	return 0;
}

int8_t ChargerUpdateTempRegulationControlStatus() {
	static int8_t status;

	regsw[7] = 0xC0; // Timer slowed by 2x when in thermal regulation, 10 � 9 hour fast charge, TS function disabled
	if (currentBatProfile!=NULL) {
		if (batteryTemp < currentBatProfile->tCool && tempSensorConfig != BAT_TEMP_SENSE_CONFIG_NOT_USED) {
			// charge current reduced to half
			regsw[7] |= 0x01;
		} else {
			// normal charge current
			regsw[7] &= ~0x01;
		}
	} else {
		regsw[7] &= ~0x01;
		if (!regsStatusRW[0x07]) return 0;
	}

	// if there were errors in previous transfers read state from register
	if (regsStatusRW[0x07]) {
		if (ChargerRegRead(0x07) != HAL_OK) {
			status = 1;
			return 1;
		}
	}

	if ( (regsw[7]&0xE9) != (regs[7]&0xE9) ) {
		// write new value to register
		if (ChargerRegWrite(0x07) != HAL_OK) {
			status = 2;
			return 2;
		}
	}

	status = 0;
	return 0;
}

int8_t ChargerUpdateControlStatus() {
	static int8_t status;

	regsw[2] = ((chargerUsbInCurrentLimit&0x07) << 4) | 0x0C; // usb in current limit code, Enable STAT output, Enable charge current termination
	if (currentBatProfile!=NULL) {
		if ( !chargingEnabled || ((batteryTemp >= currentBatProfile->tHot || batteryTemp <= currentBatProfile->tCold) && tempSensorConfig != BAT_TEMP_SENSE_CONFIG_NOT_USED) ) {
			// disable charging
			regsw[2] |= 0x02;
			regsw[2] &= ~0x01; // clear high impedance mode
		} else {
			// enable charging
			regsw[2] &= ~0x02;
			regsw[2] &= ~0x01; // clear high impedance mode
			regsw[2] |= 0x04; // Enable charge current termination
		}
	} else {
		// disable charging
		regsw[2] |= 0x02;
		regsw[2] &= ~0x01; // clear high impedance mode
	}

	// if there were errors in previous transfers, or first time update, read state from register
	if (regsStatusRW[0x02]) {
		if (ChargerRegRead(0x02) != HAL_OK) {
			status = 1;
			return 1;
		}
	}

	if ( (regsw[2]&0x7F) != (regs[2]&0x7F) ) {
		// write new value to register
		if (ChargerRegWrite(0x02) != HAL_OK) {
			status = 2;
			return 1;
		}
	}

	status = 0;
	return 0;
}

int8_t ChargerUpdateUSBInLockout() {
	static int8_t status;

	regsw[1] = noBatteryOperationEnabled != 0;
	if ( usbInEnabled && pow5vInDetStatus == POW_5V_IN_DETECTION_STATUS_PRESENT && (regs[1] & 0x06) == 0x00 ) {
		regsw[1] &= ~BQ2416X_OTG_LOCK_BIT;
	} else {
		regsw[1] |= BQ2416X_OTG_LOCK_BIT;
	}

	usbInLockoutStatus = CHG_USB_IN_UNKNOWN;

	// if there were errors in previous transfers, or first time update, read state from register
	if (regsStatusRW[0x01]) {
		if (ChargerRegRead(0x01) != HAL_OK) {
			status = 1;
			return 1;
		}
	}

	if ( (regsw[1]&0x09) != (regs[1]&0x09) ) {
		// write new value to register
		if (ChargerRegWrite(0x01) != HAL_OK) {
			status = 2;
			return 2;
		}
	}

	usbInLockoutStatus = (regs[1] & BQ2416X_OTG_LOCK_BIT) ? CHG_USB_IN_LOCK : CHG_USB_IN_UNLOCK;

	status = 0;
	return 0;
}

void ChargerInit() {
	uint8_t chReg = 0;
	uint16_t var = 0;
	//const BatteryProfile_T* batProfile = BatteryGetProfile();

	if (!resetStatus) {
		EE_ReadVariable(CHARGER_INPUTS_CONFIG_NV_ADDR, &var);
		if (((~var)&0xFF) == (var>>8)) {
			chargerInputsConfig = var&0xFF;
			ChargerSetInputsConfig(chargerInputsConfig);
		} else {
			// set default config
			chargerInputsPrecedence = 0x01; // 5V GPIO has precedence for charging
			usbInEnabled = 1; // enabled
			noBatteryTurnOn = 0;
			chargerInLimit = 1; // 2.5A
			chargerInDpm = 0; // 4.2V
			chargerInputsConfig = chargerInputsPrecedence & 0x01;
			chargerInputsConfig |= usbInEnabled << 1;
			chargerInputsConfig |= noBatteryTurnOn << 2;
			chargerInputsConfig |= chargerInLimit << 3;
			chargerInputsConfig |= chargerInDpm << 4;
		}

		EE_ReadVariable(CHARGING_CONFIG_NV_ADDR, &var);
		if (((~var)&0xFF) == (var>>8)) {
			chargingConfig = var&0xFF;
			chargingEnabled = chargingConfig & 0x01;
		} else {
			// set default config
			chargingEnabled = 1;
			chargingConfig = chargingEnabled & 0x01;
		}
	}

	MS_TIME_COUNTER_INIT(readTimeCounter);
	MS_TIME_COUNTER_INIT(wdTimeCounter);

	regsw[1] |= 0x08; // lockout usbin
	HAL_I2C_Mem_Write(&hi2c2, 0xD6, 1, 1, &regsw[1], 1, 1);

	// NOTE: do not place in high impedance mode, it will disable VSys mosfet, and no power to mcu
	regsw[2] |= 0x02; // set control register, disable charging initially
	//regsw[2] &= ~0x04; // disable termination
	regsw[2] |= 0x20; // Set USB limit 500mA
	HAL_I2C_Mem_Write(&hi2c2, 0xD6, 2, 1, &regsw[2], 1, 1);

	// reset timer
	//regsw[0] = chargerInputsPrecedence << 3;
	//chReg = regsw[0] | 0x80;
	//HAL_I2C_Mem_Write(&hi2c2, 0xD6, 0, 1, &chReg, 1, 1);

	DelayUs(500);

	// read states
	HAL_I2C_Mem_Read(&hi2c2, 0xD6, 0, 1, regs, 8, 1000);

	ChargerUpdateUSBInLockout();
	ChargerUpdateTempRegulationControlStatus();

	ChargerRegRead(0);
	ChargerRegRead(1);

	//faultStatus = CHARGER_FAULT_STATUS();
	chargerStatus = (regs[0] >> 4) & 0x07;
}

void ChargerTask(void) {
	static int8_t currRegAdr;
	chargerNeedPoll = 0;
	if (ChargerUpdateUSBInLockout() != 0) {
		chargerNeedPoll = 1;
		return;
	}

	if (chargerInterruptFlag) {
		// update status on interrupt
		ChargerRegRead(0);
		chargerInterruptFlag = 0;
		chargerNeedPoll = 1;
	}

	if (ChargerUpdateControlStatus() != 0) {
		chargerNeedPoll = 1;
		return;
	}

	if (ChargerUpdateRegulationVoltage() != 0) {
		chargerNeedPoll = 1;
		return;
	}

	if (ChargerUpdateTempRegulationControlStatus() != 0) {
		chargerNeedPoll = 1;
		return;
	}

	if (ChargerUpdateChgCurrentAndTermCurrent() != 0) {
		chargerNeedPoll = 1;
		return;
	}

	if (ChargerUpdateVinDPM() != 0) {
		chargerNeedPoll = 1;
		return;
	}

	if (MS_TIME_COUNT(wdTimeCounter) > WD_RESET_TRSH_MS) {
		// reset timer
		// NOTE: reset bit must be 0 in write register image to prevent resets for other write access
		regsw[0] = chargerInputsPrecedence << 3;
		uint8_t resetVal = regsw[0] | 0x80;
		if ( HAL_I2C_Mem_Write(&hi2c2, 0xD6, 0, 1, &resetVal, 1, 1) == HAL_OK )  {
			MS_TIME_COUNTER_INIT(wdTimeCounter);
		}
		//while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY);
	}

	// Periodically read register states from charger
	if (MS_TIME_COUNT(readTimeCounter) >= CHG_READ_PERIOD_MS) {

		if (ChargerRegRead(currRegAdr) == HAL_OK) {
			currRegAdr ++;
			currRegAdr &= 0x07; // 8 registers to read in circle
		}

		//faultStatus = regs[0] & 0x07;
		chargerStatus = (regs[0] >> 4) & 0x07;

		/*if (faultStatus) {
			// clear fault by setting to high impedance
			// set high impedance mode first
			regsw[2] |= 0x01;
			if (ChargerRegWrite(0x02) != HAL_OK) {
			}

			regsw[2] &= ~0x01;
			// return from high impedance to normal mode
			if (ChargerRegWrite(0x02) != HAL_OK) {
			}
		}*/

		MS_TIME_COUNTER_INIT(readTimeCounter);
		/*uint8_t i = 8;
		while(i--) {
			regs[i] = 0;
			HAL_I2C_Mem_Read_DMA(&hi2c2, 0xD6, i, 1, &regs[i], 1);
			HAL_Delay(1);
		}*/
		// Get status
		/*uint8_t regVal1, regVal2;
		int8_t succ = 2;
		if (ChargerRegRead(0x00, &regVal1) == HAL_OK) {
			// read once more to confirm
			regVal2 = ~regVal1;
			if (ChargerRegRead(0x00, &regVal2) == HAL_OK) {
				if (regVal2 == regVal1) {
					regs[0] &= ~0x77;
					regs[0] |= regVal1&0x77;
					faultStatus = regVal1 & 0x07;
					chargerStatus = (regVal1 >> 4) & 0x07;
					succ --;
				}
			}
		}
		if (ChargerRegRead(0x01, &regVal1) == HAL_OK) {
			// read once more to confirm
			regVal2 = ~regVal1;
			if (ChargerRegRead(0x01, &regVal2) == HAL_OK) {
				if (regVal2 == regVal1) {
					regs[1] &= ~0xF6;
					regs[1] |= regVal1&0xF6;
					succ --;
				}
			}
		}*/
	}

		//HAL_Delay(2);
		//HAL_I2C_Mem_Read_DMA(&hi2c2, 0xD6, 0, 1, regs, 8);
		//uint32_t timeout = HAL_GetTick() + 10;
		//while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY && timeout > HAL_GetTick());
		//HAL_Delay(2);

/*
		const BatteryProfile_T *bProfile = BatteryGetProfile();
		if (bProfile != NULL) {
			if ( (batteryTemp > bProfile->tHot || batteryTemp < bProfile->tCold) ) {
				// disable charging
				if (!(regs[2] & 0x02)) {
					regsw[2] |= 0x02;
					I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x02, 1, &regsw[2], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
					//HAL_Delay(1);
				}
			} else if (batteryTemp > bProfile->tWarm) {
				// reduce battery regulation voltage
				int16_t newRegVol = (int16_t)(bProfile->regulationVoltage) - (140/20);
				newRegVol = newRegVol < 0 ? 0 : newRegVol;
				regsw[3] &= 0x03;
				regsw[3] |= newRegVol << 2;
				if (regs[3] != regsw[3]) {
					regsw[2] |= 0x01;
					I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x02, 1, &regsw[2], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US); // set high impedance mode first
					//HAL_Delay(1);
					I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x03, 1, &regsw[3], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
					regsw[2] &= ~0x01;
					//HAL_Delay(1);
					I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x02, 1, &regsw[2], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US); // set not high impedance mode
					//HAL_Delay(1);
				}
				// enable charging if disabled
				if (regs[2] & 0x02) {
					regsw[2] &= ~0x02;
					I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x02, 1, &regsw[2], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
					//HAL_Delay(1);
				}
			} else if (batteryTemp < bProfile->tCool) {
				// reduce charge current to half
				if (!(regs[7] & 0x01)) {
					regsw[7] |= 0x01;
					I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x07, 1, &regsw[7], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
					//HAL_Delay(1);
				}
				// enable charging if disabled
				if (regs[2] & 0x02) {
					regsw[2] &= ~0x02;
					I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x02, 1, &regsw[2], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
					//HAL_Delay(1);
				}
			} else {
				// set normal regulation voltage
				if ( bProfile->regulationVoltage != (regs[3] >> 2)) {
					regsw[3] &= 0x03;
					regsw[3] |= bProfile->regulationVoltage << 2;
					regsw[2] |= 0x01;
					I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x02, 1, &regsw[2], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US); // set high impedance mode first
					//HAL_Delay(1);
					I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x03, 1, &regsw[3], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
					regsw[2] &= ~0x01;
					//HAL_Delay(1);
					I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x02, 1, &regsw[2], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US); // set not high impedance mode
					//HAL_Delay(1);
				}
				// verify charge current settings
				if (regsw[5] != regs[5] ) {
					regsw[5] = ((bProfile->chargeCurrent&0x1F) << 3) | (bProfile->terminationCurr&0x07);
					I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x05, 1, &regsw[5], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
					//HAL_Delay(1);
				}
				// set normal charge current
				if (regs[7] & 0x01) {
					regsw[7] &= ~0x01;
					I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x07, 1, &regsw[7], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
					//HAL_Delay(1);
				}
				// enable charging if disabled
				if (regs[2] & 0x02) {
					regsw[2] &= ~0x02;
					I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x02, 1, &regsw[2], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
					//HAL_Delay(1);
				}
			}
		} else {
			// disable charging if profile is invalid
			if (!(regs[2] & 0x02)) {
				regsw[2] |= 0x02;
				I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x02, 1, &regsw[2], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
				//HAL_Delay(1);
			}
		}
	}*/

	/*if ( chargeCurrentReq >= 0 ) {
		regsw[5] &= 0x07;
		regsw[5] |= ((chargeCurrentReq * 10 - 550) / 75) << 3;
		HAL_I2C_Mem_Write(&hi2c2, 0xD6, 0x05, 1, &regsw[5], 1, 10);
		//while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY);
		regs[5] = regsw[5];
		//EE_WriteVariable(CHARGE_CURRENT_NV_ADDR, chargeCurrentReq);
		chargeCurrentReq = -1;
	}

	if ( chargeTerminationCurrentReq >= 0 ) {
		regsw[5] = (regsw[5] & 0xF8) | ((chargeTerminationCurrentReq * 10 - 50) / 50);
		HAL_I2C_Mem_Write(&hi2c2, 0xD6, 0x05, 1, &regsw[5], 1, 10);
		//while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY);
		regs[5] = regsw[5];
		//EE_WriteVariable(CHARGE_TERM_CURRENT_NV_ADDR, chargeTerminationCurrentReq); // it should be after readback verification
		chargeTerminationCurrentReq = -1;
	}

	if ( batRegVoltageReq >= 0 ) {
		regsw[3] &= 0x03;
		regsw[3] |= batRegVoltageReq << 2;
		HAL_I2C_Mem_Write(&hi2c2, 0xD6, 0x03, 1, &regsw[3], 1, 10);
		//while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY);
		regs[3] = regsw[3];
		//EE_WriteVariable(BAT_REG_VOLTAGE_NV_ADDR, batRegVoltageReq);
		batRegVoltageReq = -1;
	}*/
	/*if (chargerSetProfileDataReq) {
		chargerSetProfileDataReq = 0;
		if (currentBatProfile == NULL) {
			// disable charging if profile is invalid
			if (!(regs[2] & 0x02)) {
				regsw[2] |= 0x02;
				I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x02, 1, &regsw[2], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
				//HAL_Delay(1);
			}
			regs[2] = regsw[2];
		} else {
			regsw[5] = ((currentBatProfile->chargeCurrent&0x1F) << 3) | (currentBatProfile->terminationCurr&0x07);
			I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x05, 1, &regsw[5], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
			//HAL_Delay(1);
			regs[5] = regsw[5];

			regsw[3] &= 0x03;
			regsw[3] |= currentBatProfile->regulationVoltage << 2;
			I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x02, 1, &regsw[2], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US); // set high impedance mode first
			//HAL_Delay(1);
			I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x03, 1, &regsw[3], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
			regsw[2] &= ~0x01;
			//HAL_Delay(1);
			I2C_SlaveRegsWrite(&hi2c2, 0xD6, 0x02, 1, &regsw[2], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US); // set not high impedance mode
			//HAL_Delay(1);
			regs[3] = regsw[3];
		}

	}*/

	/*static uint8_t curReg = 0;
	static uint32_t tick;
	if (HAL_GetTick() > tick) {
		curReg = (curReg + 1) & 0x3;
		HAL_I2C_Mem_Read(&hi2c2, 0xD6, curReg, 1, &regs[curReg], 1, 2);
		if ( (regs[curReg]&regswMask[curReg]) != (regsw[curReg]&regswMask[curReg]) ) {
			HAL_I2C_Mem_Write(&hi2c2, 0xD6, curReg, 1, &regsw[curReg], 1, 2);
		}
		tick = HAL_GetTick() + 90;
	}*/

}

void ChargerTriggerNTCMonitor(NTC_MonitorTemperature_T temp) {
	switch (temp) {
	case COLD:
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
		break;
	case COOL:
		break;
	case WARM:
		break;
	case HOT:
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
		break;
	case NORMAL: default:
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
		break;
	}
}

void ChargerSetUSBLockout(ChargerUSBInLockoutStatus_T status) {
	chargerNeedPoll |= ChargerUpdateUSBInLockout();
	/*uint8_t chReg;

	if (status == CHG_USB_IN_LOCK) {
		if (!(regs[1] & BQ2416X_OTG_LOCK_BIT)) {
			regsw[1] |= 0x08;
			I2C_SlaveRegsWrite(&hi2c2, 0xD6, 1, 1, &regsw[1], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
			//while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY);
			//regs[1] = regsw[1];
		}
	} else {
		if (regs[1] & BQ2416X_OTG_LOCK_BIT) {
			// turn on usb in
			regsw[1] &= ~0x08;
			I2C_SlaveRegsWrite(&hi2c2, 0xD6, 1, 1, &regsw[1], 1, CHARGER_I2C_REG_WRITE_TIMEOUT_US);
			//while (HAL_I2C_GetState(&hi2c2) != HAL_I2C_STATE_READY);
			//regs[1] = regsw[1];
		}
	}*/
}

/*void SetChargeCurrentReq(uint8_t current) {
	chargeCurrentReq = current;
}

void SetBatRegulationVoltageReq(uint8_t voltageCode) {
	batRegVoltageReq = voltageCode;
}

void SetChargeTerminationCurrentReq(uint8_t current) {
	chargeTerminationCurrentReq = current;
}*/

void ChargerSetBatProfileReq(const BatteryProfile_T* batProfile) {
	chargerSetProfileDataReq = 1;
}

void ChargerUsbInCurrentLimitStepUp(void) {
	if (chargerUsbInCurrentLimit < CHG_IUSB_LIMIT_1500MA) chargerUsbInCurrentLimit ++;
}

void ChargerUsbInCurrentLimitStepDown(void) {
	if (chargerUsbInCurrentLimit > CHG_IUSB_LIMIT_150MA) chargerUsbInCurrentLimit --;
}

void ChargerUsbInCurrentLimitSetMin(void) {
	chargerUsbInCurrentLimit = CHG_IUSB_LIMIT_150MA;
}

void ChargerSetInputsConfig(uint8_t config) {
	chargerInputsConfig = config;
	chargerInputsPrecedence = config & 0x01;
	usbInEnabled = (config & 0x02) == 0x02;
	noBatteryTurnOn = (config & 0x04) == 0x04;
	chargerInLimit = (config & 0x08) == 0x08;
	chargerInDpm = (config >> 4) & 0x07;
}

void ChargerWriteInputsConfig(uint8_t config) {
	ChargerSetInputsConfig(config);
	if (config & 0x80) {
		NvWriteVariableU8(CHARGER_INPUTS_CONFIG_NV_ADDR, config);
	}
}

uint8_t ChargerReadInputsConfig(void) {
	uint16_t var = 0;
	EE_ReadVariable(CHARGER_INPUTS_CONFIG_NV_ADDR, &var);
	if (((~var)&0xFF) == (var>>8)) {
		if ((chargerInputsConfig & 0x7F) == (var & 0x7F)) {
			return var&0xFF;
		} else {
			return chargerInputsConfig;
		}
	} else {
		return chargerInputsConfig;
	}
}

void ChargerWriteChargingConfig(uint8_t config) {
	chargingConfig = config;
	chargingEnabled = chargingConfig & 0x01;
	if (config & 0x80) {
		NvWriteVariableU8(CHARGING_CONFIG_NV_ADDR, config);
	}
}

uint8_t ChargerReadChargingConfig(void) {
	uint16_t var = 0;
	EE_ReadVariable(CHARGING_CONFIG_NV_ADDR, &var);
	if (((~var)&0xFF) == (var>>8)) {
		if ((chargingConfig & 0x7F) == (var & 0x7F)) {
			return var&0xFF;
		} else {
			return chargingConfig;
		}
	} else {
		return chargingConfig;
	}
}
