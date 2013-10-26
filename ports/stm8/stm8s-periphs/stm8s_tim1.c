/**
  ******************************************************************************
  * @file stm8s_tim1.c
  * @brief This file contains all the functions for the TIM1 peripheral.
  * @author STMicroelectronics - MCD Application Team
  * @version V1.1.1
  * @date 06/05/2009
  ******************************************************************************
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  * @image html logo.bmp
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm8s_tim1.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/

/**
  * @addtogroup TIM1_Public_Functions
  * @{
  */

/**
  * @brief Deinitializes the TIM1 peripheral registers to their default reset values.
  * @par Parameters:
  * None
  * @retval None
  */
void TIM1_DeInit(void)
{
    TIM1->CR1  = TIM1_CR1_RESET_VALUE;
    TIM1->CR2  = TIM1_CR2_RESET_VALUE;
    TIM1->SMCR = TIM1_SMCR_RESET_VALUE;
    TIM1->ETR  = TIM1_ETR_RESET_VALUE;
    TIM1->IER  = TIM1_IER_RESET_VALUE;
    TIM1->SR2  = TIM1_SR2_RESET_VALUE;
    /* Disable channels */
    TIM1->CCER1 = TIM1_CCER1_RESET_VALUE;
    TIM1->CCER2 = TIM1_CCER2_RESET_VALUE;
    /* Configure channels as inputs: it is necessary if lock level is equal to 2 or 3 */
    TIM1->CCMR1 = 0x01;
    TIM1->CCMR2 = 0x01;
    TIM1->CCMR3 = 0x01;
    TIM1->CCMR4 = 0x01;
    /* Then reset channel registers: it also works if lock level is equal to 2 or 3 */
    TIM1->CCER1 = TIM1_CCER1_RESET_VALUE;
    TIM1->CCER2 = TIM1_CCER2_RESET_VALUE;
    TIM1->CCMR1 = TIM1_CCMR1_RESET_VALUE;
    TIM1->CCMR2 = TIM1_CCMR2_RESET_VALUE;
    TIM1->CCMR3 = TIM1_CCMR3_RESET_VALUE;
    TIM1->CCMR4 = TIM1_CCMR4_RESET_VALUE;
    TIM1->CNTRH = TIM1_CNTRH_RESET_VALUE;
    TIM1->CNTRL = TIM1_CNTRL_RESET_VALUE;
    TIM1->PSCRH = TIM1_PSCRH_RESET_VALUE;
    TIM1->PSCRL = TIM1_PSCRL_RESET_VALUE;
    TIM1->ARRH  = TIM1_ARRH_RESET_VALUE;
    TIM1->ARRL  = TIM1_ARRL_RESET_VALUE;
    TIM1->CCR1H = TIM1_CCR1H_RESET_VALUE;
    TIM1->CCR1L = TIM1_CCR1L_RESET_VALUE;
    TIM1->CCR2H = TIM1_CCR2H_RESET_VALUE;
    TIM1->CCR2L = TIM1_CCR2L_RESET_VALUE;
    TIM1->CCR3H = TIM1_CCR3H_RESET_VALUE;
    TIM1->CCR3L = TIM1_CCR3L_RESET_VALUE;
    TIM1->CCR4H = TIM1_CCR4H_RESET_VALUE;
    TIM1->CCR4L = TIM1_CCR4L_RESET_VALUE;
    TIM1->OISR  = TIM1_OISR_RESET_VALUE;
    TIM1->EGR   = 0x01; /* TIM1_EGR_UG */
    TIM1->DTR   = TIM1_DTR_RESET_VALUE;
    TIM1->BKR   = TIM1_BKR_RESET_VALUE;
    TIM1->RCR   = TIM1_RCR_RESET_VALUE;
    TIM1->SR1   = TIM1_SR1_RESET_VALUE;
}

/**
  * @brief Initializes the TIM1 Time Base Unit according to the specified parameters.
  * @param[in]  TIM1_Prescaler specifies the Prescaler value.
  * @param[in]  TIM1_CounterMode specifies the counter mode  from @ref TIM1_CounterMode_TypeDef .
  * @param[in]  TIM1_Period specifies the Period value.
  * @param[in]  TIM1_RepetitionCounter specifies the Repetition counter value
  * @retval None
  */
