#include <obd2/obd2.h>
#include <check.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

extern void setup();
extern bool last_response_was_received;
extern DiagnosticResponse last_response_received;
extern DiagnosticShims SHIMS;
extern DiagnosticResponseReceived response_received_handler;

START_TEST (test_receive_wrong_arb_id)
{
    DiagnosticRequest request = {
        arbitration_id: 0x7df,
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

START_TEST (test_send_diag_request)
{
    DiagnosticRequest request = {
        arbitration_id: 0x7df,
        mode: OBD2_MODE_POWERTRAIN_DIAGNOSTIC_REQUEST
    };
    DiagnosticRequestHandle handle = diagnostic_request(&SHIMS, &request,
            response_received_handler);

    fail_if(handle.completed);

    fail_if(last_response_was_received);
    const uint8_t can_data[] = {0x2, request.mode + 0x40, 0x23};
    DiagnosticResponse response = diagnostic_receive_can_frame(&SHIMS, &handle,
            request.arbitration_id + 0x8, can_data, sizeof(can_data));
    fail_unless(response.success);
    fail_unless(response.completed);
    fail_unless(handle.completed);
    fail_unless(last_response_was_received);
    ck_assert(last_response_received.success);
    ck_assert_int_eq(last_response_received.arbitration_id,
            request.arbitration_id + 0x8);
    // TODO should we set it back to the original mode, or leave as mode + 0x40?
    ck_assert_int_eq(last_response_received.mode, request.mode);
    ck_assert_int_eq(last_response_received.pid, 0);
    ck_assert_int_eq(last_response_received.payload_length, 1);
    ck_assert_int_eq(last_response_received.payload[0], can_data[2]);
}
END_TEST

START_TEST (test_request_pid_standard)
{
    DiagnosticRequestHandle handle = diagnostic_request_pid(&SHIMS,
            DIAGNOSTIC_STANDARD_PID, 0x2, response_received_handler);

    fail_if(last_response_was_received);
    const uint8_t can_data[] = {0x3, 0x1 + 0x40, 0x2, 0x45};
    // TODO need a constant for the 7df broadcast functional request
    diagnostic_receive_can_frame(&SHIMS, &handle, 0x7df + 0x8,
            can_data, sizeof(can_data));
    fail_unless(last_response_was_received);
    ck_assert(last_response_received.success);
    ck_assert_int_eq(last_response_received.arbitration_id,
            0x7df + 0x8);
    // TODO should we set it back to the original mode, or leave as mode + 0x40?
    ck_assert_int_eq(last_response_received.mode, 0x1);
    ck_assert_int_eq(last_response_received.pid, 0x2);
    ck_assert_int_eq(last_response_received.payload_length, 1);
    ck_assert_int_eq(last_response_received.payload[0], can_data[3]);
}
END_TEST

START_TEST (test_request_pid_enhanced)
{
    DiagnosticRequestHandle handle = diagnostic_request_pid(&SHIMS,
            DIAGNOSTIC_ENHANCED_PID, 0x2, response_received_handler);

    fail_if(last_response_was_received);
    const uint8_t can_data[] = {0x4, 0x1 + 0x40, 0x0, 0x2, 0x45};
    // TODO need a constant for the 7df broadcast functional request
    diagnostic_receive_can_frame(&SHIMS, &handle, 0x7df + 0x8, can_data,
            sizeof(can_data));
    fail_unless(last_response_was_received);
    ck_assert(last_response_received.success);
    ck_assert_int_eq(last_response_received.arbitration_id,
            0x7df + 0x8);
    // TODO should we set it back to the original mode, or leave as mode + 0x40?
    ck_assert_int_eq(last_response_received.mode, 0x22);
    ck_assert_int_eq(last_response_received.pid, 0x2);
    ck_assert_int_eq(last_response_received.payload_length, 1);
    ck_assert_int_eq(last_response_received.payload[0], can_data[4]);
}
END_TEST

Suite* testSuite(void) {
    Suite* s = suite_create("obd2");
    TCase *tc_core = tcase_create("core");
    tcase_add_checked_fixture(tc_core, setup, NULL);
    tcase_add_test(tc_core, test_send_diag_request);
    tcase_add_test(tc_core, test_receive_wrong_arb_id);
    tcase_add_test(tc_core, test_request_pid_standard);
    tcase_add_test(tc_core, test_request_pid_enhanced);

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
