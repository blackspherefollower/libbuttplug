#include "bpws.h"

#include <string.h>
#include <libwebsockets.h>
#include <json-c/json.h>

BPWS_BEGIN_EXTERN_C

// The order here must match the oder in bpws_msg_type_t
char* bpws_msg_names[] = {
	"Ok",
	"Ping",
	"Test",
	"Error",
	"DeviceList",
	"DeviceAdded",
	"DeviceRemoved",
	"RequestDeviceList",
	"StartScanning",
	"StopScanning",
	"ScanningFinished",
	"RequestLog",
	"Log",
	"RequestServerInfo",
	"ServerInfo",
	"FleshlightLaunchFW12Cmd",
	"LovenseCmd",
	"KiirooCmd",
	"VorzeA10CycloneCmd",
	"SingleMotorVibrateCmd",
	"StopDeviceCmd",
	"StopAllDevices",
	0
};

static long long bpws_get_id(struct json_object *jobj)
{
	json_object_object_foreach(jobj, key, val) {
		if (!strncmp(key, "Id", 2) && json_object_get_type(val) == json_type_int)
			return json_object_get_int64(val);
	}

	return -1;
}

static char* bpws_get_string(struct json_object *jobj, char *field)
{
	char *ret;
	const char *str;

	ret = 0;
	json_object_object_foreach(jobj, key, val) {
		if (!strncmp(key, field, strlen(field)) &&
			json_object_get_type(val) == json_type_string)
		{
			str = json_object_get_string(val);
			ret = (char *)malloc(strlen(str) + 1);
			strcpy(ret, str);
			return ret;
		}
	}

	return 0;
}

static int bpws_get_double(struct json_object *jobj, char *field)
{
	json_object_object_foreach(jobj, key, val) {
		if (!strncmp(key, field, strlen(field)) &&
			json_object_get_type(val) == json_type_double)
			return json_object_get_double(val);
	}

	return -1;
}

static int bpws_get_int(struct json_object *jobj, char *field)
{
	json_object_object_foreach(jobj, key, val) {
		if (!strncmp(key, field, strlen(field)) &&
			json_object_get_type(val) == json_type_int)
			return json_object_get_int(val);
	}

	return -1;
}

static char** bpws_get_string_array(struct json_object *jobj, char *field)
{
	char **ret, **tmp;
	const char *str;
	int i, len, count;
	struct json_object *jarray, *jvalue;

	ret = (char**)malloc(sizeof(char*));
	ret[0] = 0;

	jarray = json_object_object_get(jobj, field);
	len = json_object_array_length( jarray );

	count = 0;
	for (i = 0; i < len; i++) {
		jvalue = json_object_array_get_idx(jarray, i);
		if (json_object_get_type(jvalue) == json_type_string)
		{
			count++;
			tmp = (char**)realloc(ret, sizeof(char*) * (count + 1));
			if (tmp)
				ret = tmp;
			else
				return ret; // we can go no further

			str = json_object_get_string(jvalue);
			ret[count - 1] = (char *)malloc(strlen(str) + 1);
			strcpy(ret[count - 1], str);
			ret[count] = 0;
		}
	}

	return ret;
}

struct bpws_device_message_info** bpws_get_device_array(struct json_object *jobj, char *field)
{
	struct bpws_device_message_info **ret, **tmp;
	int i, len, count;
	struct json_object *jarray, *jvalue;
	unsigned long long id;

	ret = (struct bpws_device_message_info**)malloc(sizeof(struct bpws_device_message_info*));
	ret[0] = 0;

	jarray = json_object_object_get(jobj, field);
	if (json_object_get_type(jarray) != json_type_array)
		return ret;

	len = json_object_array_length(jarray);

