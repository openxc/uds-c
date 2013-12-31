#include <obd2/obd2.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

DiagnosticShims SHIMS;

uint16_t last_can_frame_sent_arb_id;
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

void mock_send_can(const uint16_t arbitration_id, const uint8_t* data,
        const uint8_t size) {
    can_frame_was_sent = true;
    last_can_frame_sent_arb_id = arbitration_id;
    last_can_payload_size = size;
    if(size > 0) {
        memcpy(last_can_payload_sent, data, size);
    }
}

void mock_set_timer(uint16_t time_ms, void (*callback)) {
}

void response_received_handler(const DiagnosticResponse* response) {
    last_response_was_received = true;
    // TODO not sure if we can copy the struct like this
    last_response_received = *response;
}

void setup() {
    SHIMS = diagnostic_init_shims(debug, mock_send_can, mock_set_timer);
    memset(last_can_payload_sent, 0, sizeof(last_can_payload_sent));
    can_frame_was_sent = false;
    last_response_was_received = false;
}

