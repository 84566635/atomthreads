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


#include <stdio.h>

#include "atom.h"
#include "atomsem.h"
#include "atomport-private.h"
#include "atomport-tests.h"
#include "atomtests.h"
#include "atomtimer.h"
#include "uart.h"
#include "stm8s.h"
#include <stm8s_adc1.h>


/* Constants */
/* gfedcba
 * 0,1,2,3,4,5,6,7,8,9,a,b,c,d,e,f
 */
static const uint8_t dig_hextable[]= {
    0x3f,0x06,0x5b,0x4f,
    0x66,0x6d,0x7d,0x07,
    0x7f,0x6f,0x77,0x7c,
    0x39,0x5e,0x79,0x71
};
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
static ATOM_SEM sem_light;

/* Application threads' TCBs */
static ATOM_TCB main_tcb;

/* Main thread's stack area (large so place outside of the small page0 area on STM8) */
NEAR static uint8_t main_thread_stack[MAIN_STACK_SIZE_BYTES];

/* Idle thread's stack area (large so place outside of the small page0 area on STM8) */
NEAR static uint8_t idle_thread_stack[IDLE_STACK_SIZE_BYTES];


/* Forward declarations */
static void main_thread_func (uint32_t param);

void HardwareInit( void )
{
    /* disable unused clock */
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_I2C, DISABLE);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_SPI, DISABLE);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER2, DISABLE);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4, DISABLE);
    CLK_PeripheralClockConfig(CLK_PERIPHERAL_TIMER4, DISABLE);


    GPIO_Init(GPIOD, GPIO_PIN_6, GPIO_MODE_IN_FL_NO_IT);

    /* onboard orange led(disabled by default) */
    GPIO_DeInit(GPIOE);
    //GPIO_Init(GPIOE, GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_FAST);

    //blue led
    GPIO_Init(GPIOC, GPIO_PIN_2, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteHigh(GPIOC, GPIO_PIN_2);


    //Interrupt human sensor
    GPIO_ExternalIntSensitivity(GPIOB, GPIO_SENS_RISE);
    GPIO_Init(GPIOB, GPIO_PIN_3, GPIO_MODE_IN_FL_IT);

    //digital leds
    GPIO_Init(GPIOA, GPIO_PIN_1, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteLow(GPIOA, GPIO_PIN_1);
    GPIO_Init(GPIOA, GPIO_PIN_2, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteLow(GPIOA, GPIO_PIN_2);
    GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteLow(GPIOB, GPIO_PIN_4);
    GPIO_Init(GPIOB, GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteLow(GPIOB, GPIO_PIN_5);
    //comma
    GPIO_Init(GPIOF, GPIO_PIN_4, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteLow(GPIOF, GPIO_PIN_4);

    GPIO_Init(GPIOC, GPIO_PIN_1, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteHigh(GPIOC, GPIO_PIN_1);
    GPIO_Init(GPIOC, GPIO_PIN_3, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteHigh(GPIOC, GPIO_PIN_3);
    GPIO_Init(GPIOC, GPIO_PIN_4, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteHigh(GPIOC, GPIO_PIN_4);
    GPIO_Init(GPIOC, GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteHigh(GPIOC, GPIO_PIN_5);
    GPIO_Init(GPIOC, GPIO_PIN_6, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteHigh(GPIOC, GPIO_PIN_6);
    GPIO_Init(GPIOC, GPIO_PIN_7, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteHigh(GPIOC, GPIO_PIN_7);

    GPIO_Init(GPIOD, GPIO_PIN_0, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteHigh(GPIOD, GPIO_PIN_0);
    GPIO_Init(GPIOD, GPIO_PIN_1, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteHigh(GPIOD, GPIO_PIN_1);
    GPIO_Init(GPIOD, GPIO_PIN_2, GPIO_MODE_OUT_PP_LOW_SLOW);
    GPIO_WriteHigh(GPIOD, GPIO_PIN_2);

}

static void show_comma(uint8_t show)
{
    //pin 8
    if(show) {
        GPIO_WriteLow(GPIOF, GPIO_PIN_4);
        GPIO_WriteLow(GPIOD, GPIO_PIN_2);
    } else {
        GPIO_WriteHigh(GPIOF, GPIO_PIN_4);
        GPIO_WriteHigh(GPIOD, GPIO_PIN_2);
    }
}
static void show_dig_bits(uint8_t showbits)
{
    //a
    if(showbits&1) {
        GPIO_WriteLow(GPIOC, GPIO_PIN_7);
    } else {
        GPIO_WriteHigh(GPIOC, GPIO_PIN_7);
    }
    //b
    if(showbits&2) {
        GPIO_WriteLow(GPIOD, GPIO_PIN_1);
    } else {
        GPIO_WriteHigh(GPIOD, GPIO_PIN_1);
    }
    //c
    if(showbits&4) {
        GPIO_WriteLow(GPIOC, GPIO_PIN_5);
    } else {
        GPIO_WriteHigh(GPIOC, GPIO_PIN_5);
    }
    //d
    if(showbits&8) {
        GPIO_WriteLow(GPIOC, GPIO_PIN_3);
    } else {
        GPIO_WriteHigh(GPIOC, GPIO_PIN_3);
    }
    //e
    if(showbits&0x10) {
        GPIO_WriteLow(GPIOC, GPIO_PIN_1);
    } else {
        GPIO_WriteHigh(GPIOC, GPIO_PIN_1);
    }
    //f
    if(showbits&0x20) {
        GPIO_WriteLow(GPIOD, GPIO_PIN_0);
    } else {
        GPIO_WriteHigh(GPIOD, GPIO_PIN_0);
    }
    //g
    if(showbits&0x40) {
        GPIO_WriteLow(GPIOC, GPIO_PIN_6);
    } else {
        GPIO_WriteHigh(GPIOC, GPIO_PIN_6);
    }
    //dp
    if(showbits&0x80) {
        GPIO_WriteLow(GPIOC, GPIO_PIN_4);
    } else {
        GPIO_WriteHigh(GPIOC, GPIO_PIN_4);
    }
}

static void show_dig(uint8_t dig, uint8_t dot )
{
    if(dig < (sizeof(dig_hextable)/sizeof(dig_hextable[0]))) {
        if(dot) {
            show_dig_bits(dig_hextable[dig]|0x80);
        } else {
            show_dig_bits(dig_hextable[dig]);
        }
    }
}
static void clear_dig()
{
    show_dig_bits(0);
}

static void select_clear( void )
{
    //clear all
    GPIO_WriteHigh(GPIOA, GPIO_PIN_1);
    GPIO_WriteHigh(GPIOA, GPIO_PIN_2);
    GPIO_WriteHigh(GPIOB, GPIO_PIN_4);
    GPIO_WriteHigh(GPIOB, GPIO_PIN_5);
}
static void select_dig(uint8_t way)
{
    switch(way) {
    case 0:
        GPIO_WriteLow(GPIOA, GPIO_PIN_2);
        break;
    case 1:
        GPIO_WriteLow(GPIOB, GPIO_PIN_5);
        break;
    case 2:
        GPIO_WriteLow(GPIOB, GPIO_PIN_4);
        break;
    case 3:
        GPIO_WriteLow(GPIOA, GPIO_PIN_1);
        break;



    }
}
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

    __disable_interrupt();

    HardwareInit();

    __enable_interrupt();

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
                                  TEST_THREAD_PRIO, main_thread_func, 0,
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


static void ADC_Config()
{
    /*  Init GPIO for ADC1 */
    GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_IN_FL_NO_IT);


    /* De-Init ADC peripheral*/
    ADC1_DeInit();

    /* Init ADC2 peripheral */
    ADC1_Init(ADC1_CONVERSIONMODE_SINGLE, ADC1_CHANNEL_0, ADC1_PRESSEL_FCPU_D8, \
              ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_LEFT, ADC1_SCHMITTTRIG_CHANNEL0,\
              DISABLE);

    /* Disable EOC interrupt */
    ADC1_ITConfig(ADC1_IT_EOCIE, DISABLE);


}

void EXTI1_PortB_handler()
{
    if(GPIO_ReadInputPin(GPIOB, GPIO_PIN_3)) {
        if (atomSemPut (&sem_light) != ATOM_OK)
        {
            ATOMLOG (_STR("Put failed\n"));
        }

    }
}

static uint16_t display_num;
static uint32_t display_time;
void TIM3_handler()
{
    static uint8_t dot=0;

    if(dot<4) {
        dot++;
    } else {
        dot=0;
        if((display_time<atomTimeGet())||(display_num==0)) {
            select_clear();
            clear_dig();
            archDisableTimer3();
            display_num=0;
            return;
        }
    }


    select_clear();
    show_dig((display_num>>(dot*4))&0xF, 0);
    select_dig(dot);

    if(dot==0) {
        show_comma(0);
    }
}

static void archDisplay(uint16_t num, uint32_t time)
{
    __disable_interrupt();
    display_time = atomTimeGet() + time;
    display_num = num;
    archEnableTimer3 ();
    __enable_interrupt();
}

static void archDisplayInit()
{
    __disable_interrupt();
    display_num =0;
    archInitTimer3 ();
    __enable_interrupt();
}
static void main_thread_func (uint32_t param)
{
    int sleep_ticks;
    uint8_t status;
    uint16_t Conversion_Value;

    /* Compiler warnings */
    param = param;

    archDisplayInit();

    if (atomSemCreate (&sem_light, 0) != ATOM_OK)
    {
        ATOMLOG (_STR("Error creating test semaphore light\n"));
        return;
    }

    ADC_Config();

    /* Initialise UART (115200bps) */
    if (uart_init(115200) != 0)
    {
        /* Error initialising UART */
    }

    /* Put a message out on the UART */
    printf("Go\n");

    /* Flash LED once per second if passed, very quickly if failed */
    sleep_ticks = SYSTEM_TICKS_PER_SEC*10;

    /* Test finished, flash slowly for pass, fast for fail */
    while (1)
    {

        if ((status = atomSemGet (&sem_light, 0)) != ATOM_OK)
        {
            ATOMLOG (_STR("Error %d\n"), status);
            break;
        }

        /*Start Conversion */
        ADC1_StartConversion();
        while(ADC1_GetFlagStatus(ADC1_FLAG_EOC)==RESET) {
            atomTimerDelay(1);
        };
        Conversion_Value = ADC1_GetConversionValue();
        ADC1_ITConfig(ADC1_IT_EOC, DISABLE);


        printf("ADC[0]: 0x%x\n", Conversion_Value);
        
        if(Conversion_Value > 0x300) {
            //10s
            archDisplay(Conversion_Value, 10*100);
            /* Toggle BLUE LED  */
            GPIO_WriteLow(GPIOC, GPIO_PIN_2);
            //GPIO_WriteReverse(GPIOC, GPIO_PIN_2);
            /* Sleep then toggle LED again */
            atomTimerDelay(sleep_ticks);
        } else {
            //1s
            archDisplay(Conversion_Value, 1*100);
        }

        /* Toggle small LED on board */
        GPIO_WriteReverse(GPIOE, GPIO_PIN_5);

        GPIO_WriteHigh(GPIOC, GPIO_PIN_2);

    }
}