	id = bpws_get_id(jobj);
	count = 0;
	for (i = 0; i < len; i++) {
		jvalue = json_object_array_get_idx(jarray, i);
		if (json_object_get_type(jvalue) == json_type_object)
		{
			count++;
			tmp = (struct bpws_device_message_info**)realloc(ret, sizeof(struct bpws_device_message_info*) * (count + 1));
			if (tmp)
				ret = tmp;
			else
				return ret; // we can go no further

			ret[count - 1] = (struct bpws_device_message_info *) malloc(sizeof(struct bpws_device_message_info));
			((struct bpws_device_message_info *) ret[count - 1])->device_name = bpws_get_string(jvalue, "DeviceName");
			((struct bpws_device_message_info *) ret[count - 1])->device_index = bpws_get_int(jvalue, "DeviceIndex");
			((struct bpws_device_message_info *) ret[count - 1])->device_messages = bpws_get_string_array(jvalue, "DeviceMessages");
			ret[count] = 0;
		}
	}

	return ret;
}

static struct bpws_msg_base_t* bpws_parse_msg_json(struct json_object *jobj)
{
	struct bpws_msg_base_t *msg;
	int msgType;

	// key and val don't exist outside of this bloc
	json_object_object_foreach(jobj, key, val) {

		if (json_object_get_type(val) != json_type_object)
		{
			// We expect this to be an object, if it isn't error out
			return 0; // Would be better to construct an error object...
		}

		msgType = 0;
		while (bpws_msg_names[msgType])
		{
			if (!strncmp(key, bpws_msg_names[msgType], strlen(bpws_msg_names[msgType])))
				break;
			msgType++;
		}

		if (msgType == BPWS_MSG_TYPE_LAST)
		{
			// We expect this to be an known message type, it it isn't error out
			return 0; // Would be better to construct an error object...
		}

		switch (msgType)
		{
		case BPWS_MSG_TYPE_OK:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_ok));
			break;
		case BPWS_MSG_TYPE_PING:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_ping));
			break;
		case BPWS_MSG_TYPE_TEST:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_test));
			((struct bpws_msg_test *) msg)->test_string = bpws_get_string(val, "TestString");
			break;
		case BPWS_MSG_TYPE_ERROR:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_error));
			((struct bpws_msg_error *) msg)->error_message = bpws_get_string(val, "ErrorMessage");
			((struct bpws_msg_error *) msg)->error_code = bpws_get_int(val, "ErrorCode");
			break;
		case BPWS_MSG_TYPE_DEVICE_LIST:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_device_list));
			((struct bpws_msg_device_list *) msg)->devices = bpws_get_device_array(val, "Devices");
			break;
		case BPWS_MSG_TYPE_DEVICE_ADDED:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_device_added));
			((struct bpws_msg_device_added *) msg)->device_name = bpws_get_string(val, "DeviceName");
			((struct bpws_msg_device_added *) msg)->device_index = bpws_get_int(val, "DeviceIndex");
			((struct bpws_msg_device_added *) msg)->device_messages = bpws_get_string_array(val, "DeviceMessages");
			break;
		case BPWS_MSG_TYPE_DEVICE_REMOVED:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_device_removed));
			((struct bpws_msg_device_removed *) msg)->device_index = bpws_get_int(val, "DeviceIndex");
			break;
		case BPWS_MSG_TYPE_REQUEST_DEVICE_LIST:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_request_device_list));
			break;
		case BPWS_MSG_TYPE_START_SCANNING:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_start_scanning));
			break;
		case BPWS_MSG_TYPE_STOP_SCANNING:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_stop_scanning));
			break;
		case BPWS_MSG_TYPE_SCANNING_FINISHED:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_scanning_finished));
			break;
		case BPWS_MSG_TYPE_REQUEST_LOG:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_request_log));
			((struct bpws_msg_request_log *) msg)->log_level = bpws_get_string(val, "LogLevel");
			break;
		case BPWS_MSG_TYPE_LOG:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_log));
			((struct bpws_msg_log *) msg)->log_level = bpws_get_string(val, "LogLevel");
			((struct bpws_msg_log *) msg)->log_message = bpws_get_string(val, "LogMessage");
			break;
		case BPWS_MSG_TYPE_REQUEST_SERVER_INFO:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_request_server_info));
			((struct bpws_msg_request_server_info *) msg)->client_name = bpws_get_string(val, "ClientName");
			break;
		case BPWS_MSG_TYPE_SERVER_INFO:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_server_info));
			((struct bpws_msg_server_info *) msg)->major_version = bpws_get_int(val, "MajorVersion");
			((struct bpws_msg_server_info *) msg)->minor_version = bpws_get_int(val, "MinorVersion");
			((struct bpws_msg_server_info *) msg)->build_version = bpws_get_int(val, "BuildVersion");
			((struct bpws_msg_server_info *) msg)->message_version = bpws_get_int(val, "MessageVersion");
			((struct bpws_msg_server_info *) msg)->max_ping_time = bpws_get_int(val, "MaxPingTime");
			((struct bpws_msg_server_info *) msg)->server_name = bpws_get_string(val, "ServerName");
			break;
		case BPWS_MSG_TYPE_FLESHLIGHT_LAUNCH_FW12_CMD:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_fleshlight_launch_fw12_cmd));
			((struct bpws_msg_fleshlight_launch_fw12_cmd *) msg)->device_index = bpws_get_int(val, "DeviceIndex");
			((struct bpws_msg_fleshlight_launch_fw12_cmd *) msg)->speed = bpws_get_int(val, "Speed");
			((struct bpws_msg_fleshlight_launch_fw12_cmd *) msg)->position = bpws_get_int(val, "Position");
			break;
		case BPWS_MSG_TYPE_LOVENSE_CMD:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_lovense_cmd));
			((struct bpws_msg_lovense_cmd *) msg)->device_index = bpws_get_int(val, "DeviceIndex");
			((struct bpws_msg_lovense_cmd *) msg)->command = bpws_get_string(val, "Command");
			break;
		case BPWS_MSG_TYPE_KIIROO_CMD:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_kiiroo_cmd));
			((struct bpws_msg_kiiroo_cmd *) msg)->device_index = bpws_get_int(val, "DeviceIndex");
			((struct bpws_msg_kiiroo_cmd *) msg)->command = bpws_get_string(val, "Command");
			break;
		case BPWS_MSG_TYPE_VORZE_A10_CYCLONE_CMD:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_vorze_a10_cyclone_cmd));
			((struct bpws_msg_vorze_a10_cyclone_cmd *) msg)->device_index = bpws_get_int(val, "DeviceIndex");
			((struct bpws_msg_vorze_a10_cyclone_cmd *) msg)->speed = bpws_get_int(val, "Speed");
			((struct bpws_msg_vorze_a10_cyclone_cmd *) msg)->clockwise = bpws_get_int(val, "Clockwise");
			break;
		case BPWS_MSG_TYPE_SINGLE_MOTOR_VIBRATE_CMD:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_single_motor_vibrate_cmd));
			((struct bpws_msg_single_motor_vibrate_cmd *) msg)->device_index = bpws_get_int(val, "DeviceIndex");
			((struct bpws_msg_single_motor_vibrate_cmd *) msg)->speed = bpws_get_double(val, "Speed");
			break;
		case BPWS_MSG_TYPE_STOP_DEVICE_CMD:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_stop_device_cmd));
			((struct bpws_msg_stop_device_cmd *) msg)->device_index = bpws_get_int(val, "DeviceIndex");
			break;
		case BPWS_MSG_TYPE_STOP_ALL_DEVICES:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_stop_all_devices));
			break;
		default:
			// I don't know how we got here!
			return 0; // Would be better to construct an error object...
		}

		msg->id = bpws_get_id(val);
		msg->type = msgType;
	}

	return msg;
}

