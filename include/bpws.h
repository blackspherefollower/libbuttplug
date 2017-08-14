#ifndef BPWS_H
#define BPWS_H

#include <stddef.h>

#ifdef __cplusplus
#define BPWS_BEGIN_EXTERN_C extern "C" {
#define BPWS_END_EXTERN_C }
#else
#define BPWS_BEGIN_EXTERN_C
#define BPWS_END_EXTERN_C
#endif

#if defined(_WIN32)
#define BPWS_EXPORT_DECLARATION __declspec(dllexport)
#define BPWS_IMPORT_DECLARATION __declspec(dllimport)
#elif defined(__GNUC__)
#define BPWS_EXPORT_DECLARATION __attribute__((visibility("default")))
#define BPWS_IMPORT_DECLARATION BPWS_EXPORT_DECLARATION
#else
#define BPWS_EXPORT_DECLARATION
#define BPWS_IMPORT_DECLARATION
#endif

#if defined(BPWS_STATIC)
#define BPWS_EXPORT
#elif defined(BPWS_EXPORTS)
#define BPWS_EXPORT BPWS_EXPORT_DECLARATION
#else
#define BPWS_EXPORT BPWS_IMPORT_DECLARATION
#endif

#if defined(_MSC_VER)
#define BPWS_CDECL __cdecl
#else
#define BPWS_CDECL
#endif

#define BPWS_SCHEMA_VERSION "0.1.0"

BPWS_BEGIN_EXTERN_C

struct bpws_t;
struct bpws_msg_base_t;

enum bpws_msg_type_t {
	BPWS_MSG_TYPE_OK,
	BPWS_MSG_TYPE_PING,
	BPWS_MSG_TYPE_TEST,
	BPWS_MSG_TYPE_ERROR,
	BPWS_MSG_TYPE_DEVICE_LIST,
	BPWS_MSG_TYPE_DEVICE_ADDED,
	BPWS_MSG_TYPE_DEVICE_REMOVED,
	BPWS_MSG_TYPE_REQUEST_DEVICE_LIST,
	BPWS_MSG_TYPE_START_SCANNING,
	BPWS_MSG_TYPE_STOP_SCANNING,
	BPWS_MSG_TYPE_SCANNING_FINISHED,
	BPWS_MSG_TYPE_REQUEST_LOG,
	BPWS_MSG_TYPE_LOG,
	BPWS_MSG_TYPE_REQUEST_SERVER_INFO,
	BPWS_MSG_TYPE_SERVER_INFO,
	BPWS_MSG_TYPE_FLESHLIGHT_LAUNCH_FW12_CMD,
	BPWS_MSG_TYPE_LOVENSE_CMD,
	BPWS_MSG_TYPE_KIIROO_CMD,
	BPWS_MSG_TYPE_VORZE_A10_CYCLONE_CMD,
	BPWS_MSG_TYPE_SINGLE_MOTOR_VIBRATE_CMD,
	BPWS_MSG_TYPE_STOP_DEVICE_CMD,
	BPWS_MSG_TYPE_STOP_ALL_DEVICES,
	BPWS_MSG_TYPE_LAST
};

struct bpws_msg_base_t {
	enum bpws_msg_type_t type;
	long long id;
};

struct bpws_msg_ok {
	enum bpws_msg_type_t type;
	long long id;
};

struct bpws_msg_ping {
	enum bpws_msg_type_t type;
	long long id;
};

struct bpws_msg_test {
	enum bpws_msg_type_t type;
	long long id;
	char *test_string;
};

enum error_class {
	ERROR_UNKNOWN,
	ERROR_INIT,
	ERROR_PING,
	ERROR_MSG,
	ERROR_DEVICE
};

struct bpws_msg_error {
	enum bpws_msg_type_t type;
	long long id;
	char *error_message;
	enum error_class error_code;
};

struct bpws_device_message_info {
	char* device_name;
	unsigned int device_index;
	char** device_messages;
};

struct bpws_msg_device_list {
	enum bpws_msg_type_t type;
	long long id;
	struct bpws_device_message_info **devices;
};

struct bpws_msg_device_added {
	enum bpws_msg_type_t type;
	long long id;
	char* device_name;
	unsigned int device_index;
	char** device_messages;
};

struct bpws_msg_device_removed {
	enum bpws_msg_type_t type;
	long long id;
	unsigned int device_index;
};

struct bpws_msg_request_device_list {
	enum bpws_msg_type_t type;
	long long id;
};

struct bpws_msg_start_scanning {
	enum bpws_msg_type_t type;
	long long id;
};

struct bpws_msg_stop_scanning {
	enum bpws_msg_type_t type;
	long long id;
};

struct bpws_msg_scanning_finished {
	enum bpws_msg_type_t type;
	long long id;
};

struct bpws_msg_request_log {
	enum bpws_msg_type_t type;
	long long id;
	char* log_level;
};

struct bpws_msg_log {
	enum bpws_msg_type_t type;
	long long id;
	char* log_level;
	char* log_message;
};

struct bpws_msg_request_server_info {
	enum bpws_msg_type_t type;
	long long id;
	char* client_name;
};

struct bpws_msg_server_info {
	enum bpws_msg_type_t type;
	long long id;
	int major_version;
	int minor_version;
	int build_version;
	unsigned int message_version;
	unsigned int max_ping_time;
	char* server_name;
};

struct bpws_msg_fleshlight_launch_fw12_cmd {
	enum bpws_msg_type_t type;
	long long id;
	unsigned int device_index;
	unsigned int speed;
	unsigned int position;
};

struct bpws_msg_lovense_cmd {
	enum bpws_msg_type_t type;
	long long id;
	unsigned int device_index;
	char* command;
};

struct bpws_msg_kiiroo_cmd {
	enum bpws_msg_type_t type;
	long long id;
	unsigned int device_index;
	char* command;
};

struct bpws_msg_vorze_a10_cyclone_cmd {
	enum bpws_msg_type_t type;
	long long id;
	unsigned int device_index;
	unsigned int speed;
	int clockwise;
};

struct bpws_msg_single_motor_vibrate_cmd {
	enum bpws_msg_type_t type;
	long long id;
	unsigned int device_index;
	double speed;
};

struct bpws_msg_stop_device_cmd {
	enum bpws_msg_type_t type;
	long long id;
	unsigned int device_index;
};

struct bpws_msg_stop_all_devices {
	enum bpws_msg_type_t type;
	long long id;
};

struct bpws_t {
	size_t out_message_pre_padding;
	size_t out_message_post_padding;
};

BPWS_EXPORT struct bpws_msg_request_server_info* bpws_new_msg_request_server_info(const char* client_name);
BPWS_EXPORT struct bpws_msg_ping* bpws_new_msg_ping(long long msgId);
BPWS_EXPORT struct bpws_msg_base_t* bpws_parse_msg(char* jmsg);
BPWS_EXPORT struct bpws_msg_base_t** bpws_parse_msgs(char* jmsg);
BPWS_EXPORT void bpws_delete_msg(struct bpws_msg_base_t* msg);
BPWS_EXPORT void bpws_delete_msgs(struct bpws_msg_base_t** msgs);
BPWS_EXPORT size_t bpws_format_msg(char *buf, size_t bufsize, struct bpws_msg_base_t* msg);
BPWS_EXPORT size_t bpws_format_msgs(char *buf, size_t bufsize, struct bpws_msg_base_t** msgs);

BPWS_EXPORT struct bpws_t* bpws_create(void);
BPWS_EXPORT void bpws_delete(struct bpws_t* bpws);

BPWS_END_EXTERN_C

#endif
