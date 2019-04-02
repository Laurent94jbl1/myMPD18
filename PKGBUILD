# Maintainer: Juergen Mang <mail [at] jcgames [dot] de>
# Website: https://github.com/jcorporation/myMPD
# PKGBUILD Based on https://github.com/CultofRobots/archphile-custom/tree/master/mympd

pkgname=mympd
_pkgname=myMPD
pkgver=5.3.0
pkgrel=1
pkgdesc="myMPD is a standalone and mobile friendly web mpdclient."
arch=('x86_64' 'armv7h' 'aarch64')
url="http://github.org/jcorporation/myMPD"
license=('GPL')
depends=('libmpdclient' 'openssl')
makedepends=('cmake')
optdepends=('libmediainfo')
provides=()
conflicts=()
replaces=()
install=archlinux.install
#source=("mympd_${pkgver}.orig.tar.gz")
source=("https://github.com/jcorporation/${_pkgname}/archive/v${pkgver}.tar.gz")
sha256sums=('SKIP')

build() {
  if [ -d "${srcdir}/${_pkgname}-${pkgver}" ]
  then
    cd "${srcdir}/${_pkgname}-${pkgver}"
  else
    cd "${srcdir}"
  fi
  install -d release
  cd release
  cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=RELEASE ..
  make
}

package() {
  if [ -d "${srcdir}/${_pkgname}-${pkgver}/release" ]
  then
    cd "${srcdir}/${_pkgname}-${pkgver}/release"
  else
    cd "${srcdir}/release"
  fi
  make DESTDIR="$pkgdir/" install

  install -Dm644  "$pkgdir/usr/share/mympd/mympd.service" "$pkgdir/usr/lib/systemd/system/mympd.service"
}
