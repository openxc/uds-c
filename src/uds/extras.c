#include <uds/extras.h>
#include <uds/uds.h>

// TODO everything below here is for future work...not critical for now.

DiagnosticRequestHandle diagnostic_request_malfunction_indicator_status(
        DiagnosticShims* shims,
        DiagnosticMilStatusReceived callback) {
    // TODO request malfunction indicator light (MIL) status - request mode 1
    // pid 1, parse first bit
    DiagnosticRequestHandle handle;
    return handle;
}

DiagnosticRequestHandle diagnostic_request_vin(DiagnosticShims* shims,
        DiagnosticVinReceived callback) {
    DiagnosticRequestHandle handle;
    return handle;
}

DiagnosticRequestHandle diagnostic_request_dtc(DiagnosticShims* shims,
        DiagnosticTroubleCodeType dtc_type,
        DiagnosticTroubleCodesReceived callback) {
    DiagnosticRequestHandle handle;
    return handle;
}

bool diagnostic_clear_dtc(DiagnosticShims* shims) {
    return false;
}

DiagnosticRequestHandle diagnostic_enumerate_pids(DiagnosticShims* shims,
        DiagnosticRequest* request, DiagnosticPidEnumerationReceived callback) {
    // before calling the callback, split up the received bytes into 1 or 2 byte
    // chunks depending on the mode so the final pid list is actual 1 or 2 byte PIDs
    // TODO request supported PIDs  - request PID 0 and parse 4 bytes in response
    DiagnosticRequestHandle handle;
    return handle;
}
