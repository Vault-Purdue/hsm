/*****************************************************************************

  Copyright (C) 2021 Texas Instruments Incorporated - http://www.ti.com/ 

  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions 
  are met:

   Redistributions of source code must retain the above copyright 
   notice, this list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the 
   documentation and/or other materials provided with the   
   distribution.

   Neither the name of Texas Instruments Incorporated nor the names of
   its contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*****************************************************************************/
-uinterruptVectors
--stack_size=256

MEMORY
{
    FLASH        (RX)  : origin = 0x00000000, length = 0x00004000
    SECRET       (RX)  : origin = 0x00004000, length = 0x00000400
    LOCK_STORAGE (RX)  : origin = 0x00004400, length = 0x00000400
    FLASH_APP    (RX)  : origin = 0x00004800, length = 0x0003B800
    SRAM         (RWX) : origin = 0x20200000, length = 0x00008000
    BCR_CONFIG   (R)   : origin = 0x41C00000, length = 0x000000FF
    BSL_CONFIG   (R)   : origin = 0x41C00100, length = 0x00000080
}

SECTIONS
{
    .intvecs      :                  > 0x00000000
    .text         : palign(8) {}     > FLASH | FLASH_APP
    .const        : palign(8) {}     > FLASH | FLASH_APP
    .cinit        : palign(8) {}     > FLASH | FLASH_APP
    .pinit        : palign(8) {}     > FLASH | FLASH_APP
    .rodata       : palign(8) {}     > FLASH | FLASH_APP
    .ARM.exidx    : palign(8) {}     > FLASH | FLASH_APP
    .init_array   : palign(8) {}     > FLASH | FLASH_APP
    .binit        : palign(8) {}     > FLASH | FLASH_APP
    .TI.ramfunc   : load = FLASH, palign(8), run = SRAM, table(BINIT)
    .secret       : palign(8) {}     > SECRET
    .lockStg      : (NOINIT) palign(8) {} > LOCK_STORAGE
    .vtable       :                  > SRAM
    .args         :                  > SRAM
    .data         :                  > SRAM
    .bss          :                  > SRAM
    .sysmem       :                  > SRAM
    .stack        :                  > SRAM (HIGH)
    .BCRConfig    : {}               > BCR_CONFIG
    .BSLConfig    : {}               > BSL_CONFIG
}