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
	"DeviceMessageInfo",
	"DeviceList",
	"DeviceAdded",
	"DeviceRemoved",
	"RequestDeviceList",
	"StartScanning,"
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
	int val_type;

	json_object_object_foreach(jobj, key, val) {
		if (!strncmp(key, "Id", 2) && json_object_get_type(val) == json_type_int)
			return json_object_get_int64(val);
	}

	return -1;
}

static char* bpws_get_string(struct json_object *jobj, char *field)
{
	int val_type;
	char *ret, *str;

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

struct bpws_msg_base_t* bpws_parse_msg(char *jmsg)
{
	struct json_object *jobj;
	struct bpws_msg_base_t *msg;
	char *val_type_str;
	int val_type;
	int msgType;

	jobj = json_tokener_parse(jmsg);
	msg = 0;

	// key and val don't exist outside of this bloc
	json_object_object_foreach(jobj, key, val) {
		printf("key: \"%s\", type of val: ", key);
		val_type = json_object_get_type(val);

		if (val_type != json_type_object)
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
			break;
		case BPWS_MSG_TYPE_DEVICE_MESSAGE_INFO:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_device_message_info));
			break;
		case BPWS_MSG_TYPE_DEVICE_LIST:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_device_list));
			break;
		case BPWS_MSG_TYPE_DEVICE_ADDED:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_device_added));
			break;
		case BPWS_MSG_TYPE_DEVICE_REMOVED:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_device_removed));
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
			break;
		case BPWS_MSG_TYPE_LOG:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_log));
			break;
		case BPWS_MSG_TYPE_REQUEST_SERVER_INFO:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_request_server_info));
			break;
		case BPWS_MSG_TYPE_SERVER_INFO:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_server_info));
			break;
		case BPWS_MSG_TYPE_FLESHLIGHT_LAUNCH_FW12_CMD:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_fleshlight_launch_fw12_cmd));
			break;
		case BPWS_MSG_TYPE_LOVENSE_CMD:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_lovense_cmd));
			break;
		case BPWS_MSG_TYPE_KIIROO_CMD:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_kiiroo_cmd));
			break;
		case BPWS_MSG_TYPE_VORZE_A10_CYCLONE_CMD:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_vorze_a10_cyclone_cmd));
			break;
		case BPWS_MSG_TYPE_SINGLE_MOTOR_VIBRATE_CMD:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_single_motor_vibrate_cmd));
			break;
		case BPWS_MSG_TYPE_STOP_DEVICE_CMD:
			msg = (struct bpws_msg_base_t *) malloc(sizeof(struct bpws_msg_stop_device_cmd));
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

void bpws_delete_msg(struct bpws_msg_base_t *msg)
{
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
		free((struct bpws_msg_error *) msg);
		break;
	case BPWS_MSG_TYPE_DEVICE_MESSAGE_INFO:
		free((struct bpws_msg_device_message_info *) msg);
		break;
	case BPWS_MSG_TYPE_DEVICE_LIST:
		free((struct bpws_msg_device_list *) msg);
		break;
	case BPWS_MSG_TYPE_DEVICE_ADDED:
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
		free((struct bpws_msg_request_log *) msg);
		break;
	case BPWS_MSG_TYPE_LOG:
		free((struct bpws_msg_log *) msg);
		break;
	case BPWS_MSG_TYPE_REQUEST_SERVER_INFO:
		free((struct bpws_msg_request_server_info *) msg);
		break;
	case BPWS_MSG_TYPE_SERVER_INFO:
		free((struct bpws_msg_server_info *) msg);
		break;
	case BPWS_MSG_TYPE_FLESHLIGHT_LAUNCH_FW12_CMD:
		free((struct bpws_msg_fleshlight_launch_fw12_cmd *) msg);
		break;
	case BPWS_MSG_TYPE_LOVENSE_CMD:
		free((struct bpws_msg_lovense_cmd *) msg);
		break;
	case BPWS_MSG_TYPE_KIIROO_CMD:
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