void TIM1_TimeBaseInit(u16 TIM1_Prescaler,
                       TIM1_CounterMode_TypeDef TIM1_CounterMode,
                       u16 TIM1_Period,
                       u8 TIM1_RepetitionCounter)
{

    /* Check parameters */
    assert_param(IS_TIM1_COUNTER_MODE_OK(TIM1_CounterMode));

    /* Set the Autoreload value */
    TIM1->ARRH = (u8)(TIM1_Period >> 8);
    TIM1->ARRL = (u8)(TIM1_Period);

    /* Set the Prescaler value */
    TIM1->PSCRH = (u8)(TIM1_Prescaler >> 8);
    TIM1->PSCRL = (u8)(TIM1_Prescaler);

    /* Select the Counter Mode */
    TIM1->CR1 = (u8)(((TIM1->CR1) & (u8)(~(TIM1_CR1_CMS | TIM1_CR1_DIR))) | (u8)(TIM1_CounterMode));

    /* Set the Repetition Counter value */
    TIM1->RCR = TIM1_RepetitionCounter;

}

/**
  * @brief Initializes the TIM1 Channel2 according to the specified parameters.
  * @param[in] TIM1_OCMode specifies the Output Compare mode from @ref TIM1_OCMode_TypeDef.
  * @param[in] TIM1_OutputState specifies the Output State from @ref TIM1_OutputState_TypeDef.
  * @param[in] TIM1_OutputNState specifies the Complementary Output State from @ref TIM1_OutputNState_TypeDef.
  * @param[in] TIM1_Pulse specifies the Pulse width value.
  * @param[in] TIM1_OCPolarity specifies the Output Compare Polarity from @ref TIM1_OCPolarity_TypeDef.
  * @param[in] TIM1_OCNPolarity specifies the Complementary Output Compare Polarity from @ref TIM1_OCNPolarity_TypeDef.
  * @param[in] TIM1_OCIdleState specifies the Output Compare Idle State from @ref TIM1_OCIdleState_TypeDef.
  * @param[in] TIM1_OCNIdleState specifies the Complementary Output Compare Idle State from @ref TIM1_OCIdleState_TypeDef.
  * @retval None
  */
void TIM1_OC2Init(TIM1_OCMode_TypeDef TIM1_OCMode,
                  TIM1_OutputState_TypeDef TIM1_OutputState,
                  TIM1_OutputNState_TypeDef TIM1_OutputNState,
                  u16 TIM1_Pulse,
                  TIM1_OCPolarity_TypeDef TIM1_OCPolarity,
                  TIM1_OCNPolarity_TypeDef TIM1_OCNPolarity,
                  TIM1_OCIdleState_TypeDef TIM1_OCIdleState,
                  TIM1_OCNIdleState_TypeDef TIM1_OCNIdleState)
{


    /* Check the parameters */
    assert_param(IS_TIM1_OC_MODE_OK(TIM1_OCMode));
    assert_param(IS_TIM1_OUTPUT_STATE_OK(TIM1_OutputState));
    assert_param(IS_TIM1_OUTPUTN_STATE_OK(TIM1_OutputNState));
    assert_param(IS_TIM1_OC_POLARITY_OK(TIM1_OCPolarity));
    assert_param(IS_TIM1_OCN_POLARITY_OK(TIM1_OCNPolarity));
    assert_param(IS_TIM1_OCIDLE_STATE_OK(TIM1_OCIdleState));
    assert_param(IS_TIM1_OCNIDLE_STATE_OK(TIM1_OCNIdleState));

    /* Disable the Channel 1: Reset the CCE Bit, Set the Output State , the Output N State, the Output Polarity & the Output N Polarity*/
    TIM1->CCER1 &= (u8)(~( TIM1_CCER1_CC2E | TIM1_CCER1_CC2NE | TIM1_CCER1_CC2P | TIM1_CCER1_CC2NP));
    /* Set the Output State & Set the Output N State & Set the Output Polarity & Set the Output N Polarity */
    TIM1->CCER1 |= (u8)((TIM1_OutputState & TIM1_CCER1_CC2E  ) | (TIM1_OutputNState & TIM1_CCER1_CC2NE ) | (TIM1_OCPolarity  & TIM1_CCER1_CC2P  ) | (TIM1_OCNPolarity & TIM1_CCER1_CC2NP ));


    /* Reset the Output Compare Bits & Set the Ouput Compare Mode */
    TIM1->CCMR2 = (u8)((TIM1->CCMR2 & (u8)(~TIM1_CCMR_OCM)) | (u8)TIM1_OCMode);

    /* Reset the Output Idle state & the Output N Idle state bits */
    TIM1->OISR &= (u8)(~(TIM1_OISR_OIS2 | TIM1_OISR_OIS2N));
    /* Set the Output Idle state & the Output N Idle state configuration */
    TIM1->OISR |= (u8)((TIM1_OISR_OIS2 & TIM1_OCIdleState) | (TIM1_OISR_OIS2N & TIM1_OCNIdleState));

    /* Set the Pulse value */
    TIM1->CCR2H = (u8)(TIM1_Pulse >> 8);
    TIM1->CCR2L = (u8)(TIM1_Pulse);

}

