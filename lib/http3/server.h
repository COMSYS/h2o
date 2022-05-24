/*
 * Copyright (c) 2021 COMSYS - RWTH Aachen, Constantin Sander
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef http3__server_h
#define http3__server_h

#include "h2o/absprio.h"
#include "h2o/http3_common.h"
#include "h2o/http3_server.h"
#include "h2o/http3_internal.h"
#include "../h2vsh3/manual_strategies.h"


/**
 * the scheduler
 */
struct st_h2o_http3_req_scheduler_t {
    struct {
        struct {
            h2o_linklist_t high;
            h2o_linklist_t low;
        } urgencies[H2O_ABSPRIO_NUM_URGENCY_LEVELS];
        size_t smallest_urgency;
    } active;
    h2o_linklist_t conn_blocked;
};

enum h2o_http3_server_stream_state {
    /**
     * receiving headers
     */
    H2O_HTTP3_SERVER_STREAM_STATE_RECV_HEADERS,
    /**
     * receiving request body (runs concurrently)
     */
    H2O_HTTP3_SERVER_STREAM_STATE_RECV_BODY_BEFORE_BLOCK,
    /**
     * blocked, waiting to be unblocked one by one (either in streaming mode or in non-streaming mode)
     */
    H2O_HTTP3_SERVER_STREAM_STATE_RECV_BODY_BLOCKED,
    /**
     * in non-streaming mode, receiving body
     */
    H2O_HTTP3_SERVER_STREAM_STATE_RECV_BODY_UNBLOCKED,
    /**
     * in non-streaming mode, waiting for the request to be processed
     */
    H2O_HTTP3_SERVER_STREAM_STATE_REQ_PENDING,
    /**
     * request has been processed, waiting for the response headers
     */
    H2O_HTTP3_SERVER_STREAM_STATE_SEND_HEADERS,
    /**
     * sending body (the generator MAY have closed, but the transmission to the client is still ongoing)
     */
    H2O_HTTP3_SERVER_STREAM_STATE_SEND_BODY,
    /**
     * all data has been sent and ACKed, waiting for the transport stream to close (req might be disposed when entering this state)
     */
    H2O_HTTP3_SERVER_STREAM_STATE_CLOSE_WAIT
};

/**
 *
 */
struct st_h2o_http3_req_scheduler_node_t {
    h2o_linklist_t link;
    h2o_absprio_t priority;
    uint64_t call_cnt;
};


struct st_h2o_http3_server_conn_t {
    h2o_conn_t super;
    h2o_http3_conn_t h3;

    struct st_h2o_h2vsh3_sched_info_t sched_info;

    ptls_handshake_properties_t handshake_properties;
    h2o_linklist_t _conns; /* linklist to h2o_context_t::http3._conns */
    /**
     * link-list of pending requests using st_h2o_http3_server_stream_t::link
     */
    struct {
        /**
         * holds streams in RECV_BODY_BLOCKED state. They are promoted one by one to the POST_BLOCK State.
         */
        h2o_linklist_t recv_body_blocked;
        /**
         * holds streams that are in request streaming mode.
         */
        h2o_linklist_t req_streaming;
        /**
         * holds streams in REQ_PENDING state or RECV_BODY_POST_BLOCK state (that is using streaming; i.e., write_req.cb != NULL).
         */
        h2o_linklist_t pending;
    } delayed_streams;
    /**
     * next application-level timeout
     */
    h2o_timer_t timeout;
    /**
     * counter (the order MUST match that of h2o_http3_server_stream_state; it is accessed by index via the use of counters[])
     */
    union {
        struct {
            uint32_t recv_headers;
            uint32_t recv_body_before_block;
            uint32_t recv_body_blocked;
            uint32_t recv_body_unblocked;
            uint32_t req_pending;
            uint32_t send_headers;
            uint32_t send_body;
            uint32_t close_wait;
        };
        uint32_t counters[1];
    } num_streams;
    /**
     * Number of streams that is request streaming. The state can be in either one of SEND_HEADERS, SEND_BODY, CLOSE_WAIT.
     */
    uint32_t num_streams_req_streaming;
    /**
     * scheduler
     */
    struct {
        /**
         * States for request streams.
         */
        struct st_h2o_http3_req_scheduler_t reqs;
        /**
         * States for unidirectional streams. Each element is a bit vector where slot for each stream is defined as: 1 << stream_id.
         */
        struct {
            uint16_t active;
            uint16_t conn_blocked;
        } uni;
    } scheduler;
};

struct st_h2o_http3_server_stream_t {
    quicly_stream_t *quic;
    struct {
        h2o_buffer_t *buf;
        int (*handle_input)(struct st_h2o_http3_server_stream_t *stream, const uint8_t **src, const uint8_t *src_end,
                            const char **err_desc);
        uint64_t bytes_left_in_data_frame;
    } recvbuf;
    struct {
        H2O_VECTOR(struct st_h2o_http3_server_sendvec_t) vecs;
        size_t off_within_first_vec;
        size_t min_index_to_addref;
        uint64_t final_size, final_body_size;
        uint8_t data_frame_header_buf[9];
    } sendbuf;
    enum h2o_http3_server_stream_state state;
    h2o_linklist_t link;
    h2o_ostream_t ostr_final;
    struct st_h2o_http3_req_scheduler_node_t scheduler;
    h2o_http2_scheduler_openref_t h2_scheduler;
    /**
     * if read is blocked
     */
    uint8_t read_blocked : 1;
    /**
     * if h2o_proceed_response has been invoked, or if the invocation has been requested
     */
    uint8_t proceed_requested : 1;
    /**
     * this flag is set by on_send_emit, triggers the invocation h2o_proceed_response in scheduler_do_send, used by do_send to
     * take different actions based on if it has been called while scheduler_do_send is running.
     */
    uint8_t proceed_while_sending : 1;
    /**
     * if a PRIORITY_UPDATE frame has been received
     */
    uint8_t received_priority_update : 1;
    /**
     * used in CLOSE_WAIT state to determine if h2o_dispose_request has been called
     */
    uint8_t req_disposed : 1;
    /**
     * buffer to hold the request body (or a chunk of, if in streaming mode), or CONNECT payload
     */
    h2o_buffer_t *req_body;
    /**
     * tunnel, if used. This object is instantiated when the request is being processed, whereas `tunnel->tunnel` is assigned when
     * a tunnel is established (i.e. when 2xx response is being received).
     */
    struct st_h2o_http3_server_tunnel_t {
        /**
         * Pointer to the tunnel that is connected to the origin. This object is destroyed as soon as an error is reported on either
         * the read side or the write side of the tunnel. The send side of the H3 stream connected to the client is FINed when the
         * tunnel is destroyed; therefore, `quicly_sendstate_is_open(&stream->quic->sendstate) == (stream->tunnel->tunnel != NULL)`.
         */
        h2o_tunnel_t *tunnel;
        struct st_h2o_http3_server_stream_t *stream;
        struct {
            h2o_timer_t delayed_write;
            char bytes_inflight[16384];
            unsigned is_inflight : 1;
        } up;
    } * tunnel;
    /**
     * the request. Placed at the end, as it holds the pool.
     */
    h2o_req_t req;
};

int h2vsh3_manual_strategies_init_h3_conn(struct st_h2o_http3_server_conn_t* conn, struct st_h2o_sched_strategy_mode_t* strategy);

#endif
