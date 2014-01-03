#include <obd2/obd2.h>
#include <arpa/inet.h>

#define MODE_RESPONSE_OFFSET 0x40
#define NEGATIVE_RESPONSE_MODE 0x7f
#define MAX_DIAGNOSTIC_PAYLOAD_SIZE 6
#define MODE_BYTE_INDEX 0
#define PID_BYTE_INDEX 1
#define NEGATIVE_RESPONSE_MODE_INDEX 1
#define NEGATIVE_RESPONSE_NRC_INDEX 2

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

DiagnosticRequestHandle diagnostic_request(DiagnosticShims* shims,
        DiagnosticRequest* request, DiagnosticResponseReceived callback) {
    DiagnosticRequestHandle handle = {
        // TODO can we copy the request?
        request: *request,
        callback: callback,
        success: false,
        completed: false
    };

    uint8_t payload[MAX_DIAGNOSTIC_PAYLOAD_SIZE];
    payload[MODE_BYTE_INDEX] = request->mode;
    if(request->pid_length > 0) {
        copy_bytes_right_aligned(&request->pid, sizeof(request->pid),
                PID_BYTE_INDEX, request->pid_length, payload, sizeof(payload));
    }
    if(request->payload_length > 0) {
        memcpy(&payload[PID_BYTE_INDEX + request->pid_length],
                request->payload, request->payload_length);
    }

    handle.isotp_shims = isotp_init_shims(shims->log,
            shims->send_can_message,
            shims->set_timer);
    handle.isotp_send_handle = isotp_send(&handle.isotp_shims,
            request->arbitration_id, payload,
            1 + request->payload_length + request->pid_length,
            NULL);

    handle.isotp_receive_handle = isotp_receive(&handle.isotp_shims,
            // TODO need to either always add 0x8 or let the user specify
            request->arbitration_id + 0x8,
            NULL);

    // when a can frame is received and passes to the diagnostic handle
    // if we haven't successfuly sent the entire message yet, give it to the
    // isottp send handle
    // if we have sent it, give it to the isotp rx handle
    // if we've received properly, mark this request as completed
    // how do you get the result? we have it set up for callbacks but that's
    // getting to be kind of awkward
    //
    // say it were to call a callback...what state would it need to pass?
    //
    // the iso-tp message received callback needs to pass the handle and the
    // received message
    //
    // so in the obd2 library, we get a callback with an isotp message. how do
    // we know what diag request i went with, and which diag callback to use? we
    // could store context with the isotp handle. the problem is that context is
    // self referential and on the stack, so we really can't get a pointer to
    // it.
    //
    // the diagnostic response received callback needs to pass the diagnostic
    // handle and the diag response
    //
    // let's say we simplify things and drop the callbacks.
    //
    // isotp_receive_can_frame returns an isotp handle with a complete message
    // in it if one was received
    //
    // diagnostic_receive_can_frame returns a diaghandle if one was received
    //
    // so in the user code you would throw the can frame at each of your active
    // diag handles and see if any return a completed message.
    //
    // is there any advantage to a callback approach?  callbacks are useful when
    // you have something that will block, bt we don't have anything that will
    // block. it's async but we return immediately from each partial parsing
    // attempt.
    //
    // argh, but will we need the callbacks when we add timers for isotp multi
    // frame?
    //
    // what are the timers for exactly?
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
    // what woudl a timer callback look like...it would need to pass the handle
    // and that's all. seems like a context void* would be able to capture all
    // of the information but arg, memory allocation. look at how it's done in
    // the other library again
    //
    return handle;
}

DiagnosticRequestHandle diagnostic_request_pid(DiagnosticShims* shims,
        DiagnosticPidRequestType pid_request_type, uint16_t arbitration_id,
        uint16_t pid, DiagnosticResponseReceived callback) {
    DiagnosticRequest request = {
        arbitration_id: arbitration_id,
        mode: pid_request_type == DIAGNOSTIC_STANDARD_PID ? 0x1 : 0x22,
        pid: pid,
        pid_length: pid_request_type == DIAGNOSTIC_STANDARD_PID ? 1 : 2
    };

    return diagnostic_request(shims, &request, callback);
}

