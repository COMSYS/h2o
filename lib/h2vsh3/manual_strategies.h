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

#ifndef h2vsh3__manual_strategies_h
#define h2vsh3__manual_strategies_h

#include "h2o/http2_scheduler.h"
#include "h2o/absprio.h"
#include "khash.h"

#define h2vsh3_manual_strategies_debug 0

#if h2vsh3_manual_strategies_debug
#define debug_manual_strategies(...) fprintf (stderr, __VA_ARGS__)
#else
#define debug_manual_strategies(...)
#endif

enum st_h2o_http2_sched_strategies_t {
    none,
    firefox,
    chrome,
    safari,
    edge
};

struct st_h2o_http2_sched_info_chrome_t {
    h2o_http2_scheduler_node_t* bucket[7];
};

struct st_h2o_http2_sched_info_firefox_t {
    h2o_http2_scheduler_openref_t urgentstart_group;
    h2o_http2_scheduler_openref_t leaders_group;
    h2o_http2_scheduler_openref_t followers_group;
    h2o_http2_scheduler_openref_t unblocked_group;
    h2o_http2_scheduler_openref_t background_group;
    h2o_http2_scheduler_openref_t speculative_group;
};

struct st_h2o_http2_sched_info_parallelplus_t {
    h2o_http2_scheduler_node_t* highprio_group;
    h2o_http2_scheduler_node_t* highprio_last;
    h2o_http2_scheduler_node_t* mediumprio_group;
    h2o_http2_scheduler_node_t* lowprio_group;
};

struct st_h2o_http2_sched_info_serialplus_t {
    h2o_http2_scheduler_node_t* highprio_group;
    h2o_http2_scheduler_node_t* highprio_last;
    h2o_http2_scheduler_node_t* mediumprio_group;
    h2o_http2_scheduler_node_t* mediumprio_last;
    h2o_http2_scheduler_node_t* leaders_group;
    h2o_http2_scheduler_node_t* followers_group;
    h2o_http2_scheduler_node_t* unblocked_group;
    h2o_http2_scheduler_node_t* background_group;
    h2o_http2_scheduler_node_t* speculative_group;
};

struct st_h2o_http2_sched_info_seq_t {
    h2o_http2_scheduler_node_t* root;
    h2o_http2_scheduler_node_t* last;
};

struct st_h2o_h2vsh3_sched_info_t {
    union {
        union {
            struct st_h2o_http2_sched_info_chrome_t chrome;
            struct st_h2o_http2_sched_info_firefox_t firefox;
            struct st_h2o_http2_sched_info_parallelplus_t parallelplus;
            struct st_h2o_http2_sched_info_serialplus_t serialplus;
            struct st_h2o_http2_sched_info_seq_t seq;
        } tree;
        union {

        } ext;
    } info;
    struct st_h2o_sched_strategy_tree_cbs_t *h2_sched_strategy;
    struct st_h2o_sched_strategy_ext_cbs_t *h3_sched_strategy;
    h2o_http2_scheduler_node_t h2_sched_root;
    h2o_http2_scheduler_node_t* h2_sched_existing_root;
};


enum enum_h2o_sched_content_type_t {
    unknown,
    image,
    js,
    xhr,
    html,
    css,
    fonts
};

enum enum_h2o_sched_chrome_class_t {
    throttled = 0,
    idle = 1,
    lowest = 2,
    low = 3,
    normal = 4,
    high = 5,
    highest = 6
};

enum enum_h2o_sched_firefox_class_t {
    followers,
    leaders,
    unblocked,
    background,
    speculative,
    urgentstart
};

struct st_h2o_sched_resource_class_t {
    char* full_path;
    enum enum_h2o_sched_content_type_t content_type;
    enum enum_h2o_sched_chrome_class_t chrome_class;
    enum enum_h2o_sched_firefox_class_t firefox_class;
    uint8_t firefox_weight;
    h2o_linklist_t _link;
};

struct st_h2o_http3_server_stream_t;
struct st_h2o_http3_server_conn_t;

struct st_h2o_sched_strategy_mode_t {
    struct st_h2o_sched_strategy_tree_cbs_t* tree_strategy;
    struct st_h2o_sched_strategy_ext_cbs_t* ext_strategy;
};

struct st_h2o_sched_strategy_ext_cbs_t {
    void (*compute)(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, struct st_h2o_sched_resource_class_t*, h2o_absprio_t* priority);
};

struct st_h2o_sched_strategy_tree_cbs_t {
    void (*init)(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* root);

    void (*add)(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* stream_node,
        struct st_h2o_sched_resource_class_t *resource_class, h2o_http2_scheduler_node_t** parent,
        uint16_t* weight, int* exclusive, h2o_http2_scheduler_node_t* root);

    void (*remove)(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* stream_node,  h2o_http2_scheduler_node_t* root);

    void (*destroy)(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* root);
};

extern struct st_h2o_sched_strategy_mode_t firefox_tree_mode;
extern struct st_h2o_sched_strategy_mode_t chrome_tree_mode;
extern struct st_h2o_sched_strategy_mode_t rr_tree_mode;
extern struct st_h2o_sched_strategy_mode_t wrr_tree_mode;
extern struct st_h2o_sched_strategy_mode_t seq_tree_mode;

extern struct st_h2o_sched_strategy_mode_t firefox_ext_mode;
extern struct st_h2o_sched_strategy_mode_t chrome_ext_mode;
extern struct st_h2o_sched_strategy_mode_t rr_ext_mode;
extern struct st_h2o_sched_strategy_mode_t wrr_ext_mode;
extern struct st_h2o_sched_strategy_mode_t seq_ext_mode;

#include "../http3/server.h"

int h2vsh3_manual_strategies_init_conn(struct st_h2o_h2vsh3_sched_info_t *sched_info, struct st_h2o_sched_strategy_mode_t* mode, h2o_http2_scheduler_node_t* h2_sched_existing_root);

/*int h2vsh3_manual_strategies_priotree_init(struct st_h2o_http3_server_conn_t* conn);
int h2vsh3_manual_strategies_priotree_compute(struct st_h2o_http3_server_conn_t* conn, struct st_h2o_http3_server_stream_t* stream, h2o_http2_scheduler_node_t** parent, uint16_t* weight, int* exclusive);
int h2vsh3_manual_strategies_priotree_remove(struct st_h2o_http3_server_conn_t* conn, struct st_h2o_http3_server_stream_t* stream);*/

int h2vsh3_manual_strategies_priotree_remove(struct st_h2o_h2vsh3_sched_info_t *sched_info, h2o_http2_scheduler_openref_t* scheduler);
int h2vsh3_manual_strategies_priotree_init(struct st_h2o_h2vsh3_sched_info_t *sched_info);
int h2vsh3_manual_strategies_priotree_compute(struct st_h2o_h2vsh3_sched_info_t *sched_info, h2o_http2_scheduler_openref_t* scheduler, h2o_req_t *req, h2o_http2_scheduler_node_t** parent, uint16_t* weight, int* exclusive);

int h2vsh3_manual_strategies_ext_compute(struct st_h2o_h2vsh3_sched_info_t *sched_info, h2o_req_t *req, h2o_absprio_t* priority);

int h2vsh3_manual_strategies_init();
int h2vsh3_manual_strategies_use(char* prio);
struct st_h2o_sched_strategy_mode_t* h2vsh3_manual_strategies_get_default();

#endif