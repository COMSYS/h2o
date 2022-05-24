#include "manual_strategies.h"

/**
 * Firefox Tree Prioritization
 **/

void h2vsh3_manual_strategies_priotree_firefox_init(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* root) {
    struct st_h2o_http2_sched_info_firefox_t *sched_info = &base_sched_info->info.tree.firefox;
    //add fake streams for groups and save them
    h2o_http2_scheduler_open(&sched_info->background_group, root, 1, 0);
    h2o_http2_scheduler_open(&sched_info->unblocked_group, root, 101, 0);
    h2o_http2_scheduler_open(&sched_info->leaders_group, root, 201, 0);
    h2o_http2_scheduler_open(&sched_info->urgentstart_group, root, 241, 0);
    h2o_http2_scheduler_open(&sched_info->followers_group, &sched_info->leaders_group.node, 1, 0);
    h2o_http2_scheduler_open(&sched_info->speculative_group, &sched_info->background_group.node, 1, 0);
    debug_manual_strategies("firefox init root: %p, background %p, unblocked %p, leaders %p, urgentstart %p, "
      "followers %p, speculative %p\n", root, &sched_info->background_group.node, &sched_info->unblocked_group.node, 
      &sched_info->leaders_group.node, &sched_info->urgentstart_group.node, &sched_info->followers_group.node, &sched_info->speculative_group.node);
}

void h2vsh3_manual_strategies_priotree_firefox_add(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* stream_node, struct st_h2o_sched_resource_class_t *resource_class, h2o_http2_scheduler_node_t** parent, uint16_t* weight, int* exclusive, h2o_http2_scheduler_node_t* root) {
    struct st_h2o_http2_sched_info_firefox_t *sched_info = &base_sched_info->info.tree.firefox;

    uint8_t firefox_weight = 255;
    enum enum_h2o_sched_firefox_class_t firefox_class = urgentstart;

    if(resource_class != 0) {
        firefox_weight = resource_class->firefox_weight;
        firefox_class = resource_class->firefox_class;
    }

    switch (firefox_class) {
    case background:
        *parent = &sched_info->background_group.node;
        debug_manual_strategies("node %p background, parent %p, weight %d\n", stream_node, *parent, firefox_weight);
        break;
    case followers:
        *parent = &sched_info->followers_group.node;
        debug_manual_strategies("node %p follower, parent %p, weight %d\n", stream_node, *parent, firefox_weight);
        break;
    case unblocked:
        *parent = &sched_info->unblocked_group.node;
        debug_manual_strategies("node %p unblocked, parent %p, weight %d\n", stream_node, *parent, firefox_weight);
        break;
    case leaders:
        *parent = &sched_info->leaders_group.node;
        debug_manual_strategies("node %p leader, parent %p, weight %d\n", stream_node, *parent, firefox_weight);
        break;
    case urgentstart:
        *parent = &sched_info->urgentstart_group.node;
        debug_manual_strategies("node %p urgentstart, parent %p, weight %d\n", stream_node, *parent, firefox_weight);
        break;
    case speculative:
        *parent = &sched_info->speculative_group.node;
        debug_manual_strategies("node %p speculative, parent %p, weight %d\n", stream_node, *parent, firefox_weight);
        break;
    default:
        assert(0);
        break;
    }
    *weight = firefox_weight;
    *exclusive = 0;
}

void h2vsh3_manual_strategies_priotree_firefox_remove(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* stream_node, h2o_http2_scheduler_node_t* root) {
    //straight forward, nothing to do here
    debug_manual_strategies("firefox node %p remove\n", stream_node);
}

void h2vsh3_manual_strategies_priotree_firefox_destroy(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* root) {
    //TODO nothing to do here?
    debug_manual_strategies("firefox destroy missing!\n");
}

struct st_h2o_sched_strategy_tree_cbs_t firefox_tree_strategy = {h2vsh3_manual_strategies_priotree_firefox_init, h2vsh3_manual_strategies_priotree_firefox_add, h2vsh3_manual_strategies_priotree_firefox_remove, h2vsh3_manual_strategies_priotree_firefox_destroy};
struct st_h2o_sched_strategy_mode_t firefox_tree_mode = {&firefox_tree_strategy,0};

