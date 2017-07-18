#include <bpws_lws_main.h>

#include <assert.h>
#include <time.h>
#ifndef _WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif
#include <stdlib.h>
#include <string.h>

static void init()
{
}

static void cleanup()
{
}

BPWS_LWS_MAIN(init, cleanup)