struct bpws_msg_base_t* bpws_parse_msg(char *jmsg)
{
	struct json_object *jobj;
	enum json_tokener_error error = 0;
	struct bpws_msg_base_t* ret = 0;

	jobj = json_tokener_parse_verbose(jmsg, &error);

	if (!jobj || json_object_get_type(jobj) != json_type_object)
	{
		// We expect this to be an object, if it isn't error out
		return 0; // Would be better to construct an error object...
	}

	ret = bpws_parse_msg_json(jobj);
	json_object_put(jobj);
	return ret;
}

struct bpws_msg_base_t** bpws_parse_msgs(char *jarr)
{
	struct json_object *jobj;
	enum json_tokener_error error = 0;
	struct bpws_msg_base_t** ret = 0;
	int i = 0;

	jobj = json_tokener_parse_verbose(jarr, &error);

	if (!jobj || json_object_get_type(jobj) != json_type_array)
	{
		// We expect this to be an object, if it isn't error out
		return 0; // Would be better to construct an error object...
	}

	ret = (struct bpws_msg_base_t**) malloc(sizeof(struct bpws_msg_base_t*) * (json_object_array_length(jobj) + 1));

	for (i = 0; i < json_object_array_length(jobj); i++)
	{
		ret[i] = bpws_parse_msg_json(json_object_array_get_idx(jobj, i));
	}
	ret[i] = 0;

