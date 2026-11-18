#include <stdio.h>
#include <stdlib.h>
#include "ccr.h"
#include "hal.h"

static unsigned checks;
#define CHECK(expr) do { \
    ++checks; \
    if (!(expr)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static ccr_ctrl_t fixture(void)
{
    ccr_ctrl_t ctrl;
    hal_sim_reset();
    CHECK(ccr_ctrl_init(&ctrl, CCR_STEPS_5));
    return ctrl;
}

static void run_plant(ccr_ctrl_t *ctrl, unsigned ticks)
{
    for (unsigned i = 0U; i < ticks; ++i) {
        hal_sim_advance();
        ccr_ctrl_tick(ctrl);
    }
}

/** @req REQ-CCR-001
 *  @req REQ-CCR-002
 *  @req REQ-CCR-003
 */
static void test_steps(void)
{
    const uint16_t five[] = {0U, 2800U, 3400U, 4100U, 5200U, 6600U};
    const uint16_t three[] = {0U, 4800U, 5500U, 6600U};
    uint16_t target = 999U;
    for (uint8_t i = 0U; i <= 5U; ++i) {
        CHECK(ccr_steps_target(CCR_STEPS_5, i, &target));
        CHECK(target == five[i]);
    }
    for (uint8_t i = 0U; i <= 3U; ++i) {
        CHECK(ccr_steps_target(CCR_STEPS_3, i, &target));
        CHECK(target == three[i]);
    }
    target = 999U;
    CHECK(!ccr_steps_target(CCR_STEPS_3, 4U, &target));
    CHECK(target == 999U);
    CHECK(!ccr_steps_target(CCR_STEPS_5, 255U, &target));
    CHECK(!ccr_steps_target((ccr_steps_t)4, 0U, &target));
    CHECK(!ccr_steps_target(CCR_STEPS_5, 0U, NULL));
}

/** @req REQ-CCR-004
 *  @req REQ-CCR-005
 *  @req REQ-CCR-016
 *  @req REQ-CCR-017
 */
static void test_lifecycle(void)
{
    ccr_ctrl_t ctrl = fixture();
    CHECK(ctrl.state == CCR_OFF);
    CHECK(!hal_sim_enabled());
    CHECK(ccr_ctrl_set_step(&ctrl, 1U));
    CHECK(ctrl.state == CCR_STARTUP);
    run_plant(&ctrl, 1U);
    CHECK(hal_sim_reference() == 100U);
    run_plant(&ctrl, 30U);
    CHECK(ctrl.state == CCR_REGULATING);
    CHECK(ctrl.reference_ma == 2800U);
    CHECK(ccr_ctrl_set_step(&ctrl, 5U));
    CHECK(ctrl.state == CCR_STEP_CHANGE);
    run_plant(&ctrl, 50U);
    CHECK(ctrl.state == CCR_REGULATING);
    CHECK(ctrl.reference_ma == 6600U);
    CHECK(ccr_ctrl_set_step(&ctrl, 2U));
    run_plant(&ctrl, 1U);
    CHECK(ctrl.reference_ma == 6500U);
    run_plant(&ctrl, 40U);
    CHECK(ctrl.reference_ma == 3400U);
    CHECK(ctrl.state == CCR_REGULATING);
    CHECK(!ccr_ctrl_set_step(&ctrl, 6U));
    CHECK(ctrl.target_ma == 3400U);
    CHECK(ccr_ctrl_set_step(&ctrl, 0U));
    CHECK(ctrl.state == CCR_OFF);
    CHECK(!hal_sim_enabled());
    CHECK(hal_sim_reference() == 0U);
}

/** @req REQ-CCR-006
 *  @req REQ-CCR-009
 */
static void test_overcurrent_latch(void)
{
    ccr_ctrl_t ctrl = fixture();
    CHECK(ccr_ctrl_set_step(&ctrl, 1U));
    run_plant(&ctrl, 40U);
    hal_sim_sample(7201U, 1000U, true);
    ccr_ctrl_tick(&ctrl);
    CHECK(ctrl.state == CCR_FAULT);
    CHECK(ctrl.faults == CCR_FAULT_OVERCURRENT);
    CHECK(!hal_sim_enabled());
    CHECK(!ccr_ctrl_set_step(&ctrl, 1U));
    CHECK(!ccr_ctrl_reset_fault(&ctrl));
    hal_sim_sample(0U, 0U, true);
    ccr_ctrl_tick(&ctrl);
    CHECK(ctrl.state == CCR_FAULT);
    CHECK(ccr_ctrl_reset_fault(&ctrl));
    CHECK(ctrl.state == CCR_OFF);
    CHECK(ctrl.faults == 0U);
    CHECK(!hal_sim_enabled());
    CHECK(!ccr_ctrl_reset_fault(&ctrl));
}

/** @req REQ-CCR-006
 *  @req REQ-CCR-007
 *  @req REQ-CCR-010
 */
static void test_protection_boundaries(void)
{
    ccr_protect_t protection;
    uint16_t faults;
    ccr_protect_init(&protection);
    faults = ccr_protect_update(&protection,
        (ccr_sample_t){7200U, 1000U, true}, 6600U, true, false);
    CHECK(faults == 0U);
    for (unsigned i = 0U; i < 9U; ++i) {
        faults = ccr_protect_update(&protection,
            (ccr_sample_t){0U, 3000U, true}, 2800U, true, false);
        CHECK(faults == 0U);
    }
    faults = ccr_protect_update(&protection,
        (ccr_sample_t){500U, 3000U, true}, 2800U, true, false);
    CHECK(faults == 0U);
    CHECK(protection.open_ms == 0U);
    for (unsigned i = 0U; i < 10U; ++i) {
        faults = ccr_protect_update(&protection,
            (ccr_sample_t){499U, 3000U, true}, 2800U, true, false);
    }
    CHECK(faults == CCR_FAULT_OPEN_CIRCUIT);
    for (unsigned i = 0U; i < 7000U; ++i) {
        faults = ccr_protect_update(&protection,
            (ccr_sample_t){0U, 3000U, true}, 2800U, true, false);
    }
    CHECK(faults == CCR_FAULT_OPEN_CIRCUIT);
    CHECK(protection.open_ms == CCR_OPEN_DELAY_MS);
}

/** @req REQ-CCR-008 */
static void test_tolerance_continuous(void)
{
    ccr_protect_t protection;
    uint16_t faults = 0U;
    ccr_protect_init(&protection);
    for (unsigned i = 0U; i < 200U; ++i) {
        faults = ccr_protect_update(&protection,
            (ccr_sample_t){2700U, 1000U, true}, 2800U, true, true);
        CHECK(faults == 0U);
    }
    for (unsigned i = 0U; i < 99U; ++i) {
        faults = ccr_protect_update(&protection,
            (ccr_sample_t){2699U, 1000U, true}, 2800U, true, true);
        CHECK(faults == 0U);
    }
    faults = ccr_protect_update(&protection,
        (ccr_sample_t){2699U, 1000U, true}, 2800U, true, true);
    CHECK(faults == CCR_FAULT_TOLERANCE);
    CHECK(protection.tolerance_ms == CCR_TOL_DELAY_MS);
}

/** @req REQ-CCR-011 */
static void test_startup_timeout(void)
{
    ccr_ctrl_t ctrl = fixture();
    CHECK(ccr_ctrl_set_step(&ctrl, 1U));
    hal_sim_sample(0U, 0U, true);
    for (unsigned i = 0U; i < 299U; ++i) {
        ccr_ctrl_tick(&ctrl);
        CHECK(ctrl.state == CCR_STARTUP);
    }
    ccr_ctrl_tick(&ctrl);
    CHECK(ctrl.state == CCR_FAULT);
    CHECK(ctrl.faults == CCR_FAULT_STARTUP);
    CHECK(!hal_sim_enabled());
}

static bool send_frame(ccr_comm_t *comm, ccr_ctrl_t *ctrl,
                       uint8_t command, uint8_t data, uint8_t reserved,
                       bool corrupt, uint8_t *response)
{
    uint8_t frame[5] = {0xA5U, command, data, reserved, 0U};
    bool ready = false;
    frame[4] = (uint8_t)(frame[0] + frame[1] + frame[2] + frame[3]);
    if (corrupt) {
        frame[4] ^= 1U;
    }
    for (size_t i = 0U; i < sizeof(frame); ++i) {
        ready = ccr_comm_feed(comm, ctrl, frame[i], response);
        if (i < 4U) {
            CHECK(!ready);
        }
    }
    return ready;
}

/** @req REQ-CCR-012
 *  @req REQ-CCR-013
 *  @req REQ-CCR-014
 *  @req REQ-CCR-015
 */
static void test_protocol(void)
{
    ccr_ctrl_t ctrl = fixture();
    ccr_comm_t comm;
    uint8_t reply[5] = {0};
    ccr_comm_init(&comm);
    CHECK(!ccr_comm_feed(&comm, &ctrl, 0x77U, reply));
    CHECK(!send_frame(&comm, &ctrl, 0x10U, 1U, 0U, true, reply));
    CHECK(ctrl.state == CCR_OFF);
    CHECK(send_frame(&comm, &ctrl, 0x10U, 1U, 1U, false, reply));
    CHECK(reply[2] == 1U);
    CHECK(ctrl.state == CCR_OFF);
    CHECK(send_frame(&comm, &ctrl, 0x10U, 1U, 0U, false, reply));
    CHECK(reply[1] == 0x90U);
    CHECK(reply[2] == 0U);
    CHECK(ctrl.state == CCR_STARTUP);
    run_plant(&ctrl, 40U);
    CHECK(send_frame(&comm, &ctrl, 0x20U, 0U, 0U, false, reply));
    CHECK(reply[1] == 0xA0U);
    CHECK(reply[2] == CCR_REGULATING);
    CHECK(reply[3] == 0U);
    CHECK(reply[4] == (uint8_t)(0xA5U + 0xA0U + CCR_REGULATING));
    CHECK(send_frame(&comm, &ctrl, 0x7FU, 0U, 0U, false, reply));
    CHECK(reply[2] == 2U);
    hal_sim_sample(8000U, 1000U, true);
    ccr_ctrl_tick(&ctrl);
    CHECK(send_frame(&comm, &ctrl, 0x30U, 0U, 0U, false, reply));
    CHECK(reply[2] == 1U);
    hal_sim_sample(0U, 0U, true);
    CHECK(send_frame(&comm, &ctrl, 0x30U, 0U, 0U, false, reply));
    CHECK(reply[2] == 0U);
    CHECK(ctrl.state == CCR_OFF);
}

/** @req REQ-CCR-019 */
static void test_resynchronization(void)
{
    ccr_ctrl_t ctrl = fixture();
    ccr_comm_t comm;
    uint8_t reply[5];
    /* Partial old frame followed by a new complete frame. */
    const uint8_t stream[] = {0xA5U, 0x10U, 0xA5U, 0x10U, 1U, 0U, 0xB6U};
    ccr_comm_init(&comm);
    for (size_t i = 0U; i < sizeof(stream); ++i) {
        bool ready = ccr_comm_feed(&comm, &ctrl, stream[i], reply);
        CHECK(ready == (i == sizeof(stream) - 1U));
    }
    CHECK(ctrl.requested_step == 1U);
    CHECK(send_frame(&comm, &ctrl, 0x7FU, 0xA5U, 0U, false, reply));
    CHECK(reply[2] == 2U);
}

int main(void)
{
    test_steps();
    test_lifecycle();
    test_overcurrent_latch();
    test_protection_boundaries();
    test_tolerance_continuous();
    test_startup_timeout();
    test_protocol();
    test_resynchronization();
    printf("PASS: 8 test groups, %u checks\n", checks);
    return EXIT_SUCCESS;
}
