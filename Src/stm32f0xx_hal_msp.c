/**
  ******************************************************************************
  * File Name          : stm32f0xx_hal_msp.c
  * Description        : This file provides code for the MSP Initialization 
  *                      and de-Initialization codes.
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f0xx_hal.h"

static DMA_HandleTypeDef hdma_tx2;
static DMA_HandleTypeDef hdma_rx2;

extern void Error_Handler(void);

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
  * Initializes the Global MSP.
  */
void HAL_MspInit(void)
{
  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  __HAL_RCC_SYSCFG_CLK_ENABLE();

  /* System interrupt init*/
  /* SVC_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SVC_IRQn, 0, 0);
  /* PendSV_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(PendSV_IRQn, 0, 0);
  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);

  /* USER CODE BEGIN MspInit 1 */
  HAL_NVIC_EnableIRQ(SysTick_IRQn);
  /* USER CODE END MspInit 1 */
}
static DMA_HandleTypeDef DmaHandle;
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  if(hadc->Instance==ADC1)
  {
    /* Peripheral clock enable */
	// Enable GPIO clock
	__HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    // Enable DMA1 clock
    __HAL_RCC_DMA1_CLK_ENABLE();

    /**ADC GPIO Configuration
    PA0     ------> ADC_IN0
    PA1     ------> ADC_IN1
    PA2     ------> ADC_IN2
    PA3     ------> ADC_IN3
    PA4     ------> ADC_IN4
    PA5     ------> ADC_IN5
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN ADC1_MspInit 1 */

  /*********************** Configure DMA parameters ***************************/
  DmaHandle.Instance                 = DMA1_Channel1;
  DmaHandle.Init.Direction           = DMA_PERIPH_TO_MEMORY;
  DmaHandle.Init.PeriphInc           = DMA_PINC_DISABLE;
  DmaHandle.Init.MemInc              = DMA_MINC_ENABLE;
  DmaHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  DmaHandle.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
  DmaHandle.Init.Mode                = DMA_CIRCULAR;
  DmaHandle.Init.Priority            = DMA_PRIORITY_MEDIUM;

  /* Deinitialize  & Initialize the DMA for new transfer */
  HAL_DMA_DeInit(&DmaHandle);
  HAL_DMA_Init(&DmaHandle);

  /* Associate the DMA handle */
  __HAL_LINKDMA(hadc, DMA_Handle, DmaHandle);

  /* NVIC configuration for DMA Input data interrupt */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

  // interrupt for adc watchdog
  HAL_NVIC_SetPriority(ADC1_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(ADC1_IRQn);

  /* USER CODE END ADC1_MspInit 1 */
  }

}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{

  if(hadc->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */
	  HAL_NVIC_DisableIRQ(ADC1_IRQn);
	  HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
	  HAL_DMA_DeInit(&DmaHandle);
	  __HAL_RCC_DMA1_CLK_DISABLE();
  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC1_CLK_DISABLE();
  
    /**ADC GPIO Configuration    
    PA0     ------> ADC_IN0
    PA1     ------> ADC_IN1
    PA2     ------> ADC_IN2
    PA3     ------> ADC_IN3
    PA4     ------> ADC_IN4
    PA5     ------> ADC_IN5 
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3 
                          |GPIO_PIN_4|GPIO_PIN_5);

  }
  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */

}
/*
void HAL_WWDG_MspInit(WWDG_HandleTypeDef* hwwdg)
{

  if(hwwdg->Instance==WWDG)
  {

    //Peripheral clock enable
    __HAL_RCC_WWDG_CLK_ENABLE();

    HAL_NVIC_SetPriority(WWDG_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(WWDG_IRQn);
  }

}*/

void HAL_SMBUS_MspInit(SMBUS_HandleTypeDef *hsmbus)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(hsmbus->Instance==I2C1)
  {
	  static DMA_HandleTypeDef hdma_tx;
	  static DMA_HandleTypeDef hdma_rx;
	  RCC_PeriphCLKInitTypeDef  RCC_PeriphCLKInitStruct;

	  /*##-1- Configure the I2C clock source. The clock is derived from the SYSCLK #*/
	  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
	  RCC_PeriphCLKInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_SYSCLK;
	  HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);

	  /*##-2- Enable peripherals and GPIO Clocks #################################*/
	  /* Enable GPIO TX/RX clock */
	  __HAL_RCC_GPIOB_CLK_ENABLE();
	  /* Enable I2Cx clock */
	  __HAL_RCC_I2C1_CLK_ENABLE();

	  /* Enable DMAx clock */
	  __HAL_RCC_DMA1_CLK_ENABLE();

	  /*##-3- Configure peripheral GPIO ##########################################*/
	  /* I2C TX GPIO pin configuration  */
	  GPIO_InitStruct.Pin       = GPIO_PIN_6;
	  GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
	  GPIO_InitStruct.Pull      = GPIO_PULLUP;
	  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
	  GPIO_InitStruct.Alternate = GPIO_AF1_I2C1;
	  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	  /* I2C RX GPIO pin configuration  */
	  GPIO_InitStruct.Pin       = GPIO_PIN_7;
	  GPIO_InitStruct.Alternate = GPIO_AF1_I2C1;
	  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	  /*##-6- Configure the NVIC for I2C ########################################*/
	  /* NVIC for I2Cx */
	  HAL_NVIC_SetPriority(I2C1_IRQn, 0, 1);
	  HAL_NVIC_EnableIRQ(I2C1_IRQn);
  }
}

