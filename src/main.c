/*==================================================================================================
*   Project              : RTD AUTOSAR 4.9
*   Platform             : CORTEXM
*   Peripheral           : S32K3XX
*   Dependencies         : none
*
*   Autosar Version      : 4.9.0
*   Autosar Revision     : ASR_REL_4_9_REV_0000
*   Autosar Conf.Variant :
*   SW Version           : 7.0.1
*   Build Version        : S32K3_RTD_7_0_1_D2602_ASR_REL_4_9_REV_0000_20260206
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
#include "Platform.h"
#include "CDD_Uart.h"
#include "check_example.h"

#include <string.h>
#include <stdio.h>

/*==================================================================================================
*                          LOCAL TYPEDEFS (STRUCTURES, UNIONS, ENUMS)
==================================================================================================*/


/*==================================================================================================
*                                       LOCAL MACROS
==================================================================================================*/
#define NUM_BLINK_LED     (uint32)10U
#define DELAY_TIMER       (uint32)10000000U
#define MCL_EMIOS_1_CH_23 (uint16)23U

/** @brief LPUART logical channel used for all console output (LPUART6, 115200 8N1). */
#define UART_LPUART_INTERNAL_CHANNEL    (uint8)0U

/** @brief Full scale of the AUTOSAR PWM duty cycle (0x8000 = 100%). */
#define PWM_DUTY_MAX                    (uint32)0x8000U

/** @brief Report the fade progress over UART only every N steps (throttle). */
#define FADE_REPORT_EVERY               (uint32)16U

/** @brief Size of the scratch buffer used to format UART messages. */
#define UART_MSG_SIZE                   80U
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
static void Uart_SendBlocking(const char *pMsg);
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

