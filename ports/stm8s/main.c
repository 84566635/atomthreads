/*
 * Copyright (c) 2010, Kelvin Lawson. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. No personal names or organizations' names associated with the
 *    Atomthreads project may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ATOMTHREADS PROJECT AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 2013 Wei Shuai <cpuwolf@gmail.com>
 *     Modify to adapt STM8S
 *
 * Code is designed for
 *     STM8S mini system board
 *     get one from http://cpuwolf.taobao.com
 *
 * STM8s mini system board:
 * PB0: on-board LED
 *
 */

#include <stdio.h>
#include <atom.h>
#include <atomtimer.h>
#include <stm8s.h>
#include "uart.h"
#include "atomport-private.h"


/* Constants */

/*
 * Idle thread stack size
 *
 * This needs to be large enough to handle any interrupt handlers
 * and callbacks called by interrupt handlers (e.g. user-created
 * timer callbacks) as well as the saving of all context when
 * switching away from this thread.
 *
 * In this case, the idle stack is allocated on the BSS via the
 * idle_thread_stack[] byte array.
 */
#define IDLE_STACK_SIZE_BYTES       128


/*
 * Main thread stack size
 *
 * Note that this is not a required OS kernel thread - you will replace
 * this with your own application thread.
 *
 * In this case the Main thread is responsible for calling out to the
 * test routines. Once a test routine has finished, the test status is
 * printed out on the UART and the thread remains running in a loop
 * flashing a LED.
 *
 * The Main thread stack generally needs to be larger than the idle
 * thread stack, as not only does it need to store interrupt handler
 * stack saves and context switch saves, but the application main thread
 * will generally be carrying out more nested function calls and require
 * stack for application code local variables etc.
 *
 * With all OS tests implemented to date on the STM8, the Main thread
 * stack has not exceeded 256 bytes. To allow all tests to run we set
 * a minimum main thread stack size of 204 bytes. This may increase in
 * future as the codebase changes but for the time being is enough to
 * cope with all of the automated tests.
 */
#define MAIN_STACK_SIZE_BYTES       256


/*
 * Startup code stack
 *
 * Some stack space is required at initial startup for running the main()
 * routine. This stack space is only temporarily required at first bootup
 * and is no longer required as soon as the OS is started. By default
 * Cosmic sets this to the top of RAM and it grows down from there.
 *
 * Because we only need this temporarily you may reuse the area once the
 * OS is started, and are free to use some area other than the top of RAM.
 * For convenience we just use the default region here.
 */


/* Linker-provided startup stack location (usually top of RAM) */
extern int _stack;


/* Local data */

/* Application threads' TCBs */
static ATOM_TCB main_tcb;

/* Main thread's stack area (large so place outside of the small page0 area on STM8) */
NEAR static uint8_t main_thread_stack[MAIN_STACK_SIZE_BYTES];

/* Idle thread's stack area (large so place outside of the small page0 area on STM8) */
NEAR static uint8_t idle_thread_stack[IDLE_STACK_SIZE_BYTES];


/* Forward declarations */
static void main_thread_func (uint32_t param);

static void GPIO_Config(void);
static uint16_t ADC_Read(void);


/**
 * \b main
 *
 * Program entry point.
 *
 * Sets up the STM8 hardware resources (system tick timer interrupt) necessary
 * for the OS to be started. Creates an application thread and starts the OS.
 *
 * If the compiler supports it, stack space can be saved by preventing
 * the function from saving registers on entry. This is because we
 * are called directly by the C startup assembler, and know that we will
 * never return from here. The NO_REG_SAVE macro is used to denote such
 * functions in a compiler-agnostic way, though not all compilers support it.
 *
 */
NO_REG_SAVE void main ( void )
{
    int8_t status;

    /* GPIO configuration */
    GPIO_Config();


    /**
     * Note: to protect OS structures and data during initialisation,
     * interrupts must remain disabled until the first thread
     * has been restored. They are reenabled at the very end of
     * the first thread restore, at which point it is safe for a
     * reschedule to take place.
     */

    /* Initialise the OS before creating our threads */
    status = atomOSInit(&idle_thread_stack[IDLE_STACK_SIZE_BYTES - 1], IDLE_STACK_SIZE_BYTES);
    if (status == ATOM_OK)
    {
        /* Enable the system tick timer */
        archInitSystemTickTimer();

        /* Create an application thread */
        status = atomThreadCreate(&main_tcb,
                                  16, main_thread_func, 0,
                                  &main_thread_stack[MAIN_STACK_SIZE_BYTES - 1],
                                  MAIN_STACK_SIZE_BYTES);
        if (status == ATOM_OK)
        {
            /**
             * First application thread successfully created. It is
             * now possible to start the OS. Execution will not return
             * from atomOSStart(), which will restore the context of
             * our application thread and start executing it.
             *
             * Note that interrupts are still disabled at this point.
             * They will be enabled as we restore and execute our first
             * thread in archFirstThreadRestore().
             */
            atomOSStart();
        }
    }

    /* There was an error starting the OS if we reach here */
    while (1)
    {
    }

}
/**
  * @brief  Configure TIM1 to generate 7 PWM signals with 4 different duty cycles
  * @param  None
  * @retval None
  */
