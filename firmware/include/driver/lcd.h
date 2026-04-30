/**
 * @file lcd.h
 * @author Vault Team - Purdue
 * @brief LCD display functions
 * @date 2026
 *
 * Wrapper functions for writing information to the display. 
 */

#include <stdint.h>
#include <ti/driverlib/m0p/dl_core.h>
#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>

#ifndef __LCD__
#define __LCD__

#ifndef LCD_ENABLE
#define LCD_ENABLE 0
#endif

#define ASCII_DIGIT_OFFSET (48)
#define ASCII_LETTER_OFFSET (65)

typedef struct {
    uint32_t pin1;
    uint32_t pin2;
    uint32_t pin3;
    uint32_t pin4;
} LCD_pin;

void LCD_showChar(LCD_Regs *lcd, char ch, LCD_pin lcdPinPosition);
void LCD_print(char inputstr[6]);

#endif