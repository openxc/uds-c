#ifndef __EXTRAS_H__
#define __EXTRAS_H__

#include <isotp/isotp.h>

#ifdef __cplusplus
extern "C" {
#endif

DiagnosticRequestHandle diagnostic_request_malfunction_indicator_status(
        DiagnosticShims* shims,
        DiagnosticMilStatusReceived callback);

DiagnosticRequestHandle diagnostic_request_vin(DiagnosticShims* shims,
        DiagnosticVinReceived callback);

DiagnosticRequestHandle diagnostic_request_dtc(DiagnosticShims* shims,
        DiagnosticTroubleCodeType dtc_type,
        DiagnosticTroubleCodesReceived callback);

bool diagnostic_clear_dtc(DiagnosticShims* shims);

DiagnosticRequestHandle diagnostic_enumerate_pids(DiagnosticShims* shims,
        DiagnosticRequest* request, DiagnosticPidEnumerationReceived callback);

#ifdef __cplusplus
}
#endif

#endif // __EXTRAS_H__
