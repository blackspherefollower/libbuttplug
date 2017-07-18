#include "bpws_lws_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning(push, 2)
#endif
#include <libwebsockets.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#ifndef _WIN32_WCE
#include <signal.h>
#endif

BPWS_BEGIN_EXTERN_C

int force_exit;

#ifndef _WIN32_WCE
static void sighandler(int sig)
{
	(void)sig;
	force_exit = 1;
}
#endif

int bpws_lws_main(int argc, char* argv[], bpws_lws_init_cleanup_function_t init, bpws_lws_init_cleanup_function_t cleanup)
{
	int ret;

#ifndef _WIN32_WCE
	signal(SIGINT, sighandler);
#endif

	ret = 0;
	force_exit = 0;

	if (init)
		init();

	if (cleanup)
		cleanup();

	return ret;
}

BPWS_END_EXTERN_C
