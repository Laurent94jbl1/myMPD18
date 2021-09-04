/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "api.h"

#include "../../dist/src/mongoose/mongoose.h"
#include "lua_mympd_state.h"
#include "sds_extras.h"

#include <assert.h>
#include <mpd/client.h>
#include <string.h>

//global variables
_Atomic int worker_threads;
sig_atomic_t s_signal_received;
tiny_queue_t *web_server_queue;
tiny_queue_t *mympd_api_queue;
tiny_queue_t *mympd_script_queue;

//method to id and reverse
static const char *mympd_cmd_strs[] = { MYMPD_CMDS(GEN_STR) };

enum mympd_cmd_ids get_cmd_id(const char *cmd) {
    const size_t cmd_len = strlen(cmd);
    for (unsigned i = 0; i < TOTAL_API_COUNT; i++) {
        const size_t len = strlen(mympd_cmd_strs[i]);
        if (cmd_len == len && strncmp(cmd, mympd_cmd_strs[i], len) == 0) {
            return i;
        }
    }
    return GENERAL_API_UNKNOWN;
}

const char *get_cmd_id_method_name(enum mympd_cmd_ids cmd_id) {
    if (cmd_id >= TOTAL_API_COUNT) {
        return NULL;
    }
    return mympd_cmd_strs[cmd_id];
}

//defines methods that need authentication if a pin is set
bool is_protected_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case MYMPD_API_CONNECTION_SAVE:
        case MYMPD_API_COVERCACHE_CLEAR:
        case MYMPD_API_COVERCACHE_CROP:
        case MYMPD_API_MOUNT_MOUNT:
        case MYMPD_API_MOUNT_UNMOUNT:
        case MYMPD_API_PARTITION_NEW:
        case MYMPD_API_PARTITION_RM:
        case MYMPD_API_PARTITION_OUTPUT_MOVE:
        case MYMPD_API_PLAYER_OUTPUT_ATTRIBUTS_SET:
        case MYMPD_API_PLAYLIST_RM_ALL:
        case MYMPD_API_SESSION_LOGOUT:
        case MYMPD_API_SESSION_VALIDATE:
        case MYMPD_API_SETTINGS_SET:
        case MYMPD_API_SCRIPT_RM:
        case MYMPD_API_SCRIPT_SAVE:
        case MYMPD_API_TIMER_RM:
        case MYMPD_API_TIMER_SAVE:
        case MYMPD_API_TIMER_TOGGLE:
        case MYMPD_API_TRIGGER_RM:
        case MYMPD_API_TRIGGER_SAVE:
            return true;
        default:
            return false;
    }
}

//defines methods that are internal
bool is_public_api_method(enum mympd_cmd_ids cmd_id) {
    if (cmd_id <= INTERNAL_API_COUNT ||
        cmd_id >= TOTAL_API_COUNT)
    {
        return false;
    }
    return true;
}

//defines methods that should work with no mpd connection
//this is necessary for correct startup and changing mpd connection settings
bool is_mympd_only_api_method(enum mympd_cmd_ids cmd_id) {
    switch(cmd_id) {
        case MYMPD_API_CONNECTION_SAVE:
        case MYMPD_API_HOME_LIST:
        case MYMPD_API_SCRIPT_LIST:
        case MYMPD_API_SETTINGS_GET:
            return true;
        default:
            return false;
    }
}



t_work_result *create_result(t_work_request *request) {
    t_work_result *response = create_result_new(request->conn_id, request->id, request->cmd_id);
    return response;
}

t_work_result *create_result_new(long long conn_id, long request_id, unsigned cmd_id) {
    t_work_result *response = (t_work_result *)malloc(sizeof(t_work_result));
    assert(response);
    response->conn_id = conn_id;
    response->id = request_id;
    response->cmd_id = cmd_id;
    const char *method = get_cmd_id_method_name(cmd_id);
    response->method = sdsnew(method);
    response->data = sdsempty();
    response->binary = sdsempty();
    response->extra = NULL;
    return response;
}

t_work_request *create_request(long long conn_id, long request_id, unsigned cmd_id, const char *data) {
    t_work_request *request = (t_work_request *)malloc(sizeof(t_work_request));
    assert(request);
    request->conn_id = conn_id;
    request->cmd_id = cmd_id;
    request->id = request_id;
    const char *method = get_cmd_id_method_name(cmd_id);
    request->method = sdsnew(method);
    if (data == NULL) {
        request->data = sdscatfmt(sdsempty(), "{\"jsonrpc\":\"2.0\",\"id\":0,\"method\":\"%s\",\"params\":{", method);
    }
    else {
        request->data = sdsnew(data);
    }
    request->extra = NULL;
    return request;
}

void free_request(t_work_request *request) {
    if (request != NULL) {
        FREE_SDS(request->data);
        FREE_SDS(request->method);
        free(request);
    }
}

void free_result(t_work_result *result) {
    if (result != NULL) {
        FREE_SDS(result->data);
        FREE_SDS(result->method);
        FREE_SDS(result->binary);
        free(result);
    }
}

int expire_result_queue(tiny_queue_t *queue, time_t age) {
    t_work_result *response = NULL;
    int i = 0;
    while ((response = tiny_queue_expire(queue, age)) != NULL) {
        if (response->extra != NULL) {
            if (response->cmd_id == INTERNAL_API_SCRIPT_INIT) {
                free_lua_mympd_state(response->extra);
            }
            else {
                free(response->extra);
            }
        }
        free_result(response);
        response = NULL;
        i++;
    }
    return i;
}

int expire_request_queue(tiny_queue_t *queue, time_t age) {
    t_work_request *request = NULL;
    int i = 0;
    while ((request = tiny_queue_expire(queue, age)) != NULL) {
        if (request->extra != NULL) {
            if (request->cmd_id == INTERNAL_API_SCRIPT_INIT) {
                free_lua_mympd_state(request->extra);
            }
            else {
                free(request->extra);
            }
        }
        free_request(request);
        request = NULL;
        i++;
    }
    return i;
}
