#include <uds/uds.h>
#include <bitfield/bitfield.h>
#include <canutil/read.h>
#include <string.h>
#include <limits.h>
#include <stddef.h>
#include <sys/param.h>
#include <inttypes.h>

#define ARBITRATION_ID_OFFSET 0x8
#define MODE_RESPONSE_OFFSET 0x40
#define NEGATIVE_RESPONSE_MODE 0x7f
#define MAX_DIAGNOSTIC_PAYLOAD_SIZE 6
#define MODE_BYTE_INDEX 0
#define PID_BYTE_INDEX 1
#define NEGATIVE_RESPONSE_MODE_INDEX 1
#define NEGATIVE_RESPONSE_NRC_INDEX 2

#ifndef MAX
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

DiagnosticShims diagnostic_init_shims(LogShim log,
        SendCanMessageShim send_can_message,
        SetTimerShim set_timer) {
    DiagnosticShims shims = {
        log: log,
        send_can_message: send_can_message,
        set_timer: set_timer
    };
    return shims;
}

static void setup_receive_handle(DiagnosticRequestHandle* handle) {
    if(handle->request.arbitration_id == OBD2_FUNCTIONAL_BROADCAST_ID) {
        uint32_t response_id;
        for(response_id = 0;
                response_id < OBD2_FUNCTIONAL_RESPONSE_COUNT; ++response_id) {
            handle->isotp_receive_handles[response_id] = isotp_receive(
                    &handle->isotp_shims,
                    OBD2_FUNCTIONAL_RESPONSE_START + response_id,
                    NULL);
        }
        handle->isotp_receive_handle_count = OBD2_FUNCTIONAL_RESPONSE_COUNT;
    } else {
        handle->isotp_receive_handle_count = 1;
        handle->isotp_receive_handles[0] = isotp_receive(&handle->isotp_shims,
                handle->request.arbitration_id + ARBITRATION_ID_OFFSET,
                NULL);
    }
}

static uint16_t autoset_pid_length(uint8_t mode, uint16_t pid,
        uint8_t pid_length) {
    if(pid_length == 0) {
        if(mode <= 0xa || mode == 0x3e ) {
            pid_length = 1;
        } else if(pid > 0xffff || ((pid & 0xFF00) > 0x0)) {
            pid_length = 2;
        } else {
            pid_length = 1;
        }
    }
    return pid_length;
}

static void send_diagnostic_request(DiagnosticShims* shims,
        DiagnosticRequestHandle* handle) {
    uint8_t payload[MAX_DIAGNOSTIC_PAYLOAD_SIZE] = {0};
    payload[MODE_BYTE_INDEX] = handle->request.mode;
    if(handle->request.has_pid) {
        handle->request.pid_length = autoset_pid_length(handle->request.mode,
                handle->request.pid, handle->request.pid_length);
        set_bitfield(handle->request.pid, PID_BYTE_INDEX * CHAR_BIT,
                handle->request.pid_length * CHAR_BIT, payload,
                sizeof(payload));
    }

    if(handle->request.payload_length > 0) {
        memcpy(&payload[PID_BYTE_INDEX + handle->request.pid_length],
                handle->request.payload, handle->request.payload_length);
    }

    handle->isotp_send_handle = isotp_send(&handle->isotp_shims,
            handle->request.arbitration_id, payload,
            1 + handle->request.payload_length + handle->request.pid_length,
            NULL);
    if(handle->isotp_send_handle.completed &&
            !handle->isotp_send_handle.success) {
        handle->completed = true;
        handle->success = false;
        if(shims->log != NULL) {
            shims->log("%s", "Diagnostic request not sent");
        }
    } else if(shims->log != NULL) {
        char request_string[128] = {0};
        diagnostic_request_to_string(&handle->request, request_string,
                sizeof(request_string));
        shims->log("Sending diagnostic request: %s", request_string);
    }
}

bool diagnostic_request_sent(DiagnosticRequestHandle* handle) {
    return handle->isotp_send_handle.completed;
}

void start_diagnostic_request(DiagnosticShims* shims,
        DiagnosticRequestHandle* handle) {
    handle->success = false;
    handle->completed = false;
    send_diagnostic_request(shims, handle);
    if(!handle->completed) {
        setup_receive_handle(handle);
    }
}

