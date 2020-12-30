#ifndef _rtsp_push_internal_h_
#define _rtsp_push_internal_h_

#include "rtsp-media.h"
#include "rtsp-push.h"
#include "rtp-over-rtsp.h"
#include "http-header-auth.h"
#include "rtsp-header-session.h"
#include "rtsp-header-transport.h"
#include "http-parser.h"
#include "sdp.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#if defined(OS_WINDOWS)
#define strcasecmp	_stricmp
#endif

#define USER_AGENT "RTSP PUSH v0.1"
#define N_MEDIA 8

enum rtsp_state_t
{
	RTSP_INIT,
	RTSP_ANNOUNCE,
    RTSP_RECORD,
	RTSP_DESCRIBE,
	RTSP_SETUP,
	RTSP_PLAY,
	RTSP_PAUSE,
	RTSP_TEARDWON,
	RTSP_OPTIONS,
	RTSP_GET_PARAMETER,
	RTSP_SET_PARAMETER,
};

struct rtsp_push_t
{
	struct rtsp_client_handler_t handler;
	void* param;

    const char* announce; // announce sdp
	http_parser_t* parser;
	enum rtsp_state_t state;
	int parser_need_more_data;
	int progress;
	unsigned int cseq; // rtsp sequence

	struct rtp_over_rtsp_t rtp;

	sdp_t* sdp;
	int media_count;
	struct rtsp_media_t media[N_MEDIA];
	struct rtsp_header_session_t session[N_MEDIA];
	struct rtsp_header_transport_t transport[N_MEDIA];

	// media
	char range[64]; // rtsp header Range
	char speed[16]; // rtsp header speed
    char scale[16]; // rtsp header scale
    char req[2048];

	char uri[256];
	char baseuri[256]; // Content-Base
	char location[256]; // Content-Location

	int aggregate; // 1-aggregate control available
	char aggregate_uri[256]; // aggregate control uri, valid if 1==aggregate

	int auth_failed;
	char usr[128];
	char pwd[256];
	char authenrization[1024];
	struct http_header_www_authenticate_t auth;
};


int rtsp_push_sdp(struct rtsp_push_t* rtsp, const char* sdp);
int rtsp_push_options(struct rtsp_push_t *rtsp, const char* commands);

int rtsp_push_announce_onreply(struct rtsp_push_t* rtsp, void* parser);
int rtsp_push_describe_onreply(struct rtsp_push_t* rtsp, void* parser);
int rtsp_push_setup_onreply(struct rtsp_push_t* rtsp, void* parser);
int rtsp_push_play_onreply(struct rtsp_push_t* rtsp, void* parser);
int rtsp_push_pause_onreply(struct rtsp_push_t* rtsp, void* parser);
int rtsp_push_teardown_onreply(struct rtsp_push_t* rtsp, void* parser);
int rtsp_push_options_onreply(struct rtsp_push_t* rtsp, void* parser);
int rtsp_push_record_onreply(struct rtsp_push_t* rtsp, void* parser);



#endif /* !_rtsp_push_internal_h_ */
