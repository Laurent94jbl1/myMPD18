var CACHE = 'myMPD-cache-v5.4.0';
var subdir = self.location.pathname.replace('/sw.min.js', '').replace(/\/$/, '');
var urlsToCache = [
    subdir + '/',
    subdir + '/css/bootstrap.min.css',
    subdir + '/css/mympd.min.css',
    subdir + '/js/bootstrap-native-v4.min.js',
    subdir + '/js/mympd.min.js',
    subdir + '/js/keymap.min.js',
    subdir + '/assets/appicon-167.png',
    subdir + '/assets/appicon-192.png',
    subdir + '/assets/appicon-512.png',
    subdir + '/assets/coverimage-stream.png',
    subdir + '/assets/coverimage-notavailable.png',
    subdir + '/assets/coverimage-loading.png',
    subdir + '/assets/favicon.ico',
    subdir + '/assets/MaterialIcons-Regular.woff2'
];

var ignoreRequests = new RegExp('(' + [
  subdir + '/api',
  subdir + '/ws',
  subdir + '/library\/(.*)',
  subdir + '/pics\/(.*)'].join('(\/?)|\\') + ')$')

self.addEventListener('install', function(event) {
    event.waitUntil(
        caches.open(CACHE).then(function(cache) {
            return cache.addAll(urlsToCache);
        })
    );
});

self.addEventListener('fetch', function(event) {
    if (event.request.url.match('^http://')) {
        return false;
    }
    if (ignoreRequests.test(event.request.url)) {
        return false;
    }
    event.respondWith(
        caches.match(event.request).then(function(response) {
            if (response) {
                return response
            }
            else {
                return fetch(event.request);
            }
        })
    );    
});

self.addEventListener('activate', function(event) {
    event.waitUntil(
        caches.keys().then(function(cacheNames) {
            return Promise.all(
                cacheNames.map(function(cacheName) {
                    if (cacheName != CACHE) {
                        return caches.delete(cacheName);
                    }
                })
            );
        })
    );
});
