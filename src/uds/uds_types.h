#ifndef __UDS_TYPES_H__
#define __UDS_TYPES_H__

#include <isotp/isotp.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// TODO This still doesn't have enough space for the largest possible 
// multiframe response. May need to dynamically allocate in the future.
#define MAX_UDS_RESPONSE_PAYLOAD_LENGTH 127
#define MAX_UDS_REQUEST_PAYLOAD_LENGTH 7
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
 * has_pid - (optional) If the requests uses a PID, this should be true.
 * pid - (optional) The PID to request, if the mode requires one. has_pid must
 *      be true.
 * pid_length - The length of the PID field, either 1 (standard) or 2 bytes
 *      (extended). If 0, it will be set automatically based on the request
 *      mode.
 * payload - (optional) The payload for the request, if the request requires
 *      one. If payload_length is 0 this field is ignored.
 * payload_length - The length of the payload, or 0 if no payload is used.
 * no_frame_padding - false if sent CAN payloads should *not* be padded out to a
 *      full 8 byte CAN frame. Many ECUs require this, but others require the
 *      size of the CAN message to only be the actual data. By default padding
 *      is enabled (so this struct value can default to 0).
 * type - the type of the request (TODO unused)
 */
typedef struct {
    uint32_t arbitration_id;
    uint8_t mode;
    bool has_pid;
    uint16_t pid;
    uint8_t pid_length;
    uint8_t payload[MAX_UDS_REQUEST_PAYLOAD_LENGTH];
    uint8_t payload_length;
    bool no_frame_padding;
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
    NRC_INCORRECT_LENGTH_OR_FORMAT = 0x13,
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
 * multi_frame - True if this response (whether completed or not) required
 *      multi-frame CAN support. Can be used for updating time-out functions.
 * mode - The OBD-II mode for the original request.
 * has_pid - If this is a response to a PID request, this will be true and the
 *      'pid' field will be valid.
 * pid - If the request was for a PID, this is the PID echo. Only valid if
 *      'has_pid' is true.
 * negative_response_code - If the request was not successful, 'success' will be
 *      false and this will be set to a DiagnosticNegativeResponseCode returned
 *      by the other node.
 * payload - An optional payload for the response - NULL if no payload.
 * payload_length - The length of the payload or 0 if none.
 */
typedef struct {
    bool completed;
    bool success;
    bool multi_frame;
    uint32_t arbitration_id;
    uint8_t mode;
    bool has_pid;
    uint16_t pid;
    DiagnosticNegativeResponseCode negative_response_code;
    uint8_t payload[MAX_UDS_RESPONSE_PAYLOAD_LENGTH];
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
    // this one isn't technically in uds, but both of the enhanced standards
    // have their PID requests at 0x22
    OBD2_MODE_ENHANCED_DIAGNOSTIC_REQUEST = 0x22
} DiagnosticMode;

/* Public: The signature for an optional function to be called when a diagnostic
 * request is complete, and a response is received or there is a fatal error.
 *
 * response - the completed DiagnosticResponse.
 */
typedef void (*DiagnosticResponseReceived)(const DiagnosticResponse* response);

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

#endif // __UDS_TYPES_H__
