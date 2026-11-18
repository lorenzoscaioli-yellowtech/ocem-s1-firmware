#include "ccr.h"

#define FRAME_SOF 0xA5U
#define CMD_SET_STEP 0x10U
#define CMD_STATUS 0x20U
#define CMD_RESET 0x30U
#define REPLY_OK 0U
#define REPLY_REJECTED 1U
#define REPLY_UNSUPPORTED 2U

/** @req REQ-CCR-012
 * Frame checksum is the XOR of the four preceding bytes.
 */
static uint8_t frame_checksum(const uint8_t *frame)
{
    uint8_t checksum = 0U;
    size_t i;
    for (i = 0U; i < (CCR_FRAME_SIZE - 1U); ++i) {
        checksum = (uint8_t)(checksum + frame[i]);
    }
    return checksum;
}

/** @req REQ-CCR-012 */
void ccr_comm_init(ccr_comm_t *comm)
{
    *comm = (ccr_comm_t){0};
}

/** @req REQ-CCR-013
 *  @req REQ-CCR-014
 *  @req REQ-CCR-015
 */
static void dispatch(ccr_ctrl_t *ctrl, const uint8_t *request, uint8_t *response)
{
    uint8_t result = REPLY_OK;
    response[0] = FRAME_SOF;
    response[1] = (uint8_t)(request[1] | 0x80U);
    response[3] = 0U;
    switch (request[1]) {
    case CMD_SET_STEP:
        if ((request[3] != 0U) || !ccr_ctrl_set_step(ctrl, request[2])) {
            result = REPLY_REJECTED;
        }
        break;
    case CMD_STATUS:
        if ((request[2] != 0U) || (request[3] != 0U)) {
            result = REPLY_REJECTED;
        } else {
            response[2] = (uint8_t)ctrl->state;
            response[3] = (uint8_t)ctrl->faults;
            response[4] = frame_checksum(response);
            return;
        }
        break;
    case CMD_RESET:
        if ((request[2] != 0U) || (request[3] != 0U)
            || !ccr_ctrl_reset_fault(ctrl)) {
            result = REPLY_REJECTED;
        }
        break;
    default:
        result = REPLY_UNSUPPORTED;
        break;
    }
    response[2] = result;
    response[4] = frame_checksum(response);
}

/** @req REQ-CCR-012
 *  @req REQ-CCR-019
 * Fixed-size streaming decoder. Invalid frames never reach dispatch.
 * A valid frame may contain SOF in its payload or checksum.
 */
bool ccr_comm_feed(ccr_comm_t *comm, ccr_ctrl_t *ctrl,
                   uint8_t byte, uint8_t response[CCR_FRAME_SIZE])
{
    size_t i;
    size_t remaining;
    if ((comm == NULL) || (ctrl == NULL) || (response == NULL)) {
        return false;
    }
    if ((comm->used == 0U) && (byte != FRAME_SOF)) {
        return false;
    }
    comm->bytes[comm->used] = byte;
    ++comm->used;
    if (comm->used < CCR_FRAME_SIZE) {
        return false;
    }
    if (frame_checksum(comm->bytes) == comm->bytes[4]) {
        comm->used = 0U;
        dispatch(ctrl, comm->bytes, response);
        return true;
    }
    /* Retain a candidate header already received inside a corrupt frame. */
    comm->used = 0U;
    for (i = 1U; i < CCR_FRAME_SIZE; ++i) {
        if (comm->bytes[i] == FRAME_SOF) {
            remaining = CCR_FRAME_SIZE - i;
            for (size_t j = 0U; j < remaining; ++j) {
                comm->bytes[j] = comm->bytes[i + j];
            }
            comm->used = (uint8_t)remaining;
            break;
        }
    }
    return false;
}