	json_object_put(jobj);
	return ret;
}


static struct json_object *bpws_format_dmi_json(struct bpws_device_message_info* dmi)
{
	struct json_object *jobj;
	struct json_object *jarr;
	int i;

	jobj = json_object_new_object();
	json_object_object_add(jobj, "DeviceIndex", json_object_new_int(dmi->device_index));
	json_object_object_add(jobj, "DeviceName", json_object_new_string(dmi->device_name));
	jarr = json_object_new_array();
	for (i = 0; dmi->device_messages[i]; i++)
	{
		json_object_array_add(jarr, json_object_new_string(dmi->device_messages[i]));
	}
	json_object_object_add(jobj, "DeviceMessages", jarr);

	return jobj;
}

static struct json_object *bpws_format_msg_json(struct bpws_msg_base_t* msg)
{
	int len = 0;
	struct json_object *jobj;
	struct json_object *jobj2;
	struct json_object *jarr;
	int i;

	jobj = json_object_new_object();
	jobj2 = json_object_new_object();

	if (msg->type >= 0 && msg->type < BPWS_MSG_TYPE_LAST)
	{
		json_object_object_add(jobj, bpws_msg_names[msg->type], jobj2);
	}
	else
	{
		json_object_object_add(jobj, "Unknown", jobj2);
	}
	json_object_object_add(jobj2, "Id", json_object_new_int64(msg->id));

