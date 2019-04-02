/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
   myMPD ist fork of: ympd (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   ympd project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <pwd.h>
#include <grp.h>
#include <libgen.h>
#include <pthread.h>
#include <dirent.h>
#include <mpd/client.h>
#include <stdbool.h>
#include <signal.h>
#include <dlfcn.h>

#include "list.h"
#include "tiny_queue.h"
#include "global.h"
#include "mpd_client.h"
#include "../dist/src/mongoose/mongoose.h"
#include "web_server.h"
#include "mympd_api.h"
#include "../dist/src/inih/ini.h"


static void mympd_signal_handler(int sig_num) {
    signal(sig_num, mympd_signal_handler);  // Reinstantiate signal handler
    s_signal_received = sig_num;
    //Wakeup mympd_api_loop
    pthread_cond_signal(&mympd_api_queue->wakeup);
}

static int mympd_inihandler(void *user, const char *section, const char *name, const char* value) {
    t_config* p_config = (t_config*)user;
    char *crap;

    #define MATCH(s, n) strcasecmp(section, s) == 0 && strcasecmp(name, n) == 0

    if (MATCH("mpd", "host")) {
        FREE_PTR(p_config->mpdhost);
        p_config->mpdhost = strdup(value);
    }
    else if (MATCH("mpd", "port")) {
        p_config->mpdport = strtol(value, &crap, 10);
    }
    else if (MATCH("mpd", "pass")) {
        FREE_PTR(p_config->mpdpass);
        p_config->mpdpass = strdup(value);
    }
    else if (MATCH("mpd", "streamport")) {
        p_config->streamport = strtol(value, &crap, 10);
    }
    else if (MATCH("mpd", "musicdirectory")) {
        FREE_PTR(p_config->music_directory);
        p_config->music_directory = strdup(value);
    }
    else if (MATCH("webserver", "webport")) {
        FREE_PTR(p_config->webport);
        p_config->webport = strdup(value);
    }
    else if (MATCH("webserver", "ssl")) {
        p_config->ssl = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("webserver", "sslport")) {
        FREE_PTR(p_config->sslport);
        p_config->sslport = strdup(value);
    }
    else if (MATCH("webserver", "sslcert")) {
        FREE_PTR(p_config->sslcert);
        p_config->sslcert = strdup(value);
    }
    else if (MATCH("webserver", "sslkey")) {
        FREE_PTR(p_config->sslkey);
        p_config->sslkey = strdup(value);
    }
    else if (MATCH("mympd", "user")) {
        FREE_PTR(p_config->user);
        p_config->user = strdup(value);
    }
    else if (MATCH("mympd", "coverimage")) {
        p_config->coverimage = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "coverimagename")) {
        FREE_PTR(p_config->coverimagename);
        p_config->coverimagename = strdup(value);
    }
    else if (MATCH("mympd", "coverimagesize")) {
        p_config->coverimagesize = strtol(value, &crap, 10);
    }
    else if (MATCH("mympd", "varlibdir")) {
        FREE_PTR(p_config->varlibdir);
        p_config->varlibdir = strdup(value);
    }
    else if (MATCH("mympd", "stickers")) {
        p_config->stickers = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "smartpls")) {
        p_config->smartpls =  strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "mixramp")) {
        p_config->mixramp = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "taglist")) {
        FREE_PTR(p_config->taglist);
        p_config->taglist = strdup(value);
    }
    else if (MATCH("mympd", "searchtaglist")) {
        FREE_PTR(p_config->searchtaglist);
        p_config->searchtaglist = strdup(value);
    }
    else if (MATCH("mympd", "browsetaglist")) {
        FREE_PTR(p_config->browsetaglist);
        p_config->browsetaglist = strdup(value);
    }
    else if (MATCH("mympd", "pagination")) {
        p_config->max_elements_per_page = strtol(value, &crap, 10);
        if (p_config->max_elements_per_page > MAX_ELEMENTS_PER_PAGE) {
            printf("Setting max_elements_per_page to maximal value %d", MAX_ELEMENTS_PER_PAGE);
            p_config->max_elements_per_page = MAX_ELEMENTS_PER_PAGE;
        }
    }
    else if (MATCH("mympd", "syscmds")) {
        p_config->syscmds = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "localplayer")) {
        p_config->localplayer = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "streamurl")) {
        FREE_PTR(p_config->streamurl);
        p_config->streamurl = strdup(value);
    }
    else if (MATCH("mympd", "lastplayedcount")) {
        p_config->last_played_count = strtol(value, &crap, 10);
    }
    else if (MATCH("mympd", "loglevel")) {
        p_config->loglevel = strtol(value, &crap, 10);
    }
    else if (MATCH("theme", "background")) {
        FREE_PTR(p_config->background);
        p_config->background = strdup(value);
    }
    else if (MATCH("theme", "backgroundfilter")) {
        FREE_PTR(p_config->backgroundfilter);
        p_config->backgroundfilter = strdup(value);
    }
    else if (MATCH("mympd", "love")) {
        p_config->love = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("mympd", "lovechannel")) {
        FREE_PTR(p_config->lovechannel);
        p_config->lovechannel = strdup(value);
    }
    else if (MATCH("mympd", "lovemessage")) {
        FREE_PTR(p_config->lovemessage);
        p_config->lovemessage = strdup(value);
    }
    else if (MATCH("plugins", "coverextract")) {
        p_config->plugins_coverextract = strcmp(value, "true") == 0 ? true : false;
    }
    else if (MATCH("plugins", "coverextractlib")) {
        FREE_PTR(p_config->plugins_coverextractlib);
        p_config->plugins_coverextractlib = strdup(value);
    }
    else {
        LOG_WARN("Unkown config option: %s - %s", section, name);
        return 0;  
    }

    return 1;
}

