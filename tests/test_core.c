#include <uds/uds.h>
#include <check.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

extern bool can_frame_was_sent;
extern void setup();
extern bool last_response_was_received;
extern DiagnosticResponse last_response_received;
extern DiagnosticShims SHIMS;
extern uint16_t last_can_frame_sent_arb_id;
extern uint8_t last_can_payload_sent[8];
extern uint8_t last_can_payload_size;

void response_received_handler(const DiagnosticResponse* response) {
    last_response_was_received = true;
    last_response_received = *response;
}

START_TEST (test_receive_wrong_arb_id)
{
    DiagnosticRequest request = {
        arbitration_id: 0x100,
        mode: OBD2_MODE_POWERTRAIN_DIAGNOSTIC_REQUEST
    };
    DiagnosticRequestHandle handle = diagnostic_request(&SHIMS, &request,
            response_received_handler);

    fail_if(last_response_was_received);
    const uint8_t can_data[] = {0x2, request.mode + 0x40, 0x23};
    diagnostic_receive_can_frame(&SHIMS, &handle, request.arbitration_id,
            can_data, sizeof(can_data));
    fail_if(last_response_was_received);
}
END_TEST

START_TEST (test_send_diag_request_with_payload)
{
    DiagnosticRequest request = {
        arbitration_id: 0x100,
        mode: OBD2_MODE_POWERTRAIN_DIAGNOSTIC_REQUEST,
        payload: {0x12, 0x34},
        payload_length: 2,
        no_frame_padding: true
    };
    DiagnosticRequestHandle handle = diagnostic_request(&SHIMS, &request,
            response_received_handler);

    fail_if(handle.completed);
    // TODO it'd be better to check the ISO-TP message instead of the CAN frame,
    // but we don't have a good way to do that
    ck_assert_int_eq(last_can_frame_sent_arb_id, request.arbitration_id);
    ck_assert_int_eq(last_can_payload_sent[1], request.mode);
    ck_assert_int_eq(last_can_payload_size, 4);
    ck_assert_int_eq(last_can_payload_sent[2], request.payload[0]);
    ck_assert_int_eq(last_can_payload_sent[3], request.payload[1]);
}
END_TEST

START_TEST (test_send_functional_request)
{
    DiagnosticRequest request = {
        arbitration_id: OBD2_FUNCTIONAL_BROADCAST_ID,
        mode: OBD2_MODE_EMISSIONS_DTC_REQUEST,
        no_frame_padding: true
    };
    DiagnosticRequestHandle handle = diagnostic_request(&SHIMS, &request,
            response_received_handler);

    fail_if(handle.completed);
    ck_assert_int_eq(last_can_frame_sent_arb_id, request.arbitration_id);
    ck_assert_int_eq(last_can_payload_sent[1], request.mode);
    ck_assert_int_eq(last_can_payload_size, 2);

    fail_if(last_response_was_received);
    const uint8_t can_data[] = {0x2, request.mode + 0x40, 0x23};
    for(uint16_t filter = OBD2_FUNCTIONAL_RESPONSE_START; filter <
            OBD2_FUNCTIONAL_RESPONSE_START + OBD2_FUNCTIONAL_RESPONSE_COUNT;
            filter++) {
        DiagnosticResponse response = diagnostic_receive_can_frame(&SHIMS,
                &handle, filter, can_data, sizeof(can_data));
        fail_unless(response.success);
        fail_unless(response.completed);
        fail_unless(handle.completed);
        ck_assert(last_response_received.success);
        ck_assert_int_eq(last_response_received.arbitration_id,
                filter);
        ck_assert_int_eq(last_response_received.mode, request.mode);
        fail_if(last_response_received.has_pid);
        ck_assert_int_eq(last_response_received.payload_length, 1);
        ck_assert_int_eq(last_response_received.payload[0], can_data[2]);
    }
}
END_TEST

START_TEST (test_sent_message_no_padding)
{
    DiagnosticRequest request = {
        arbitration_id: 0x100,
        mode: OBD2_MODE_EMISSIONS_DTC_REQUEST,
        no_frame_padding: true
    };
    DiagnosticRequestHandle handle = diagnostic_request(&SHIMS, &request,
            response_received_handler);

    fail_if(handle.completed);
    ck_assert_int_eq(last_can_frame_sent_arb_id, request.arbitration_id);
    ck_assert_int_eq(last_can_payload_size, 2);
}
END_TEST

