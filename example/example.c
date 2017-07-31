#include <bpws.h>

#include <assert.h>
#include <time.h>
#ifdef _WIN32
#else
#include <sys/time.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <libwebsockets.h>

#ifdef _WIN32

int gettimeofday(struct timeval * tp, struct timezone * tzp)
{
	// Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
	// This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
	// until 00:00:00 January 1, 1970 
	static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

	SYSTEMTIME  system_time;
	FILETIME    file_time;
	uint64_t    time;

	GetSystemTime(&system_time);
	SystemTimeToFileTime(&system_time, &file_time);
	time = ((uint64_t)file_time.dwLowDateTime);
	time += ((uint64_t)file_time.dwHighDateTime) << 32;

	tp->tv_sec = (long)((time - EPOCH) / 10000000L);
	tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
	return 0;
}

#endif


static int deny_deflate, longlived, test_post;
static struct lws *wsi_mirror;
static volatile int force_exit;
static unsigned int opts, rl_multi[3];
static int flag_no_mirror_traffic;

void sighandler(int sig)
{
	force_exit = 1;
}

static int
callback_lws_buttplug(struct lws *wsi, enum lws_callback_reasons reason,
	void *user, void *in, size_t len)
{
	int bufsize = LWS_PRE + 4096;
	unsigned char buf[LWS_PRE + 4096];
	int l = 0;
	int n;
	int i;
	struct bpws_msg_base_t **msgs = 0;
	struct bpws_msg_base_t *msg = 0;

	switch (reason) {
	case LWS_CALLBACK_CLIENT_ESTABLISHED:

		lwsl_notice("butplug: LWS_CALLBACK_CLIENT_ESTABLISHED\n");
		/*
		 * start the ball rolling,
		 * LWS_CALLBACK_CLIENT_WRITEABLE will come next service
		 */
		if (!flag_no_mirror_traffic)
			lws_callback_on_writable(wsi);
		break;

	case LWS_CALLBACK_CLOSED:
		lwsl_notice("buttplug: LWS_CALLBACK_CLOSED\n");
		wsi_mirror = NULL;
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:
		if (flag_no_mirror_traffic)
			return 0;

		msg = bpws_new_msg_request_server_info("C Example");
		l += bpws_format_msg((char *)&buf[LWS_PRE + l], bufsize, msg);
		bpws_delete_msg(msg);

		n = lws_write(wsi, &buf[LWS_PRE], l, opts | LWS_WRITE_TEXT);
		if (n < 0)
			return -1;
		if (n < l) {
			lwsl_err("Partial write LWS_CALLBACK_CLIENT_WRITEABLE\n");
			return -1;
		}
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		((char *)in)[len] = '\0';
		lwsl_notice("rx %d '%s'\n", (int)len, (char *)in);
		msgs = bpws_parse_msgs(in);
		if (msgs)
		{
			for (i = 0; (msg = msgs[i]); i++)
			{
				lwsl_notice("Buttplug message (Id:%d, Idx:%d) type: %d\n", msg->id, i, msg->type);
			}
		}
		else
		{
			lwsl_notice("Error parsing messages!\n");
		}
		bpws_delete_msgs(msgs);
		break;


	/* because we are protocols[0] ... */

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		
		lwsl_err("CLIENT_CONNECTION_ERROR: %s\n",
			in ? (char *)in : "(null)");
		break;

	case LWS_CALLBACK_CLIENT_CONFIRM_EXTENSION_SUPPORTED:
		if ((strcmp((const char *)in, "deflate-stream") == 0) && deny_deflate) {
			lwsl_notice("denied deflate-stream extension\n");
			return 1;
		}
		if ((strcmp((const char *)in, "x-webkit-deflate-frame") == 0))
			return 1;
		if ((strcmp((const char *)in, "deflate-frame") == 0))
			return 1;
		break;

	case LWS_CALLBACK_ESTABLISHED_CLIENT_HTTP:
		lwsl_notice("lws_http_client_http_response %d\n",
			lws_http_client_http_response(wsi));
		break;

		/* chunked content */
	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ:
		lwsl_notice("LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ: %ld\n",
			(long)len);
		((char *)in)[len] = '\0';
		lwsl_notice("rx %d '%s'\n", (int)len, (char *)in);
		break;

		/* unchunked content */
	case LWS_CALLBACK_RECEIVE_CLIENT_HTTP:
	{
		char buffer[1024 + LWS_PRE];
		char *px = buffer + LWS_PRE;
		int lenx = sizeof(buffer) - LWS_PRE;

		/*
		* Often you need to flow control this by something
		* else being writable.  In that case call the api
		* to get a callback when writable here, and do the
		* pending client read in the writeable callback of
		* the output.
		*
		* In the case of chunked content, this will call back
		* LWS_CALLBACK_RECEIVE_CLIENT_HTTP_READ once per
		* chunk or partial chunk in the buffer, and report
		* zero length back here.
		*/
		if (lws_http_client_read(wsi, &px, &lenx) < 0)
			return -1;
	}
	break;


	case LWS_CALLBACK_CLIENT_APPEND_HANDSHAKE_HEADER:
		if (test_post) {
			unsigned char **p = (unsigned char **)in, *end = (*p) + len;

			if (lws_add_http_header_by_token(wsi,
				WSI_TOKEN_HTTP_CONTENT_LENGTH,
				(unsigned char *)"29", 2, p, end))
				return -1;
			if (lws_add_http_header_by_token(wsi,
				WSI_TOKEN_HTTP_CONTENT_TYPE,
				(unsigned char *)"application/x-www-form-urlencoded", 33, p, end))
				return -1;

			/* inform lws we have http body to send */
			lws_client_http_body_pending(wsi, 1);
			lws_callback_on_writable(wsi);
		}
		break;

	case LWS_CALLBACK_CLIENT_HTTP_WRITEABLE:
		strcpy(buf + LWS_PRE, "text=hello&send=Send+the+form");
		n = lws_write(wsi, (unsigned char *)&buf[LWS_PRE], strlen(&buf[LWS_PRE]), LWS_WRITE_HTTP);
		if (n < 0)
			return -1;
		/* we only had one thing to send, so inform lws we are done
		* if we had more to send, call lws_callback_on_writable(wsi);
		* and just return 0 from callback.  On having sent the last
		* part, call the below api instead.*/
		lws_client_http_body_pending(wsi, 0);
		break;

	case LWS_CALLBACK_COMPLETED_CLIENT_HTTP:
		force_exit = 1;
		break;

#if defined(LWS_OPENSSL_SUPPORT) && defined(LWS_HAVE_SSL_CTX_set1_param)
	case LWS_CALLBACK_OPENSSL_LOAD_EXTRA_CLIENT_VERIFY_CERTS:
		break;
#endif

	default:
		break;
	}

	return 0;
}