static void mympd_free_config(t_config *config) {
    FREE_PTR(config->mpdhost);
    FREE_PTR(config->mpdpass);
    FREE_PTR(config->webport);
    FREE_PTR(config->sslport);
    FREE_PTR(config->sslcert);
    FREE_PTR(config->sslkey);
    FREE_PTR(config->user);
    FREE_PTR(config->coverimagename);
    FREE_PTR(config->taglist);
    FREE_PTR(config->searchtaglist);
    FREE_PTR(config->browsetaglist);
    FREE_PTR(config->varlibdir);
    FREE_PTR(config->etcdir);
    FREE_PTR(config->streamurl);
    FREE_PTR(config->background);
    FREE_PTR(config->backgroundfilter);
    FREE_PTR(config->lovechannel);
    FREE_PTR(config->lovemessage);
    FREE_PTR(config->plugins_coverextractlib);
    FREE_PTR(config->music_directory);
    FREE_PTR(config);
}

static void mympd_parse_env(struct t_config *config, const char *envvar) {
    char *name, *section;
    const char *value = getenv(envvar);
    if (value != NULL) {
        char *var = strdup(envvar);
        section = strtok_r(var, "_", &name);
        if (section != NULL && name != NULL) {
            LOG_DEBUG("Using environment variable %s_%s: %s", section, name, value);
            mympd_inihandler(config, section, name, value);
        }
        value = NULL;
        FREE_PTR(var);
    }
}

static void mympd_get_env(struct t_config *config) {
    const char* env_vars[]={"MPD_HOST", "MPD_PORT", "MPD_PASS", "MPD_STREAMPORT", "MPD_MUSICDIRECTORY",
        "WEBSERVER_WEBPORT", "WEBSERVER_SSL", "WEBSERVER_SSLPORT", "WEBSERVER_SSLCERT", "WEBSERVER_SSLKEY",
        "MYMPD_LOGLEVEL", "MYMPD_USER", "MYMPD_LOCALPLAYER", "MYMPD_COVERIMAGE", "MYMPD_COVERIMAGENAME", 
        "MYMPD_COVERIMAGESIZE", "MYMPD_VARLIBDIR", "MYMPD_MIXRAMP", "MYMPD_STICKERS", "MYMPD_TAGLIST", 
        "MYMPD_SEARCHTAGLIST", "MYMPD_BROWSETAGLIST", "MYMPD_SMARTPLS", "MYMPD_SYSCMDS", 
        "MYMPD_PAGINATION", "MYMPD_LASTPLAYEDCOUNT", "MYMPD_LOVE", "MYMPD_LOVECHANNEL", "MYMPD_LOVEMESSAGE",
        "PLUGINS_COVEREXTRACT", "PLUGINS_COVEREXTRACTLIB",
        "THEME_BACKGROUND", "THEME_BACKGROUNDFILTER", 0};
    const char** ptr = env_vars;
    while (*ptr != 0) {
        mympd_parse_env(config, *ptr);
        ++ptr;
    }
}

static bool init_plugins(struct t_config *config) {
    char *error;
    handle_plugins_coverextract = NULL;
    if (config->plugins_coverextract == true) {
        LOG_INFO("Loading plugin %s", config->plugins_coverextractlib);
        handle_plugins_coverextract = dlopen(config->plugins_coverextractlib, RTLD_LAZY);
        if (!handle_plugins_coverextract) {
            LOG_ERROR("Can't load plugin %s: %s", config->plugins_coverextractlib, dlerror());
            return false;
        }
        *(void **) (&plugin_coverextract) = dlsym(handle_plugins_coverextract, "coverextract");
        if ((error = dlerror()) != NULL)  {
            LOG_ERROR("Can't load plugin %s: %s", config->plugins_coverextractlib, error);
            return false;
        }
    }
    return true;
}

