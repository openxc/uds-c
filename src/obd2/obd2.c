#include <obd2/obd2.h>

DiagnosticShims diagnostic_init_shims(LogShim log,
        SendCanMessageShim send_can_message,
        SetTimerShim set_timer) {
}

DiagnosticRequestHandle diagnostic_request(DiagnosticShims* shims,
        DiagnosticRequest* request, DiagnosticResponseReceived callback) {
}

// decide mode 0x1 / 0x22 based on pid type
DiagnosticRequestHandle diagnostic_request_pid(DiagnosticShims* shims,
        DiagnosticPidRequestType pid_request_type, uint16_t pid,
        DiagnosticResponseReceived callback) {
}

// TODO request malfunction indicator light (MIL) status - request mode 1 pid 1,
//      parse first bit
DiagnosticRequestHandle diagnostic_request_malfunction_indicator_status(
        DiagnosticShims* shims,
        DiagnosticMilStatusReceived callback) {
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

// before calling the callback, split up the received bytes into 1 or 2 byte
// chunks depending on the mode so the final pid list is actual 1 or 2 byte PIDs
// TODO request supported PIDs  - request PID 0 and parse 4 bytes in response
DiagnosticRequestHandle diagnostic_enumerate_pids(DiagnosticShims* shims,
        DiagnosticRequest* request, DiagnosticPidEnumerationReceived callback) {
}

void diagnostic_receive_can_frame(DiagnosticRequestHandle* handler,
        const uint16_t arbitration_id, const uint8_t data[],
        const uint8_t size) {
}