static const struct lws_protocols protocols[] = {
	{
		"buttplug",
		callback_lws_buttplug,
		0,
		20,
	},
	{ NULL, NULL, 0, 0 } /* end */
};

static const struct lws_extension exts[] = {
	{
		"permessage-deflate",
		lws_extension_callback_pm_deflate,
		"permessage-deflate; client_no_context_takeover"
	},
	{
		"deflate-frame",
		lws_extension_callback_pm_deflate,
		"deflate_frame"
	},
	{ NULL, NULL, NULL /* terminator */ }
};

static int ratelimit_connects(unsigned int *last, unsigned int secs)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);

	if (tv.tv_sec - (*last) < secs)
		return 0;

	*last = tv.tv_sec;

	return 1;
}

int main(int argc, char **argv)
{
	int n = 0, m, ret = 0, port = 7681, use_ssl = 0, ietf_version = -1;
	unsigned int rl_dumb = 0, rl_mirror = 0, do_ws = 1, pp_secs = 0, do_multi = 0;
	struct lws_context_creation_info info;
	struct lws_client_connect_info i;
	struct lws_context *context;
	const char *prot, *p;
	char path[300];
	char cert_path[1024] = "";
	char key_path[1024] = "";
	char ca_path[1024] = "";

	signal(SIGINT, sighandler);

	memset(&info, 0, sizeof info);
	memset(&i, 0, sizeof(i));

	i.port = port;
	if (lws_parse_uri("ws://localhost:12345/buttplug", &prot, &i.address, &i.port, &p))
		return 1;

	/* add back the leading / on path */
	path[0] = '/';
	strncpy(path + 1, p, sizeof(path) - 2);
	path[sizeof(path) - 1] = '\0';
	i.path = path;

	if (!strcmp(prot, "http") || !strcmp(prot, "ws"))
		use_ssl = 0;
	if (!strcmp(prot, "https") || !strcmp(prot, "wss"))
		use_ssl = 1;

	/*
	* create the websockets context.  This tracks open connections and
	* knows how to route any traffic and which protocol version to use,
	* and if each connection is client or server side.
	*
	* For this client-only demo, we tell it to not listen on any port.
	*/

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;
	info.ws_ping_pong_interval = pp_secs;
	info.extensions = exts;

#if defined(LWS_OPENSSL_SUPPORT)
	info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
#endif

	if (use_ssl) {
		/*
		* If the server wants us to present a valid SSL client certificate
		* then we can set it up here.
		*/

		if (cert_path[0])
			info.ssl_cert_filepath = cert_path;
		if (key_path[0])
			info.ssl_private_key_filepath = key_path;

		/*
		* A CA cert and CRL can be used to validate the cert send by the server
		*/
		if (ca_path[0])
			info.ssl_ca_filepath = ca_path;

#if defined(LWS_OPENSSL_SUPPORT) && defined(LWS_HAVE_SSL_CTX_set1_param)
		//else if (crl_path[0])
		//	lwsl_notice("WARNING, providing a CRL requires a CA cert!\n");
#endif
	}

	if (use_ssl & LCCSCF_USE_SSL)
		lwsl_notice(" Using SSL\n");
	else
		lwsl_notice(" SSL disabled\n");
	if (use_ssl & LCCSCF_ALLOW_SELFSIGNED)
		lwsl_notice(" Selfsigned certs allowed\n");
	else
		lwsl_notice(" Cert must validate correctly (use -s to allow selfsigned)\n");
	if (use_ssl & LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK)
		lwsl_notice(" Skipping peer cert hostname check\n");
	else
		lwsl_notice(" Requiring peer cert hostname matches\n");

	context = lws_create_context(&info);
	if (context == NULL) {
		fprintf(stderr, "Creating libwebsocket context failed\n");
		return 1;
	}

	i.context = context;
	i.ssl_connection = use_ssl;
	i.host = i.address;
	i.origin = i.address;
	i.ietf_version_or_minus_one = ietf_version;

	if (!strcmp(prot, "http") || !strcmp(prot, "https")) {
		lwsl_notice("using %s mode (non-ws)\n", prot);
		if (test_post) {
			i.method = "POST";
			lwsl_notice("POST mode\n");
		}
		else
			i.method = "GET";
		do_ws = 0;
	}
	else
		if (!strcmp(prot, "raw")) {
			i.method = "RAW";
			i.protocol = "lws-test-raw-client";
			lwsl_notice("using RAW mode connection\n");
			do_ws = 0;
		}
		else
			lwsl_notice("using %s mode (ws)\n", prot);

	/*
	* sit there servicing the websocket context to handle incoming
	* packets, and drawing random circles on the mirror protocol websocket
	*
	* nothing happens until the client websocket connection is
	* asynchronously established... calling lws_client_connect() only
	* instantiates the connection logically, lws_service() progresses it
	* asynchronously.
	*/

	lwsl_notice("buttplug: connecting\n");
	i.protocol = protocols[0].name;
	i.pwsi = &wsi_mirror;
	wsi_mirror = lws_client_connect_via_info(&i);

	m = 0;
	while (!force_exit && wsi_mirror) {

		lws_service(context, 500);
	}

	lwsl_err("Exiting\n");
	lws_context_destroy(context);

	return ret;
}
