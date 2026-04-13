#include "ti_drivers_config.h"
#include "timer.h"

/************************ FUNCTIONS ***********************/

/** TODO: Uodate the function's description
 * @brief Powers on and initializes the TRNG module
 * 
 * This function powers on the TRNG module and
 * initializes the TRNG clock and decimation rates.
 */
void TIMER_0_INST_IRQHandler(void)
{
    uint32_t status = DL_TimerG_getPendingInterrupt(TIMER_0_INST);
    
    if (status & DL_TIMERG_INTERRUPT_ZERO_EVENT) {
        DL_TimerG_clearInterruptStatus(TIMER_0_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT);
        DL_GPIO_togglePins(GPIO_RED_LED_PORT, GPIO_RED_LED_PIN);
    }
}