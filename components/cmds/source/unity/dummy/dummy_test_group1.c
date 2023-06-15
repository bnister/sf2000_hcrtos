#include <unity/unity.h>
#include <unity/unity_fixture.h>
#include <unity/unity_runner.h>

TEST_GROUP(dummy_group1);

TEST_SETUP(dummy_group1)
{
	printf("\nTEST_SETUP dummy_group1\n");
}

TEST_TEAR_DOWN(dummy_group1)
{
	printf("\nTEST_TEAR_DOWN dummy_group1\n");
}

TEST_CASE(dummy_group1, dummy_test_show_ignore)
{
	printf("Test case running.\n");
	TEST_IGNORE();
}

TEST_CASE(dummy_group1, dummy_test_show_ignore_message)
{
	printf("Test case running.\n");
	TEST_IGNORE_MESSAGE("This is my ignore message");
}

TEST_CASE(dummy_group1, dummy_test_will_be_passed)
{
	printf("Test case running.\n");
	TEST_ONLY();
}

TEST_CASE(dummy_group1, dummy_test_will_be_failed)
{
	printf("Test case running.\n");
	TEST_FAIL();
}

TEST_CASE(dummy_group1, dummy_test_will_be_failed_with_message)
{
	printf("Test case running.\n");
	TEST_FAIL_MESSAGE("This is my fail message");
}