START_TEST (test_sent_message_is_padded_by_default)
{
    DiagnosticRequest request = {
        arbitration_id: 0x100,
        mode: OBD2_MODE_EMISSIONS_DTC_REQUEST
    };
    DiagnosticRequestHandle handle = diagnostic_request(&SHIMS, &request,
            response_received_handler);

    fail_if(handle.completed);
    ck_assert_int_eq(last_can_frame_sent_arb_id, request.arbitration_id);
    ck_assert_int_eq(last_can_payload_size, 8);
}
END_TEST

START_TEST (test_sent_message_is_padded)
{
    DiagnosticRequest request = {
        arbitration_id: 0x100,
        mode: OBD2_MODE_EMISSIONS_DTC_REQUEST,
        no_frame_padding: false
    };
    DiagnosticRequestHandle handle = diagnostic_request(&SHIMS, &request,
            response_received_handler);

    fail_if(handle.completed);
    ck_assert_int_eq(last_can_frame_sent_arb_id, request.arbitration_id);
    ck_assert_int_eq(last_can_payload_size, 8);
}
END_TEST

START_TEST (test_send_diag_request)
{
    DiagnosticRequest request = {
        arbitration_id: 0x100,
        mode: OBD2_MODE_EMISSIONS_DTC_REQUEST,
        no_frame_padding: true
    };
    DiagnosticRequestHandle handle = diagnostic_request(&SHIMS, &request,
            response_received_handler);

    fail_if(handle.completed);
    fail_unless(can_frame_was_sent);
    ck_assert_int_eq(last_can_frame_sent_arb_id, request.arbitration_id);
    ck_assert_int_eq(last_can_payload_sent[1], request.mode);
    ck_assert_int_eq(last_can_payload_size, 2);

    fail_if(last_response_was_received);
    const uint8_t can_data[] = {0x2, request.mode + 0x40, 0x23};
    DiagnosticResponse response = diagnostic_receive_can_frame(&SHIMS, &handle,
            request.arbitration_id + 0x8, can_data, sizeof(can_data));
    fail_unless(response.success);
    fail_unless(response.completed);
    fail_unless(handle.completed);
    ck_assert(last_response_received.success);
    ck_assert_int_eq(last_response_received.arbitration_id,
            request.arbitration_id + 0x8);
    ck_assert_int_eq(last_response_received.mode, request.mode);
        fail_if(last_response_received.has_pid);
    ck_assert_int_eq(last_response_received.payload_length, 1);
    ck_assert_int_eq(last_response_received.payload[0], can_data[2]);
}
END_TEST

START_TEST (test_generate_then_send_request)
{
    DiagnosticRequest request = {
        arbitration_id: 0x100,
        mode: OBD2_MODE_EMISSIONS_DTC_REQUEST,
        no_frame_padding: true
    };
    DiagnosticRequestHandle handle = generate_diagnostic_request(&SHIMS,
            &request, response_received_handler);

    fail_if(handle.completed);
    fail_if(can_frame_was_sent);

    start_diagnostic_request(&SHIMS, &handle);
    fail_unless(can_frame_was_sent);
    ck_assert_int_eq(last_can_frame_sent_arb_id, request.arbitration_id);
    ck_assert_int_eq(last_can_payload_sent[1], request.mode);
    ck_assert_int_eq(last_can_payload_size, 2);

    fail_if(last_response_was_received);
    const uint8_t can_data[] = {0x2, request.mode + 0x40, 0x23};
    DiagnosticResponse response = diagnostic_receive_can_frame(&SHIMS, &handle,
            request.arbitration_id + 0x8, can_data, sizeof(can_data));
    fail_unless(response.success);
    fail_unless(response.completed);
    fail_unless(handle.completed);
    ck_assert(last_response_received.success);
    ck_assert_int_eq(last_response_received.arbitration_id,
            request.arbitration_id + 0x8);
    ck_assert_int_eq(last_response_received.mode, request.mode);
        fail_if(last_response_received.has_pid);
    ck_assert_int_eq(last_response_received.payload_length, 1);
    ck_assert_int_eq(last_response_received.payload[0], can_data[2]);
}
END_TEST

START_TEST (test_generate_diag_request)
{
    DiagnosticRequest request = {
        arbitration_id: 0x100,
        mode: OBD2_MODE_EMISSIONS_DTC_REQUEST,
        no_frame_padding: true
    };
    DiagnosticRequestHandle handle = generate_diagnostic_request(&SHIMS,
            &request, response_received_handler);

    fail_if(handle.completed);
    fail_if(can_frame_was_sent);
}
END_TEST