static void close_plugins(struct t_config *config) {
    if (config->plugins_coverextract == true && handle_plugins_coverextract != NULL) {
        dlclose(handle_plugins_coverextract);
    }
}

int main(int argc, char **argv) {
    s_signal_received = 0;
    char testdirname[400];
    bool init_webserver = false;
    bool init_thread_webserver = false;
    bool init_thread_mpdclient = false;
    bool init_thread_mympdapi = false;
    int rc = EXIT_FAILURE;
    loglevel = 2;
    
    mpd_client_queue = tiny_queue_create();
    mympd_api_queue = tiny_queue_create();
    web_server_queue = tiny_queue_create();

    t_mg_user_data *mg_user_data = (t_mg_user_data *)malloc(sizeof(t_mg_user_data));

    srand((unsigned int)time(NULL));
    
    //mympd config defaults
    t_config *config = (t_config *)malloc(sizeof(t_config));
    config->mpdhost = strdup("127.0.0.1");
    config->mpdport = 6600;
    config->mpdpass = NULL;
    config->webport = strdup("80");
    config->ssl = true;
    config->sslport = strdup("443");
    config->sslcert = strdup("/etc/mympd/ssl/server.pem");
    config->sslkey = strdup("/etc/mympd/ssl/server.key");
    config->user = strdup("mympd");
    config->streamport = 8000;
    config->streamurl = strdup("");
    config->coverimage = true;
    config->coverimagename = strdup("folder.jpg");
    config->coverimagesize = 240;
    config->varlibdir = strdup("/var/lib/mympd");
    config->stickers = true;
    config->mixramp = true;
    config->taglist = strdup("Artist,Album,AlbumArtist,Title,Track,Genre,Date,Composer,Performer");
    config->searchtaglist = strdup("Artist,Album,AlbumArtist,Title,Genre,Composer,Performer");
    config->browsetaglist = strdup("Artist,Album,AlbumArtist,Genre,Composer,Performer");
    config->smartpls = true;
    config->max_elements_per_page = 100;
    config->last_played_count = 20;
    config->etcdir = strdup("/etc/mympd");
    config->syscmds = false;
    config->localplayer = true;
    config->loglevel = 1;
    config->background = strdup("#888");
    config->backgroundfilter = strdup("blur(5px)");
    config->love = false;
    config->lovechannel = strdup("");
    config->lovemessage = strdup("love");
    config->plugins_coverextract = false;
    config->plugins_coverextractlib = strdup("/usr/share/mympd/lib/mympd_coverextract.so");
    config->music_directory = NULL;

    char *configfile = strdup("/etc/mympd/mympd.conf");
    if (argc == 2) {
        if (strncmp(argv[1], "/", 1) == 0) {
            FREE_PTR(configfile);
            configfile = argv[1];
            char *etcdir = strdup(configfile);
            FREE_PTR(config->etcdir);
            config->etcdir = strdup(dirname(etcdir));
            FREE_PTR(etcdir);
        }
        else {
            printf("myMPD  %s\n"
                "Copyright (C) 2018-2019 Juergen Mang <mail@jcgames.de>\n"
                "https://github.com/jcorporation/myMPD\n"
                "Built " __DATE__ " "__TIME__"\n\n"
                "Usage: %s [/path/to/mympd.conf]\n",
                MYMPD_VERSION,
                argv[0]
            );
            goto cleanup;
        }
    }
    
    LOG_INFO("Starting myMPD %s", MYMPD_VERSION);
    LOG_INFO("Libmpdclient %i.%i.%i", LIBMPDCLIENT_MAJOR_VERSION, LIBMPDCLIENT_MINOR_VERSION, LIBMPDCLIENT_PATCH_VERSION);
    LOG_INFO("Mongoose %s", MG_VERSION);
    
    if (access(configfile, F_OK ) != -1) {
        LOG_INFO("Parsing config file: %s", configfile);
        if (ini_parse(configfile, mympd_inihandler, config) < 0) {
            LOG_ERROR("Can't load config file \"%s\"", configfile);
            goto cleanup;
        }
    }
    else {
        LOG_WARN("No config file found, using defaults");
    }

    //read environment - overwrites config file definitions
    mympd_get_env(config);
    
    //set loglevel    
    set_loglevel(config);
    
    //init plugins
    if (init_plugins(config) == false) {
        goto cleanup;
    }

    //check music_directory
    if (strncmp(config->mpdhost, "/", 1) != 0 && strcmp(config->music_directory, "auto") == 0) {
        FREE_PTR(config->music_directory);
        config->music_directory = strdup("/var/lib/mpd/music");
        LOG_WARN("Not connected to socket, setting music_directory to %s", config->music_directory);
    }

    //set signal handler
    signal(SIGTERM, mympd_signal_handler);
    signal(SIGINT, mympd_signal_handler);
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);

    //init webserver
    struct mg_mgr mgr;
    if (!web_server_init(&mgr, config, mg_user_data)) {
        goto cleanup;
    }
    else {
        init_webserver = true;
    }

    //drop privileges
    if (getuid() == 0) {
        if (config->user != NULL || strlen(config->user) != 0) {
            LOG_INFO("Droping privileges to %s", config->user);
            struct passwd *pw;
            if ((pw = getpwnam(config->user)) == NULL) {
                LOG_ERROR("getpwnam() failed, unknown user");
                goto cleanup;
            } else if (setgroups(0, NULL) != 0) { 
                LOG_ERROR("setgroups() failed");
                goto cleanup;
            } else if (setgid(pw->pw_gid) != 0) {
                LOG_ERROR("setgid() failed");
                goto cleanup;
            } else if (setuid(pw->pw_uid) != 0) {
                LOG_ERROR("setuid() failed");
                goto cleanup;
            }
        }
    }
    
    if (getuid() == 0) {
        LOG_ERROR("myMPD should not be run with root privileges");
        goto cleanup;
    }

    //check needed directories
    if (!testdir("Document root", DOC_ROOT)) {
        goto cleanup;
    }

    snprintf(testdirname, 400, "%s/tmp", config->varlibdir);
    if (!testdir("Temp dir", testdirname)) {
        goto cleanup;
    }

    snprintf(testdirname, 400, "%s/smartpls", config->varlibdir);
    if (!testdir("Smartpls dir", testdirname)) {
        goto cleanup;
    }

    snprintf(testdirname, 400, "%s/state", config->varlibdir);
    if (!testdir("State dir", testdirname)) {
        goto cleanup;
    }
    
    if (config->plugins_coverextract == true) {
        snprintf(testdirname, 400, "%s/covercache", config->varlibdir);
        if (!testdir("Covercache dir", testdirname)) {
            goto cleanup;
        }
    }

    if (config->syscmds == true) {
        snprintf(testdirname, 400, "%s/syscmds", config->etcdir);
        if (!testdir("Syscmds directory", testdirname)) {
            LOG_INFO("Disabling syscmd support");
            config->syscmds = false;
        }
    }

    //Create working threads
    pthread_t mpd_client_thread, web_server_thread, mympd_api_thread;
    //mpd connection
    if (pthread_create(&mpd_client_thread, NULL, mpd_client_loop, config) == 0) {
        pthread_setname_np(mpd_client_thread, "mympd_mpdclient");
        init_thread_mpdclient = true;
    }
    else {
        LOG_ERROR("Can't create mympd_client thread");
        s_signal_received = SIGTERM;
    }
    //webserver
    if (pthread_create(&web_server_thread, NULL, web_server_loop, &mgr) == 0) {
        pthread_setname_np(web_server_thread, "mympd_webserver");
        init_thread_webserver = true;
    }
    else {
        LOG_ERROR("Can't create mympd_webserver thread");
        s_signal_received = SIGTERM;
    }
    //mympd api
    if (pthread_create(&mympd_api_thread, NULL, mympd_api_loop, config) == 0) {
        pthread_setname_np(mympd_api_thread, "mympd_mympdapi");
        init_thread_mympdapi = true;
    }
    else {
        LOG_ERROR("Can't create mympd_mympdapi thread");
        s_signal_received = SIGTERM;
    }

    //Outsourced all work to separate threads, do nothing...
    rc = EXIT_SUCCESS;

    //Try to cleanup all
    cleanup:
    if (init_thread_mpdclient)
        pthread_join(mpd_client_thread, NULL);
    if (init_thread_webserver)
        pthread_join(web_server_thread, NULL);
    if (init_thread_mympdapi)
        pthread_join(mympd_api_thread, NULL);
    if (init_webserver)
        web_server_free(&mgr);
    tiny_queue_free(web_server_queue);
    tiny_queue_free(mpd_client_queue);
    tiny_queue_free(mympd_api_queue);
    close_plugins(config);
    mympd_free_config(config);
    FREE_PTR(configfile);
    if (mg_user_data->music_directory != NULL)
        FREE_PTR(mg_user_data->music_directory);
    if (mg_user_data->pics_directory != NULL)
        FREE_PTR(mg_user_data->pics_directory);
    if (mg_user_data->rewrite_patterns != NULL)
        FREE_PTR(mg_user_data->rewrite_patterns);
    FREE_PTR(mg_user_data);
    return rc;
}