	switch (msg->type)
	{
	case BPWS_MSG_TYPE_OK:
		break;
	case BPWS_MSG_TYPE_PING:
		break;
	case BPWS_MSG_TYPE_TEST:
		json_object_object_add(jobj2, "TestString", json_object_new_string(((struct bpws_msg_test *)msg)->test_string));
		break;
	case BPWS_MSG_TYPE_ERROR:
		json_object_object_add(jobj2, "ErrorCode", json_object_new_int(((struct bpws_msg_error *)msg)->error_code));
		json_object_object_add(jobj2, "ErrorMessage", json_object_new_string(((struct bpws_msg_error *)msg)->error_message));
		break;
	case BPWS_MSG_TYPE_DEVICE_LIST:
		jarr = json_object_new_array();
		for (i = 0; ((struct bpws_msg_device_list *) msg)->devices[i]; i++)
		{
			json_object_array_add(jarr, bpws_format_dmi_json(((struct bpws_msg_device_list *) msg)->devices[i]));
		}
		json_object_object_add(jobj2, "Devices", jarr);
		break;
	case BPWS_MSG_TYPE_DEVICE_ADDED:
		json_object_object_add(jobj2, "DeviceIndex", json_object_new_int(((struct bpws_msg_device_added *)msg)->device_index));
		json_object_object_add(jobj2, "DeviceName", json_object_new_string(((struct bpws_msg_device_added *)msg)->device_name));
		jarr = json_object_new_array();
		for (i = 0; ((struct bpws_msg_device_added *) msg)->device_messages[i]; i++)
		{
			json_object_array_add(jarr, json_object_new_string(((struct bpws_msg_device_added *) msg)->device_messages[i]));
		}
		json_object_object_add(jobj2, "DeviceMessages", jarr);
		break;
	case BPWS_MSG_TYPE_DEVICE_REMOVED:
		json_object_object_add(jobj2, "DeviceIndex", json_object_new_int(((struct bpws_msg_device_removed *)msg)->device_index));
		break;
	case BPWS_MSG_TYPE_REQUEST_DEVICE_LIST:
		break;
	case BPWS_MSG_TYPE_START_SCANNING:
		break;
	case BPWS_MSG_TYPE_STOP_SCANNING:
		break;
	case BPWS_MSG_TYPE_SCANNING_FINISHED:
		break;
	case BPWS_MSG_TYPE_REQUEST_LOG:
		json_object_object_add(jobj2, "LogLevel", json_object_new_string(((struct bpws_msg_request_log *)msg)->log_level));
		break;
	case BPWS_MSG_TYPE_LOG:
		json_object_object_add(jobj2, "LogLevel", json_object_new_string(((struct bpws_msg_log *)msg)->log_level));
		json_object_object_add(jobj2, "LogMessage", json_object_new_string(((struct bpws_msg_log *)msg)->log_message));
		break;
	case BPWS_MSG_TYPE_REQUEST_SERVER_INFO:
		json_object_object_add(jobj2, "ClientName", json_object_new_string(((struct bpws_msg_request_server_info *)msg)->client_name));
		break;
	case BPWS_MSG_TYPE_SERVER_INFO:
		json_object_object_add(jobj2, "MajorVersion", json_object_new_int(((struct bpws_msg_server_info *)msg)->major_version));
		json_object_object_add(jobj2, "MinorVersion", json_object_new_int(((struct bpws_msg_server_info *)msg)->minor_version));
		json_object_object_add(jobj2, "BuildVersion", json_object_new_int(((struct bpws_msg_server_info *)msg)->build_version));
		json_object_object_add(jobj2, "MessageVersion", json_object_new_int(((struct bpws_msg_server_info *)msg)->message_version));
		json_object_object_add(jobj2, "MaxPingTime", json_object_new_int(((struct bpws_msg_server_info *)msg)->max_ping_time));
		json_object_object_add(jobj2, "ServerName", json_object_new_string(((struct bpws_msg_server_info *)msg)->server_name));
		break;
	case BPWS_MSG_TYPE_FLESHLIGHT_LAUNCH_FW12_CMD:
		json_object_object_add(jobj2, "DeviceIndex", json_object_new_int(((struct bpws_msg_fleshlight_launch_fw12_cmd *)msg)->device_index));
		json_object_object_add(jobj2, "Speed", json_object_new_int(((struct bpws_msg_fleshlight_launch_fw12_cmd *)msg)->speed));
		json_object_object_add(jobj2, "Position", json_object_new_int(((struct bpws_msg_fleshlight_launch_fw12_cmd *)msg)->position));
		break;
	case BPWS_MSG_TYPE_LOVENSE_CMD:
		json_object_object_add(jobj2, "DeviceIndex", json_object_new_int(((struct bpws_msg_lovense_cmd *)msg)->device_index));
		json_object_object_add(jobj2, "Command", json_object_new_string(((struct bpws_msg_lovense_cmd *)msg)->command));
		break;
	case BPWS_MSG_TYPE_KIIROO_CMD:
		json_object_object_add(jobj2, "DeviceIndex", json_object_new_int(((struct bpws_msg_kiiroo_cmd *)msg)->device_index));
		json_object_object_add(jobj2, "Command", json_object_new_string(((struct bpws_msg_kiiroo_cmd *)msg)->command));
		break;
	case BPWS_MSG_TYPE_VORZE_A10_CYCLONE_CMD:
		json_object_object_add(jobj2, "DeviceIndex", json_object_new_int(((struct bpws_msg_vorze_a10_cyclone_cmd *)msg)->device_index));
		json_object_object_add(jobj2, "Speed", json_object_new_int(((struct bpws_msg_vorze_a10_cyclone_cmd *)msg)->speed));
		json_object_object_add(jobj2, "Clockwise", json_object_new_int(((struct bpws_msg_vorze_a10_cyclone_cmd *)msg)->clockwise));
		break;
	case BPWS_MSG_TYPE_SINGLE_MOTOR_VIBRATE_CMD:
		json_object_object_add(jobj2, "DeviceIndex", json_object_new_int(((struct bpws_msg_single_motor_vibrate_cmd *)msg)->device_index));
		json_object_object_add(jobj2, "Speed", json_object_new_double(((struct bpws_msg_single_motor_vibrate_cmd *)msg)->speed));
		break;
	case BPWS_MSG_TYPE_STOP_DEVICE_CMD:
		json_object_object_add(jobj2, "DeviceIndex", json_object_new_int(((struct bpws_msg_stop_device_cmd *)msg)->device_index));
		break;
	case BPWS_MSG_TYPE_STOP_ALL_DEVICES:
		break;
	}