START_TEST (test_autoset_pid_length)
{
    uint16_t arb_id = OBD2_MODE_POWERTRAIN_DIAGNOSTIC_REQUEST;
    diagnostic_request_pid(&SHIMS, DIAGNOSTIC_STANDARD_PID, arb_id, 0x2,
            response_received_handler);

    ck_assert_int_eq(last_can_frame_sent_arb_id, arb_id);
    ck_assert_int_eq(last_can_payload_sent[1], 0x1);
    ck_assert_int_eq(last_can_payload_sent[2], 0x2);
    // padding is on for the diagnostic_request_pid helper function - if you
    // need to turn it off, use the more manual diagnostic_request(...)
    ck_assert_int_eq(last_can_payload_size, 8);

    DiagnosticRequest request = {
        arbitration_id: 0x100,
        mode: 0x22,
        has_pid: true,
        pid: 0x1234,
        no_frame_padding: true
    };
    diagnostic_request(&SHIMS, &request, response_received_handler);

    ck_assert_int_eq(last_can_frame_sent_arb_id, request.arbitration_id);
    ck_assert_int_eq(last_can_payload_sent[1], request.mode);
    ck_assert_int_eq(last_can_payload_sent[2], (request.pid & 0xFF00) >> 8);
    ck_assert_int_eq(last_can_payload_sent[3], request.pid & 0xFF);
    ck_assert_int_eq(last_can_payload_size, 4);

    request.arbitration_id = 0x101;
    request.pid = 0x12;

    diagnostic_request(&SHIMS, &request, response_received_handler);

    ck_assert_int_eq(last_can_frame_sent_arb_id, request.arbitration_id);
    ck_assert_int_eq(last_can_payload_sent[1], request.mode);
    ck_assert_int_eq(last_can_payload_sent[2], request.pid);
    ck_assert_int_eq(last_can_payload_size, 3);
}
END_TEST

START_TEST (test_request_pid_standard)
{
    uint16_t arb_id = OBD2_MODE_POWERTRAIN_DIAGNOSTIC_REQUEST;
    DiagnosticRequestHandle handle = diagnostic_request_pid(&SHIMS,
            DIAGNOSTIC_STANDARD_PID, arb_id, 0x2, response_received_handler);

    fail_if(last_response_was_received);
    const uint8_t can_data[] = {0x3, 0x1 + 0x40, 0x2, 0x45};
    diagnostic_receive_can_frame(&SHIMS, &handle, arb_id + 0x8,
            can_data, sizeof(can_data));
    fail_unless(last_response_was_received);
    ck_assert(last_response_received.success);
    ck_assert_int_eq(last_response_received.arbitration_id,
            arb_id + 0x8);
    ck_assert_int_eq(last_response_received.mode, 0x1);
    fail_unless(last_response_received.has_pid);
    ck_assert_int_eq(last_response_received.pid, 0x2);
    ck_assert_int_eq(last_response_received.payload_length, 1);
    ck_assert_int_eq(last_response_received.payload[0], can_data[3]);
}
END_TEST

START_TEST (test_request_pid_enhanced)
{
    uint16_t arb_id = 0x100;
    DiagnosticRequestHandle handle = diagnostic_request_pid(&SHIMS,
            DIAGNOSTIC_ENHANCED_PID, arb_id, 0x1234, response_received_handler);

    fail_if(last_response_was_received);
    const uint8_t can_data[] = {0x4, 0x22 + 0x40, 0x12, 0x34, 0x45};
    diagnostic_receive_can_frame(&SHIMS, &handle, arb_id + 0x8, can_data,
            sizeof(can_data));
    fail_unless(last_response_was_received);
    ck_assert(last_response_received.success);
    ck_assert_int_eq(last_response_received.arbitration_id,
            arb_id + 0x8);
    ck_assert_int_eq(last_response_received.mode, 0x22);
    fail_unless(last_response_received.has_pid);
    ck_assert_int_eq(last_response_received.pid, 0x1234);
    ck_assert_int_eq(last_response_received.payload_length, 1);
    ck_assert_int_eq(last_response_received.payload[0], can_data[4]);
}
END_TEST

START_TEST (test_wrong_mode_response)
{
    uint16_t arb_id = 0x100;
    DiagnosticRequestHandle handle = diagnostic_request_pid(&SHIMS,
            DIAGNOSTIC_ENHANCED_PID, arb_id, 0x1234, response_received_handler);

    fail_if(last_response_was_received);
    const uint8_t can_data[] = {0x4, 0x1 + 0x40, 0x12, 0x34, 0x45};
    diagnostic_receive_can_frame(&SHIMS, &handle, arb_id + 0x8, can_data,
            sizeof(can_data));
    fail_if(last_response_was_received);
    fail_if(handle.completed);
}
END_TEST

