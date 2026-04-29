/**
 * @file lcd.c
 * @author Vault Team - Purdue
 * @brief LCD display functions
 * @date 2026
 *
 * Wrapper functions for writing information to the display. 
 */

#include "lcd.h"



/* Onboard LED positions */
LCD_pin gLCDPinPosition[6];


/* LCD memory map for numeric digits */
const char digit[10][4] = {
    {0x07, 0x09, 0x08, 0x0A}, /* "0" LCD segments a+b+c+d+e+f+k+q */
    {0x00, 0x00, 0x00, 0x0A}, /* "1" */
    {0x03, 0x0A, 0x00, 0x0C}, /* "2" */
    {0x01, 0x0A, 0x00, 0x0E}, /* "3" */
    {0x04, 0x02, 0x00, 0x0E}, /* "4" */
    {0x05, 0x0A, 0x01, 0x00}, /* "5" */
    {0x07, 0x0A, 0x00, 0x06}, /* "6" */
    {0x00, 0x08, 0x00, 0x0A}, /* "7" */
    {0x07, 0x0A, 0x00, 0x0E}, /* "8" */
    {0x05, 0x0A, 0x00, 0x0E}  /* "9" */
};

/* LCD memory map for uppercase letters */
const char alphabetUpper[26][4] = {
    {0x06, 0x0A, 0x00, 0x0E}, /* "A" LCD segments a+b+c+e+f+g+m */
    {0x01, 0x08, 0x06, 0x0E}, /* "B" */
    {0x07, 0x08, 0x00, 0x00}, /* "C" */
    {0x01, 0x08, 0x06, 0x0A}, /* "D" */
    {0x07, 0x0A, 0x00, 0x00}, /* "E" */
    {0x06, 0x0A, 0x00, 0x00}, /* "F" */
    {0x07, 0x08, 0x00, 0x06}, /* "G" */
    {0x06, 0x02, 0x00, 0x0E}, /* "H" */
    {0x01, 0x08, 0x06, 0x00}, /* "I" */
    {0x03, 0x00, 0x00, 0x0A}, /* "J" */
    {0x06, 0x02, 0x09, 0x00}, /* "K" */
    {0x07, 0x00, 0x00, 0x00}, /* "L" */
    {0x06, 0x04, 0x08, 0x0A}, /* "M" */
    {0x06, 0x04, 0x01, 0x0A}, /* "N" */
    {0x07, 0x08, 0x00, 0x0A}, /* "O" */
    {0x06, 0x0A, 0x00, 0x0C}, /* "P" */
    {0x07, 0x08, 0x01, 0x0A}, /* "Q" */
    {0x06, 0x0A, 0x01, 0x0C}, /* "R" */
    {0x05, 0x0A, 0x00, 0x06}, /* "S" */
    {0x00, 0x08, 0x06, 0x00}, /* "T" */
    {0x07, 0x00, 0x00, 0x0A}, /* "U" */
    {0x06, 0x01, 0x08, 0x00}, /* "V" */
    {0x06, 0x01, 0x01, 0x0A}, /* "W" */
    {0x00, 0x05, 0x09, 0x00}, /* "X" */
    {0x05, 0x02, 0x00, 0x0E}, /* "Y" */
    {0x01, 0x09, 0x08, 0x00}  /* "Z" */
};
void LCD_showChar(LCD_Regs *lcd, char ch, LCD_pin lcdPinPosition)
{
    uint32_t pin1 = lcdPinPosition.pin1;
    uint32_t pin2 = lcdPinPosition.pin2;
    uint32_t pin3 = lcdPinPosition.pin3;
    uint32_t pin4 = lcdPinPosition.pin4;

    /* Divide by 2 because mux rate < 5 */
    uint32_t pin1memIdx = pin1 / 2;
    uint32_t pin2memIdx = pin2 / 2;
    uint32_t pin3memIdx = pin3 / 2;
    uint32_t pin4memIdx = pin4 / 2;

    uint8_t mem;
    uint32_t memMask;

    if (ch >= '0' && ch <= '9') {
        /* Write digits */
        if (pin1 % 2) {
            /* Even memory location */
            mem     = DL_LCD_getMemory(LCD, pin1memIdx) & 0x0F;
            memMask = (mem | digit[ch - ASCII_DIGIT_OFFSET][0] << 4);
        } else {
            /* Odd memory location */
            mem     = DL_LCD_getMemory(LCD, pin1memIdx) & 0xF0;
            memMask = (mem | digit[ch - ASCII_DIGIT_OFFSET][0]);
        }
        DL_LCD_writeMemory(LCD, pin1memIdx, memMask);

        if (pin2 % 2) {
            mem     = DL_LCD_getMemory(LCD, pin2memIdx) & 0x0F;
            memMask = (mem | digit[ch - ASCII_DIGIT_OFFSET][1] << 4);
        } else {
            mem     = DL_LCD_getMemory(LCD, pin2memIdx) & 0xF0;
            memMask = (mem | digit[ch - ASCII_DIGIT_OFFSET][1]);
        }
        DL_LCD_writeMemory(LCD, pin2memIdx, memMask);

        if (pin3 % 2) {
            mem     = DL_LCD_getMemory(LCD, pin3memIdx) & 0x0F;
            memMask = (mem | digit[ch - ASCII_DIGIT_OFFSET][2] << 4);
        } else {
            mem     = DL_LCD_getMemory(LCD, pin3memIdx) & 0xF0;
            memMask = (mem | digit[ch - ASCII_DIGIT_OFFSET][2]);
        }
        DL_LCD_writeMemory(LCD, pin3memIdx, memMask);

        if (pin4 % 2) {
            mem     = DL_LCD_getMemory(LCD, pin4memIdx) & 0x0F;
            memMask = (mem | digit[ch - ASCII_DIGIT_OFFSET][3] << 4);
        } else {
            mem     = DL_LCD_getMemory(LCD, pin4memIdx) & 0xF0;
            memMask = (mem | digit[ch - ASCII_DIGIT_OFFSET][3]);
        }
        DL_LCD_writeMemory(LCD, pin4memIdx, memMask);

    } else if (ch >= 'A' && ch <= 'Z') {
        /* Write letters */
        if (pin1 % 2) {
            /* Even memory location */
            mem     = DL_LCD_getMemory(LCD, pin1memIdx) & 0x0F;
            memMask = (mem | alphabetUpper[ch - ASCII_LETTER_OFFSET][0] << 4);
        } else {
            /* Odd memory location */
            mem     = DL_LCD_getMemory(LCD, pin1memIdx) & 0xF0;
            memMask = (mem | alphabetUpper[ch - ASCII_LETTER_OFFSET][0]);
        }
        DL_LCD_writeMemory(LCD, pin1memIdx, memMask);

        if (pin2 % 2) {
            mem     = DL_LCD_getMemory(LCD, pin2memIdx) & 0x0F;
            memMask = (mem | alphabetUpper[ch - ASCII_LETTER_OFFSET][1] << 4);
        } else {
            mem     = DL_LCD_getMemory(LCD, pin2memIdx) & 0xF0;
            memMask = (mem | alphabetUpper[ch - ASCII_LETTER_OFFSET][1]);
        }
        DL_LCD_writeMemory(LCD, pin2memIdx, memMask);

        if (pin3 % 2) {
            mem     = DL_LCD_getMemory(LCD, pin3memIdx) & 0x0F;
            memMask = (mem | alphabetUpper[ch - ASCII_LETTER_OFFSET][2] << 4);
        } else {
            mem     = DL_LCD_getMemory(LCD, pin3memIdx) & 0xF0;
            memMask = (mem | alphabetUpper[ch - ASCII_LETTER_OFFSET][2]);
        }
        DL_LCD_writeMemory(LCD, pin3memIdx, memMask);

        if (pin4 % 2) {
            mem     = DL_LCD_getMemory(LCD, pin4memIdx) & 0x0F;
            memMask = (mem | alphabetUpper[ch - ASCII_LETTER_OFFSET][3] << 4);
        } else {
            mem     = DL_LCD_getMemory(LCD, pin4memIdx) & 0xF0;
            memMask = (mem | alphabetUpper[ch - ASCII_LETTER_OFFSET][3]);
        }
        DL_LCD_writeMemory(LCD, pin4memIdx, memMask);
    }
}

