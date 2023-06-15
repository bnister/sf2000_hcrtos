#include <unity/unity.h>
#include <unity/unity_fixture.h>
#include <unity/unity_runner.h>

TEST_GROUP(dummy_group2);

TEST_SETUP(dummy_group2)
{
}

TEST_TEAR_DOWN(dummy_group2)
{
}

TEST_CASE(dummy_group2, ignored_test)
{
	TEST_IGNORE_MESSAGE("This Test Was Ignored On Purpose");
}

TEST_CASE(dummy_group2, another_ignored_test)
{
	TEST_IGNORE_MESSAGE(
		"These Can Be Useful For Leaving Yourself Notes On What You Need To Do Yet");
}

TEST_CASE(dummy_group2, test_case_need_to_be_implemented)
{
	TEST_IGNORE(); //Like This
}
