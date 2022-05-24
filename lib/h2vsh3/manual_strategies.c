#include "manual_strategies.h"

#define h2o_linklist_foreach2(list, node) for (node = (list)->next; node != (list); node = node->next)

struct st_resource_map {
    h2o_linklist_t resources;
};

KHASH_MAP_INIT_STR(resource_map, struct st_resource_map *)

struct {
    khash_t(resource_map) *class_info;
} h2_strategy;

static struct st_h2o_sched_resource_class_t* longest_matching_resource(char* full_path) {
    debug_manual_strategies("search for %s\n", full_path);
    size_t l;
    for(l = 0; 1; l++) {
        if(full_path[l] == 0 || full_path[l] == '?') {
            break;
        }
    }
    char key[l+1];
    memcpy(key, full_path, l);
    key[l] = 0;
    struct st_resource_map* rm;
    khiter_t iter = kh_get(resource_map, h2_strategy.class_info, key);
    if (iter == kh_end(h2_strategy.class_info)) {
        return 0;
    }
    rm = kh_val(h2_strategy.class_info, iter);

    h2o_linklist_t *node;
    int longest_match_len = 0;
    struct st_h2o_sched_resource_class_t* longest_match_rc = 0;

    h2o_linklist_foreach2(&rm->resources, node) {
        struct st_h2o_sched_resource_class_t* rc = H2O_STRUCT_FROM_MEMBER(struct st_h2o_sched_resource_class_t, _link, node);
        int i;
        for(i = 0; 1; i++) {
            if(full_path[i] == 0 || rc->full_path[i] == 0 || full_path[i] != rc->full_path[i]) {
                if(full_path[i] == 0 && rc->full_path[i] == 0) {
                    return rc;
                }
                break;
            }
        }
        if(i > longest_match_len) {
            longest_match_rc = rc;
            longest_match_len = i;
        }
    }
    return longest_match_rc;
}

//#define H2VSH3_INCLUDE_SCHEME 1
//#define H2VSH3_INCLUDE_METHOD 1

static struct st_h2o_sched_resource_class_t* lookup_scheduling_class(const h2o_iovec_t *method, const h2o_iovec_t *path, const h2o_iovec_t *host, const h2o_iovec_t *scheme) {
    char key[method->len + 3 + 1 + scheme->len + 3 + host->len + path->len + 1];
    int l = 0;
    #ifdef H2VSH3_INCLUDE_METHOD
    memcpy(&key[l], method->base, method->len);
    l += method->len;
    #else
    key[l] = 'G';
    l += 1;
    key[l] = 'E';
    l += 1;
    key[l] = 'T';
    l += 1;
    #endif
    key[l] = ':';
    l += 1;
    #ifdef H2VSH3_INCLUDE_SCHEME
    memcpy(&key[l], scheme->base, scheme->len);
    l += scheme->len;
    key[l] = ':';
    l += 1;
    key[l] = '/';
    l += 1;
    key[l] = '/';
    l += 1;
    #endif
    memcpy(&key[l], host->base, host->len);
    l += host->len;
    memcpy(&key[l], path->base, path->len);
    l += path->len;
    key[l] = 0;
    struct st_h2o_sched_resource_class_t* rc = longest_matching_resource(key);
    return rc;
}

static void add_resource(struct st_h2o_sched_resource_class_t* rc) {
    size_t l;
    for(l = 0; 1; l++) {
        if(rc->full_path[l] == 0 || rc->full_path[l] == '?') {
            break;
        }
    }
    debug_manual_strategies("add %s\n", rc->full_path);
    char key[l+1];
    memcpy(key, rc->full_path, l);
    key[l] = 0;
    struct st_resource_map* rm;
    khiter_t iter = kh_get(resource_map, h2_strategy.class_info, key);
    if (iter == kh_end(h2_strategy.class_info)) {
        rm = malloc(sizeof(struct st_resource_map));
        h2o_linklist_init_anchor(&rm->resources);
        int r;
        khiter_t iter_put = kh_put(resource_map, h2_strategy.class_info, strdup(key), &r);
        assert(iter_put != kh_end(h2_strategy.class_info));
        assert(r > 0);
        kh_val(h2_strategy.class_info, iter_put) = rm;
    }
    else {
        rm = kh_val(h2_strategy.class_info, iter);
    }
    h2o_linklist_init_anchor(&rc->_link);
    h2o_linklist_unlink(&rc->_link);
    h2o_linklist_insert(&rm->resources, &rc->_link);
}

