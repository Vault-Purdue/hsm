/*
 * Copyright (c) 2015-2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <ti/driverlib/driverlib.h>

#include "csc_keyload.h"
#include "ti_msp_dl_config.h"

/* .secret region addresses, matching the linker script. */
#define VAULT_SECRET_ADDR_START   (0x00004000U)
#define VAULT_SECRET_ADDR_END     (0x000043FFU)

static void csc_init_power(void);

static void start_app(uint32_t *vector_table)
{
    __asm volatile(
        "LDR R3,[%[vectab],#0x0] \n"
        "MOV SP, R3              \n"
        ::[vectab] "r"(vector_table));

    SCB->VTOR = (uint32_t) vector_table;

    ((void (*)(void))(*(vector_table + 1)))();
}

static void cscHalt(void)
{
    while (1) {
        __asm volatile("wfi");
    }
}

int main(void)
{
    csc_init_power();

    if (!DL_SYSCTL_isINITDONEIssued()) {

        /* Read vault root key from .secret region, write into
         * KEYSTORE slot 0. This is done before the firewall is
         * turned on */
        if (CSC_loadVaultRootKey() != HSM_KEYSTORE_OK) {
            cscHalt();
        }

        /* Configure read-execute-protect firewall over .secret.
         * Only takes effect when INITDONE is issued below. */
        DL_SYSCTL_setReadExecuteProtectFirewallAddrStart(VAULT_SECRET_ADDR_START);
        DL_SYSCTL_setReadExecuteProtectFirewallAddrEnd(VAULT_SECRET_ADDR_END);
        DL_SYSCTL_enableReadExecuteProtectFirewall();

        DL_SYSCTL_issueINITDONE();
    }

    start_app((uint32_t *) 0x00004800);

    cscHalt();
    return 0;
}

static void csc_init_power(void)
{
    DL_AESADV_reset(AESADV);
    DL_AESADV_enablePower(AESADV);

    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);
    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
    delay_cycles(POWER_STARTUP_DELAY);
}