void HAL_SMBUS_MspDeInit(SMBUS_HandleTypeDef *hsmbus)
{


	__HAL_RCC_I2C1_FORCE_RESET();
	__HAL_RCC_I2C1_RELEASE_RESET();

  /* Peripheral clock disable */
  __HAL_RCC_I2C1_CLK_DISABLE();

  /**I2C1 GPIO Configuration
  PB6     ------> I2C1_SCL
  PB7     ------> I2C1_SDA
  */
  HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);

  /*##-3- Disable the NVIC for I2C ##########################################*/
  HAL_NVIC_DisableIRQ(I2C1_IRQn);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef* hi2c)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(hi2c->Instance==I2C1)
  {
	  static DMA_HandleTypeDef hdma_tx;
	  static DMA_HandleTypeDef hdma_rx;
	  RCC_PeriphCLKInitTypeDef  RCC_PeriphCLKInitStruct;

	  /*##-1- Configure the I2C clock source. The clock is derived from the SYSCLK #*/
	  RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
	  RCC_PeriphCLKInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_SYSCLK;
	  HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);

	  /*##-2- Enable peripherals and GPIO Clocks #################################*/
	  /* Enable GPIO TX/RX clock */
	  __HAL_RCC_GPIOB_CLK_ENABLE();
	  /* Enable I2Cx clock */
	  __HAL_RCC_I2C1_CLK_ENABLE();

	  /* Enable DMAx clock */
	  __HAL_RCC_DMA1_CLK_ENABLE();

	  /*##-3- Configure peripheral GPIO ##########################################*/
	  /* I2C TX GPIO pin configuration  */
	  GPIO_InitStruct.Pin       = GPIO_PIN_6;
	  GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
	  GPIO_InitStruct.Pull      = GPIO_NOPULL;//GPIO_PULLUP;
	  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
	  GPIO_InitStruct.Alternate = GPIO_AF1_I2C1;
	  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	  /* I2C RX GPIO pin configuration  */
	  GPIO_InitStruct.Pin       = GPIO_PIN_7;
	  GPIO_InitStruct.Alternate = GPIO_AF1_I2C1;
	  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	  /*##-4- Configure the DMA Channels #########################################*/
	  /* Configure the DMA handler for Transmission process */
	  /*hdma_tx.Instance                 = DMA1_Channel2;
	  hdma_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
	  hdma_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
	  hdma_tx.Init.MemInc              = DMA_MINC_ENABLE;
	  hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	  hdma_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
	  hdma_tx.Init.Mode                = DMA_NORMAL;
	  hdma_tx.Init.Priority            = DMA_PRIORITY_LOW;*/

	  //HAL_DMA_Init(&hdma_tx);

	  /* Associate the initialized DMA handle to the the I2C handle */
	  //__HAL_LINKDMA(hi2c, hdmatx, hdma_tx);

	  /* Configure the DMA handler for Transmission process */
	  /*hdma_rx.Instance                 = DMA1_Channel3;
	  hdma_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
	  hdma_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
	  hdma_rx.Init.MemInc              = DMA_MINC_ENABLE;
	  hdma_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	  hdma_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
	  hdma_rx.Init.Mode                = DMA_NORMAL;
	  hdma_rx.Init.Priority            = DMA_PRIORITY_HIGH;*/

	  //HAL_DMA_Init(&hdma_rx);

	  /* Associate the initialized DMA handle to the the I2C handle */
	  //__HAL_LINKDMA(hi2c, hdmarx, hdma_rx);

	  /*##-5- Configure the NVIC for DMA #########################################*/
	  /* NVIC configuration for DMA transfer complete interrupt (I2Cx_TX) */
	  //HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 1);
	  //HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

	  /* NVIC configuration for DMA transfer complete interrupt (I2Cx_RX) */
	  //HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, 0, 0);
	  //HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

	  /*##-6- Configure the NVIC for I2C ########################################*/
	  /* NVIC for I2Cx */
	  HAL_NVIC_SetPriority(I2C1_IRQn, 0, 1);
	  HAL_NVIC_EnableIRQ(I2C1_IRQn);
  }
  else if(hi2c->Instance==I2C2)
  {
	  //RCC_PeriphCLKInitTypeDef  RCC_PeriphCLKInitStruct2;

	  /*##-1- Configure the I2C clock source. The clock is derived from the SYSCLK #*/
	  /*RCC_PeriphCLKInitStruct2.PeriphClockSelection = RCC_PERIPHCLK_I2C2;
	  RCC_PeriphCLKInitStruct2.I2c2ClockSelection = RCC_I2C2CLKSOURCE_SYSCLK;
	  HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct2);*/

	  /*##-2- Enable peripherals and GPIO Clocks #################################*/
	  /* Enable GPIO TX/RX clock */
	  __HAL_RCC_GPIOB_CLK_ENABLE();
	  /* Enable I2Cx clock */
	  __HAL_RCC_I2C2_CLK_ENABLE();

	  /* Enable DMAx clock */
	  __HAL_RCC_DMA1_CLK_ENABLE();

	  /*##-3- Configure peripheral GPIO ##########################################*/
	  /* I2C TX GPIO pin configuration  */
	  GPIO_InitStruct.Pin       = GPIO_PIN_10;
	  GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
	  GPIO_InitStruct.Pull      = GPIO_PULLUP;
	  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
	  GPIO_InitStruct.Alternate = GPIO_AF1_I2C2;
	  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	  /* I2C RX GPIO pin configuration  */
	  GPIO_InitStruct.Pin       = GPIO_PIN_11;
	  GPIO_InitStruct.Alternate = GPIO_AF1_I2C2;
	  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	  /*##-4- Configure the DMA Channels #########################################*/
	  /* Configure the DMA handler for Transmission process */
	  hdma_tx2.Instance                 = DMA1_Channel4;
	  hdma_tx2.Init.Direction           = DMA_MEMORY_TO_PERIPH;
	  hdma_tx2.Init.PeriphInc           = DMA_PINC_DISABLE;
	  hdma_tx2.Init.MemInc              = DMA_MINC_ENABLE;
	  hdma_tx2.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	  hdma_tx2.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
	  hdma_tx2.Init.Mode                = DMA_NORMAL;
	  hdma_tx2.Init.Priority            = DMA_PRIORITY_LOW;

	  HAL_DMA_Init(&hdma_tx2);

	  /* Associate the initialized DMA handle to the the I2C handle */
	  __HAL_LINKDMA(hi2c, hdmatx, hdma_tx2);

	  /* Configure the DMA handler for Transmission process */
	  hdma_rx2.Instance                 = DMA1_Channel5;
	  hdma_rx2.Init.Direction           = DMA_PERIPH_TO_MEMORY;
	  hdma_rx2.Init.PeriphInc           = DMA_PINC_DISABLE;
	  hdma_rx2.Init.MemInc              = DMA_MINC_ENABLE;
	  hdma_rx2.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	  hdma_rx2.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
	  hdma_rx2.Init.Mode                = DMA_NORMAL;
	  hdma_rx2.Init.Priority            = DMA_PRIORITY_HIGH;

	  HAL_DMA_Init(&hdma_rx2);

	  /* Associate the initialized DMA handle to the the I2C handle */
	  __HAL_LINKDMA(hi2c, hdmarx, hdma_rx2);

	  /*##-5- Configure the NVIC for DMA #########################################*/
	  /* NVIC configuration for DMA transfer complete interrupt (I2Cx_TX) */
	  HAL_NVIC_SetPriority(DMA1_Channel4_5_IRQn, 0, 1);
	  HAL_NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);

	  /* NVIC configuration for DMA transfer complete interrupt (I2Cx_RX) */
	  HAL_NVIC_SetPriority(DMA1_Channel4_5_IRQn, 0, 0);
	  HAL_NVIC_EnableIRQ(DMA1_Channel4_5_IRQn);

	  /*##-6- Configure the NVIC for I2C ########################################*/
	  /* NVIC for I2Cx */
	  HAL_NVIC_SetPriority(I2C2_IRQn, 0, 1);
	  HAL_NVIC_EnableIRQ(I2C2_IRQn);
  }

}

void HAL_I2C_MspDeInit(I2C_HandleTypeDef* hi2c)
{

  if(hi2c->Instance==I2C1)
  {
  /* USER CODE BEGIN I2C1_MspDeInit 0 */
	/*##-1- Reset peripherals ##################################################*/
	__HAL_RCC_I2C1_FORCE_RESET();
	__HAL_RCC_I2C1_RELEASE_RESET();
	//HAL_DMA_DeInit(&hdma_tx2);
	//HAL_DMA_DeInit(&hdma_rx2);
	HAL_NVIC_DisableIRQ(DMA1_Channel2_3_IRQn);
	HAL_NVIC_DisableIRQ(I2C1_IRQn);
  /* USER CODE END I2C1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C1_CLK_DISABLE();
  
    /**I2C1 GPIO Configuration    
    PB6     ------> I2C1_SCL
    PB7     ------> I2C1_SDA 
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6|GPIO_PIN_7);

  /* USER CODE BEGIN I2C1_MspDeInit 1 */

  /* USER CODE END I2C1_MspDeInit 1 */
  }
  else if(hi2c->Instance==I2C2)
  {
  /* USER CODE BEGIN I2C2_MspDeInit 0 */
	/*##-1- Reset peripherals ##################################################*/
	__HAL_RCC_I2C2_FORCE_RESET();
	__HAL_RCC_I2C2_RELEASE_RESET();
	//HAL_DMA_DeInit(&hdma_tx2);
	//HAL_DMA_DeInit(&hdma_rx2);
	HAL_NVIC_DisableIRQ(DMA1_Channel4_5_IRQn);
	HAL_NVIC_DisableIRQ(I2C2_IRQn);
  /* USER CODE END I2C2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_I2C2_CLK_DISABLE();
  
    /**I2C2 GPIO Configuration    
    PB10     ------> I2C2_SCL
    PB11     ------> I2C2_SDA 
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10|GPIO_PIN_11);

  /* USER CODE BEGIN I2C2_MspDeInit 1 */

  /* USER CODE END I2C2_MspDeInit 1 */
  }

}

