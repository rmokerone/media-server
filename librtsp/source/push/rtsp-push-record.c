#include "rtsp-push.h"
#include "rtsp-push-internal.h"
#include "rtsp-header-range.h"
#include "rtsp-header-rtp-info.h"
#include <assert.h>




static const char* sc_format =
    "RECORD %s RTSP/1.0\r\n"
    "CSeq: %u\r\n"
    "Session: %s\r\n"
    "Range: npt=0.0.0-\r\n" // Range
    "User-Agent: %s\r\n"
    "\r\n";

static int rtsp_push_media_record(struct rtsp_push_t *rtsp, int i,const char* session,const char* url)
{
    int r;
    assert(0 == rtsp->aggregate);
	assert(i < rtsp->media_count);
    assert(RTSP_RECORD == rtsp->state);
	if (i >= rtsp->media_count) return -1;

    r = snprintf(rtsp->req, sizeof(rtsp->req), sc_format, url, rtsp->cseq++, session, USER_AGENT);
    assert(r > 0 && r < sizeof(rtsp->req));
    return r == rtsp->handler.send(rtsp->param, rtsp->media[i].uri, rtsp->req, r) ? 0 : -1;
}

int rtsp_push_record(struct rtsp_push_t *rtsp, const uint64_t *npt, const float *scale,const char* session,const char* url)
{
    int r;
    assert(RTSP_SETUP == rtsp->state || RTSP_RECORD == rtsp->state || RTSP_PAUSE == rtsp->state);
    rtsp->state = RTSP_RECORD;
    rtsp->progress = 0;
    rtsp->scale[0] = rtsp->range[0] = '\0';

    r = scale ? snprintf(rtsp->scale, sizeof(rtsp->scale), "Scale: %f\r\n", *scale) : 0;
    r = npt ? snprintf(rtsp->range, sizeof(rtsp->range), "Range: npt=%" PRIu64 ".%" PRIu64 "-\r\n", *npt / 1000, *npt % 1000) : 0;
    if (r < 0 || r >= sizeof(rtsp->range))
		return -1;

    printf("status rtsp->aggregate 333: %d\n",rtsp->aggregate); 
    if (rtsp->aggregate)
    {
        assert(rtsp->media_count > 0);
        assert(rtsp->aggregate_uri[0]);
        r = rtsp_client_authenrization(rtsp, "RECORD", rtsp->aggregate_uri, NULL, 0, rtsp->authenrization, sizeof(rtsp->authenrization));
        r = snprintf(rtsp->req, sizeof(rtsp->req), sc_format, rtsp->aggregate_uri, rtsp->cseq++, rtsp->session[0].session, rtsp->range, rtsp->scale, rtsp->authenrization, USER_AGENT);
        return r == rtsp->handler.send(rtsp->param, rtsp->aggregate_uri, rtsp->req, r) ? 0 : -1;
    }
    else
    {
        return rtsp_push_media_record(rtsp, rtsp->progress,session,url);
    }
}

// aggregate control reply
static int rtsp_push_media_record_onreply(struct rtsp_push_t* rtsp, void* parser)
{
    int code;
    code = http_get_status_code(parser);
    if (200 == code)
        return 0;
    else
        return -1;
}

int rtsp_push_record_onreply(struct rtsp_push_t* rtsp, void* parser)
{
    return rtsp_push_media_record_onreply(rtsp, parser);
}