/**
 * Chrome FCFS Prioritization
 **/

void h2vsh3_manual_strategies_priotree_chrome_init(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* root) {
    struct st_h2o_http2_sched_info_chrome_t *sched_info = &base_sched_info->info.tree.chrome;
    for(int i = 0; i < 7; i++) {
        sched_info->bucket[i] = 0;
    }
    debug_manual_strategies("chrome init\n");
}

void h2vsh3_manual_strategies_priotree_chrome_add(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* stream_node, struct st_h2o_sched_resource_class_t *resource_class, h2o_http2_scheduler_node_t** parent, uint16_t* weight, int* exclusive, h2o_http2_scheduler_node_t* root) {
    struct st_h2o_http2_sched_info_chrome_t *sched_info = &base_sched_info->info.tree.chrome;
    enum enum_h2o_sched_chrome_class_t chrome_class = lowest;
    if (resource_class != 0) {
        chrome_class = resource_class->chrome_class;
    }
    switch (chrome_class) {
    case throttled:
        *weight = 37;
        break;
    case idle:
        *weight =74;    
        break;
    case lowest:
        *weight = 110;
        break;
    case low:
        *weight = 147;
        break;
    case normal:
        *weight = 183;
        break;
    case high:
        *weight = 220;
        break;
    case highest:
        *weight = 256;
        break;
    default:
        assert(0);
        break;
    }
    *parent = root;
    for(int i = chrome_class; i < 7; i++) {
        if (sched_info->bucket[i] != 0) {
            *parent = sched_info->bucket[i];
            break;
        }
    }
    debug_manual_strategies("node %p, parent %p, weight %d, class %d, root %p\n", stream_node, *parent, *weight, chrome_class, root);
    sched_info->bucket[chrome_class] = stream_node;
    *exclusive = 1;
}


void h2vsh3_manual_strategies_priotree_chrome_remove(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* stream_node, h2o_http2_scheduler_node_t* root) {
    struct st_h2o_http2_sched_info_chrome_t *sched_info = &base_sched_info->info.tree.chrome;
    h2o_http2_scheduler_node_t* parent = stream_node->_parent;
    debug_manual_strategies("node %p had parent %p\n", stream_node, parent);
    for(int i = 0; i < 7; i++) {
        if (sched_info->bucket[i] == stream_node) {
            //oh oh, we are the last entry of a bucket class, we have to clean up behind us
            debug_manual_strategies("node %p had parent %p, is tail of bucket class %d\n", stream_node, parent, i);
            assert(parent != 0);
            for(int j = i; j < 7; j++) {
                if (parent == sched_info->bucket[j]) {
                    debug_manual_strategies("node %p had parent %p, parent is tail of class %d, bucket now empty\n", stream_node, parent, j);
                    //bucket now empty
                    sched_info->bucket[i] = 0;
                    return;
                }
            }
            if (parent == root) {
                debug_manual_strategies("node %p had parent %p, parent is root, bucket now empty\n", stream_node, parent);
                //bucket now empty
                sched_info->bucket[i] = 0;
                return;
            }
            debug_manual_strategies("node %p had parent %p, parent new tail\n", stream_node, parent);
            sched_info->bucket[i] = parent;
            return;
        }
    }
    debug_manual_strategies("node %p had parent %p, not tail of bucket, everything alright\n", stream_node, parent);
}

void h2vsh3_manual_strategies_priotree_chrome_destroy(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* root) {
    debug_manual_strategies("chrome destroy\n");
}

struct st_h2o_sched_strategy_tree_cbs_t chrome_tree_strategy  = {h2vsh3_manual_strategies_priotree_chrome_init, h2vsh3_manual_strategies_priotree_chrome_add, h2vsh3_manual_strategies_priotree_chrome_remove, h2vsh3_manual_strategies_priotree_chrome_destroy };
struct st_h2o_sched_strategy_mode_t chrome_tree_mode = {&chrome_tree_strategy,0};

