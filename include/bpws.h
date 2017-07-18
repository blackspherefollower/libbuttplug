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
	BPWS_MSG_TYPE_ERROR,
	BPWS_MSG_TYPE_TEST,
	BPWS_MSG_TYPE_PING,
};

struct bpws_msg_base_t {
	unsigned int id;
	enum bpws_msg_type_t type;
};

struct bpws_t {
	size_t out_message_pre_padding;
	size_t out_message_post_padding;
};


BPWS_EXPORT struct bpws_t* bpws_create(void);
BPWS_EXPORT void bpws_delete(struct bpws_t* bpws);

BPWS_END_EXTERN_C

#endif