	return jobj;
}

size_t bpws_format_msg(char *buf, size_t bufsize, struct bpws_msg_base_t* msg)
{
	struct json_object *jarr;
	char *tmp;
	size_t len;

	jarr = json_object_new_array();
	json_object_array_add(jarr, bpws_format_msg_json(msg));

	tmp = json_object_to_json_string(jarr);
	if (!tmp)
	{
		json_object_put(jarr);
		return 0;
	}

	len = strlen(tmp);
	strncpy(buf, tmp, bufsize);

	json_object_put(jarr);
	return len;
}

size_t bpws_format_msgs(char *buf, size_t bufsize, struct bpws_msg_base_t** msgs)
{
	struct json_object *jarr;
	int i;
	char *tmp;
	size_t len;

	jarr = json_object_new_array();
	for (i = 0; msgs[i]; i++)
	{
		json_object_array_add(jarr, bpws_format_msg_json(msgs[i]));
	}

	tmp = json_object_to_json_string(jarr);
	if (!tmp)
	{
		json_object_put(jarr);
		return 0;
	}

	len = strlen(tmp);
	strncpy(buf, tmp, bufsize);

	json_object_put(jarr);
	return len;
}

struct bpws_msg_request_server_info* bpws_new_msg_request_server_info(const char* client_name)
{
	struct bpws_msg_request_server_info *msg;
	msg = (struct bpws_msg_request_server_info *) malloc(sizeof(struct bpws_msg_request_server_info));
	msg->type = BPWS_MSG_TYPE_REQUEST_SERVER_INFO;
	msg->id = 1;
	msg->client_name = (char *)malloc(strlen(client_name) + 1);
	strcpy(msg->client_name, client_name);
	return msg;
}

struct bpws_msg_ping* bpws_new_msg_ping(long long msgId)
{
	struct bpws_msg_ping *msg;
	msg = (struct bpws_msg_ping *) malloc(sizeof(struct bpws_msg_ping));
	msg->type = BPWS_MSG_TYPE_PING;
	msg->id = msgId;
	return msg;
}

static void bpws_delete_dmi(struct bpws_device_message_info *dmi)
{
	int i;
	void **arr;

	arr = dmi->device_messages;
	for (i = 0; arr[i]; i++)
		free(((char**)arr)[i]);
	free((char**)arr);
	free(dmi->device_name);
	free(dmi);
}