/**
 * Naive Prioritization
 **/

void h2vsh3_manual_strategies_priotree_rr_init(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* root) {

}

void h2vsh3_manual_strategies_priotree_rr_add(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* stream_node, struct st_h2o_sched_resource_class_t *resource_class, h2o_http2_scheduler_node_t** parent, uint16_t* weight, int* exclusive, h2o_http2_scheduler_node_t* root) {
    *parent = root;
    *weight = 1;
    *exclusive = 0;
}

void h2vsh3_manual_strategies_priotree_rr_remove(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* stream_node,  h2o_http2_scheduler_node_t* root) {

}

void h2vsh3_manual_strategies_priotree_rr_destroy(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* root) {

}

struct st_h2o_sched_strategy_tree_cbs_t rr_tree_strategy  = {h2vsh3_manual_strategies_priotree_rr_init, h2vsh3_manual_strategies_priotree_rr_add, h2vsh3_manual_strategies_priotree_rr_remove, h2vsh3_manual_strategies_priotree_rr_destroy};
struct st_h2o_sched_strategy_mode_t rr_tree_mode = {&rr_tree_strategy,0};

/**
 * wRR Prioritization
 **/

void h2vsh3_manual_strategies_priotree_wrr_init(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* root) {

}

void h2vsh3_manual_strategies_priotree_wrr_add(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* stream_node, struct st_h2o_sched_resource_class_t *resource_class, h2o_http2_scheduler_node_t** parent, uint16_t* weight, int* exclusive, h2o_http2_scheduler_node_t* root) {
    *parent = root;
    enum enum_h2o_sched_content_type_t content_type = html;
    if (resource_class != 0) {
        content_type = resource_class->content_type;
    }
    switch (content_type) {
    case unknown:
        *weight = 8;
        break;
    case image:
        *weight = 8;
        break;
    case js:
        *weight = 24;
        break;
    case xhr:
        *weight = 16;
        break;
    case html:
        *weight = 255;
        break;
    case css:
        *weight = 24;
        break;
    case fonts:
        *weight = 16;
        break;
    default:
        assert(0);
        break;
    }
    *exclusive = 0;
}

void h2vsh3_manual_strategies_priotree_wrr_remove(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* stream_node,  h2o_http2_scheduler_node_t* root) {

}

void h2vsh3_manual_strategies_priotree_wrr_destroy(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* root) {

}

struct st_h2o_sched_strategy_tree_cbs_t wrr_tree_strategy  = {h2vsh3_manual_strategies_priotree_wrr_init, h2vsh3_manual_strategies_priotree_wrr_add, h2vsh3_manual_strategies_priotree_wrr_remove, h2vsh3_manual_strategies_priotree_wrr_destroy};
struct st_h2o_sched_strategy_mode_t wrr_tree_mode = {&wrr_tree_strategy,0};

/**
 * Naive Prioritization
 **/

void h2vsh3_manual_strategies_priotree_seq_init(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* root) {
    base_sched_info->info.tree.seq.last = root;
    base_sched_info->info.tree.seq.root = root;
}

void h2vsh3_manual_strategies_priotree_seq_add(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* stream_node, struct st_h2o_sched_resource_class_t *resource_class, h2o_http2_scheduler_node_t** parent, uint16_t* weight, int* exclusive, h2o_http2_scheduler_node_t* root) {
    *parent = base_sched_info->info.tree.seq.last;
    *weight = 1;
    *exclusive = 1;
    base_sched_info->info.tree.seq.last = stream_node;
}

void h2vsh3_manual_strategies_priotree_seq_remove(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* stream_node,  h2o_http2_scheduler_node_t* root) {
    if(base_sched_info->info.tree.seq.last == stream_node) {
        base_sched_info->info.tree.seq.last = base_sched_info->info.tree.seq.root;
    }
}

