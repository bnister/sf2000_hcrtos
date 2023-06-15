#include <unity/unity.h>
#include <unity/unity_fixture.h>
#include <unity/unity_runner.h>

UNITY_GROUP_RUNNER(dummy_group1)
{
	RUN_TEST_CASE(dummy_group1, dummy_test_show_ignore);
	RUN_TEST_CASE(dummy_group1, dummy_test_show_ignore_message);
	RUN_TEST_CASE(dummy_group1, dummy_test_will_be_passed);
	RUN_TEST_CASE(dummy_group1, dummy_test_will_be_failed);
	RUN_TEST_CASE(dummy_group1, dummy_test_will_be_failed_with_message);
}