void bpws_delete_msg(struct bpws_msg_base_t *msg)
{
	int i;
	void **arr;

	switch (msg->type)
	{
	case BPWS_MSG_TYPE_OK:
		free((struct bpws_msg_ok *) msg);
		break;
	case BPWS_MSG_TYPE_PING:
		free((struct bpws_msg_ok *) msg);
		break;
	case BPWS_MSG_TYPE_TEST:
		free(((struct bpws_msg_test *) msg)->test_string);
		free((struct bpws_msg_test *) msg);
		break;
	case BPWS_MSG_TYPE_ERROR:
		free(((struct bpws_msg_error *) msg)->error_message);
		free((struct bpws_msg_error *) msg);
		break;
	case BPWS_MSG_TYPE_DEVICE_LIST:
		arr = ((struct bpws_msg_device_list *) msg)->devices;
		for (i = 0; arr[i]; i++)
			bpws_delete_dmi(arr[i]);
		free((struct bpws_device_message_info **)arr);
		free((struct bpws_msg_device_list *) msg);
		break;
	case BPWS_MSG_TYPE_DEVICE_ADDED:
		arr = ((struct bpws_msg_device_added *) msg)->device_messages;
		for (i = 0; arr[i]; i++)
			free((char*)arr[i]);
		free(arr);
		free(((struct bpws_msg_device_added *) msg)->device_name);
		free((struct bpws_msg_device_added *) msg);
		break;
	case BPWS_MSG_TYPE_DEVICE_REMOVED:
		free((struct bpws_msg_device_removed *) msg);
		break;
	case BPWS_MSG_TYPE_REQUEST_DEVICE_LIST:
		free((struct bpws_msg_request_device_list *) msg);
		break;
	case BPWS_MSG_TYPE_START_SCANNING:
		free((struct bpws_msg_start_scanning *) msg);
		break;
	case BPWS_MSG_TYPE_STOP_SCANNING:
		free((struct bpws_msg_stop_scanning *) msg);
		break;
	case BPWS_MSG_TYPE_SCANNING_FINISHED:
		free((struct bpws_msg_scanning_finished *) msg);
		break;
	case BPWS_MSG_TYPE_REQUEST_LOG:
		free(((struct bpws_msg_request_log *) msg)->log_level);
		free((struct bpws_msg_request_log *) msg);
		break;
	case BPWS_MSG_TYPE_LOG:
		free(((struct bpws_msg_log *) msg)->log_level);
		free(((struct bpws_msg_log *) msg)->log_message);
		free((struct bpws_msg_log *) msg);
		break;
	case BPWS_MSG_TYPE_REQUEST_SERVER_INFO:
		free(((struct bpws_msg_request_server_info *) msg)->client_name);
		free((struct bpws_msg_request_server_info *) msg);
		break;
	case BPWS_MSG_TYPE_SERVER_INFO:
		free(((struct bpws_msg_server_info *) msg)->server_name);
		free((struct bpws_msg_server_info *) msg);
		break;
	case BPWS_MSG_TYPE_FLESHLIGHT_LAUNCH_FW12_CMD:
		free((struct bpws_msg_fleshlight_launch_fw12_cmd *) msg);
		break;
	case BPWS_MSG_TYPE_LOVENSE_CMD:
		free(((struct bpws_msg_lovense_cmd *) msg)->command);
		free((struct bpws_msg_lovense_cmd *) msg);
		break;
	case BPWS_MSG_TYPE_KIIROO_CMD:
		free(((struct bpws_msg_kiiroo_cmd *) msg)->command);
		free((struct bpws_msg_kiiroo_cmd *) msg);
		break;
	case BPWS_MSG_TYPE_VORZE_A10_CYCLONE_CMD:
		free((struct bpws_msg_vorze_a10_cyclone_cmd *) msg);
		break;
	case BPWS_MSG_TYPE_SINGLE_MOTOR_VIBRATE_CMD:
		free((struct bpws_msg_single_motor_vibrate_cmd *) msg);
		break;
	case BPWS_MSG_TYPE_STOP_DEVICE_CMD:
		free((struct bpws_msg_stop_device_cmd *) msg);
		break;
	case BPWS_MSG_TYPE_STOP_ALL_DEVICES:
		free((struct bpws_msg_stop_all_devices *) msg);
		break;
	}
}

void bpws_delete_msgs(struct bpws_msg_base_t **msgs)
{
	int i;
	if (!msgs)
		return;
	for (i = 0; msgs[i]; i++)
		bpws_delete_msg(msgs[i]);
	free(msgs);
}


struct bpws_t* bpws_create(void)
{
	/*
	struct bpws_internal_t* ret;
	ret = wpcp_calloc(1, sizeof(*ret));
	if (!ret)
		return NULL;
	return &ret->bpws;
	*/
	return 0;
}

void bpws_delete(struct bpws_t* bpws)
{
	//bpws_free(bpws);
}



BPWS_END_EXTERN_C
