/*==================================================================================================
*   Project              : RTD AUTOSAR 4.9
*   Platform             : CORTEXM
*   Peripheral           : S32K3XX
*   Dependencies         : none
*
*   Autosar Version      : 4.9.0
*   Autosar Revision     : ASR_REL_4_9_REV_0000
*   Autosar Conf.Variant :
*   SW Version           : 7.0.0
*   Build Version        : S32K3_RTD_7_0_0_QLP03_D2512_ASR_REL_4_9_REV_0000_20251210
*
*   Copyright 2020 - 2026 NXP
*
*   NXP Proprietary. This software is owned or controlled by NXP and may only be
*   used strictly in accordance with the applicable license terms. By expressly
*   accepting such terms or by downloading, installing, activating and/or otherwise
*   using the software, you are agreeing that you have read, and that you agree to
*   comply with and are bound by, such license terms. If you do not agree to be
*   bound by the applicable license terms, then you may not retain, install,
*   activate or otherwise use the software.
==================================================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/

#include "Pwm.h"
#include "Mcu.h"
#include "Mcl.h"
#include "Siul2_Port_Ip.h"
#include "check_example.h"

/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/


/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/
#define NUM_BLINK_LED     (uint32)10U
#define DELAY_TIMER       (uint32)10000000U
#define MCL_EMIOS_1_CH_23 (uint16)23U
/*==================================================================================================
*                                      LOCAL CONSTANTS
==================================================================================================*/


/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/


/*==================================================================================================
*                                      GLOBAL CONSTANTS
==================================================================================================*/


/*==================================================================================================
*                                      GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
*                                   LOCAL FUNCTION PROTOTYPES
==================================================================================================*/

void TestDelay(uint32 delay);
/*==================================================================================================
*                                       LOCAL FUNCTIONS
==================================================================================================*/

void TestDelay(uint32 delay)
{
    static volatile uint32 DelayTimer = 0;
    while(DelayTimer<delay)
    {
        DelayTimer++;
    }
    DelayTimer=0;
}

/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/

/**
* @brief        Main function of the example
* @details      Initialize the used drivers and use the PWM driver to control the LED brightness
*               and blink an LED
*/
int main(void)
{
    uint8 num_blink = 0U;

    /* Initialize the Mcu driver */
    Mcu_Init(&Mcu_Config);

    /* Initialize the clock tree */
    Mcu_InitClock(McuClockSettingConfig_0);

    /* Apply a mode configuration */
    Mcu_SetMode(McuModeSettingConf_0);

    /* Initialize all pins using the Siul2_Port driver */
    Siul2_Port_Ip_Init(NUM_OF_CONFIGURED_PINS, g_pin_mux_InitConfigArr);

    /* Initialize Mcl driver */
    Mcl_Init(&Mcl_Config);

    /* Initialize Pwm driver , after that Led on*/
    Pwm_Init(&Pwm_Config);

    /* Set new period for all channels used external counter bus */
    Mcl_Emios_SetCounterBusPeriod(MCL_EMIOS_1_CH_23, 0x9000U, FALSE);

    /* Increasing LED brightness using Pwm_SetDutyCycle*/
    Pwm_SetDutyCycle(EMIOS_1_CH_12, 0x200);

    /* Decreasing LED brightness using Pwm_SetDutyCycle*/
    Pwm_SetDutyCycle(EMIOS_1_CH_12, 0x7000);

    /* Using duty cycle 0% and 100% to Blink LED */
    while(num_blink < NUM_BLINK_LED)
    {
        /* Led off when duty cycle is 0% */
        Pwm_SetDutyCycle(EMIOS_1_CH_12, 0x8000);
        TestDelay(DELAY_TIMER);

        /* Increasing LED brightness using Pwm_SetDutyCycle */
        Pwm_SetDutyCycle(EMIOS_1_CH_12, 0x7000);
        TestDelay(DELAY_TIMER);

        /* Led on when duty cycle is 100% */
        Pwm_SetDutyCycle(EMIOS_1_CH_12, 0x0000);
        TestDelay(DELAY_TIMER);
        num_blink++;
    }

    /* De-Initialize Pwm driver */
    Pwm_DeInit();

    Exit_Example(TRUE);

    while(1);

    return 0U;
}

#ifdef __cplusplus
}
#endif

/** @} */
