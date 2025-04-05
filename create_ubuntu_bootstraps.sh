#!/usr/bin/env bash

## A script for creating Ubuntu bootstraps for Wine compilation.
##
## debootstrap and perl are required
## root rights are required
##
## About 5.5 GB of free space is required
## And additional 2.5 GB is required for Wine compilation

if [ "$EUID" != 0 ]; then
	echo "This script requires root rights!"
	exit 1
fi

if ! command -v debootstrap 1>/dev/null || ! command -v perl 1>/dev/null; then
	echo "Please install debootstrap and perl and run the script again"
	exit 1
fi

# Keep in mind that although you can choose any version of Ubuntu/Debian
# here, but this script has only been tested with Ubuntu 18.04 Bionic
export CHROOT_DISTRO="bionic"
#export CHROOT_MIRROR="https://ftp.uni-stuttgart.de/ubuntu/"
export CHROOT_MIRROR="https://bouyguestelecom.ubuntu.lafibre.info/ubuntu/"

# Set your preferred path for storing chroots
# Also don't forget to change the path to the chroots in the build_wine.sh
# script, if you are going to use it
export MAINDIR=/opt/chroots
export CHROOT_X64="${MAINDIR}"/${CHROOT_DISTRO}64_chroot
export CHROOT_X32="${MAINDIR}"/${CHROOT_DISTRO}32_chroot

prepare_chroot () {
	if [ "$1" = "32" ]; then
		CHROOT_PATH="${CHROOT_X32}"
	else
		CHROOT_PATH="${CHROOT_X64}"
	fi

	echo "Unmount chroot directories. Just in case."
	umount -Rl "${CHROOT_PATH}"

	echo "Mount directories for chroot"
	mount --bind "${CHROOT_PATH}" "${CHROOT_PATH}"
	mount -t proc /proc "${CHROOT_PATH}"/proc
	mount --bind /sys "${CHROOT_PATH}"/sys
	mount --make-rslave "${CHROOT_PATH}"/sys
	mount --bind /dev "${CHROOT_PATH}"/dev
	mount --bind /dev/pts "${CHROOT_PATH}"/dev/pts
	mount --bind /dev/shm "${CHROOT_PATH}"/dev/shm
	mount --make-rslave "${CHROOT_PATH}"/dev

	rm -f "${CHROOT_PATH}"/etc/resolv.conf
	cp /etc/resolv.conf "${CHROOT_PATH}"/etc/resolv.conf

	echo "Chrooting into ${CHROOT_PATH}"
	chroot "${CHROOT_PATH}" /usr/bin/env LANG=en_US.UTF-8 TERM=xterm PATH="/bin:/sbin:/usr/bin:/usr/sbin" /opt/prepare_chroot.sh

	echo "Unmount chroot directories"
	umount -l "${CHROOT_PATH}"
	umount "${CHROOT_PATH}"/proc
	umount "${CHROOT_PATH}"/sys
	umount "${CHROOT_PATH}"/dev/pts
	umount "${CHROOT_PATH}"/dev/shm
	umount "${CHROOT_PATH}"/dev
}

