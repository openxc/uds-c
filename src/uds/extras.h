#ifndef __EXTRAS_H__
#define __EXTRAS_H__

#include <uds/uds_types.h>

#ifdef __cplusplus
extern "C" {
#endif

// TODO everything in here is unused for the moment!

typedef enum {
    POWERTRAIN = 0x0,
    CHASSIS = 0x1,
    BODY = 0x2,
    NETWORK = 0x3
} DiagnosticTroubleCodeGroup;

typedef struct {
    DiagnosticTroubleCodeGroup group;
    uint8_t group_num;
    uint8_t code;
} DiagnosticTroubleCode;


/* Private: TODO unused for now
 */
typedef enum {
    DTC_EMISSIONS,
    DTC_DRIVE_CYCLE,
    DTC_PERMANENT
} DiagnosticTroubleCodeType;


// TODO should we enumerate every OBD-II PID? need conversion formulas, too
typedef struct {
    uint16_t pid;
    uint8_t bytes_returned;
    float min_value;
    float max_value;
} DiagnosticParameter;

typedef void (*DiagnosticMilStatusReceived)(bool malfunction_indicator_status);
typedef void (*DiagnosticVinReceived)(uint8_t vin[]);
typedef void (*DiagnosticTroubleCodesReceived)(
        DiagnosticMode mode, DiagnosticTroubleCode* codes);
typedef void (*DiagnosticPidEnumerationReceived)(
        const DiagnosticResponse* response, uint16_t* pids);

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
