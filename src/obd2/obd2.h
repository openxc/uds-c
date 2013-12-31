#ifndef __OBD2_H__
#define __OBD2_H__

#include <isotp/isotp.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_OBD2_PAYLOAD_LENGTH 7
#define VIN_LENGTH 17

typedef struct {
    uint16_t arbitration_id;
    uint8_t mode;
    uint16_t pid;
    uint8_t payload[MAX_OBD2_PAYLOAD_LENGTH];
    uint8_t payload_length;
} DiagnosticRequest;

// Thanks to
// http://www.canbushack.com/blog/index.php?title=scanning-for-diagnostic-data&more=1&c=1&tb=1&pb=1
// for the list of NRCs
typedef enum {
    NRC_SUCCESS = 0x0,
    NRC_SERVICE_NOT_SUPPORTED = 0x11,
    NRC_SUB_FUNCTION_NOT_SUPPORTED = 0x12,
    NRC_CONDITIONS_NOT_CORRECT = 0x22,
    NRC_REQUEST_OUT_OF_RANGE = 0x31,
    NRC_SECURITY_ACCESS_DENIED = 0x33,
    NRC_INVALID_KEY = 0x35,
    NRC_TOO_MANY_ATTEMPS = 0x36,
    NRC_TIME_DELAY_NOT_EXPIRED = 0x37,
    NRC_RESPONSE_PENDING = 0x78
} DiagnosticNegativeResponseCode;

typedef enum {
    OBD2_MODE_POWERTRAIN_DIAGNOSTIC_REQUEST = 0x1,
    OBD2_MODE_POWERTRAIN_FREEZE_FRAME_REQUEST = 0x2,
    OBD2_MODE_EMISSIONS_DTC_REQUEST = 0x3,
    OBD2_MODE_EMISSIONS_DTC_CLEAR = 0x4,
    // 0x5 is for non-CAN only
    // OBD2_MODE_OXYGEN_SENSOR_TEST = 0x5,
    OBD2_MODE_TEST_RESULTS = 0x6,
    OBD2_MODE_DRIVE_CYCLE_DTC_REQUEST = 0x7,
    OBD2_MODE_CONTROL = 0x8,
    OBD2_MODE_VEHICLE_INFORMATION = 0x9,
    OBD2_MODE_PERMANENT_DTC_REQUEST = 0xa,
    // this one isn't technically in OBD2, but both of the enhanced standards
    // have their PID requests at 0x22
    OBD2_MODE_ENHANCED_DIAGNOSTIC_REQUEST = 0x22
} DiagnosticMode;

typedef enum {
    DTC_EMISSIONS,
    DTC_DRIVE_CYCLE,
    DTC_PERMANENT
} DiagnosticTroubleCodeType;

typedef struct {
    uint16_t arbitration_id;
    uint8_t mode;
    bool success;
    // if mode is one with a PID, read the correct numbers of PID bytes (1 or 2)
    // into this field, then store the remainder of the payload in the payload
    // field
    uint16_t pid;
    DiagnosticNegativeResponseCode negative_response_code;
    // if response mode is a negative response, read first byte of payload into
    // NRC and store remainder of payload in payload field
    uint8_t payload[MAX_OBD2_PAYLOAD_LENGTH];
    uint8_t payload_length;
} DiagnosticResponse;

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

typedef void (*DiagnosticResponseReceived)(const DiagnosticResponse* response);
typedef void (*DiagnosticMilStatusReceived)(bool malfunction_indicator_status);
typedef void (*DiagnosticVinReceived)(uint8_t vin[]);
typedef void (*DiagnosticTroubleCodesReceived)(
        DiagnosticMode mode, DiagnosticTroubleCode* codes);
typedef void (*DiagnosticPidEnumerationReceived)(
        const DiagnosticResponse* response, uint16_t* pids);

// TODO should we enumerate every OBD-II PID? need conversion formulas, too
typedef struct {
    uint16_t pid;
    uint8_t bytes_returned;
    float min_value;
    float max_value;
} DiagnosticParameter;

typedef enum {
    DIAGNOSTIC_REQUEST_TYPE_PID,
    DIAGNOSTIC_REQUEST_TYPE_DTC,
    DIAGNOSTIC_REQUEST_TYPE_MIL_STATUS,
    DIAGNOSTIC_REQUEST_TYPE_VIN
} DiagnosticRequestType;

typedef struct {
    IsoTpHandler isotp_handler;
    DiagnosticRequestType type;
    DiagnosticResponseReceived callback;
    DiagnosticMilStatusReceived mil_status_callback;
    DiagnosticVinReceived vin_callback;
    bool status;
} DiagnosticRequestHandle;

typedef enum {
    DIAGNOSTIC_STANDARD_PID,
    DIAGNOSTIC_ENHANCED_PID
} DiagnosticPidRequestType;

typedef struct {
    IsoTpShims isotp_shims;
    LogShim log;
    SendCanMessageShim send_can_message;
} DiagnosticShims;

DiagnosticShims diagnostic_init_shims(LogShim log,
        SendCanMessageShim send_can_message,
        SetTimerShim set_timer);

DiagnosticRequestHandle diagnostic_request(DiagnosticShims* shims,
        DiagnosticRequest* request, DiagnosticResponseReceived callback);

// decide mode 0x1 / 0x22 based on pid type
DiagnosticRequestHandle diagnostic_request_pid(DiagnosticShims* shims,
        DiagnosticPidRequestType pid_request_type, uint16_t pid,
        DiagnosticResponseReceived callback);

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

void diagnostic_receive_can_frame(DiagnosticRequestHandle* handle,
        const uint16_t arbitration_id, const uint8_t data[],
        const uint8_t size);

#ifdef __cplusplus
}
#endif

#endif // __OBD2_H__
