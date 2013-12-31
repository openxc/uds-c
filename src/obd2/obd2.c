#include <obd2/obd2.h>

DiagnosticShims diagnostic_init_shims(LogShim log,
        SendCanMessageShim send_can_message,
        SetTimerShim set_timer) {
    DiagnosticShims shims = {
        isotp_shims: {
            log: log,
            send_can_message: send_can_message,
            set_timer: set_timer
        },
        log: log
    };
    return shims;
}

DiagnosticRequestHandle diagnostic_request(DiagnosticShims* shims,
        DiagnosticRequest* request, DiagnosticResponseReceived callback) {
    // TODO hmm, where is message_received coming from? we would need 2 layers
    // of callbacks. if we do it in the obd2 library, we have to have some
    // context passed to that message_received handler so we can then retreive
    // the obd2 callback. there was an option question of if we should pass a
    // context with that callback, and maybe this answers it.
    //
    // alternatively, what if don't hide isotp and allow that to also be
    // injected. the user has the iso_tp message_received callback, and in that
    // they call a message_received handler from obd2.
    //
    // in fact that makes more sense - all the diagnostic_can_frame_received
    // function is going to be able to do is call the equivalent function in the
    // isotp library. it may or may not have a complete ISO-TP message. huh.
    DiagnosticRequestHandle handle = {
        // TODO why are teh shims stored as a reference in the isotp handler?
        // it's just 3 pointers
        isotp_handler: isotp_init(&shims->isotp_shims, request->arbitration_id,
               NULL, // TODO need a callback here!
               NULL, NULL),
        type: 0 //DIAGNOSTIC_.... // TODO how would we know the type?
            //does it matter? we were going to have a different callback
    };
}

DiagnosticRequestHandle diagnostic_request_pid(DiagnosticShims* shims,
        DiagnosticPidRequestType pid_request_type, uint16_t pid,
        DiagnosticResponseReceived callback) {
    // decide mode 0x1 / 0x22 based on pid type
    DiagnosticRequest request = {
        mode: pid_request_type == DIAGNOSTIC_STANDARD_PID ? 0x1 : 0x22,
        pid: pid
    };

    return diagnostic_request(shims, &request, callback);
}

void diagnostic_receive_can_frame(DiagnosticRequestHandle* handler,
        const uint16_t arbitration_id, const uint8_t data[],
        const uint8_t size) {
    isotp_receive_can_frame(handler->isotp_handler, arbitration_id, data, size);
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
