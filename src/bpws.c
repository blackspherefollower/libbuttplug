#include "bpws.h"

#include <string.h>
#include <libwebsockets.h>
#include <json-c/json.h>

BPWS_BEGIN_EXTERN_C


struct bpws_msg_base_t* bpws_parse_msg(char *msg)
{
	struct json_object *jobj;
	char *val_type_str, *str;
	int val_type;

	jobj = json_tokener_parse(msg);
	str = NULL;

	// key and val don't exist outside of this bloc
	json_object_object_foreach(jobj, key, val) {
		printf("key: \"%s\", type of val: ", key);
		val_type = json_object_get_type(val);

		switch (val_type) {
		case json_type_null:
			val_type_str = "val is NULL";
			break;

		case json_type_boolean:
			val_type_str = "val is a boolean";
			break;

		case json_type_double:
			val_type_str = "val is a double";
			break;

		case json_type_int:
			val_type_str = "val is an integer";
			break;

		case json_type_string:
			val_type_str = "val is a string";
			str = (char *)json_object_get_string(val);
			break;

		case json_type_object:
			val_type_str = "val is an object";
			break;

		case json_type_array:
			val_type_str = "val is an array";
			break;
		}
		printf("%s", val_type_str);

		if (str)
			printf("\t->\t\"%s\"", str);

		printf("\n");
		str = NULL;
	}

	return 0;
}

void bpws_delete_msg(struct bpws_msg_base_t *msg)
{

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
