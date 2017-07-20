#ifndef TEST_H
#define TEST_H

#ifdef _MSC_VER
#define pid_t int
#pragma warning(disable:4100 4127)
#endif

#include <check.h>
#include "bpws.h"

BPWS_BEGIN_EXTERN_C

void testcase_set_malloc_fails(int* malloc_fails);
void testcase_setup(void);
void testcase_teardown(void);

TCase* testcase_bpws(void);
TCase* testcase_message_parse(void);

BPWS_END_EXTERN_C

#endif
