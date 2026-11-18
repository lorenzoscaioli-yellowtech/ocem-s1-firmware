#include <stdio.h>
#include "ccr.h"
#include "hal.h"

/** @req REQ-CCR-017
 * Host entry point: the board scheduler replaces this finite scenario.
 */
int main(void)
{
    ccr_ctrl_t ctrl;
    ccr_state_t previous = CCR_OFF;
    hal_sim_reset();
    if (!ccr_ctrl_init(&ctrl, CCR_STEPS_5)
        || !ccr_ctrl_set_step(&ctrl, 3U)) {
        return 1;
    }
    for (unsigned tick = 0U; tick < 160U; ++tick) {
        if (tick == 80U) {
            (void)ccr_ctrl_set_step(&ctrl, 5U);
        }
        if (tick == 140U) {
            (void)ccr_ctrl_set_step(&ctrl, 0U);
        }
        hal_sim_advance();
        ccr_ctrl_tick(&ctrl);
        if (previous != ctrl.state) {
            printf("%4u ms %-12s ref=%u mA measured=%u mA faults=0x%02x\n",
                tick * CCR_TICK_MS, ccr_state_name(ctrl.state),
                (unsigned)ctrl.reference_ma, (unsigned)ctrl.sample.current_ma,
                (unsigned)ctrl.faults);
            previous = ctrl.state;
        }
    }
    return ctrl.faults == 0U ? 0 : 1;
}
