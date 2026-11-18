#include "ccr.h"
#include "hal.h"

/** @req REQ-CCR-005
 * Slew reference by at most 100 mA per scheduler invocation.
 */
static uint16_t ramp_next(uint16_t actual, uint16_t target)
{
    uint16_t distance;
    if (actual < target) {
        distance = (uint16_t)(target - actual);
        return (distance <= CCR_RAMP_MA_PER_TICK)
            ? target : (uint16_t)(actual + CCR_RAMP_MA_PER_TICK);
    }
    distance = (uint16_t)(actual - target);
    return (distance <= CCR_RAMP_MA_PER_TICK)
        ? target : (uint16_t)(actual - CCR_RAMP_MA_PER_TICK);
}

/** @req REQ-CCR-009
 * Latching shutdown: a new command cannot reactivate the output.
 */
static void enter_fault(ccr_ctrl_t *ctrl, uint16_t faults)
{
    ctrl->faults |= faults;
    ctrl->state = CCR_FAULT;
    ctrl->requested_step = 0U;
    ctrl->target_ma = 0U;
    ctrl->reference_ma = 0U;
    hal_output_write(false, 0U);
}

/** @req REQ-CCR-004
 *  @req REQ-CCR-017
 */
bool ccr_ctrl_init(ccr_ctrl_t *ctrl, ccr_steps_t variant)
{
    uint16_t unused;
    hal_output_write(false, 0U);
    if (ctrl == NULL) {
        return false;
    }
    *ctrl = (ccr_ctrl_t){0};
    ctrl->variant = variant;
    ctrl->state = CCR_OFF;
    ccr_protect_init(&ctrl->protection);
    if (!ccr_steps_target(variant, 0U, &unused)) {
        enter_fault(ctrl, CCR_FAULT_SENSOR);
        return false;
    }
    return true;
}

/** @req REQ-CCR-003
 *  @req REQ-CCR-004
 *  @req REQ-CCR-005
 *  @req REQ-CCR-009
 *  @req REQ-CCR-016
 * Commands and ticks share the same scheduler context; no ISR access.
 */
bool ccr_ctrl_set_step(ccr_ctrl_t *ctrl, uint8_t step)
{
    uint16_t target;
    if ((ctrl == NULL)
        || !ccr_steps_target(ctrl->variant, step, &target)) {
        return false;
    }
    if (ctrl->state == CCR_FAULT) {
        return false;
    }
    ctrl->requested_step = step;
    ctrl->target_ma = target;
    if (step == 0U) {
        ctrl->state = CCR_OFF;
        ctrl->reference_ma = 0U;
        ctrl->startup_ms = 0U;
        ccr_protect_init(&ctrl->protection);
        hal_output_write(false, 0U);
    } else if (ctrl->state == CCR_OFF) {
        ctrl->state = CCR_STARTUP;
        ctrl->startup_ms = 0U;
    } else if ((ctrl->state == CCR_REGULATING)
               && (target != ctrl->reference_ma)) {
        ctrl->state = CCR_STEP_CHANGE;
    }
    return true;
}

/** @req REQ-CCR-009
 * Reset acknowledgement always returns to OFF; it never restarts a load.
 */
bool ccr_ctrl_reset_fault(ccr_ctrl_t *ctrl)
{
    ccr_sample_t sample;
    if ((ctrl == NULL) || (ctrl->state != CCR_FAULT)) {
        return false;
    }
    sample = hal_adc_read();
    if (!sample.valid || (sample.current_ma > CCR_CURRENT_TOL_MA)
        || (sample.voltage_v >= CCR_OPEN_VOLTAGE_V)) {
        return false;
    }
    ctrl->sample = sample;
    ctrl->faults = CCR_FAULT_NONE;
    ctrl->state = CCR_OFF;
    ctrl->startup_ms = 0U;
    ccr_protect_init(&ctrl->protection);
    hal_output_write(false, 0U);
    return true;
}

/** @req REQ-CCR-004
 *  @req REQ-CCR-005
 *  @req REQ-CCR-009
 *  @req REQ-CCR-011
 *  @req REQ-CCR-017
 *  @req REQ-CCR-018
 */
void ccr_ctrl_tick(ccr_ctrl_t *ctrl)
{
    bool enabled;
    uint16_t faults;
    uint16_t error_ma;
    if (ctrl == NULL) {
        hal_output_write(false, 0U);
        return;
    }
    ctrl->sample = hal_adc_read();
    enabled = (ctrl->state == CCR_STARTUP)
        || (ctrl->state == CCR_REGULATING)
        || (ctrl->state == CCR_STEP_CHANGE);
    faults = ccr_protect_update(&ctrl->protection, ctrl->sample,
        ctrl->reference_ma, enabled, ctrl->state == CCR_REGULATING);
    if (faults != CCR_FAULT_NONE) {
        enter_fault(ctrl, faults);
    }
    if ((ctrl->state == CCR_OFF) || (ctrl->state == CCR_FAULT)) {
        hal_output_write(false, 0U);
        return;
    }
    if (!enabled) {
        enter_fault(ctrl, CCR_FAULT_SENSOR);
        return;
    }

    ctrl->reference_ma = ramp_next(ctrl->reference_ma, ctrl->target_ma);
    error_ma = (ctrl->sample.current_ma >= ctrl->target_ma)
        ? (uint16_t)(ctrl->sample.current_ma - ctrl->target_ma)
        : (uint16_t)(ctrl->target_ma - ctrl->sample.current_ma);
    if (ctrl->state == CCR_STARTUP) {
        ctrl->startup_ms = (uint16_t)(ctrl->startup_ms + CCR_TICK_MS);
        if ((ctrl->reference_ma == ctrl->target_ma)
            && (error_ma <= CCR_CURRENT_TOL_MA)) {
            ctrl->state = CCR_REGULATING;
        } else if (ctrl->startup_ms >= CCR_STARTUP_LIMIT_MS) {
            enter_fault(ctrl, CCR_FAULT_STARTUP);
            return;
        }
    } else if ((ctrl->state == CCR_STEP_CHANGE)
               && (ctrl->reference_ma == ctrl->target_ma)) {
        ctrl->state = CCR_REGULATING;
    }
    hal_output_write(true, ctrl->reference_ma);
}

const char *ccr_state_name(ccr_state_t state)
{
    switch (state) {
    case CCR_OFF: return "OFF";
    case CCR_STARTUP: return "STARTUP";
    case CCR_REGULATING: return "REGULATING";
    case CCR_STEP_CHANGE: return "STEP_CHANGE";
    case CCR_FAULT: return "FAULT";
    default: return "UNKNOWN";
    }
}
