#ifndef __OBD2_TYPES_H__
#define __OBD2_TYPES_H__

#include <isotp/isotp.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// TODO This isn't true for multi frame messages - we may need to dynamically
// allocate this in the future
#define MAX_OBD2_PAYLOAD_LENGTH 7
#define VIN_LENGTH 17

typedef enum {
    DIAGNOSTIC_REQUEST_TYPE_PID,
    DIAGNOSTIC_REQUEST_TYPE_DTC,
    DIAGNOSTIC_REQUEST_TYPE_MIL_STATUS,
    DIAGNOSTIC_REQUEST_TYPE_VIN
} DiagnosticRequestType;

typedef struct {
    DiagnosticRequestType type;
    uint16_t arbitration_id;
    uint8_t mode;
    uint16_t pid;
    uint8_t pid_length;
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
    bool completed;
    bool success;
    uint16_t pid;
    DiagnosticNegativeResponseCode negative_response_code;
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

typedef struct {
    DiagnosticRequest request;
    bool success;
    bool completed;

    IsoTpShims isotp_shims;
    IsoTpSendHandle isotp_send_handle;
    IsoTpReceiveHandle isotp_receive_handle;
    DiagnosticResponseReceived callback;
    // DiagnosticMilStatusReceived mil_status_callback;
    // DiagnosticVinReceived vin_callback;
} DiagnosticRequestHandle;

typedef enum {
    DIAGNOSTIC_STANDARD_PID,
    DIAGNOSTIC_ENHANCED_PID
} DiagnosticPidRequestType;

typedef struct {
    LogShim log;
    SendCanMessageShim send_can_message;
    SetTimerShim set_timer;
} DiagnosticShims;

#ifdef __cplusplus
}
#endif

#endif // __OBD2_TYPES_H__