static void TIM1_Config(void)
{
    uint16_t ICValue;
    uint16_t rpm;
    uint16_t Conversion_Value;

    TIM1_DeInit();

    /* Time Base configuration 10KHz=100us*/
    /* gernal speed of PC fan
       1000rpm=16.66Hz=60ms=60000us*/
    /*
    TIM1_Period = 4095
    TIM1_Prescaler = 1
    TIM1_CounterMode = TIM1_COUNTERMODE_UP
    TIM1_RepetitionCounter = 0
    */

    TIM1_TimeBaseInit(200-1, TIM1_COUNTERMODE_UP, 8195, 0);

    /* Channel 1 in PWM mode */

    /*
    TIM1_OCMode = TIM1_OCMODE_PWM2
    TIM1_OutputState = TIM1_OUTPUTSTATE_ENABLE
    TIM1_OutputNState = TIM1_OUTPUTNSTATE_ENABLE
    TIM1_Pulse = CCR1_Val
    TIM1_OCPolarity = TIM1_OCPOLARITY_LOW
    TIM1_OCNPolarity = TIM1_OCNPOLARITY_HIGH
    TIM1_OCIdleState = TIM1_OCIDLESTATE_SET
    TIM1_OCNIdleState = TIM1_OCIDLESTATE_RESET

    */
    TIM1_OC1Init(TIM1_OCMODE_PWM2, TIM1_OUTPUTSTATE_ENABLE, TIM1_OUTPUTNSTATE_ENABLE,
                 4000, TIM1_OCPOLARITY_LOW, TIM1_OCNPOLARITY_HIGH, TIM1_OCIDLESTATE_SET,
                 TIM1_OCNIDLESTATE_RESET);

    /* Channel 2 in Capure/Compare mode */
    TIM1_ICInit( TIM1_CHANNEL_2, TIM1_ICPOLARITY_RISING, TIM1_ICSELECTION_DIRECTTI,
                 TIM1_ICPSC_DIV1, 0x0);

    TIM1_SelectInputTrigger(TIM1_TS_TI2FP2);
    TIM1_SelectSlaveMode(TIM1_SLAVEMODE_RESET);



    /* TIM1 counter enable */
    TIM1_Cmd(ENABLE);
    /* TIM1 Main Output Enable */
    TIM1_CtrlPWMOutputs(ENABLE);


    /* Clear CC2 Flag*/
    TIM1_ClearFlag(TIM1_FLAG_CC2);

read_speed:
    /* wait a capture on CC2 */
    while((TIM1->SR1 & TIM1_FLAG_CC2) != TIM1_FLAG_CC2);
    /* Get CCR2 value*/
    ICValue = TIM1_GetCapture2();
    TIM1_ClearFlag(TIM1_FLAG_CC2);

    Conversion_Value = ADC_Read();

    if(ICValue > 65535) {
        printf("Invalid value\n");
    } else {
        rpm = 10000/ICValue;
        rpm *= 60;
        printf("[%d]rpm %d ADC 0x%x\n", ICValue, rpm, Conversion_Value);
    }

    goto read_speed;


}

static void ADC_Config(void)
{
    /*  Init GPIO for ADC1 */
    GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_IN_FL_NO_IT);

    /* De-Init ADC peripheral*/
    ADC1_DeInit();

    /* Init ADC2 peripheral */
    ADC1_Init(ADC1_CONVERSIONMODE_SINGLE, ADC1_CHANNEL_0, ADC1_PRESSEL_FCPU_D18, \
              ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_LEFT, ADC1_SCHMITTTRIG_CHANNEL0,\
              DISABLE);

    /* Disable EOC interrupt */
    ADC1_ITConfig(ADC1_IT_EOCIE, DISABLE);
}

static uint16_t ADC_Read(void)
{
    uint16_t Conversion_Value;
    ADC1_StartConversion();
    while(ADC1_GetFlagStatus(ADC1_FLAG_EOC)==RESET) {
        atomTimerDelay(1);
    };
    Conversion_Value = ADC1_GetConversionValue();
    //ADC1_ITConfig(ADC1_IT_EOC, DISABLE);
    return Conversion_Value;
}
/**
  * \b  Configure GPIOs
  * @param  None
  * @retval None
  */
static void GPIO_Config(void)
{
    /* Configure GPIO for flashing STM8S mini system board GPIO E5 */
    GPIO_DeInit(GPIOE);
    GPIO_Init(GPIOE, GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_FAST);
}

/**
 * \b main_thread_func
 *
 * Entry point for main application thread.
 *
 * This is the first thread that will be executed when the OS is started.
 *
 * @param[in] param Unused (optional thread entry parameter)
 *
 * @return None
 */
static void main_thread_func (uint32_t param)
{
    uint32_t test_status;

    ADC_Config();

    /* Compiler warnings */
    param = param;

    /* Initialise UART (115200bps) */
    if (uart_init(115200) != 0)
    {
        /* Error initialising UART */
    }

    /* Put a message out on the UART */
    printf("Go\n");

    /* Start test. All tests use the same start API. */
    test_status = 0;

    /* Check main thread stack usage (if enabled) */
#ifdef ATOM_STACK_CHECKING
    if (test_status == 0)
    {
        uint32_t used_bytes, free_bytes;

        /* Check idle thread stack usage */
        if (atomThreadStackCheck (&main_tcb, &used_bytes, &free_bytes) == ATOM_OK)
        {
            /* Check the thread did not use up to the end of stack */
            if (free_bytes == 0)
            {
                printf ("Main stack overflow\n");
                test_status++;
            }

            /* Log the stack usage */
#ifdef TESTS_LOG_STACK_USAGE
            printf ("MainUse:%d\n", (int)used_bytes);
#endif
        }

    }
#endif

    


    /* Test finished, flash slowly for pass, fast for fail */
    while (1)
    {
        /* Toggle LED on pin E5 (STM8S mini board-specific) */
        GPIO_WriteReverse(GPIOE, GPIO_PIN_5);

        /* Test printf function */
        printf("Temp ADC (0x%x)\n", ADC_Read());

        /* Sleep then toggle LED again */
        atomTimerDelay(SYSTEM_TICKS_PER_SEC);
    }
}

#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif