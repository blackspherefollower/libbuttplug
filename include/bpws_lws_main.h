#ifndef BPWS_LWS_MAIN_H
#define BPWS_LWS_MAIN_H

#include <bpws_lws.h>

BPWS_BEGIN_EXTERN_C

typedef void(*bpws_lws_init_cleanup_function_t)();

BPWS_LWS_EXPORT int bpws_lws_main(int argc, char* argv[], bpws_lws_init_cleanup_function_t init, bpws_lws_init_cleanup_function_t cleanup);

#define BPWS_LWS_MAIN(init, cleanup) int main(int argc, char* argv[]) { return bpws_lws_main(argc, argv, init, cleanup); }

BPWS_END_EXTERN_C

#endif
