/*
C->S:
SETUP rtsp://example.com/foo/bar/baz.rm RTSP/1.0
CSeq: 302
Transport: RTP/AVP;unicast;client_port=4588-4589

S->C: 
RTSP/1.0 200 OK
CSeq: 302
Date: 23 Jan 1997 15:35:06 GMT
Session: 47112344
Transport: RTP/AVP;unicast;client_port=4588-4589;server_port=6256-6257
*/
#include "rtsp-push-internal.h"

static const char* sc_rtsp_setup = "SETUP %s RTSP/1.0\r\n"
								"CSeq: %u\r\n"
								"Session: %s\r\n"
								"Transport: RTP/AVP/TCP;unicast;interleaved=0-1;\r\n"
								"User-Agent: %s\r\n"
								"\r\n";

int rtsp_addr_is_multicast(const char* ip);

static int rtsp_push_media_setup(struct rtsp_push_t* rtsp, int i,const char* session)
{
	int len;
    len = snprintf(rtsp->req, sizeof(rtsp->req), sc_rtsp_setup, rtsp->media[i].uri, rtsp->cseq++, session, USER_AGENT);
	return len == rtsp->handler.send(rtsp->param, rtsp->media[i].uri, rtsp->req, len) ? 0 : -1;
}

int rtsp_push_setup(struct rtsp_push_t* rtsp, const char* sdp,const char* session)
{
	int i, r;
    unsigned short port[2];
    struct rtsp_media_t *m;
	struct rtsp_header_transport_t *t;

	if (NULL == sdp || 0 == *sdp)
		return -1;

	r = rtsp_media_sdp(sdp, rtsp->media, sizeof(rtsp->media)/sizeof(rtsp->media[0]));
	if (r < 0 || r > sizeof(rtsp->media) / sizeof(rtsp->media[0]))
		return r < 0 ? r : -E2BIG; // too many media stream

	rtsp->media_count = r;
	for (i = 0; i < rtsp->media_count; i++)
	{
        m = rtsp->media + i;
        t = rtsp->transport + i;
        
		// rfc 2326 C.1.1 Control URL (p80)
		// If found at the session level, the attribute indicates the URL for aggregate control
		rtsp->aggregate = rtsp->media[0].session_uri[0] ? 1 : 0;
		rtsp_media_set_url(rtsp->media+i, rtsp->baseuri, rtsp->location, rtsp->uri);
		if(rtsp->aggregate)
			snprintf(rtsp->aggregate_uri, sizeof(rtsp->aggregate_uri), "%s", m->session_uri);

        port[0] = (unsigned short)m->port[0];
        port[1] = (unsigned short)m->port[1];
        snprintf(t->source, sizeof(t->source), "%s", m->source);
        snprintf(t->destination, sizeof(t->destination), "%s", m->address);
        r = rtsp->handler.rtpport(rtsp->param, i, t->source, port, t->destination, sizeof(t->destination));
		if (r < 0)
			return r;

        if(RTSP_TRANSPORT_RTP_TCP == r)
		{
			t->transport = RTSP_TRANSPORT_RTP_TCP;
            t->interleaved1 = 0==port[0] ? 2 * (unsigned short)i : port[0];
            t->interleaved2 = 0==port[1] ? t->interleaved1 + 1 : port[1];
		}
		else if((RTSP_TRANSPORT_RTP_UDP == r || RTSP_TRANSPORT_RAW == r) && rtsp_addr_is_multicast(t->destination))
		{
			assert(0 == t->rtp.u.client_port1 % 2);
            t->transport = r;
            t->multicast = 1;
            t->rtp.m.ttl = 16; // default RTT
			t->rtp.m.port1 = port[0];
            t->rtp.m.port2 = 0 == port[1] ? t->rtp.m.port1 + 1 : port[1];
		}
        else if(RTSP_TRANSPORT_RTP_UDP == r || RTSP_TRANSPORT_RAW == r)
        {
            assert(0 == t->rtp.u.client_port1 % 2);
            t->transport = r;
            t->multicast = 0;
            t->rtp.u.client_port1 = port[0];
            t->rtp.u.client_port2 = 0 == port[1] ? t->rtp.u.client_port1 + 1 : port[1];
        }
        else
        {
            assert(0);
            return -1;
        }
	}

	rtsp->state = RTSP_SETUP;
	rtsp->progress = 0;
	return rtsp_push_media_setup(rtsp, rtsp->progress,session);
}

int rtsp_push_setup_onreply(struct rtsp_push_t* rtsp, void* parser)
{
	int code;
	const char *session;
	const char *transport;
	struct rtsp_header_range_t* range;

	assert(RTSP_SETUP == rtsp->state);
	assert(rtsp->progress < rtsp->media_count);
	code = http_get_status_code(parser);

	return (code == 200) ? 0 : -1;
}