/**
* @brief        Send a null-terminated string over UART and block until it completes.
* @details      The transfer is started with Uart_AsyncSend and completed by the LPUART6
*               interrupt handler installed by the Platform driver, so the status is
*               polled here until the driver reports that no transfer is ongoing.
* @param[in]    pMsg    Pointer to the null-terminated message string.
*/
static void Uart_SendBlocking(const char *pMsg)
{
    Std_ReturnType  uartStatus;
    Uart_StatusType uartTxStatus;
    uint32          bytesRemaining;

    uartStatus = Uart_AsyncSend(UART_LPUART_INTERNAL_CHANNEL,
                                (const uint8 *)pMsg,
                                (uint32)strlen(pMsg));

    if (E_OK == uartStatus)
    {
        do
        {
            uartTxStatus = Uart_GetStatus(UART_LPUART_INTERNAL_CHANNEL,
                                          &bytesRemaining,
                                          UART_SEND);
        }
        while (UART_STATUS_NO_ERROR != uartTxStatus);
    }
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
    uint8  num_blink = 0U;
    uint16 dutyCycle = 0U;
    uint32 fadeStep;
    uint32 dutyPercent;
    char   msg[UART_MSG_SIZE];

    /* Initialize the Mcu driver */
    Mcu_Init(&Mcu_Config);

    /* Initialize the clock tree */
    Mcu_InitClock(McuClockSettingConfig_0);

#if (MCU_NO_PLL == STD_OFF)
    /* Wait until the System PLL is locked, then switch the system clock over to
     * it. Without this the core keeps running from the FIRC and every derived
     * clock, including the LPUART baud clock, is far off its configured value. */
    while (MCU_PLL_LOCKED != Mcu_GetPllStatus())
    {
        /* Busy wait for the PLL to lock */
    }
    (void)Mcu_DistributePllClock();
#endif

    /* Apply a mode configuration */
    Mcu_SetMode(McuModeSettingConf_0);

    /* Initialize all pins using the Siul2_Port driver */
    Siul2_Port_Ip_Init(NUM_OF_CONFIGURED_PINS, g_pin_mux_InitConfigArr);

    /* Initialize Mcl driver */
    Mcl_Init(&Mcl_Config);

    /* Initialize the Platform driver. This installs and enables the configured
     * interrupts, including LPUART6_IRQn -> LPUART_UART_IP_6_IRQHandler, which
     * the Uart driver needs to complete the interrupt driven transfers. */
    Platform_Init(NULL_PTR);

    /* Initialize the Uart driver. UART_PRECOMPILE_SUPPORT is STD_ON, so the
     * precompile configuration is used and no config pointer is required. */
    Uart_Init(NULL_PTR);

    /* Initialize Pwm driver */
    Pwm_Init(&Pwm_Config);

    /* Set new period for all channels used external counter bus */
    Mcl_Emios_SetCounterBusPeriod(MCL_EMIOS_1_CH_23, 0x9000U, FALSE);

    Uart_SendBlocking("\r\n");
    Uart_SendBlocking("=======================================\r\n");
    Uart_SendBlocking(" S32K312 eMIOS PWM LED dimming example\r\n");
    Uart_SendBlocking("=======================================\r\n");
    (void)snprintf(msg, sizeof(msg),
                   "Running %lu dimming cycles on EMIOS_1_CH_12\r\n",
                   (unsigned long)NUM_BLINK_LED);
    Uart_SendBlocking(msg);

    /* Slow LED dimming - 10 cycles */
    while(num_blink < NUM_BLINK_LED)
    {
        (void)snprintf(msg, sizeof(msg),
                       "\r\n[LED]  cycle %lu/%lu\r\n",
                       (unsigned long)(num_blink + 1U),
                       (unsigned long)NUM_BLINK_LED);
        Uart_SendBlocking(msg);

        /* Fade IN - Gradually increase brightness (0% to 100%) */
        fadeStep = 0U;
        for(dutyCycle = 0x8000; dutyCycle > 0x0000; dutyCycle -= 0x100)
        {
            Pwm_SetDutyCycle(EMIOS_1_CH_12, dutyCycle);

            /* Report only every N steps so the UART traffic does not
             * noticeably slow down the dimming ramp. */
            if ((fadeStep % FADE_REPORT_EVERY) == 0U)
            {
                dutyPercent = ((uint32)dutyCycle * 100U) / PWM_DUTY_MAX;
                (void)snprintf(msg, sizeof(msg),
                               "[FADE] in    duty=%3lu%%\r\n",
                               (unsigned long)dutyPercent);
                Uart_SendBlocking(msg);
            }
            fadeStep++;

            TestDelay(50000); /* Adjust this value for slower/faster dimming */
        }

        /* Hold at full brightness */
        Uart_SendBlocking("[FADE] hold  full brightness\r\n");
        TestDelay(DELAY_TIMER);

        /* Fade OUT - Gradually decrease brightness (100% to 0%) */
        fadeStep = 0U;
        for(dutyCycle = 0x0000; dutyCycle < 0x8000; dutyCycle += 0x100)
        {
            Pwm_SetDutyCycle(EMIOS_1_CH_12, dutyCycle);

            if ((fadeStep % FADE_REPORT_EVERY) == 0U)
            {
                dutyPercent = ((uint32)dutyCycle * 100U) / PWM_DUTY_MAX;
                (void)snprintf(msg, sizeof(msg),
                               "[FADE] out   duty=%3lu%%\r\n",
                               (unsigned long)dutyPercent);
                Uart_SendBlocking(msg);
            }
            fadeStep++;

            TestDelay(50000); /* Adjust this value for slower/faster dimming */
        }

        /* Hold at minimum brightness */
        Uart_SendBlocking("[FADE] hold  minimum brightness\r\n");
        TestDelay(DELAY_TIMER);
        num_blink++;
    }

    Uart_SendBlocking("\r\nAll dimming cycles completed. Example done.\r\n");

    /* De-Initialize Pwm driver */
    Pwm_DeInit();

    /* De-Initialize Uart driver */
    Uart_Deinit();

    Exit_Example(TRUE);

    while(1);

    return 0U;
}

#ifdef __cplusplus
}
#endif

/** @} */
