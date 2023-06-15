#include <unity/unity.h>
#include <unity/unity_fixture.h>
#include <unity/unity_runner.h>

UNITY_GROUP_RUNNER(dummy_group2)
{
	RUN_TEST_CASE(dummy_group2, ignored_test);
	RUN_TEST_CASE(dummy_group2, another_ignored_test);
	RUN_TEST_CASE(dummy_group2, test_case_need_to_be_implemented);
}