START_TEST (test_missing_pid)
{
    uint16_t arb_id = 0x100;
    DiagnosticRequestHandle handle = diagnostic_request_pid(&SHIMS,
            DIAGNOSTIC_ENHANCED_PID, arb_id, 0x1234, response_received_handler);

    fail_if(last_response_was_received);
    const uint8_t can_data[] = {0x1, 0x22 + 0x40};
    diagnostic_receive_can_frame(&SHIMS, &handle, arb_id + 0x8, can_data,
            sizeof(can_data));
    fail_if(last_response_was_received);
    fail_if(handle.completed);
}
END_TEST

START_TEST (test_wrong_pid_response)
{
    uint16_t arb_id = 0x100;
    DiagnosticRequestHandle handle = diagnostic_request_pid(&SHIMS,
            DIAGNOSTIC_ENHANCED_PID, arb_id, 0x1234, response_received_handler);

    fail_if(last_response_was_received);
    const uint8_t can_data[] = {0x4, 0x22 + 0x40, 0x12, 0x33, 0x45};
    diagnostic_receive_can_frame(&SHIMS, &handle, arb_id + 0x8, can_data,
            sizeof(can_data));
    fail_if(last_response_was_received);
    fail_if(handle.completed);
}
END_TEST

START_TEST (test_wrong_pid_then_right_completes)
{
    uint16_t arb_id = 0x100;
    DiagnosticRequestHandle handle = diagnostic_request_pid(&SHIMS,
            DIAGNOSTIC_ENHANCED_PID, arb_id, 0x1234, response_received_handler);

    fail_if(last_response_was_received);
    uint8_t can_data[] = {0x4, 0x22 + 0x40, 0x12, 0x33, 0x45};
    diagnostic_receive_can_frame(&SHIMS, &handle, arb_id + 0x8, can_data,
            sizeof(can_data));
    fail_if(last_response_was_received);
    fail_if(handle.completed);

    can_data[3] = 0x34;
    diagnostic_receive_can_frame(&SHIMS, &handle, arb_id + 0x8, can_data,
            sizeof(can_data));
    fail_unless(last_response_was_received);
    fail_unless(handle.completed);
    fail_unless(handle.success);
    fail_unless(last_response_received.success);
    ck_assert_int_eq(last_response_received.pid, 0x1234);
}
END_TEST

START_TEST (test_negative_response)
{
    DiagnosticRequest request = {
        arbitration_id: 0x100,
        mode: OBD2_MODE_POWERTRAIN_DIAGNOSTIC_REQUEST
    };
    DiagnosticRequestHandle handle = diagnostic_request(&SHIMS, &request,
            response_received_handler);
    const uint8_t can_data[] = {0x3, 0x7f, request.mode,
        NRC_SERVICE_NOT_SUPPORTED};
    DiagnosticResponse response = diagnostic_receive_can_frame(&SHIMS, &handle,
            request.arbitration_id + 0x8, can_data, sizeof(can_data));
    fail_unless(response.completed);
    fail_if(response.success);
    fail_unless(handle.completed);

    fail_if(last_response_received.success);
    ck_assert_int_eq(last_response_received.arbitration_id,
            request.arbitration_id + 0x8);
    ck_assert_int_eq(last_response_received.mode, request.mode);
    ck_assert_int_eq(last_response_received.pid, 0);
    ck_assert_int_eq(last_response_received.negative_response_code,
            NRC_SERVICE_NOT_SUPPORTED);
    ck_assert_int_eq(last_response_received.payload_length, 0);
}
END_TEST

START_TEST (test_payload_to_integer)
{
    uint16_t arb_id = OBD2_MODE_POWERTRAIN_DIAGNOSTIC_REQUEST;
    DiagnosticRequestHandle handle = diagnostic_request_pid(&SHIMS,
            DIAGNOSTIC_STANDARD_PID, arb_id, 0x2, response_received_handler);

    fail_if(last_response_was_received);
    const uint8_t can_data[] = {0x4, 0x1 + 0x40, 0x2, 0x45, 0x12};
    DiagnosticResponse response = diagnostic_receive_can_frame(&SHIMS, &handle,
            arb_id + 0x8, can_data, sizeof(can_data));
    ck_assert_int_eq(diagnostic_payload_to_integer(&response), 0x4512);
}
END_TEST

