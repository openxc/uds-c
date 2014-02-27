#include <uds/uds.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

DiagnosticShims SHIMS;

uint32_t last_can_frame_sent_arb_id;
uint8_t last_can_payload_sent[8];
uint8_t last_can_payload_size;
bool can_frame_was_sent;

DiagnosticResponse last_response_received;
bool last_response_was_received;

void debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    printf("\r\n");
    va_end(args);
}

bool mock_send_can(const uint32_t arbitration_id, const uint8_t* data,
        const uint8_t size) {
    can_frame_was_sent = true;
    last_can_frame_sent_arb_id = arbitration_id;
    last_can_payload_size = size;
    if(size > 0) {
        memcpy(last_can_payload_sent, data, size);
    }
    return true;
}

void setup() {
    SHIMS = diagnostic_init_shims(debug, mock_send_can, NULL);
    memset(last_can_payload_sent, 0, sizeof(last_can_payload_sent));
    can_frame_was_sent = false;
    last_response_was_received = false;
}

