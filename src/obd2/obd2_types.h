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
#define MAX_RESPONDING_ECU_COUNT 8
#define VIN_LENGTH 17

/* Private: The four main types of diagnositc requests that determine how the
 * request should be parsed and what type of callback should be used.
 *
 * TODO this may not be used...yet?
 */
typedef enum {
    DIAGNOSTIC_REQUEST_TYPE_PID,
    DIAGNOSTIC_REQUEST_TYPE_DTC,
    DIAGNOSTIC_REQUEST_TYPE_MIL_STATUS,
    DIAGNOSTIC_REQUEST_TYPE_VIN
} DiagnosticRequestType;

/* Public: A container for a single diagnostic request.
 *
 * The only required fields are the arbitration_id and mode.
 *
 * arbitration_id - The arbitration ID to send the request.
 * mode - The OBD-II mode for the request.
 * pid - (optional) The PID to request, if the mode requires one.
 * pid_length - The length of the PID field, either 1 (standard) or 2 bytes
 *      (extended).
 * payload - (optional) The payload for the request, if the request requires
 *      one. If payload_length is 0 this field is ignored.
 * payload_length - The length of the payload, or 0 if no payload is used.
 * type - the type of the request (TODO unused)
 */
typedef struct {
    uint16_t arbitration_id;
    uint8_t mode;
    uint16_t pid;
    uint8_t pid_length;
    uint8_t payload[MAX_OBD2_PAYLOAD_LENGTH];
    uint8_t payload_length;
    DiagnosticRequestType type;
} DiagnosticRequest;

/* Public: All possible negative response codes that could be received from a
 * requested node.
 *
 * When a DiagnosticResponse is received and the 'completed' field is true, but
 * the 'success' field is false, the 'negative_response_code' field will contain
 * one of these values as reported by the requested node.
 *
 * Thanks to canbushack.com for the list of NRCs.
 */
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

/* Public: A partially or fully completed response to a diagnostic request.
 *
 * completed - True if the request is complete - some functions return a
 *      DiagnosticResponse even when it's only partially completed, so be sure
 *      to check this field.
 * success - True if the request was successful. The value if this
 *      field isn't valid if 'completed' isn't true. If this is 'false', check
 *      the negative_response_code field for the reason.
 * arbitration_id - The arbitration ID the response was received on.
 * mode - The OBD-II mode for the original request.
 * pid - If the request was for a PID, this is the PID echo.
 * negative_response_code - If the request was not successful, 'success' will be
 *      false and this will be set to a DiagnosticNegativeResponseCode returned
 *      by the other node.
 * payload - An optional payload for the response - NULL if no payload.
 * payload_length - The length of the payload or 0 if none.
 */
typedef struct {
    bool completed;
    bool success;
    uint16_t arbitration_id;
    uint8_t mode;
    uint16_t pid;
    DiagnosticNegativeResponseCode negative_response_code;
    uint8_t payload[MAX_OBD2_PAYLOAD_LENGTH];
    uint8_t payload_length;
} DiagnosticResponse;

/* Public: Friendly names for all OBD-II modes.
 */
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

/* Public the signature for an optional function to be called when a diagnostic
 * request is complete, and a response is received or there is a fatal error.
 *
 * response - the completed DiagnosticResponse.
 */
typedef void (*DiagnosticResponseReceived)(const DiagnosticResponse* response);

typedef float (*DiagnosticResponseDecoder)(const DiagnosticResponse* response);

/* Public: A handle for initiating and continuing a single diagnostic request.
 *
 * A diagnostic request requires one or more CAN messages to be sent, and one
 * or more CAN messages to be received before it is completed. This struct
 * encapsulates the local state required to track the request while it is in
 * progress.
 *
 * request - The original DiagnosticRequest that this handle was created for.
 * completed - True if the request was completed successfully, or was otherwise
 *      cancelled.
 * success - True if the request send and receive process was successful. The
 *      value if this field isn't valid if 'completed' isn't true.
 */
typedef struct {
    DiagnosticRequest request;
    bool success;
    bool completed;

    // Private
    IsoTpShims isotp_shims;
    IsoTpSendHandle isotp_send_handle;
    IsoTpReceiveHandle isotp_receive_handles[MAX_RESPONDING_ECU_COUNT];
    uint8_t isotp_receive_handle_count;
    DiagnosticResponseReceived callback;
    // DiagnosticMilStatusReceived mil_status_callback;
    // DiagnosticVinReceived vin_callback;
} DiagnosticRequestHandle;

/* Public: The two major types of PIDs that determine the OBD-II mode and PID
 * field length.
 */
typedef enum {
    DIAGNOSTIC_STANDARD_PID,
    DIAGNOSTIC_ENHANCED_PID
} DiagnosticPidRequestType;

/* Public: A container for the 3 shim functions used by the library to interact
 * with the wider system.
 *
 * Use the diagnostic_init_shims(...) function to create an instance of this
 * struct.
 */
typedef struct {
    LogShim log;
    SendCanMessageShim send_can_message;
    SetTimerShim set_timer;
} DiagnosticShims;

#ifdef __cplusplus
}
#endif

#endif // __OBD2_TYPES_H__