create_build_scripts () {
	sdl2_version="2.32.4"
	faudio_version="23.03"
	vulkan_headers_version="1.4.312"
	vulkan_loader_version="1.4.312"
	spirv_headers_version="sdk-1.3.296.0"
 	libpcap_version="1.10.5"
  	libxkbcommon_version="1.6.0"
   	python3_version="3.12.7"
    	meson_version="1.3.2"
     	cmake_version="3.30.6"
      	ccache_version="4.10.2"
	mingw64_version="12.0.0.r458.g03d8a40f5"
	gmp_version="6.3.0"
	nasm_version="2.16.03"
	yasm_version="1.3.0"
	libgpg_error_version="1.51"
	libgcrypt_version="1.10.3"
	winetricks_version="20250102";
	libglvnd_version="1.7.0";

	cat <<EOF > "${MAINDIR}"/prepare_chroot.sh
#!/bin/bash

apt-get update
apt-get -y install nano
apt-get -y install locales
echo ru_RU.UTF_8 UTF-8 >> /etc/locale.gen
echo en_US.UTF_8 UTF-8 >> /etc/locale.gen
locale-gen
echo deb '${CHROOT_MIRROR}' ${CHROOT_DISTRO} main universe > /etc/apt/sources.list
echo deb '${CHROOT_MIRROR}' ${CHROOT_DISTRO}-updates main universe >> /etc/apt/sources.list
echo deb '${CHROOT_MIRROR}' ${CHROOT_DISTRO}-security main universe >> /etc/apt/sources.list
echo deb-src '${CHROOT_MIRROR}' ${CHROOT_DISTRO} main universe >> /etc/apt/sources.list
echo deb-src '${CHROOT_MIRROR}' ${CHROOT_DISTRO}-updates main universe >> /etc/apt/sources.list
echo deb-src '${CHROOT_MIRROR}' ${CHROOT_DISTRO}-security main universe >> /etc/apt/sources.list
apt-get update
apt-get -y upgrade
apt-get -y dist-upgrade
apt-get -y install software-properties-common
add-apt-repository -y ppa:ubuntu-toolchain-r/test
add-apt-repository -y ppa:cybermax-dexter/mingw-w64-backport
apt-get update
apt-get -y build-dep wine-development libsdl2 libvulkan1 python3
apt-get -y install ccache gcc-11 g++-11 wget git gcc-mingw-w64 g++-mingw-w64 ninja-build
apt-get -y install libxpresent-dev libjxr-dev libusb-1.0-0-dev libgcrypt20-dev libpulse-dev libudev-dev libsane-dev libv4l-dev libkrb5-dev libgphoto2-dev liblcms2-dev libcapi20-dev
apt-get -y install libjpeg62-dev samba-dev
apt-get -y install libpcsclite-dev libcups2-dev
apt-get -y install python3-pip libxcb-xkb-dev libbz2-dev liblzma-dev libzstd-dev liblz4-dev
apt-get -y install libncurses5-dev libgdbm-dev libnss3-dev libssl-dev libreadline-dev libffi-dev libsqlite3-dev
apt-get -y purge libvulkan-dev libvulkan1 libsdl2-dev libsdl2-2.0-0 libpcap0.8-dev libpcap0.8 --purge --autoremove
apt-get -y purge *gstreamer* --purge --autoremove
apt-get -y clean
apt-get -y autoclean
export PATH="/usr/local/bin:\${PATH}"
mkdir /opt/build_libs
cd /opt/build_libs
wget -O sdl.tar.gz https://www.libsdl.org/release/SDL2-${sdl2_version}.tar.gz
wget -O faudio.tar.gz https://github.com/FNA-XNA/FAudio/archive/${faudio_version}.tar.gz
wget -O vulkan-loader.tar.gz https://github.com/KhronosGroup/Vulkan-Loader/archive/v${vulkan_loader_version}.tar.gz
wget -O vulkan-headers.tar.gz https://github.com/KhronosGroup/Vulkan-Headers/archive/v${vulkan_headers_version}.tar.gz
wget -O spirv-headers.tar.gz https://github.com/KhronosGroup/SPIRV-Headers/archive/refs/tags/vulkan-${spirv_headers_version}.tar.gz
wget -O libpcap.tar.gz https://www.tcpdump.org/release/libpcap-${libpcap_version}.tar.gz
wget -O libxkbcommon.tar.xz https://xkbcommon.org/download/libxkbcommon-${libxkbcommon_version}.tar.xz
wget -O python3.tar.gz https://www.python.org/ftp/python/${python3_version}/Python-${python3_version}.tgz
wget -O meson.tar.gz https://github.com/mesonbuild/meson/releases/download/${meson_version}/meson-${meson_version}.tar.gz
wget -O mingw.tar.xz http://techer.pascal.free.fr/Red-Rose_MinGW-w64-Toolchain/Red-Rose-MinGW-w64-Posix-Urct-v${mingw64_version}-Gcc-11.5.0.tar.xz
wget -O cmake.tar.gz https://github.com/Kitware/CMake/releases/download/v${cmake_version}/cmake-${cmake_version}.tar.gz
wget -O ccache.tar.gz https://github.com/ccache/ccache/releases/download/v${ccache_version}/ccache-${ccache_version}.tar.gz
wget -O gmp.tar.xz https://gmplib.org/download/gmp/gmp-${gmp_version}.tar.xz
wget -O nasm.tar.gz https://www.nasm.us/pub/nasm/releasebuilds/${nasm_version}/nasm-${nasm_version}.tar.gz
wget -O yasm.tar.gz https://github.com/yasm/yasm/releases/download/v${yasm_version}/yasm-${yasm_version}.tar.gz
wget -O libgpg-error.tar.bz2 https://www.gnupg.org/ftp/gcrypt/libgpg-error/libgpg-error-${libgpg_error_version}.tar.bz2
wget -O libgcrypt.tar.bz2 https://www.gnupg.org/ftp/gcrypt/libgcrypt/libgcrypt-${libgcrypt_version}.tar.bz2
wget -O /usr/include/linux/ntsync.h https://raw.githubusercontent.com/zen-kernel/zen-kernel/refs/heads/6.13/main/include/uapi/linux/ntsync.h
wget -O /usr/include/linux/userfaultfd.h https://raw.githubusercontent.com/zen-kernel/zen-kernel/refs/heads/6.13/main/include/uapi/linux/userfaultfd.h
if [ -d /usr/lib/i386-linux-gnu ]; then wget -O wine.deb https://dl.winehq.org/wine-builds/ubuntu/dists/bionic/main/binary-i386/wine-stable_4.0.3~bionic_i386.deb; fi
if [ -d /usr/lib/x86_64-linux-gnu ]; then wget -O wine.deb https://dl.winehq.org/wine-builds/ubuntu/dists/bionic/main/binary-amd64/wine-stable_4.0.3~bionic_amd64.deb; fi
git clone https://gitlab.freedesktop.org/gstreamer/gstreamer.git -b 1.22
tar xf sdl.tar.gz
tar xf faudio.tar.gz
tar xf vulkan-loader.tar.gz
tar xf vulkan-headers.tar.gz
tar xf spirv-headers.tar.gz
tar xf libpcap.tar.gz
tar xf libxkbcommon.tar.xz
tar xf python3.tar.gz
tar xf cmake.tar.gz
tar xf ccache.tar.gz
tar xf gmp.tar.xz
tar xf nasm.tar.gz
tar xf yasm.tar.gz
tar xf mingw.tar.xz -C /
tar xf mingw.tar.xz -C /
tar xf mingw.tar.xz -C /
tar xf mingw.tar.xz -C /
tar xf meson.tar.gz -C /usr/local
tar xf libgpg-error.tar.bz2
tar xf libgcrypt.tar.bz2
ln -s /usr/local/meson-${meson_version}/meson.py /usr/local/bin/meson
if [ -d /usr/lib/i386-linux-gnu ];
then
          export CC="gcc-11 -m32";
          export CXX="g++-11 -m32";
          export local_arch="i686";
	  export local_lib="i386";
	  export my_extra_CFLAGS=" -mstackrealign ";
fi
if [ -d /usr/lib/x86_64-linux-gnu ]; 
then
	 export CC="gcc-11";
	 export CXX="g++-11";
	 export local_arch="x86_64";
	 export local_lib="x86_64";
	 export my_extra_CFLAGS=" -mcmodel=small ";
fi
export CFLAGS="-O2"
export CXXFLAGS="-O2"
cd /opt/build_libs/nasm-${nasm_version}
./configure --prefix=/usr && make -j$(nproc) && make install
cd /opt/build_libs/yasm-${yasm_version}
./configure --prefix=/usr && make -j$(nproc) && make install
cd /opt/build_libs/cmake-${cmake_version}
cd /opt/build_libs
wget https://ftp.gnu.org/gnu/libunistring/libunistring-1.3.tar.xz && tar xf libunistring-1.3.tar.xz && cd libunistring-1.3
./configure --prefix=/usr/ --libdir=/usr/lib/\${local_lib}-linux-gnu  --build=\${local_arch}-pc-linux-gnu --host=\${local_arch}-pc-linux-gnu && make -j$(nproc) && make install
cd /opt/build_libs
wget https://ftp.gnu.org/gnu/libidn/libidn2-2.3.7.tar.gz && tar xf libidn2-2.3.7.tar.gz && cd libidn2-2.3.7
./configure --prefix=/usr/ --libdir=/usr/lib/\${local_lib}-linux-gnu  --build=\${local_arch}-pc-linux-gnu --host=\${local_arch}-pc-linux-gnu && make -j$(nproc) && make install
cd /opt/build_libs
wget https://github.com/rockdaboot/libpsl/releases/download/0.21.5/libpsl-0.21.5.tar.gz && tar xf libpsl-0.21.5.tar.gz && cd libpsl-0.21.5
./configure --prefix=/usr/ --libdir=/usr/lib/\${local_lib}-linux-gnu  --build=\${local_arch}-pc-linux-gnu --host=\${local_arch}-pc-linux-gnu && make -j$(nproc) && make install
cd /opt/build_libs
wget https://github.com/nghttp2/nghttp2/releases/download/v1.64.0/nghttp2-1.64.0.tar.xz && tar xf nghttp2-1.64.0.tar.xz && cd nghttp2-1.64.0
./configure --prefix=/usr/ --libdir=/usr/lib/\${local_lib}-linux-gnu  --build=\${local_arch}-pc-linux-gnu --host=\${local_arch}-pc-linux-gnu --with-openssl && make -j$(nproc) && make install
cd /opt/build_libs
wget https://curl.se/download/curl-8.11.1.tar.xz && tar xf curl-8.11.1.tar.xz  && cd curl-8.11.1 
wget "https://glfs-book.github.io/glfs/patches/curl-eventfd_regression-1.patch" -O - | patch -Np1
./configure --prefix=/usr/ --libdir=/usr/lib/\${local_lib}-linux-gnu  --build=\${local_arch}-pc-linux-gnu --host=\${local_arch}-pc-linux-gnu --with-openssl && make -j$(nproc) && make install
cd /opt/build_libs/cmake-${cmake_version}
./bootstrap --prefix=/usr --parallel=$(nproc) && make -j$(nproc) && make -j$(nproc) install
cd ../ && mkdir build && cd build
cmake ../ccache-${ccache_version} && make -j$(nproc) && make install
cd ../ && rm -r build && mkdir build && cd build
if [ -d /usr/lib/i386-linux-gnu ]; then ABI=32 ../gmp-${gmp_version}/configure --enable-cxx --prefix=/usr/ --libdir=/usr/lib/i386-linux-gnu/; fi
if [ -d /usr/lib/x86_64-linux-gnu ]; then ../gmp-${gmp_version}/configure --enable-cxx --prefix=/usr/ --libdir=/usr/lib/x86_64-linux-gnu/; fi
make -j$(nproc) && make install
cd ../ && rm -r build && mkdir build && cd build
cmake ../Vulkan-Headers-${vulkan_headers_version} && make -j$(nproc) && make install
cd ../ && rm -r build && mkdir build && cd build
cmake ../Vulkan-Loader-${vulkan_loader_version} && make -j$(nproc) && make install
cd ../ && rm -r build && mkdir build && cd build
cmake ../SPIRV-Headers-vulkan-${spirv_headers_version} && make -j$(nproc) && make install
cd ../ && rm -r build && mkdir build && cd build
cmake ../SDL2-${sdl2_version} && make -j$(nproc) && make install
cd ../ && rm -r build && mkdir build && cd build
cmake ../FAudio-${faudio_version} && make -j$(nproc) && make install
cd ../ && dpkg -x wine.deb .
cp opt/wine-stable/bin/widl /usr/bin
rm -r build && mkdir build && cd build
../libpcap-${libpcap_version}/configure && make -j$(nproc) install
cd ../ && rm -r build && mkdir build && cd build
../Python-${python3_version}/configure --enable-optimizations
make -j$(nproc)
make -j$(nproc) altinstall
ln -s /usr/local/bin/python3.12 /usr/local/bin/python
ln -s /usr/local/bin/python3.12 /usr/local/bin/python3
ln -s /usr/local/bin/pip3.12 /usr/local/bin/pip3
python -m pip install --upgrade pip
pip3 install setuptools
python -m pip install meson
python -m pip install ninja
pip3 install piper
pip3 install vosk
cd ../libxkbcommon-${libxkbcommon_version}
meson setup build -Denable-docs=false
meson compile -C build
meson install -C build
cd ../gstreamer
meson setup build
ninja -C build
ninja -C build install
cd /opt/build_libs/libgpg-error-${libgpg_error_version}
./configure --prefix=/usr/ --libdir=/usr/lib/\${local_lib}-linux-gnu --build=\${local_arch}-pc-linux-gnu --host=\${local_arch}-pc-linux-gnu && make -j$(nproc) && make install
export PKG_CONFIG_PATH=/usr/lib/\${local_lib}-linux-gnu:\${PKG_CONFIG_PATH}
cd /opt/build_libs/libgcrypt-${libgcrypt_version}
if [ -d /usr/lib/i386-linux-gnu ];
then
	  sed 's:i\[34567\]86\*-\*-\*:x86_64-*-*:' -i mpi/config.links;
	  sed 's:x86_64-\*-\*:ignore:;s:i?86-\*-\*:x86_64-*-*:' -i configure.ac;
	  autoreconf -fi;
	  ABI=32 ./configure --prefix=/usr/ --libdir=/usr/lib/\${local_lib}-linux-gnu  --build=\${local_arch}-pc-linux-gnu --host=\${local_arch}-pc-linux-gnu && make -j$(nproc) && make install;
fi
if [ -d /usr/lib/x86_64-linux-gnu ]; 
then 
	  ./configure --prefix=/usr/ --libdir=/usr/lib/\${local_lib}-linux-gnu  --build=\${local_arch}-pc-linux-gnu --host=\${local_arch}-pc-linux-gnu && make -j$(nproc) && make install; 
fi

cd /opt/build_libs
git clone https://github.com/dwbuiten/obuparse.git && cd obuparse/ && git checkout 918524abdc19b6582d853c03373d8e2e0b9f11ee
make PREFIX=/usr/local install
cd /opt/build_libs
git clone https://github.com/vimeo/l-smash.git && cd l-smash/ && git checkout 30270d0d8b551b36b6f46c43bd3ffe997f13e157
make prefix=/usr/local libdir=/usr/local/lib install
cd /opt/build_libs
git clone https://code.videolan.org/videolan/x264.git && cd x264 && git checkout 52f7694ddd35209cb95225e7acce91d8a30cb57d
export LSMASH_LIBS="\$(pkg-config  --libs liblsmash)"
export LSMASH_CFLAGS="\$(pkg-config  --cflags liblsmash)"
export CXXFLAGS="-O3 -march=znver2 -mmmx -mpopcnt -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mavx -mavx2 -msse4a -mfma -mbmi -mbmi2 -maes -mpclmul -madx -mabm -mclflushopt -mclwb -mclzero -mcx16 -mf16c -mfsgsbase -mfxsr -msahf -mlzcnt -mmovbe -mmwaitx -mprfchw -mrdpid -mrdrnd -mrdseed -msha -mwbnoinvd -mxsave -mxsavec -mxsaveopt -mxsaves --param l1-cache-size=32 --param l1-cache-line-size=64 --param l2-cache-size=512 -mtune=znver2 -fasynchronous-unwind-tables -Wformat -Wformat-security -dumpbase - -pipe -mfpmath=sse -fwrapv -fno-strict-aliasing \${my_extra_CFLAGS} -fno-stack-protector "
export CFLAGS="-O3 -march=znver2 -mmmx -mpopcnt -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mavx -mavx2 -msse4a -mfma -mbmi -mbmi2 -maes -mpclmul -madx -mabm -mclflushopt -mclwb -mclzero -mcx16 -mf16c -mfsgsbase -mfxsr -msahf -mlzcnt -mmovbe -mmwaitx -mprfchw -mrdpid -mrdrnd -mrdseed -msha -mwbnoinvd -mxsave -mxsavec -mxsaveopt -mxsaves --param l1-cache-size=32 --param l1-cache-line-size=64 --param l2-cache-size=512 -mtune=znver2 -fasynchronous-unwind-tables -Wformat -Wformat-security -dumpbase - -pipe -mfpmath=sse -fwrapv -fno-strict-aliasing \${my_extra_CFLAGS} -fno-stack-protector "
./configure --prefix=/usr/ --libdir=/usr/lib/\${local_lib}-linux-gnu  --build=\${local_arch}-pc-linux-gnu --host=\${local_arch}-pc-linux-gnu  --disable-cli --enable-shared --enable-lto && make -j$(nproc) && make install

if [ -d /usr/lib/x86_64-linux-gnu ]
then 
	cd /opt/build_libs
	wget https://github.com/kyz/libmspack/archive/refs/tags/v1.11.tar.gz && tar xf v1.11.tar.gz && cd libmspack-1.11/libmspack/
	./autogen.sh 
	./configure --prefix=/usr/ --libdir=/usr/lib/x86_64-linux-gnu/ && make -j$(nproc) && make install
	cd /opt/build_libs
	wget https://www.cabextract.org.uk/cabextract-1.11.tar.gz  && tar xf cabextract-1.11.tar.gz && cd cabextract-1.11
	./configure --prefix=/usr/ --libdir=/usr/lib/x86_64-linux-gnu/ --with-external-libmspack && make -j$(nproc) && make install
	cd /opt/build_libs
	wget https://github.com/Winetricks/winetricks/archive/refs/tags/${winetricks_version}.tar.gz && tar xf ${winetricks_version}.tar.gz && cd winetricks-${winetricks_version}/
	make prefix=/usr/ install
fi

cd /opt/build_libs
wget https://github.com/OpenPrinting/cups/releases/download/v2.4.11/cups-2.4.11-source.tar.gz && tar xf cups-2.4.11-source.tar.gz && cd cups-2.4.11
./configure --prefix=/usr/ --libdir=/usr/lib/\${local_lib}-linux-gnu  --build=\${local_arch}-pc-linux-gnu --host=\${local_arch}-pc-linux-gnu && make -j$(nproc) && make install && cp -vf *.pc /usr/lib/\${local_lib}-linux-gnu

cd /opt/build_libs
wget http://download.savannah.nongnu.org/releases/libunwind/libunwind-1.6.2.tar.gz &&  tar xf libunwind-1.6.2.tar.gz && cd libunwind-1.6.2
./configure --prefix=/usr/ --libdir=/usr/lib/\${local_lib}-linux-gnu  --build=\${local_arch}-pc-linux-gnu --host=\${local_arch}-pc-linux-gnu && make -j$(nproc) && make install

cd /opt/build_libs
wget -O libglvnd.tar.gz https://gitlab.freedesktop.org/glvnd/libglvnd/-/archive/v${libglvnd_version}/libglvnd-v${libglvnd_version}.tar.gz
tar xf libglvnd.tar.gz
cd libglvnd-v1.7.0/
meson setup build
meson compile -C build
meson install -C build


cd /opt && rm -r /opt/build_libs
EOF

	chmod +x "${MAINDIR}"/prepare_chroot.sh
	cp "${MAINDIR}"/prepare_chroot.sh "${CHROOT_X32}"/opt
	mv "${MAINDIR}"/prepare_chroot.sh "${CHROOT_X64}"/opt
}

mkdir -p "${MAINDIR}"

debootstrap --arch amd64 $CHROOT_DISTRO "${CHROOT_X64}" $CHROOT_MIRROR
debootstrap --arch i386 $CHROOT_DISTRO "${CHROOT_X32}" $CHROOT_MIRROR

create_build_scripts
prepare_chroot 32
prepare_chroot 64

rm "${CHROOT_X64}"/opt/prepare_chroot.sh
rm "${CHROOT_X32}"/opt/prepare_chroot.sh

clear
echo "Done"