DiagnosticRequestHandle generate_diagnostic_request(DiagnosticShims* shims,
        DiagnosticRequest* request, DiagnosticResponseReceived callback) {
    DiagnosticRequestHandle handle = {
        request: *request,
        callback: callback,
        success: false,
        completed: false
    };

    handle.isotp_shims = isotp_init_shims(shims->log,
            shims->send_can_message,
            shims->set_timer);
    handle.isotp_shims.frame_padding = !request->no_frame_padding;

    return handle;
    // TODO notes on multi frame:
    // TODO what are the timers for exactly?
    //
    // when sending multi frame, send 1 frame, wait for a response
    // if it says send all, send all right away
    // if it says flow control, set the time for the next send
    // instead of creating a timer with an async callback, add a process_handle
    // function that's called repeatedly in the main loop - if it's time to
    // send, we do it. so there's a process_handle_send and receive_can_frame
    // that are just called continuously from the main loop. it's a waste of a
    // few cpu cycles but it may be more  natural than callbacks.
    //
    // what would a timer callback look like...it would need to pass the handle
    // and that's all. seems like a context void* would be able to capture all
    // of the information but arg, memory allocation. look at how it's done in
    // the other library again
    //
}

DiagnosticRequestHandle diagnostic_request(DiagnosticShims* shims,
        DiagnosticRequest* request, DiagnosticResponseReceived callback) {
    DiagnosticRequestHandle handle = generate_diagnostic_request(
            shims, request, callback);
    start_diagnostic_request(shims, &handle);
    return handle;
}

DiagnosticRequestHandle diagnostic_request_pid(DiagnosticShims* shims,
        DiagnosticPidRequestType pid_request_type, uint32_t arbitration_id,
        uint16_t pid, DiagnosticResponseReceived callback) {
    DiagnosticRequest request = {
        arbitration_id: arbitration_id,
        mode: pid_request_type == DIAGNOSTIC_STANDARD_PID ? 0x1 : 0x22,
        has_pid: true,
        pid: pid
    };

    return diagnostic_request(shims, &request, callback);
}

static bool handle_negative_response(IsoTpMessage* message,
        DiagnosticResponse* response, DiagnosticShims* shims) {
    bool response_was_negative = false;
    if(response->mode == NEGATIVE_RESPONSE_MODE) {
        response_was_negative = true;
        if(message->size > NEGATIVE_RESPONSE_MODE_INDEX) {
            response->mode = message->payload[NEGATIVE_RESPONSE_MODE_INDEX];
        }

        if(message->size > NEGATIVE_RESPONSE_NRC_INDEX) {
            response->negative_response_code =
                    message->payload[NEGATIVE_RESPONSE_NRC_INDEX];
        }

        response->success = false;
        response->completed = true;
    }
    return response_was_negative;
}

static bool handle_positive_response(DiagnosticRequestHandle* handle,
        IsoTpMessage* message, DiagnosticResponse* response,
        DiagnosticShims* shims) {
    bool response_was_positive = false;
    if(response->mode == handle->request.mode + MODE_RESPONSE_OFFSET) {
        response_was_positive = true;
        // hide the "response" version of the mode from the user
        // if it matched
        response->mode = handle->request.mode;
        response->has_pid = false;
        if(handle->request.has_pid && message->size > 1) {
            response->has_pid = true;
            if(handle->request.pid_length == 2) {
                response->pid = get_bitfield(message->payload, message->size,
                        PID_BYTE_INDEX * CHAR_BIT, sizeof(uint16_t) * CHAR_BIT);
            } else {
                response->pid = message->payload[PID_BYTE_INDEX];
            }

        }

        if((!handle->request.has_pid && !response->has_pid)
                || response->pid == handle->request.pid) {
            response->success = true;
            response->completed = true;

            uint8_t payload_index = 1 + handle->request.pid_length;
            response->payload_length = MAX(0, message->size - payload_index);
            if(response->payload_length > 0) {
                memcpy(response->payload, &message->payload[payload_index],
                        response->payload_length);
            }
        } else {
            response_was_positive = false;
        }
    }
    return response_was_positive;
}