static void h2vsh3_manual_strategies_check() {
    #ifdef H2VSH3_INCLUDE_SCHEME
    struct st_h2o_sched_resource_class_t rc1 = {"GET:https://abc/def?ghidef",image, low, leaders, 0};
    struct st_h2o_sched_resource_class_t rc2 = {"GET:https://abc/def?ghiabc",image, low, leaders, 0};
    struct st_h2o_sched_resource_class_t rc3 = {"GET:https://abc/hij?nmo",image, low, leaders, 0};
    struct st_h2o_sched_resource_class_t rc4 = {"GET:https://abc/def",image, low, leaders, 0};
    #else
    struct st_h2o_sched_resource_class_t rc1 = {"GET:abc/def?ghidef",image, low, leaders, 0};
    struct st_h2o_sched_resource_class_t rc2 = {"GET:abc/def?ghiabc",image, low, leaders, 0};
    struct st_h2o_sched_resource_class_t rc3 = {"GET:abc/hij?nmo",image, low, leaders, 0};
    struct st_h2o_sched_resource_class_t rc4 = {"GET:abc/def",image, low, leaders, 0};
    #endif
    add_resource(&rc1);
    add_resource(&rc2);
    add_resource(&rc3);
    add_resource(&rc4);
    #ifdef H2VSH3_INCLUDE_SCHEME
    assert((longest_matching_resource("GET:https://abc/def?ghidef") == &rc1));
    assert((longest_matching_resource("GET:https://abc/def?ghiabc") == &rc2));
    assert((longest_matching_resource("GET:https://abc/def?ghia") == &rc2));
    assert((longest_matching_resource("GET:https://abc/def") == &rc4));
    assert((longest_matching_resource("GET:https://abc/hij?ghia") == &rc3));
    assert((longest_matching_resource("GET:https://abc/xyz") == 0));
    #else
    assert((longest_matching_resource("GET:abc/def?ghidef") == &rc1));
    assert((longest_matching_resource("GET:abc/def?ghiabc") == &rc2));
    assert((longest_matching_resource("GET:abc/def?ghia") == &rc2));
    assert((longest_matching_resource("GET:abc/def") == &rc4));
    assert((longest_matching_resource("GET:abc/hij?ghia") == &rc3));
    assert((longest_matching_resource("GET:abc/xyz") == 0));
    #endif
    assert(0);
}

