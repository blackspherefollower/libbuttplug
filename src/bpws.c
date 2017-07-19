#include "bpws.h"

#include <string.h>
#include <libwebsockets.h>

BPWS_BEGIN_EXTERN_C

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

static int deny_deflate, longlived, mirror_lifetime, test_post;
static struct lws *wsi_dumb, *wsi_mirror;
static struct lws *wsi_multi[3];
static volatile int force_exit;
static unsigned int opts, rl_multi[3];
static int flag_no_mirror_traffic;

#if defined(LWS_OPENSSL_SUPPORT) && defined(LWS_HAVE_SSL_CTX_set1_param)
char crl_path[1024] = "";
#endif


static int
callback_lws_mirror(struct lws *wsi, enum lws_callback_reasons reason,
	void *user, void *in, size_t len)
{
	unsigned char buf[LWS_PRE + 4096];
	unsigned int rands[4];
	int l = 0;
	int n;

	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED:

		lwsl_notice("mirror: LWS_CALLBACK_CLIENT_ESTABLISHED\n");

		lws_get_random(lws_get_context(wsi), rands, sizeof(rands[0]));
		mirror_lifetime = 16384 + (rands[0] & 65535);
		/* useful to test single connection stability */
		if (longlived)
			mirror_lifetime += 500000;

		lwsl_info("opened mirror connection with "
			"%d lifetime\n", mirror_lifetime);

		/*
		* mirror_lifetime is decremented each send, when it reaches
		* zero the connection is closed in the send callback.
		* When the close callback comes, wsi_mirror is set to NULL
		* so a new connection will be opened
		*
		* start the ball rolling,
		* LWS_CALLBACK_CLIENT_WRITEABLE will come next service
		*/
		if (!flag_no_mirror_traffic)
			lws_callback_on_writable(wsi);
		break;

	case LWS_CALLBACK_CLOSED:
		lwsl_notice("mirror: LWS_CALLBACK_CLOSED mirror_lifetime=%d\n", mirror_lifetime);
		wsi_mirror = NULL;
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:
		if (flag_no_mirror_traffic)
			return 0;
		for (n = 0; n < 1; n++) {
			lws_get_random(lws_get_context(wsi), rands, sizeof(rands));
			l += sprintf((char *)&buf[LWS_PRE + l],
				"c #%06X %u %u %u;",
				rands[0] & 0xffffff,	/* colour */
				rands[1] & 511,		/* x */
				rands[2] & 255,		/* y */
				(rands[3] & 31) + 1);	/* radius */
		}

		n = lws_write(wsi, &buf[LWS_PRE], l,
			opts | LWS_WRITE_TEXT);
		if (n < 0)
			return -1;
		if (n < l) {
			lwsl_err("Partial write LWS_CALLBACK_CLIENT_WRITEABLE\n");
			return -1;
		}

		mirror_lifetime--;
		if (!mirror_lifetime) {
			lwsl_info("closing mirror session\n");
			return -1;
		}
		/* get notified as soon as we can write again */
		lws_callback_on_writable(wsi);
		break;

	default:
		break;
	}

	return 0;
}



BPWS_END_EXTERN_C