void h2vsh3_manual_strategies_priotree_seq_destroy(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, h2o_http2_scheduler_node_t* root) {
    //nothing to do
}

struct st_h2o_sched_strategy_tree_cbs_t seq_tree_strategy  = {h2vsh3_manual_strategies_priotree_seq_init, h2vsh3_manual_strategies_priotree_seq_add, h2vsh3_manual_strategies_priotree_seq_remove, h2vsh3_manual_strategies_priotree_seq_destroy};
struct st_h2o_sched_strategy_mode_t seq_tree_mode = {&seq_tree_strategy,0};

//

void h2vsh3_manual_strategies_ext_chrome_compute(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, struct st_h2o_sched_resource_class_t* resource_class, h2o_absprio_t* priority) {
    enum enum_h2o_sched_chrome_class_t chrome_class = lowest;
    if (resource_class != 0) {
        chrome_class = resource_class->chrome_class;
    }
    switch (chrome_class) {
    case throttled:
        priority->urgency = 7;
        break;
    case idle:
        priority->urgency = 6;
        break;
    case lowest:
        priority->urgency = 5;
        break;
    case low:
        priority->urgency = 4;
        break;
    case normal:
        priority->urgency = 3;
        break;
    case high:
        priority->urgency = 2;
        break;
    case highest:
        priority->urgency = 1;
        break;
    default:
        assert(0);
        break;
    }
    priority->incremental = 0;
}

struct st_h2o_sched_strategy_ext_cbs_t chrome_ext_strategy  = {h2vsh3_manual_strategies_ext_chrome_compute};
struct st_h2o_sched_strategy_mode_t chrome_ext_mode = {0, &chrome_ext_strategy};

void h2vsh3_manual_strategies_ext_firefox_compute(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, struct st_h2o_sched_resource_class_t* resource_class, h2o_absprio_t* priority) {
    enum enum_h2o_sched_firefox_class_t firefox_class = urgentstart;

    if(resource_class != 0) {
        firefox_class = resource_class->firefox_class;
    }

    switch (firefox_class) {
    case background:
        priority->urgency = 5;
        break;
    case followers:
        priority->urgency = 3;
        break;
    case unblocked:
        priority->urgency = 4;
        break;
    case leaders:
        priority->urgency = 2;
        break;
    case urgentstart:
        priority->urgency = 1;
        break;
    case speculative:
        priority->urgency = 6;
        break;
    default:
        assert(0);
        break;
    }
    priority->incremental = 1;
}

struct st_h2o_sched_strategy_ext_cbs_t firefox_ext_strategy  = {h2vsh3_manual_strategies_ext_firefox_compute};
struct st_h2o_sched_strategy_mode_t firefox_ext_mode = {0, &firefox_ext_strategy};

void h2vsh3_manual_strategies_ext_rr_compute(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, struct st_h2o_sched_resource_class_t* rc, h2o_absprio_t* priority) {
    priority->urgency = 3;
    priority->incremental = 1;
}

struct st_h2o_sched_strategy_ext_cbs_t rr_ext_strategy  = {h2vsh3_manual_strategies_ext_rr_compute};
struct st_h2o_sched_strategy_mode_t rr_ext_mode = {0, &rr_ext_strategy};

//mind, wrr = rr in our case
struct st_h2o_sched_strategy_ext_cbs_t wrr_ext_strategy  = {h2vsh3_manual_strategies_ext_rr_compute};
struct st_h2o_sched_strategy_mode_t wrr_ext_mode = {0, &wrr_ext_strategy};

void h2vsh3_manual_strategies_ext_seq_compute(struct st_h2o_h2vsh3_sched_info_t *base_sched_info, struct st_h2o_sched_resource_class_t* rc, h2o_absprio_t* priority) {
    priority->urgency = 3;
    priority->incremental = 0;
}

struct st_h2o_sched_strategy_ext_cbs_t seq_ext_strategy  = {h2vsh3_manual_strategies_ext_seq_compute};
struct st_h2o_sched_strategy_mode_t seq_ext_mode = {0, &seq_ext_strategy};