int h2vsh3_manual_strategies_init(char* file) {
    h2_strategy.class_info = kh_init(resource_map);


    if(file != 0) {
        FILE * fp;
        char * line = NULL;
        size_t len = 0;
        ssize_t read;

        fp = fopen(file, "r");
        if (fp == NULL) {
            fprintf(stderr, "priofile %s not accessible\n", file);
            return 0;
        }

        while ((read = getline(&line, &len, fp)) != -1) {
            //csv for dummies
            char *str_full_path, *str_content_type, *str_chrome_class, *str_firefox_class, *str_firefox_weight, *str, *tofree;

            tofree = str = strdup(line);
            str_full_path = strsep(&str, "#");
            str_content_type = strsep(&str, "#");
            str_chrome_class = strsep(&str, "#");
            str_firefox_class = strsep(&str, "#");
            str_firefox_weight = strsep(&str, "\n");
            struct st_h2o_sched_resource_class_t* rc = malloc(sizeof(struct st_h2o_sched_resource_class_t));

            if (strcmp(str_content_type, "image") == 0) {
                rc->content_type = image;
            }
            else if (strcmp(str_content_type, "js") == 0) {
                rc->content_type = js;
            }
            else if (strcmp(str_content_type, "xhr") == 0) {
                rc->content_type = xhr;
            }
            else if (strcmp(str_content_type, "ping") == 0) {
                //see ping as xhr
                rc->content_type = xhr;
            }
            else if (strcmp(str_content_type, "html") == 0) {
                rc->content_type = html;
            }
            else if (strcmp(str_content_type, "css") == 0) {
                rc->content_type = css;
            }
            else if (strcmp(str_content_type, "fonts") == 0) {
                rc->content_type = fonts;
            }
            else if (strcmp(str_content_type, "iframe") == 0) {
                rc->content_type = html;
            }
            else if (strcmp(str_content_type, "unknown") == 0) {
                rc->content_type = unknown;
            }
            else {
                fprintf(stderr, "type %s unknown\n", str_content_type);
                assert(0);
            }

            if (strcmp(str_chrome_class, "throttled") == 0) {
                rc->chrome_class = throttled;
            }
            else if (strcmp(str_chrome_class, "idle") == 0) {
                rc->chrome_class = idle;
            }
            else if (strcmp(str_chrome_class, "lowest") == 0) {
                rc->chrome_class = lowest;
            }
            else if (strcmp(str_chrome_class, "low") == 0) {
                rc->chrome_class = low;
            }
            else if (strcmp(str_chrome_class, "normal") == 0) {
                rc->chrome_class = normal;
            }
            else if (strcmp(str_chrome_class, "high") == 0) {
                rc->chrome_class = high;
            }
            else if (strcmp(str_chrome_class, "highest") == 0) {
                rc->chrome_class = highest;
            }
            else {
                fprintf(stderr, "class %s unknown\n", str_chrome_class);
                assert(0);
            }

            if (strcmp(str_firefox_class, "urgent-start") == 0) {
                rc->firefox_class = urgentstart;
            }
            else if (strcmp(str_firefox_class, "followers") == 0) {
                rc->firefox_class = followers;
            }
            else if (strcmp(str_firefox_class, "leaders") == 0) {
                rc->firefox_class = leaders;
            }
            else if (strcmp(str_firefox_class, "unblocked") == 0) {
                rc->firefox_class = unblocked;
            }
            else if (strcmp(str_firefox_class, "background") == 0) {
                rc->firefox_class = background;
            }
            else if (strcmp(str_firefox_class, "speculative") == 0) {
                rc->firefox_class = speculative;
            }
            else {
                fprintf(stderr, "ffclass %s unknown\n", str_firefox_class);
                assert(0);
            }

            rc->full_path = str_full_path;
            rc->firefox_weight = atoi(str_firefox_weight);
            add_resource(rc);
        }

        fclose(fp);
        if (line)
            free(line);
        return 1;
    }
    return 0;
}

int h2vsh3_manual_strategies_priotree_remove(struct st_h2o_h2vsh3_sched_info_t *sched_info, h2o_http2_scheduler_openref_t* scheduler) {
    h2o_http2_scheduler_node_t* stream_node = &scheduler->node;
    sched_info->h2_sched_strategy->remove(sched_info, stream_node, sched_info->h2_sched_existing_root);
    return 0;
}

int h2vsh3_manual_strategies_priotree_init(struct st_h2o_h2vsh3_sched_info_t *sched_info) {
    sched_info->h2_sched_strategy->init(sched_info, sched_info->h2_sched_existing_root);
    return 0;
}

int h2vsh3_manual_strategies_init_conn(struct st_h2o_h2vsh3_sched_info_t *sched_info, struct st_h2o_sched_strategy_mode_t* mode, h2o_http2_scheduler_node_t* h2_sched_existing_root) {
    if(mode != 0) {
        if(mode->tree_strategy) {
            if(!h2_sched_existing_root) {
                h2o_http2_scheduler_init(&sched_info->h2_sched_root);
                sched_info->h2_sched_existing_root = &sched_info->h2_sched_root;
            }
            else {
                sched_info->h2_sched_existing_root = h2_sched_existing_root;
            }
            sched_info->h2_sched_strategy = mode->tree_strategy;
            h2vsh3_manual_strategies_priotree_init(sched_info);
            sched_info->h3_sched_strategy = 0;
        }
        else if(mode->ext_strategy) {
            sched_info->h3_sched_strategy = mode->ext_strategy;
            sched_info->h2_sched_strategy = 0;
        }
        else {
            assert(0);
        }
    }
    else {
        sched_info->h2_sched_strategy = 0;
        sched_info->h3_sched_strategy = 0;
    }
    return 0;
}

struct st_h2o_sched_strategy_mode_t* default_prio = 0;