void LCD_print(char inputstr[6])
{
    
    gLCDPinPosition[0].pin1 = DL_LCD_SEGMENT_LINE_58;
    gLCDPinPosition[0].pin2 = DL_LCD_SEGMENT_LINE_57;
    gLCDPinPosition[0].pin3 = DL_LCD_SEGMENT_LINE_56;
    gLCDPinPosition[0].pin4 = DL_LCD_SEGMENT_LINE_55;

    gLCDPinPosition[1].pin1 = DL_LCD_SEGMENT_LINE_36;
    gLCDPinPosition[1].pin2 = DL_LCD_SEGMENT_LINE_37;
    gLCDPinPosition[1].pin3 = DL_LCD_SEGMENT_LINE_38;
    gLCDPinPosition[1].pin4 = DL_LCD_SEGMENT_LINE_18;

    gLCDPinPosition[2].pin1 = DL_LCD_SEGMENT_LINE_19;
    gLCDPinPosition[2].pin2 = DL_LCD_SEGMENT_LINE_20;
    gLCDPinPosition[2].pin3 = DL_LCD_SEGMENT_LINE_23;
    gLCDPinPosition[2].pin4 = DL_LCD_SEGMENT_LINE_39;

    gLCDPinPosition[3].pin1 = DL_LCD_SEGMENT_LINE_40;
    gLCDPinPosition[3].pin2 = DL_LCD_SEGMENT_LINE_41;
    gLCDPinPosition[3].pin3 = DL_LCD_SEGMENT_LINE_54;
    gLCDPinPosition[3].pin4 = DL_LCD_SEGMENT_LINE_53;

    gLCDPinPosition[4].pin1 = DL_LCD_SEGMENT_LINE_52;
    gLCDPinPosition[4].pin2 = DL_LCD_SEGMENT_LINE_51;
    gLCDPinPosition[4].pin3 = DL_LCD_SEGMENT_LINE_50;
    gLCDPinPosition[4].pin4 = DL_LCD_SEGMENT_LINE_49;

    gLCDPinPosition[5].pin1 = DL_LCD_SEGMENT_LINE_48;
    gLCDPinPosition[5].pin2 = DL_LCD_SEGMENT_LINE_47;
    gLCDPinPosition[5].pin3 = DL_LCD_SEGMENT_LINE_46;
    gLCDPinPosition[5].pin4 = DL_LCD_SEGMENT_LINE_45;

    for(uint8_t i = 0; i < 6; i++)
    {
        if(inputstr[i] != '_')
        {
            LCD_showChar(LCD, inputstr[i], gLCDPinPosition[i]);
        }
    }
    
} // comment