#ifndef BPWS_LWS_H
#define BPWS_LWS_H

#include <bpws.h>

#if defined(BPWS_LWS_STATIC)
#define BPWS_LWS_EXPORT
#elif defined(BPWS_LWS_EXPORTS)
#define BPWS_LWS_EXPORT BPWS_EXPORT_DECLARATION
#else
#define BPWS_LWS_EXPORT BPWS_IMPORT_DECLARATION
#endif

BPWS_BEGIN_EXTERN_C

BPWS_END_EXTERN_C

#endif
