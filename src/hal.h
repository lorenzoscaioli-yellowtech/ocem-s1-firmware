#ifndef HAL_H
#define HAL_H

#include "ccr.h"

/* Board adapter: coherent ADC snapshot and reference to inner current loop.
 * The hardware port must implement independent fast shutdown protections.
 */
ccr_sample_t hal_adc_read(void);
void hal_output_write(bool enabled, uint16_t reference_ma);

/* Host-only test interface, not part of a production board port. */
void hal_sim_reset(void);
void hal_sim_sample(uint16_t current_ma, uint16_t voltage_v, bool valid);
void hal_sim_advance(void);
bool hal_sim_enabled(void);
uint16_t hal_sim_reference(void);

#endif
