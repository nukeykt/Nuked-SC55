# Copyright 1999-2024 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=8

inherit cmake-multilib desktop

DESCRIPTION="Roland SoundCanvas SC-55 emulator"
HOMEPAGE="http://nukeykt.retrohost.net/"

MY_PN="Nuked-SC55"

if [[ ${PV} == 9999 ]]; then
	EGIT_REPO_URI="https://github.com/nukeykt/${MY_PN}.git"
	inherit git-r3
else
	SRC_URI="https://github.com/nukeykt/${MY_PN}/archive/refs/tags/${PV}.tar.gz -> ${P}.tar.gz"
	KEYWORDS="~amd64 ~x86"
fi

LICENSE="XMAME"
SLOT="0"

DEPEND="
	media-libs/libsdl2
	media-libs/rtmidi
"
RDEPEND="${DEPEND}"

S="${WORKDIR}/${MY_PN}-${PV}"

DOCS="${S}/README.md"

multilib_src_configure() {
        local mycmakeargs=(
                -DUSE_RTMIDI=ON
                -DUSE_SYSTEM_RTMIDI=ON
                -DCMAKE_INSTALL_PREFIX=/usr
        )
        cmake_src_configure
}

multilib_src_install() {
        cmake_src_install
        make_desktop_entry nuked-sc55 "Nuked-SC55"
}