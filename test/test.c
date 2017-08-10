#include "test.h"
#include <stdlib.h>
#include <stdint.h>

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

START_TEST(string_2_test_message)
{
	char *in = "{\"Test\": {\"Id\": 3, \"TestString\": \"foo bar\"}}";
	struct bpws_msg_base_t *msg = bpws_parse_msg(in);
	ck_assert_int_ne(msg, 0);
	ck_assert_int_eq(msg->id, 3);
	ck_assert_int_eq(msg->type, BPWS_MSG_TYPE_TEST);
	ck_assert_str_eq(((struct bpws_msg_test *) msg)->test_string, "foo bar");
	bpws_delete_msg(msg);
}
END_TEST


START_TEST(string_2_server_info_message)
{
	char *in = "{\"ServerInfo\": {\"Id\": 2, \"ServerName\": \"foo bar\", \"MajorVersion\": 1, \"MinorVersion\": 2, \"BuildVersion\": 3, \"MessageVersion\": 4, \"MaxPingTime\": 500}}";
	struct bpws_msg_base_t *msg = bpws_parse_msg(in);
	ck_assert_int_ne(msg, 0);
	ck_assert_int_eq(msg->id, 2);
	ck_assert_int_eq(msg->type, BPWS_MSG_TYPE_SERVER_INFO);
	ck_assert_str_eq(((struct bpws_msg_server_info *) msg)->server_name, "foo bar");
	ck_assert_int_eq(((struct bpws_msg_server_info *) msg)->major_version, 1);
	ck_assert_int_eq(((struct bpws_msg_server_info *) msg)->minor_version, 2);
	ck_assert_int_eq(((struct bpws_msg_server_info *) msg)->build_version, 3);
	ck_assert_int_eq(((struct bpws_msg_server_info *) msg)->message_version, 4);
	ck_assert_int_eq(((struct bpws_msg_server_info *) msg)->max_ping_time, 500);
	bpws_delete_msg(msg);
}
END_TEST


START_TEST(string_2_device_list_message)
{
	struct bpws_device_message_info *dev;
	char *in = "{\"DeviceList\": {\"Id\": 5, \"Devices\": [{ \"DeviceName\": \"foo\", \"DeviceIndex\": 6, \"DeviceMessages\": [ \"foo_cmd_1\", \"foo_cmd_2\" ]}, { \"DeviceName\": \"bar\", \"DeviceIndex\": 7, \"DeviceMessages\": [ \"bar_cmd_1\", \"bar_cmd_2\" ]}]}}";
	struct bpws_msg_base_t *msg = bpws_parse_msg(in);
	ck_assert_int_ne(msg, 0);
	ck_assert_int_eq(msg->id, 5);
	ck_assert_int_eq(msg->type, BPWS_MSG_TYPE_DEVICE_LIST);
	ck_assert_int_ne(((struct bpws_msg_device_list *) msg)->devices, 0);
	ck_assert_int_ne(((struct bpws_msg_device_list *) msg)->devices[0], 0);
	ck_assert_int_ne(((struct bpws_msg_device_list *) msg)->devices[1], 0);
	ck_assert_int_eq(((struct bpws_msg_device_list *) msg)->devices[2], 0);

	dev = ((struct bpws_msg_device_list *) msg)->devices[0];
	ck_assert_str_eq(dev->device_name, "foo");
	ck_assert_int_eq(dev->device_index, 6);
	ck_assert_int_ne(dev->device_messages, 0);
	ck_assert_int_ne(dev->device_messages[0], 0);
	ck_assert_int_ne(dev->device_messages[1], 0);
	ck_assert_int_eq(dev->device_messages[2], 0);
	ck_assert_str_eq(dev->device_messages[0], "foo_cmd_1");
	ck_assert_str_eq(dev->device_messages[1], "foo_cmd_2");


	dev = ((struct bpws_msg_device_list *) msg)->devices[1];
	ck_assert_str_eq(dev->device_name, "bar");
	ck_assert_int_eq(dev->device_index, 7);
	ck_assert_int_ne(dev->device_messages, 0);
	ck_assert_int_ne(dev->device_messages[0], 0);
	ck_assert_int_ne(dev->device_messages[1], 0);
	ck_assert_int_eq(dev->device_messages[2], 0);
	ck_assert_str_eq(dev->device_messages[0], "bar_cmd_1");
	ck_assert_str_eq(dev->device_messages[1], "bar_cmd_2");

	bpws_delete_msg(msg);
}
END_TEST

TCase *testcase_message_parse(void)
{
	TCase *ret = tcase_create("BPWS_MSG");
	tcase_add_test(ret, string_2_test_message);
	tcase_add_test(ret, string_2_server_info_message);
	tcase_add_test(ret, string_2_device_list_message);
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
