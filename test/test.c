#include "test.h"
#include <stdlib.h>

int* g_malloc_fails;
int g_malloc_count;

static int fail_alloc(void)
{
	if (g_malloc_fails) {
		int* mf;
		g_malloc_count += 1;
		for (mf = g_malloc_fails; *mf; ++mf) {
			if (*mf == g_malloc_count)
				return 1;
		}
	}
	return 0;
}

void testcase_set_malloc_fails(int* malloc_fails)
{
	g_malloc_fails = malloc_fails;
	g_malloc_count = 0;
}

void* bpws_calloc(size_t count, size_t size)
{
	if (fail_alloc())
		return NULL;
	return calloc(count, size);
}

void* bpws_malloc(size_t size)
{
	if (fail_alloc())
		return NULL;
	return malloc(size);
}

void* bpws_realloc(void* data, size_t size)
{
	if (fail_alloc())
		return NULL;
	if (size)
		return realloc(data, size);
	free(data);
	return NULL;
}

void bpws_free(void* data)
{
	free(data);
}


void testcase_setup(void)
{
	testcase_set_malloc_fails(NULL);
}

void testcase_teardown(void)
{
}

TCase *testcase_bpws(void)
{
	TCase *ret = tcase_create("BPWS");
	return ret;
}

START_TEST(string_2_message)
{
	char *in = "{\"Test\": {\"Id\": 3, \"TestString\": \"foo bar\"}}";
	struct bpws_msg_base_t *msg = bpws_parse_msg(in);
}
END_TEST

TCase *testcase_message_parse(void)
{
	TCase *ret = tcase_create("BPWS_MSG");
	tcase_add_test(ret, string_2_message);
	return ret;
}

int main(int argc, char* argv[])
{
	int number_failed;
	SRunner* sr;
	Suite* s = suite_create("BPWS");

	suite_add_tcase(s, testcase_bpws());
	suite_add_tcase(s, testcase_message_parse());

	sr = srunner_create(s);
	srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? 0 : 1;
}