void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
  RCC_OscInitTypeDef        RCC_OscInitStruct;
  RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

  /*##-1- Enables the PWR Clock and Enables access to the backup domain ###################################*/
  /* To change the source clock of the RTC feature (LSE, LSI), You have to:
     - Enable the power clock using __HAL_RCC_PWR_CLK_ENABLE()
     - Enable write access using HAL_PWR_EnableBkUpAccess() function before to
       configure the RTC clock source (to be done once after reset).
     - Reset the Back up Domain using __HAL_RCC_BACKUPRESET_FORCE() and
       __HAL_RCC_BACKUPRESET_RELEASE().
     - Configure the needed RTC clock source */
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();

  /*##-2- Configue LSE/LSI as RTC clock soucre ###############################*/
#ifdef RTC_CLOCK_SOURCE_LSE
  RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
#elif defined (RTC_CLOCK_SOURCE_LSI)
  RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_OFF;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
#else
#error Please select the RTC Clock source inside the main.h file
#endif /*RTC_CLOCK_SOURCE_LSE*/

  /*##-2- Enable RTC peripheral Clocks #######################################*/
  /* Enable RTC Clock */
  __HAL_RCC_RTC_ENABLE();

  /*##-4- Configure the NVIC for RTC Alarm ###################################*/
  HAL_NVIC_SetPriority(RTC_IRQn, 0x0F, 0);
  HAL_NVIC_EnableIRQ(RTC_IRQn);
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef* hrtc)
{

  if(hrtc->Instance==RTC)
  {
  /* USER CODE BEGIN RTC_MspDeInit 0 */

  /* USER CODE END RTC_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_RTC_DISABLE();
  }
  /* USER CODE BEGIN RTC_MspDeInit 1 */

  /* USER CODE END RTC_MspDeInit 1 */

}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef* htim_pwm)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(htim_pwm->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspInit 0 */

  /* USER CODE END TIM3_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_TIM3_CLK_ENABLE();
  
    /**TIM3 GPIO Configuration    
    PB4     ------> TIM3_CH1
    PB5     ------> TIM3_CH2 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM3_MspInit 1 */

  /* USER CODE END TIM3_MspInit 1 */
  }
  else if(htim_pwm->Instance==TIM15)
  {
  /* USER CODE BEGIN TIM15_MspInit 0 */

  /* USER CODE END TIM15_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_TIM15_CLK_ENABLE();
  /* USER CODE BEGIN TIM15_MspInit 1 */

  /* USER CODE END TIM15_MspInit 1 */
  }

}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{

  if(htim_base->Instance==TIM1)
  {
  /* USER CODE BEGIN TIM1_MspInit 0 */

  /* USER CODE END TIM1_MspInit 0 */
	/* Peripheral clock enable */
	__HAL_RCC_TIM1_CLK_ENABLE();
  /* USER CODE BEGIN TIM1_MspInit 1 */

  /* USER CODE END TIM1_MspInit 1 */
  }
  else if(htim_base->Instance==TIM17)
  {
  /* USER CODE BEGIN TIM17_MspInit 0 */

  /* USER CODE END TIM17_MspInit 0 */
    /* Peripheral clock enable */
    __HAL_RCC_TIM17_CLK_ENABLE();
  /* USER CODE BEGIN TIM17_MspInit 1 */

  /* USER CODE END TIM17_MspInit 1 */
  }
  else if(htim_base->Instance==TIM14)
  {
	  __HAL_RCC_TIM14_CLK_ENABLE();
  }

}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(htim->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspPostInit 0 */

  /* USER CODE END TIM3_MspPostInit 0 */
    /**TIM3 GPIO Configuration    
    PB0     ------> TIM3_CH3 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM3_MspPostInit 1 */

  /* USER CODE END TIM3_MspPostInit 1 */
  }
  else if(htim->Instance==TIM15)
  {
  /* USER CODE BEGIN TIM15_MspPostInit 0 */

  /* USER CODE END TIM15_MspPostInit 0 */
  
    /**TIM15 GPIO Configuration    
    PB14     ------> TIM15_CH1
    PB15     ------> TIM15_CH2 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM15;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM15_MspPostInit 1 */

  /* USER CODE END TIM15_MspPostInit 1 */
  }
  else if(htim->Instance==TIM17)
  {
  /* USER CODE BEGIN TIM17_MspPostInit 0 */

  /* USER CODE END TIM17_MspPostInit 0 */
  
    /**TIM17 GPIO Configuration    
    PB9     ------> TIM17_CH1 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM17;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM17_MspPostInit 1 */

  /* USER CODE END TIM17_MspPostInit 1 */
  } else   if(htim->Instance==TIM1)
  {
  /* USER CODE BEGIN TIM1_MspPostInit 0 */

  /* USER CODE END TIM1_MspPostInit 0 */
    /**TIM1 GPIO Configuration
    PA7     ------> TIM1_CH1N
    PA8     ------> TIM1_CH1
    */
    GPIO_InitStruct.Pin = /*GPIO_PIN_7|*/GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM1_MspPostInit 1 */

  /* USER CODE END TIM1_MspPostInit 1 */
  }
  else if(htim->Instance==TIM14)
  {
  /* USER CODE BEGIN TIM14_MspPostInit 0 */

  /* USER CODE END TIM14_MspPostInit 0 */

    /**TIM14 GPIO Configuration
    PA4     ------> TIM14_CH1
    */
    GPIO_InitStruct.Pin = GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_TIM14;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN TIM14_MspPostInit 1 */

  /* USER CODE END TIM14_MspPostInit 1 */
  }

}

