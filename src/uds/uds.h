#ifndef __UDS_H__
#define __UDS_H__

#include <uds/uds_types.h>
#include <stdint.h>
#include <stdbool.h>

#define OBD2_FUNCTIONAL_BROADCAST_ID 0x7df
#define OBD2_FUNCTIONAL_RESPONSE_START 0x7e8
#define OBD2_FUNCTIONAL_RESPONSE_COUNT 8

#ifdef __cplusplus
extern "C" {
#endif

/* Public: Initialize an DiagnosticShims with the given callback functions.
 *
 * If any callbacks are not to be used, set them to NULL. For documentation of
 * the function type signatures, see higher up in this header file. This struct
 * is a handy encapsulation used to pass the shims around to the various
 * diagnostic_* functions.
 *
 * Returns a struct with the fields initailized to the callbacks.
 */
DiagnosticShims diagnostic_init_shims(LogShim log,
        SendCanMessageShim send_can_message,
        SetTimerShim set_timer);

/* Public: Generate a new diagnostic request, send the first CAN message frame
 * and set up the handle required to process the response via
 * diagnostic_receive_can_frame(...).
 *
 * shims -  Low-level shims required to send CAN messages, etc.
 * request -
 * callback - an optional function to be called when the response is receved
 *      (use NULL if no callback is required).
 *
 * Returns a handle to be used with diagnostic_receive_can_frame to complete
 * sending the request and receive the response. The 'completed' field in the
 * returned DiagnosticRequestHandle will be true when the message is completely
 * sent. The first frame of the message will already be sent.
 */
DiagnosticRequestHandle diagnostic_request(DiagnosticShims* shims,
        DiagnosticRequest* request, DiagnosticResponseReceived callback);

/* Public: Generate the handle for a new diagnostic request, but do not send any
 * data to CAN yet - you must call start_diagnostic_request(...) on the handle
 * returned from this function actually kick off the request.
 *
 * shims -  Low-level shims required to send CAN messages, etc.
 * request -
 * callback - an optional function to be called when the response is receved
 *      (use NULL if no callback is required).
 *
 * Returns a handle to be used with start_diagnostic_request and then
 * diagnostic_receive_can_frame to complete sending the request and receive the
 * response. The 'completed' field in the returned DiagnosticRequestHandle will
 * be true when the message is completely sent.
 */
DiagnosticRequestHandle generate_diagnostic_request(DiagnosticShims* shims,
        DiagnosticRequest* request, DiagnosticResponseReceived callback);

/* Public: Send the first frame of the request to CAN for the handle, generated
 * by generate_diagnostic_request.
 *
 * You can also call this method to re-do the request for a handle that has
 * already completed.
 */
void start_diagnostic_request(DiagnosticShims* shims,
                DiagnosticRequestHandle* handle);

/* Public: Request a PID from the given arbitration ID, determining the mode
 * automatically based on the PID type.
 *
 * shims -  Low-level shims required to send CAN messages, etc.
 * pid_request_type - either DIAGNOSTIC_STANDARD_PID (will use mode 0x1 and 1
 *      byte PIDs) or DIAGNOSTIC_ENHANCED_PID (will use mode 0x22 and 2 byte
 *      PIDs)
 * arbitration_id - The arbitration ID to send the request to.
 * pid - The PID to request from the other node.
 * callback - an optional function to be called when the response is receved
 *      (use NULL if no callback is required).
 *
 * Returns a handle to be used with diagnostic_receive_can_frame to complete
 * sending the request and receive the response. The 'completed' field in the
 * returned DiagnosticRequestHandle will be true when the message is completely
 * sent.
 */
DiagnosticRequestHandle diagnostic_request_pid(DiagnosticShims* shims,
        DiagnosticPidRequestType pid_request_type, uint32_t arbitration_id,
        uint16_t pid, DiagnosticResponseReceived callback);

/* Public: Continue to send and receive a single diagnostic request, based on a
 * freshly received CAN message.
 *
 * shims -  Low-level shims required to send CAN messages, etc.
 * handle - A DiagnosticRequestHandle previously returned by one of the
 *      diagnostic_request*(..) functions.
 * arbitration_id - The arbitration_id of the received CAN message.
 * data - The data of the received CAN message.
 * size - The size of the data in the received CAN message.
 *
 * Returns true if the request was completed and response received, or the
 * request was otherwise cancelled. Check the 'success' field of the handle to
 * see if it was successful.
 */
DiagnosticResponse diagnostic_receive_can_frame(DiagnosticShims* shims,
        DiagnosticRequestHandle* handle,
        const uint32_t arbitration_id, const uint8_t data[],
        const uint8_t size);

/* Public: Parse the entier payload of the reponse as a single integer.
 *
 * response - the received DiagnosticResponse.
 */
int diagnostic_payload_to_integer(const DiagnosticResponse* response);

/* Public: Render a DiagnosticResponse as a string into the given buffer.
 *
 * response - the response to convert to a string, for debug logging.
 * destination - the target string buffer.
 * destination_length - the size of the destination buffer, i.e. the max size
 *      for the rendered string.
 */
void diagnostic_response_to_string(const DiagnosticResponse* response,
        char* destination, size_t destination_length);

/* Public: Render a DiagnosticRequest as a string into the given buffer.
 *
 * request - the request to convert to a string, for debug logging.
 * destination - the target string buffer.
 * destination_length - the size of the destination buffer, i.e. the max size
 *      for the rendered string.
 */
void diagnostic_request_to_string(const DiagnosticRequest* request,
        char* destination, size_t destination_length);

/* Public: For many OBD-II PIDs with a numerical result, translate a diagnostic
 * response payload into a meaningful number using the standard formulas.
 *
 * Functions pulled from http://en.wikipedia.org/wiki/OBD-II_PIDs#Mode_01
 *
 * Returns the translated value or 0 if the PID is not in the OBD-II standard or
 * does not use a numerical value (e.g. VIN).
 */
float diagnostic_decode_obd2_pid(const DiagnosticResponse* response);

/* Public: Returns true if the "fingerprint" of the two diagnostic messages
 * matches - the arbitration_id, mode and pid (or lack of pid).
 */
bool diagnostic_request_equals(const DiagnosticRequest* ours,
        const DiagnosticRequest* theirs);

/* Public: Returns true if the request has been completely sent - if false, make
 * sure you called start_diagnostic_request once to start it, and then pass
 * incoming CAN messages to it with diagnostic_receive_can_frame(...) so it can
 * continue the ISO-TP transfer.
 */
bool diagnostic_request_sent(DiagnosticRequestHandle* handle);

#ifdef __cplusplus
}
#endif

#endif // __UDS_H__