DiagnosticResponse diagnostic_receive_can_frame(DiagnosticShims* shims,
        DiagnosticRequestHandle* handle, const uint16_t arbitration_id,
        const uint8_t data[], const uint8_t size) {

    DiagnosticResponse response = {
        arbitration_id: arbitration_id,
        success: false,
        completed: false
    };

    if(!handle->isotp_send_handle.completed) {
        // TODO when completing a send, this returns...a Message? we have to
        // check when the isotp_send_handle is completed, and if it is, start
        isotp_continue_send(&handle->isotp_shims,
                &handle->isotp_send_handle, arbitration_id, data, size);
    } else if(!handle->isotp_receive_handle.completed) {
        IsoTpMessage message = isotp_continue_receive(&handle->isotp_shims,
                &handle->isotp_receive_handle, arbitration_id, data, size);

        if(message.completed) {
            if(message.size > 0) {
                response.mode = message.payload[0];
                if(response.mode == NEGATIVE_RESPONSE_MODE) {
                    if(message.size > NEGATIVE_RESPONSE_MODE_INDEX) {
                        // TODO we're setting the mode to the originating
                        // request for the error, so the user can confirm - i
                        // think this is OK since we're storing the failure
                        // status elsewhere, but think about it.
                        response.mode = message.payload[NEGATIVE_RESPONSE_MODE_INDEX];
                    }

                    if(message.size > NEGATIVE_RESPONSE_NRC_INDEX) {
                        response.negative_response_code = message.payload[NEGATIVE_RESPONSE_NRC_INDEX];
                    }
                    response.success = false;
                } else {
                    if(response.mode == handle->request.mode + MODE_RESPONSE_OFFSET) {
                        // hide the "response" version of the mode from the user
                        // if it matched
                        response.mode = handle->request.mode;
                        if(handle->request.pid_length > 0 && message.size > 1) {
                            if(handle->request.pid_length == 2) {
                                response.pid = *(uint16_t*)&message.payload[PID_BYTE_INDEX];
                                response.pid = ntohs(response.pid);
                            } else {
                                response.pid = message.payload[PID_BYTE_INDEX];
                            }
                        }

                        uint8_t payload_index = 1 + handle->request.pid_length;
                        response.payload_length = message.size - payload_index;
                        if(response.payload_length > 0) {
                            memcpy(response.payload, &message.payload[payload_index],
                                    response.payload_length);
                        }
                        response.success = true;
                    } else {
                        shims->log("Response was for a mode 0x%x request, not our mode 0x%x request",
                                response.mode - MODE_RESPONSE_OFFSET,
                                handle->request.mode);
                    }
                }
            }

            response.completed = true;
            // TODO what does it mean for the handle to be successful, vs. the
            // request to be successful? if we get a NRC, is that a successful
            // request?
            handle->success = true;
            handle->completed = true;

            if(handle->callback != NULL) {
                handle->callback(&response);
            }
        }

    } else {
        shims->log("Handle is already completed");
    }
    return response;
}

// TODO everything below here is for future work...not critical for now.

DiagnosticRequestHandle diagnostic_request_malfunction_indicator_status(
        DiagnosticShims* shims,
        DiagnosticMilStatusReceived callback) {
    // TODO request malfunction indicator light (MIL) status - request mode 1
    // pid 1, parse first bit
}

DiagnosticRequestHandle diagnostic_request_vin(DiagnosticShims* shims,
        DiagnosticVinReceived callback) {
}

DiagnosticRequestHandle diagnostic_request_dtc(DiagnosticShims* shims,
        DiagnosticTroubleCodeType dtc_type,
        DiagnosticTroubleCodesReceived callback) {
}

bool diagnostic_clear_dtc(DiagnosticShims* shims) {
}

DiagnosticRequestHandle diagnostic_enumerate_pids(DiagnosticShims* shims,
        DiagnosticRequest* request, DiagnosticPidEnumerationReceived callback) {
    // before calling the callback, split up the received bytes into 1 or 2 byte
    // chunks depending on the mode so the final pid list is actual 1 or 2 byte PIDs
    // TODO request supported PIDs  - request PID 0 and parse 4 bytes in response
}