START_TEST (test_response_multi_frame)
{
    DiagnosticRequest request = {
        arbitration_id: 0x100,
        mode: OBD2_MODE_VEHICLE_INFORMATION,
        has_pid: true,
        pid: 0x2
    };
    DiagnosticRequestHandle handle = diagnostic_request(&SHIMS, &request,
            response_received_handler);

    const uint8_t can_data[] = {0x10, 0x14, 0x9 + 0x40, 0x2, 0x1, 0x31, 0x46, 0x4d};
    DiagnosticResponse response = diagnostic_receive_can_frame(&SHIMS, &handle,
            request.arbitration_id + 0x8, can_data, sizeof(can_data));

    fail_unless(can_frame_was_sent);
    fail_unless(!response.success);
    fail_unless(!response.completed);
    fail_unless(response.multi_frame);
    ck_assert_int_eq(last_can_frame_sent_arb_id, request.arbitration_id);
    ck_assert_int_eq(last_can_payload_sent[0], 0x30);

    const uint8_t can_data_1[] = {0x21, 0x43, 0x55, 0x39, 0x4a, 0x39, 0x34, 0x48};
    response = diagnostic_receive_can_frame(&SHIMS, &handle,
            request.arbitration_id + 0x8, can_data_1, sizeof(can_data_1));
    fail_unless(!response.success);
    fail_unless(!response.completed);
    fail_unless(response.multi_frame);

    const uint8_t can_data_2[] = {0x22, 0x55, 0x41, 0x30, 0x34, 0x35, 0x32, 0x34};
    response = diagnostic_receive_can_frame(&SHIMS, &handle,
            request.arbitration_id + 0x8, can_data_2, sizeof(can_data_2));
    fail_unless(response.success);
    fail_unless(response.completed);
    fail_unless(response.multi_frame);
    ck_assert_int_eq(response.mode, OBD2_MODE_VEHICLE_INFORMATION);
    ck_assert_int_eq(response.pid, 0x2);
    ck_assert_int_eq(response.payload_length, 18);
    ck_assert_int_eq(response.payload[0], 0x01);
    ck_assert_int_eq(response.payload[1], 0x31);
    ck_assert_int_eq(response.payload[2], 0x46);
    ck_assert_int_eq(response.payload[3], 0x4d);
    ck_assert_int_eq(response.payload[4], 0x43);
    ck_assert_int_eq(response.payload[5], 0x55);
    ck_assert_int_eq(response.payload[6], 0x39);
    ck_assert_int_eq(response.payload[7], 0x4a);
    ck_assert_int_eq(response.payload[8], 0x39);
    ck_assert_int_eq(response.payload[9], 0x34);
    ck_assert_int_eq(response.payload[10], 0x48);
    ck_assert_int_eq(response.payload[11], 0x55);
    ck_assert_int_eq(response.payload[12], 0x41);
    ck_assert_int_eq(response.payload[13], 0x30);
    ck_assert_int_eq(response.payload[14], 0x34);
    ck_assert_int_eq(response.payload[15], 0x35);
    ck_assert_int_eq(response.payload[16], 0x32);
    ck_assert_int_eq(response.payload[17], 0x34);
}
END_TEST

Suite* testSuite(void) {
    Suite* s = suite_create("uds");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_sent_message_no_padding);
    tcase_add_test(tc_core, test_sent_message_is_padded);
    tcase_add_test(tc_core, test_sent_message_is_padded_by_default);
    tcase_add_test(tc_core, test_generate_diag_request);
    tcase_add_test(tc_core, test_generate_then_send_request);
    tcase_add_test(tc_core, test_send_diag_request);
    tcase_add_test(tc_core, test_send_functional_request);
    tcase_add_test(tc_core, test_send_diag_request_with_payload);
    tcase_add_test(tc_core, test_receive_wrong_arb_id);
    tcase_add_test(tc_core, test_autoset_pid_length);
    tcase_add_test(tc_core, test_request_pid_standard);
    tcase_add_test(tc_core, test_request_pid_enhanced);
    tcase_add_test(tc_core, test_wrong_mode_response);
    tcase_add_test(tc_core, test_wrong_pid_response);
    tcase_add_test(tc_core, test_missing_pid);
    tcase_add_test(tc_core, test_wrong_pid_then_right_completes);
    tcase_add_test(tc_core, test_negative_response);
    tcase_add_test(tc_core, test_payload_to_integer);
    tcase_add_test(tc_core, test_response_multi_frame);

    // TODO these are future work:
    // TODO test request MIL
    // TODO test request VIN
    // TODO test request DTC
    // TODO test clear DTC
    // TODO test enumerate PIDs
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void) {
    int numberFailed;
    Suite* s = testSuite();
    SRunner *sr = srunner_create(s);
    // Don't fork so we can actually use gdb
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    numberFailed = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (numberFailed == 0) ? 0 : 1;
}
