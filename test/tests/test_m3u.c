/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"

#include "../../dist/utest/utest.h"
#include "../../dist/sds/sds.h"
#include "../../src/lib/jsonrpc.h"
#include "../../src/lib/m3u.h"
#include "../../src/mympd_api/mympd_api_webradios.h"

#include <sys/stat.h>

UTEST(mympd_api_webradios, test_mympd_api_webradio_save) {
    sds workdir = sdsnew("/tmp");
    sds name = sdsnew("Yumi Co. Radio");
    sds uri = sdsnew("http://yumicoradio.net:8000/stream");
    sds uri_old = sdsnew("");
    sds genre = sdsnew("Future Funk, City Pop, Anime Groove, Vaporwave, Nu Disco, Electronic");
    sds picture = sdsnew("http___yumicoradio_net_8000_stream.webp");
    sds homepage = sdsnew("http://yumicoradio.net");
    sds country = sdsnew("France");
    sds language = sdsnew("English");
    sds codec = sdsnew("MP3");
    sds description = sdsnew("24/7 webradio that plays Future Funk, City Pop, Anime Groove, Nu Disco, Electronica, a little bit of Vaporwave and some of the sub-genres derived.");
    int bitrate = 256;
    mkdir("/tmp/webradios", 0770);
    bool rc = mympd_api_webradio_save(workdir, name, uri, uri_old, genre, picture, homepage, country, language, codec, bitrate, description);
    ASSERT_TRUE(rc);
    sdsfree(workdir);
    sdsfree(name);
    sdsfree(uri);
    sdsfree(uri_old);
    sdsfree(genre);
    sdsfree(picture);
    sdsfree(homepage);
    sdsfree(country);
    sdsfree(language);
    sdsfree(codec);
    sdsfree(description);
}

UTEST(m3u, test_m3u_get_field) {
    sds s = sdsempty();
    s = m3u_get_field(s, "#EXTIMG", "/tmp/webradios/http___yumicoradio_net_8000_stream.m3u");
    ASSERT_STREQ("http___yumicoradio_net_8000_stream.webp", s);
    sdsfree(s);
}

UTEST(m3u, test_m3u_to_json) {
    sds s = sdsempty();
    sds m3ufields = sdsempty();
    s = m3u_to_json(s, "/tmp/webradios/http___yumicoradio_net_8000_stream.m3u", &m3ufields);
    const char *e = "-1,yumi co. radiofuture funk, city pop, anime groove, vaporwave, nu disco, electronicyumi co. radiohttp___yumicoradio_net_8000_stream.webphttp://yumicoradio.netfranceenglish24/7 webradio that plays future funk, city pop, anime groove, nu disco, electronica, a little bit of vaporwave and some of the sub-genres derived.mp3256";
    ASSERT_STREQ(e, m3ufields);
    sdsfree(s);
    sdsfree(m3ufields);
}

UTEST(mympd_api_webradios, test_get_webradio_from_uri) {
    sds workdir = sdsnew("/tmp");
    sds m3u = get_webradio_from_uri(workdir, "http://yumicoradio.net:8000/stream");
    ASSERT_GT(sdslen(m3u), (size_t)0);
    sdsfree(workdir);
    sdsfree(m3u);
}

UTEST(mympd_api_webradios, test_mympd_api_webradio_list) {
    sds workdir = sdsnew("/tmp");
    sds method = sdsnew("METHOD");
    sds searchstr = sdsempty();
    sds buffer = mympd_api_webradio_list(workdir, sdsempty(), method, 0, searchstr, 0, 10);
    sds error = sdsempty();
    int result;
    bool rc = json_get_int_max(buffer, "$.result.totalEntities", &result, &error);
    ASSERT_TRUE(rc);
    ASSERT_EQ(result, 1);
    sdsfree(error);
    sdsfree(method);
    sdsfree(searchstr);
    sdsfree(buffer);
    sdsfree(workdir);
}

UTEST(mympd_api_webradios, test_mympd_api_webradio_delete) {
    sds workdir = sdsnew("/tmp");
    sds filename = sdsnew("http___yumicoradio_net_8000_stream.m3u");
    bool rc = mympd_api_webradio_delete(workdir, filename);
    sdsfree(workdir);
    sdsfree(filename);
    ASSERT_TRUE(rc);
    rmdir("/tmp/webradios");
}
