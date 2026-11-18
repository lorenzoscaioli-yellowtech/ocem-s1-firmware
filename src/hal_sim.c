#include "hal.h"

static ccr_sample_t adc;
static bool gate;
static uint16_t reference;

/** @req REQ-CCR-017 */
ccr_sample_t hal_adc_read(void)
{
    return adc;
}

/** @req REQ-CCR-017 */
void hal_output_write(bool enabled, uint16_t reference_ma)
{
    gate = enabled;
    reference = enabled ? reference_ma : 0U;
}

/** @req REQ-CCR-017 */
void hal_sim_reset(void)
{
    adc = (ccr_sample_t){0U, 0U, true};
    gate = false;
    reference = 0U;
}

/** @req REQ-CCR-017 */
void hal_sim_sample(uint16_t current_ma, uint16_t voltage_v, bool valid)
{
    adc = (ccr_sample_t){current_ma, voltage_v, valid};
}

/** @req REQ-CCR-017
 * Ideal plant with one scheduler tick of latency, no power-stage model.
 */
void hal_sim_advance(void)
{
    adc.current_ma = gate ? reference : 0U;
    adc.voltage_v = gate ? 1000U : 0U;
    adc.valid = true;
}

/** @req REQ-CCR-017 */
bool hal_sim_enabled(void)
{
    return gate;
}

/** @req REQ-CCR-017 */
uint16_t hal_sim_reference(void)
{
    return reference;
}