struct st_h2o_sched_strategy_mode_t* h2vsh3_manual_strategies_get_default() {
    return default_prio;
}

int h2vsh3_manual_strategies_use(char* prio) {
    if(prio == 0) {
        return 0;
    }
    if(strcmp(prio, "chrometree") == 0) {
        default_prio = &chrome_tree_mode;
        return 1;
    }
    if(strcmp(prio, "firefoxtree") == 0) {
        default_prio = &firefox_tree_mode;
        return 1;
    }
    if(strcmp(prio, "wrrtree") == 0) {
        default_prio = &wrr_tree_mode;
        return 1;
    }
    if(strcmp(prio, "rrtree") == 0) {
        default_prio = &rr_tree_mode;
        return 1;
    }
    if(strcmp(prio, "seqtree") == 0) {
        default_prio = &seq_tree_mode;
        return 1;
    }
    if(strcmp(prio, "chromeext") == 0) {
        default_prio = &chrome_ext_mode;
        return 1;
    }
    if(strcmp(prio, "firefoxext") == 0) {
        default_prio = &firefox_ext_mode;
        return 1;
    }
    if(strcmp(prio, "wrrext") == 0) {
        default_prio = &wrr_ext_mode;
        return 1;
    }
    if(strcmp(prio, "rrext") == 0) {
        default_prio = &rr_ext_mode;
        return 1;
    }
    if(strcmp(prio, "seqext") == 0) {
        default_prio = &seq_ext_mode;
        return 1;
    }
    return 0;
}

int h2vsh3_manual_strategies_priotree_compute(struct st_h2o_h2vsh3_sched_info_t *sched_info, h2o_http2_scheduler_openref_t* scheduler, h2o_req_t *req, h2o_http2_scheduler_node_t** parent, uint16_t* weight, int* exclusive) {
    const h2o_iovec_t *method = &req->input.method;
    const h2o_iovec_t *path = &req->input.path;
    const h2o_iovec_t *host = &req->input.authority;
    const h2o_iovec_t *scheme = &req->input.scheme->name;
    h2o_http2_scheduler_node_t* root = sched_info->h2_sched_existing_root;
    h2o_http2_scheduler_node_t* stream_node = &scheduler->node;
    struct st_h2o_sched_resource_class_t *rc = lookup_scheduling_class(method, path, host, scheme);
    enum enum_h2o_sched_content_type_t content_type = html;
    int firefox_weight = 256;
    if(rc != 0) {
        content_type = rc->content_type;
        firefox_weight = rc->firefox_weight;
        debug_manual_strategies("node %p known: content_type %d firefox_weight %d %.*s\n", stream_node, content_type, firefox_weight, (int)path->len, path->base);
    }
    else {
        debug_manual_strategies("node %p unknown: default content_type %d firefox_weight %d %.*s\n", stream_node, content_type, firefox_weight, (int)path->len, path->base);
    }

    sched_info->h2_sched_strategy->add(sched_info, stream_node, rc, parent, weight, exclusive, root);

    return 0;
}

int h2vsh3_manual_strategies_ext_compute(struct st_h2o_h2vsh3_sched_info_t *sched_info, h2o_req_t *req, h2o_absprio_t* priority) {
    const h2o_iovec_t *method = &req->input.method;
    const h2o_iovec_t *path = &req->input.path;
    const h2o_iovec_t *host = &req->input.authority;
    const h2o_iovec_t *scheme = &req->input.scheme->name;
    struct st_h2o_sched_resource_class_t *rc = lookup_scheduling_class(method, path, host, scheme);
    enum enum_h2o_sched_content_type_t content_type = html;
    int firefox_weight = 256;
    if(rc != 0) {
        content_type = rc->content_type;
        firefox_weight = rc->firefox_weight;
        debug_manual_strategies("stream %p known: content_type %d firefox_weight %d %.*s\n", req, content_type, firefox_weight, (int)path->len, path->base);
    }
    else {
        debug_manual_strategies("stream %p unknown: default content_type %d firefox_weight %d %.*s\n", req, content_type, firefox_weight, (int)path->len, path->base);
    }

    sched_info->h3_sched_strategy->compute(sched_info, rc, priority);

    return 0;
}