/**
  * @brief Enables or disables the TIM1 peripheral.
  * @param[in] NewState new state of the TIM1 peripheral.
	* This parameter can be ENABLE or DISABLE.
  * @retval None
  */
void TIM1_Cmd(FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_FUNCTIONALSTATE_OK(NewState));

    /* set or Reset the CEN Bit */
    if (NewState != DISABLE)
    {
        TIM1->CR1 |= TIM1_CR1_CEN;
    }
    else
    {
        TIM1->CR1 &= (u8)(~TIM1_CR1_CEN);
    }
}


/**
  * @brief Enables or disables the specified TIM1 interrupts.
  * @param[in] NewState new state of the TIM1 peripheral.
  * This parameter can be: ENABLE or DISABLE.
  * @param[in] TIM1_IT specifies the TIM1 interrupts sources to be enabled or disabled.
  * This parameter can be any combination of the following values:
  *  - TIM1_IT_UPDATE: TIM1 update Interrupt source
  *  - TIM1_IT_CC1: TIM1 Capture Compare 1 Interrupt source
  *  - TIM1_IT_CC2: TIM1 Capture Compare 2 Interrupt source
  *  - TIM1_IT_CC3: TIM1 Capture Compare 3 Interrupt source
  *  - TIM1_IT_CC4: TIM1 Capture Compare 4 Interrupt source
  *  - TIM1_IT_CCUpdate: TIM1 Capture Compare Update Interrupt source
  *  - TIM1_IT_TRIGGER: TIM1 Trigger Interrupt source
  *  - TIM1_IT_BREAK: TIM1 Break Interrupt source
  * @param[in] NewState new state of the TIM1 peripheral.
  * @retval None
  */
void TIM1_ITConfig(TIM1_IT_TypeDef  TIM1_IT, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_TIM1_IT_OK(TIM1_IT));
    assert_param(IS_FUNCTIONALSTATE_OK(NewState));

    if (NewState != DISABLE)
    {
        /* Enable the Interrupt sources */
        TIM1->IER |= (u8)TIM1_IT;
    }
    else
    {
        /* Disable the Interrupt sources */
        TIM1->IER &= (u8)(~(u8)TIM1_IT);
    }
}


/**
/**
  * @brief Sets the TIM1 Capture Compare2 Register value.
  * @param[in] Compare2 specifies the Capture Compare2 register new value.
  * This parameter is between 0x0000 and 0xFFFF.
  * @retval None
  */
void TIM1_SetCompare2(u16 Compare2)
{
    /* Set the Capture Compare2 Register value */
    TIM1->CCR2H = (u8)(Compare2 >> 8);
    TIM1->CCR2L = (u8)(Compare2);

}


/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
