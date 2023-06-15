#ifndef __UNITY_RUNNER_H_
#define __UNITY_RUNNER_H_

#include "unity_fixture.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EXPORT_TEST_SETUP(group) void TEST_##group##_SETUP(void)
#define EXPORT_TEST_TEAR_DOWN(group) void TEST_##group##_TEAR_DOWN(void)

#define __unity_runner_init                                                    \
	__attribute__((__used__)) __attribute__((section(".unity.runner")))

struct unity_runner_init {
	const char *group;
	void (*runner)(void);
};

#define UNITY_GROUP_RUNNER(_group)                                             \
	void TEST_##_group##_GROUP_RUNNER(void);                               \
	static __unity_runner_init struct unity_runner_init                    \
		runner_init_##_group = {                                       \
			.group = #_group,                                      \
			.runner = TEST_##_group##_GROUP_RUNNER,                \
		};                                                             \
	void TEST_##_group##_GROUP_RUNNER(void)

#define __unity_test_case_init                                                 \
	__attribute__((__used__)) __attribute__((section(".unity.testcase")))

struct unity_test_case_init {
	const char *group;
	const char *name;
};

#define TEST_CASE(_group, _name)                                               \
	static __unity_test_case_init struct unity_test_case_init              \
		test_case_init_##_group##_##_name = {                          \
			.group = #_group,                                      \
			.name = #_name,                                        \
		};                                                             \
	TEST(_group, _name)

int unity_print_groups(void);
int unity_print_test_cases(const char *group);
int unity_run(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif

