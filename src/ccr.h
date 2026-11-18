/* CCR laboratory firmware. Synthetic project; no OCEM source material. */
#ifndef CCR_H
#define CCR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CCR_TICK_MS              10U
#define CCR_RAMP_MA_PER_TICK     100U
#define CCR_CURRENT_TOL_MA      100U
#define CCR_OVERCURRENT_MA     7200U
#define CCR_OPEN_VOLTAGE_V     3000U
#define CCR_OPEN_CURRENT_MA    500U
#define CCR_OPEN_DELAY_MS      100U
#define CCR_TOL_DELAY_MS      1000U
#define CCR_STARTUP_LIMIT_MS  3000U
#define CCR_FRAME_SIZE           5U

/* Wire values are stable; append new states without reordering. */
typedef enum {
    CCR_OFF = 0,
    CCR_STARTUP = 1,
    CCR_REGULATING = 2,
    CCR_STEP_CHANGE = 3,
    CCR_FAULT = 4
} ccr_state_t;

typedef enum {
    CCR_FAULT_NONE = 0,
    CCR_FAULT_OVERCURRENT = 1U << 0,
    CCR_FAULT_OPEN_CIRCUIT = 1U << 1,
    CCR_FAULT_TOLERANCE = 1U << 2,
    CCR_FAULT_STARTUP = 1U << 3,
    CCR_FAULT_SENSOR = 1U << 4
} ccr_fault_t;

typedef enum {
    CCR_STEPS_3 = 3,
    CCR_STEPS_5 = 5
} ccr_steps_t;

typedef struct {
    uint16_t current_ma;
    uint16_t voltage_v;
    bool valid;
} ccr_sample_t;

typedef struct {
    uint16_t open_ms;
    uint16_t tolerance_ms;
} ccr_protect_t;

typedef struct {
    ccr_steps_t variant;
    ccr_state_t state;
    uint8_t requested_step;
    uint16_t target_ma;
    uint16_t reference_ma;
    uint16_t startup_ms;
    uint16_t faults;
    ccr_sample_t sample;
    ccr_protect_t protection;
} ccr_ctrl_t;

typedef struct {
    uint8_t bytes[CCR_FRAME_SIZE];
    uint8_t used;
} ccr_comm_t;

bool ccr_steps_target(ccr_steps_t variant, uint8_t step, uint16_t *target);
void ccr_protect_init(ccr_protect_t *protection);
uint16_t ccr_protect_update(ccr_protect_t *protection,
                            ccr_sample_t sample,
                            uint16_t reference_ma,
                            bool enabled,
                            bool monitor_tolerance);
bool ccr_ctrl_init(ccr_ctrl_t *ctrl, ccr_steps_t variant);
bool ccr_ctrl_set_step(ccr_ctrl_t *ctrl, uint8_t step);
bool ccr_ctrl_reset_fault(ccr_ctrl_t *ctrl);
void ccr_ctrl_tick(ccr_ctrl_t *ctrl);
const char *ccr_state_name(ccr_state_t state);
void ccr_comm_init(ccr_comm_t *comm);
bool ccr_comm_feed(ccr_comm_t *comm, ccr_ctrl_t *ctrl,
                   uint8_t byte, uint8_t response[CCR_FRAME_SIZE]);

#endif
