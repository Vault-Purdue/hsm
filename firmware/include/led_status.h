/**
 * @file led_status.h
 * @author Vault Team - Purdue
 * @brief LED Implementation
 * @date 2026
 */

#ifndef __STATUS_LED__
#define __STATUS_LED__

#include "ti_msp_dl_config.h"

#define STATUS_LED_ON()  DL_GPIO_setPins(GPIOA, GPIO_RED_LED_PIN)
#define STATUS_LED_OFF() DL_GPIO_clearPins(GPIOA, GPIO_RED_LED_PIN)

#endif // __STATUS_LED__