DiagnosticResponse diagnostic_receive_can_frame(DiagnosticShims* shims,
        DiagnosticRequestHandle* handle, const uint32_t arbitration_id,
        const uint8_t data[], const uint8_t size) {

    DiagnosticResponse response = {
        arbitration_id: arbitration_id,
        multi_frame: false,
        success: false,
        completed: false
    };

    if(!handle->isotp_send_handle.completed) {
        isotp_continue_send(&handle->isotp_shims,
                &handle->isotp_send_handle, arbitration_id, data, size);
    } else {
        uint8_t i;
        for(i = 0; i < handle->isotp_receive_handle_count; ++i) {
            IsoTpMessage message = isotp_continue_receive(&handle->isotp_shims,
                    &handle->isotp_receive_handles[i], arbitration_id, data,
                    size);
            response.multi_frame = message.multi_frame;

            if(message.completed) {
                if(message.size > 0) {
                    response.mode = message.payload[0];
                    if(handle_negative_response(&message, &response, shims) ||
                            handle_positive_response(handle, &message,
                                &response, shims)) {
                        if(shims->log != NULL) {
                            char response_string[128] = {0};
                            diagnostic_response_to_string(&response,
                                    response_string, sizeof(response_string));
                            shims->log("Diagnostic response received: %s",
                                    response_string);
                        }

                        handle->success = true;
                        handle->completed = true;
                    }
                } else {
                    if(shims->log != NULL) {
                        shims->log("Received an empty response on arb ID 0x%x",
                                response.arbitration_id);
                    }
                }

                if(handle->completed && handle->callback != NULL) {
                    handle->callback(&response);
                }

                break;
            }
        }
    }
    return response;
}

int diagnostic_payload_to_integer(const DiagnosticResponse* response) {
    return get_bitfield(response->payload, response->payload_length, 0,
            response->payload_length * CHAR_BIT);
}

float diagnostic_decode_obd2_pid(const DiagnosticResponse* response) {
    // handles on the single number values, not the bit encoded ones
    switch(response->pid) {
        case 0xa:
            return response->payload[0] * 3;
        case 0xc:
            return (response->payload[0] * 256 + response->payload[1]) / 4.0;
        case 0xd:
        case 0x33:
        case 0xb:
            return response->payload[0];
        case 0x10:
            return (response->payload[0] * 256 + response->payload[1]) / 100.0;
        case 0x11:
        case 0x2f:
        case 0x45:
        case 0x4c:
        case 0x52:
        case 0x5a:
        case 0x4:
            return response->payload[0] * 100.0 / 255.0;
        case 0x46:
        case 0x5c:
        case 0xf:
        case 0x5:
            return response->payload[0] - 40;
        case 0x62:
            return response->payload[0] - 125;
        default:
            return diagnostic_payload_to_integer(response);
    }
}

void diagnostic_response_to_string(const DiagnosticResponse* response,
        char* destination, size_t destination_length) {
    int bytes_used = snprintf(destination, destination_length,
            "arb_id: 0x%lx, mode: 0x%x, ",
            (unsigned long) response->arbitration_id,
            response->mode);

    if(response->has_pid) {
        bytes_used += snprintf(destination + bytes_used,
                destination_length - bytes_used,
                "pid: 0x%x, ",
                response->pid);
    }

    if(!response->success) {
        bytes_used += snprintf(destination + bytes_used,
                destination_length - bytes_used,
                "nrc: 0x%x, ",
                response->negative_response_code);
    }

    if(response->payload_length > 0) {
        snprintf(destination + bytes_used, destination_length - bytes_used,
                "payload: 0x%02x%02x%02x%02x%02x%02x%02x",
                response->payload[0],
                response->payload[1],
                response->payload[2],
                response->payload[3],
                response->payload[4],
                response->payload[5],
                response->payload[6]);
    } else {
        snprintf(destination + bytes_used, destination_length - bytes_used,
                "no payload");
    }
}

void diagnostic_request_to_string(const DiagnosticRequest* request,
        char* destination, size_t destination_length) {
    int bytes_used = snprintf(destination, destination_length,
            "arb_id: 0x%lx, mode: 0x%x, ",
            (unsigned long) request->arbitration_id,
            request->mode);

    if(request->has_pid) {
        bytes_used += snprintf(destination + bytes_used,
                destination_length - bytes_used,
                "pid: 0x%x, ",
                request->pid);
    }

    int remaining_space = destination_length - bytes_used;
    if(request->payload_length > 0) {
        snprintf(destination + bytes_used, remaining_space,
                "payload: 0x%02x%02x%02x%02x%02x%02x%02x",
                request->payload[0],
                request->payload[1],
                request->payload[2],
                request->payload[3],
                request->payload[4],
                request->payload[5],
                request->payload[6]);
    } else {
        snprintf(destination + bytes_used, remaining_space, "no payload");
    }
}

bool diagnostic_request_equals(const DiagnosticRequest* ours,
        const DiagnosticRequest* theirs) {
    bool equals = ours->arbitration_id == theirs->arbitration_id &&
        ours->mode == theirs->mode;
    equals &= ours->has_pid == theirs->has_pid;
    equals &= ours->pid == theirs->pid;
    return equals;
}
