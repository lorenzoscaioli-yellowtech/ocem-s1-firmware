#include "ccr.h"

/* Values are laboratory configuration, not normative product data. */
static const uint16_t steps_5_ma[] = {0U, 2800U, 3400U, 4100U, 5200U, 6600U};
static const uint16_t steps_3_ma[] = {0U, 4800U, 5500U, 6600U};

/** @req REQ-CCR-001
 *  @req REQ-CCR-002
 *  @req REQ-CCR-003
 * Convert a validated step into an integer current reference.
 * On failure the caller's target is left untouched.
 */
bool ccr_steps_target(ccr_steps_t variant, uint8_t step, uint16_t *target)
{
    if (target == NULL) {
        return false;
    }
    if ((variant == CCR_STEPS_5) && (step <= 5U)) {
        *target = steps_5_ma[step];
        return true;
    }
    if ((variant == CCR_STEPS_3) && (step <= 3U)) {
        *target = steps_3_ma[step];
        return true;
    }
    return false;
}
