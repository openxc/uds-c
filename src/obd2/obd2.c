#include <obd2/obd2.h>

#define MAX_DIAGNOSTIC_PAYLOAD_SIZE 6
#define MODE_BYTE_INDEX 0
#define PID_BYTE_INDEX 1

DiagnosticShims diagnostic_init_shims(LogShim log,
        SendCanMessageShim send_can_message,
        SetTimerShim set_timer) {
    DiagnosticShims shims = {
        send_can_message: send_can_message,
        set_timer: set_timer,
        log: log
    };
    return shims;
}

DiagnosticRequestHandle diagnostic_request(DiagnosticShims* shims,
        DiagnosticRequest* request, DiagnosticResponseReceived callback) {
    DiagnosticRequestHandle handle = {
        type: DIAGNOSTIC_REQUEST_TYPE_PID,
        callback: callback,
        status: true
    };
    uint8_t payload[MAX_DIAGNOSTIC_PAYLOAD_SIZE];
    payload[MODE_BYTE_INDEX] = request->mode;
    if(request->pid_length > 0) {
        copy_bytes_right_aligned(request->pid, sizeof(request->pid),
                PID_BYTE_INDEX, request->pid_length, payload, sizeof(payload));
    }
    if(request->payload_length > 0) {
        memcpy(payload[PID_BYTE_INDEX + request->pid_length],
                request->payload, request->payload_length);
    }

    IsoTpShims isotp_shims = isotp_init_shims(shims->log,
            shims->send_can_message,
            shims->set_timer);
    handle.status = isotp_send(&isotp_shims, request->arbitration_id,
            payload, 1 + request->payload_length + request->pid_length,
            diagnostic_receive_isotp_message);

    // TODO need to set up an isotp receive handler. in isotp, rx and tx are
    // kind of intermingled at this point. really, there's not explicit link
    // between send and receveice...well except for flow control. hm, damn.
    // so there's 2 things:
    //
    // isotp_send needs to return a handle. if it was a single frame, we
    // probably sent it right away so the status true and the callback was hit.
    // the handle needs another flag to say if it was completed or not, so you
    // know you can destroy it. you will continue to throw can frames at that
    // handler until it returns completed (either with a  flag, or maybe
    // receive_can_frame returns true if it's complete)
    //
    // the second thing is that we need to be able to arbitrarly set up to
    // receive an iso-tp message on a particular arb id. again, you keep
    // throwing can frames at it until it returns a handle with the status
    // completed and calls your callback
    //
    // so the diagnostic request needs 2 isotp handles and they should both be
    // hidden from the user
    //
    // when a can frame is received and passes to the diagnostic handle
    // if we haven't successfuly sent the entire message yet, give it to the
    // isottp send handle
    // if we have sent it, give it to the isotp rx handle
    // if we've received properly, mark this request as completed
    return handle;
}

DiagnosticRequestHandle diagnostic_request_pid(DiagnosticShims* shims,
        DiagnosticPidRequestType pid_request_type, uint16_t pid,
        DiagnosticResponseReceived callback) {
    DiagnosticRequest request = {
        mode: pid_request_type == DIAGNOSTIC_STANDARD_PID ? 0x1 : 0x22,
        pid: pid
    };

    return diagnostic_request(shims, &request, callback);
}

void diagnostic_receive_isotp_message(const IsoTpMessage* message) {
    // TODO
}

void diagnostic_receive_can_frame(DiagnosticRequestHandle* handle,
        const uint16_t arbitration_id, const uint8_t data[],
        const uint8_t size) {
    isotp_receive_can_frame(handle->isotp_handler, arbitration_id, data, size);
}

// TODO argh, we're now saying that user code will rx CAN messages, but who does
// it hand them to? isotp handlers are encapsulated in diagnostic handles



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
