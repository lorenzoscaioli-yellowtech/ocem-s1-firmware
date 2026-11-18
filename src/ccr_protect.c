#include "ccr.h"

/** @req REQ-CCR-010
 * Bound elapsed counters to avoid wrap on long-running faults.
 */
static uint16_t elapsed_tick(uint16_t elapsed, uint16_t limit)
{
    if (elapsed >= limit) {
        return limit;
    }
    if ((uint16_t)(limit - elapsed) <= CCR_TICK_MS) {
        return limit;
    }
    return (uint16_t)(elapsed + CCR_TICK_MS);
}

/** @req REQ-CCR-007
 *  @req REQ-CCR-008
 */
void ccr_protect_init(ccr_protect_t *protection)
{
    protection->open_ms = 0U;
    protection->tolerance_ms = 0U;
}

/** @req REQ-CCR-006
 *  @req REQ-CCR-007
 *  @req REQ-CCR-008
 *  @req REQ-CCR-010
 *  @req REQ-CCR-018
 * Called exactly once per 10 ms, before updating the next reference.
 */
uint16_t ccr_protect_update(ccr_protect_t *protection,
                            ccr_sample_t sample,
                            uint16_t reference_ma,
                            bool enabled,
                            bool monitor_tolerance)
{
    uint16_t faults = CCR_FAULT_NONE;
    uint16_t error_ma;
    bool open;

    if (!sample.valid) {
        ccr_protect_init(protection);
        return CCR_FAULT_SENSOR;
    }
    if (sample.current_ma > CCR_OVERCURRENT_MA) {
        faults |= CCR_FAULT_OVERCURRENT;
    }
    if (!enabled) {
        ccr_protect_init(protection);
        return faults;
    }

    open = (sample.voltage_v >= CCR_OPEN_VOLTAGE_V)
        && (sample.current_ma < CCR_OPEN_CURRENT_MA)
        && (reference_ma >= CCR_OPEN_CURRENT_MA);
    protection->open_ms = open
        ? elapsed_tick(protection->open_ms, CCR_OPEN_DELAY_MS) : 0U;
    if (protection->open_ms >= CCR_OPEN_DELAY_MS) {
        faults |= CCR_FAULT_OPEN_CIRCUIT;
    }

    error_ma = (sample.current_ma >= reference_ma)
        ? (uint16_t)(sample.current_ma - reference_ma)
        : (uint16_t)(reference_ma - sample.current_ma);
    if (!monitor_tolerance) {
        protection->tolerance_ms = 0U;
    } else if (error_ma > CCR_CURRENT_TOL_MA) {
        protection->tolerance_ms = elapsed_tick(
            protection->tolerance_ms, CCR_TOL_DELAY_MS);
    }
    if (protection->tolerance_ms >= CCR_TOL_DELAY_MS) {
        faults |= CCR_FAULT_TOLERANCE;
    }
    return faults;
}