void HAL_TIM_PWM_MspDeInit(TIM_HandleTypeDef* htim_pwm)
{

  if(htim_pwm->Instance==TIM3)
  {
  /* USER CODE BEGIN TIM3_MspDeInit 0 */

  /* USER CODE END TIM3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM3_CLK_DISABLE();
  
    /**TIM3 GPIO Configuration    
    PB0     ------> TIM3_CH3
    PB4     ------> TIM3_CH1
    PB5     ------> TIM3_CH2 
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0|GPIO_PIN_4|GPIO_PIN_5);

  /* USER CODE BEGIN TIM3_MspDeInit 1 */

  /* USER CODE END TIM3_MspDeInit 1 */
  }
  else if(htim_pwm->Instance==TIM15)
  {
  /* USER CODE BEGIN TIM15_MspDeInit 0 */

  /* USER CODE END TIM15_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM15_CLK_DISABLE();
  /* USER CODE BEGIN TIM15_MspDeInit 1 */

  /* USER CODE END TIM15_MspDeInit 1 */
  }

}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
{

  if(htim_base->Instance==TIM17)
  {
  /* USER CODE BEGIN TIM17_MspDeInit 0 */

  /* USER CODE END TIM17_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM17_CLK_DISABLE();
  }
  else if(htim_base->Instance==TIM1)
  {
  /* USER CODE BEGIN TIM1_MspDeInit 0 */

  /* USER CODE END TIM1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM1_CLK_DISABLE();
  /* USER CODE BEGIN TIM1_MspDeInit 1 */

  /* USER CODE END TIM1_MspDeInit 1 */
  }
  else if(htim_base->Instance==TIM14)
  {
  /* USER CODE BEGIN TIM14_MspDeInit 0 */

  /* USER CODE END TIM14_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_TIM14_CLK_DISABLE();
  /* USER CODE BEGIN TIM14_MspDeInit 1 */

  /* USER CODE END TIM14_MspDeInit 1 */
  }
  /* USER CODE BEGIN TIM17_MspDeInit 1 */

  /* USER CODE END TIM17_MspDeInit 1 */

}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
