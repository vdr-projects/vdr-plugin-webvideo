#ifndef PIPE_TESTS_H
#define PIPE_TESTS_H

void test_pipe_one_component();
void test_pipe_two_components();
void test_pipe_failing_component();
void test_pipe_not_appending_after_finished();
void test_pipe_state_not_chaning_after_finished();
void test_pipe_delete_all();
void test_pipe_fdset();

void test_pipe_menu_validator_valid_menu();
void test_pipe_menu_validator_invalid_xml();
void test_pipe_menu_validator_invalid_root();

#endif // PIPE_TESTS_H
