#!/bin/bash

# DISCLAIMER
#=============
# This script is a pure re-work of the original script 'build-mingw-w64.sh' used by old version of Proton before docker switch
# You could fine its original version at 
#    https://github.com/Valloric/proton-ge-custom/blob/master/build-mingw-w64.sh 
#
# before Proton Team took decision to use dockers
#
#I tried here to 
# - update this script with more recents sources for both binutils 2.38 and gcc 11.5
# - keep the original idea as-it-is
# - add more options for configuration
# - keep original comments as-they-are
# - re-use this MinGW-w64 toolchain for my own usage
# - apply patches taken from the Internet and from both Debian and Gentoo 

# Packages required from the host machine I use on Linux Mint -- not an exhaustive list
# apt-get install build-essential git texinfo mingw-w64 automake libzstd-dev autotools-dev libtool libsrt-openssl-dev libssl-dev autopoint bison flex gettext

set -e

# We need two cross-compilers to build Proton:
#
# 64-bit cross-compiler:
#  Build (where the compiler is built): 64-bit linux (our VM)
#  Host (where the compiler is run):    64-bit linux (64-bit Steam runtime)
#  Target (what the compiler outputs):  64-bit win32 (PE files to be run)
#
# 32-bit cross-compiler:
#  Build (where the compiler is built): 64-bit linux (our VM)
#  Host (where the compiler is run):    64-bit linux (64-bit Steam runtime)
#  Target (what the compiler outputs):  32-bit win32 (PE files to be run)
#
# From https://github.com/ValveSoftware/Proton/blob/proton_8.0/Makefile.in
# Refer to comments in https://aur.archlinux.org/cgit/aur.git/tree/PKGBUILD?h=vkd3d-proton-mingw-git
#
# Let's re-use Proton flags
#
export OPTIMIZE_FLAGS=" -O3 -march=znver2 -mmmx -mpopcnt -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mavx -mavx2 -msse4a -mfma -mbmi -mbmi2 -maes -mpclmul -madx -mabm -mclflushopt -mclwb -mclzero -mcx16 -mf16c -mfsgsbase -mfxsr -msahf -mlzcnt -mmovbe -mmwaitx -mprfchw -mrdpid -mrdrnd -mrdseed -msha -mwbnoinvd -mxsave -mxsavec -mxsaveopt -mxsaves --param l1-cache-size=32 --param l1-cache-line-size=64 --param l2-cache-size=512 -mtune=znver2 -fasynchronous-unwind-tables -Wformat -Wformat-security -dumpbase - -pipe  "
export SANITY_FLAGS=" -mfpmath=sse -fwrapv -fno-strict-aliasing "
export COMMON_FLAGS=" ${OPTIMIZE_FLAGS} ${SANITY_FLAGS} "

export CFLAGS32=" ${COMMON_FLAGS} -mstackrealign -fno-stack-protector "
export CFLAGS64=" ${COMMON_FLAGS} -mcmodel=small -fno-stack-protector "

if [ -z "$1" ]; then
    echo "Makes a local build of mingw-w64 in this directory and installs it to the given path."
    echo ""
    echo "Note: Requires a system mingw-w64 compiler to be present already on your build machine, for us to bootstrap with."
    echo ""
    echo "usage:"
    echo -e "\t$0 <installation path e.g. \$HOME/.local>"
    exit 1
fi

if [ -z "$MAKEFLAGS" ]; then
    JOBS=-j$(($(nproc) - 1))
fi

DST_DIR="$1"

# Point to a fresh snapshot
# Tarball was generated cloning using this command
#  git clone git://sourceware.org/git/binutils-gdb.git -b binutils-2_38-branch
# Its repository could be available at 
#
BINUTILS_VER="2.38.r183.g4d71e17a9fd"
BINUTILS_SRCTARBALL=binutils-$BINUTILS_VER.tar.xz
BINUTILS_URL="http://techer.pascal.free.fr/download/$BINUTILS_SRCTARBALL"
BINUTILS_SRCDIR=binutils-$BINUTILS_VER

GCC_VER=11.5.0
GCC_SRCTARBALL=gcc-$GCC_VER.tar.xz
GCC_URL="https://gcc.gnu.org/pub/gcc/releases/gcc-${GCC_VER}/$GCC_SRCTARBALL"
GCC_SRCDIR=gcc-$GCC_VER

MINGW_W64_BRANCH=master
MINGW_W64_PKGVER="12.0.0.r647.g16335b3c4"
MINGW_W64_GITVER="16335b3c4d9ac195f9ebd065849f4f6e2db95903"
MINGW_W64_GITURL="https://github.com/mingw-w64/mingw-w64.git" 
# repo from github =>  https://github.com/mingw-w64/mingw-w64.git
# repo from sourceforge.net => https://git.code.sf.net/p/mingw-w64/mingw-w64 # pretty slow !!!
MINGW_W64_SRCDIR=mingw-w64-git

ISL_VER=0.27
ISL_SRCTARBALL=isl-$ISL_VER.tar.bz2
ISL_URL="https://libisl.sourceforge.io//isl-$ISL_VER.tar.bz2"
ISL_SRCDIR=isl-$ISL_VER

function setup_src {
    if [ ! -e "$BINUTILS_SRCTARBALL" ]; then
        wget --no-check-certificate -O "$BINUTILS_SRCTARBALL" "$BINUTILS_URL"
    fi

    if [ ! -e "$BINUTILS_SRCDIR" ]; then
        tar -xf "$BINUTILS_SRCTARBALL"
    fi

    if [ ! -e "$ISL_SRCTARBALL" ]; then
        wget --no-check-certificate -O "$ISL_SRCTARBALL" "$ISL_URL"
    fi

    if [ ! -e "$ISL_SRCDIR" ]; then
        tar -xf "$ISL_SRCTARBALL"
    fi

    if [ ! -e "$GCC_SRCTARBALL" ]; then
        wget --no-check-certificate -O "$GCC_SRCTARBALL" "$GCC_URL"
    fi
    
    wget --no-check-certificate https://ftp.gnu.org/gnu/mpfr/mpfr-4.2.1.tar.xz && tar xJf mpfr-4.2.1.tar.xz
    wget --no-check-certificate https://ftp.gnu.org/gnu/gmp/gmp-6.3.0.tar.xz && tar xJf gmp-6.3.0.tar.xz
    wget --no-check-certificate https://ftp.gnu.org/gnu/mpc/mpc-1.3.1.tar.gz && tar xzf mpc-1.3.1.tar.gz
    wget --no-check-certificate https://libisl.sourceforge.io//isl-0.22.1.tar.bz2 && tar xjf isl-0.22.1.tar.bz2
    wget --no-check-certificate https://github.com/periscop/cloog/releases/download/cloog-0.21.1/cloog-0.21.1.tar.gz && tar xzf cloog-0.21.1.tar.gz
    wget --no-check-certificate https://github.com/periscop/openscop/releases/download/0.9.7/osl-0.9.7.tar.gz && tar xzf osl-0.9.7.tar.gz
    wget --no-check-certificate https://ubuntu.mirror.root.lu/ubuntu/ubuntu/pool/universe/g/gcc-11/gcc-11_11.5.0-1ubuntu1~24.04.debian.tar.xz && tar xvJf gcc-11_11.5.0-1ubuntu1~24.04.debian.tar.xz
    #wget --no-check-certificate http://archive.ubuntu.com/ubuntu/pool/main/b/binutils/binutils_2.38-4ubuntu2.6.debian.tar.xz && tar xJf binutils_2.38-4ubuntu2.6.debian.tar.xz -C /tmp/
    wget --no-check-certificate http://archive.ubuntu.com/ubuntu/pool/main/b/binutils/binutils_2.38-4ubuntu2.7.debian.tar.xz && tar xJf binutils_2.38-4ubuntu2.7.debian.tar.xz -C /tmp/

    # Get patches from Alex's repository for mingw-w64-gcc and mingw-w64-binutils
    git clone -n --depth=1 --filter=tree:0  https://github.com/Alexpux/MINGW-packages.git
    cd MINGW-packages/
    git sparse-checkout set --no-cone mingw-w64-gcc mingw-w64-binutils
    git checkout
    cd ..

    # gcc: Additional patches from Gentoo, refreshed on December 2024
    git clone https://github.com/gentoo/gcc-patches 
    cd gcc-patches && git checkout 814ca13216462c81498ecbefb2a25fe54b27c1bc #a0a1379a3be2a614058045a6bf24eb67d6007044 #6ea4cfc078ef8e37f206e79fc8940fa109237cd1
    cd ..

    # gcc: Additional patches, added on Monday 06-Jan-2025
    git clone https://github.com/bitsavers/mingw-builds.git 
    cd mingw-builds && git checkout 99d57aede9ca7af5f6010889b79dc92c83471dc2
    cd .. 

    # Add pkgconf to the toolchain
    wget https://github.com/pkgconf/pkgconf/archive/refs/tags/pkgconf-2.3.0.tar.gz && tar xzf pkgconf-2.3.0.tar.gz && \
        cd pkgconf-pkgconf-2.3.0/ && ./autogen.sh && cd .. && mv pkgconf-pkgconf-2.3.0 pkgconf-2.3.0


    if [ ! -e "$GCC_SRCDIR" ]; then
        tar -xf "$GCC_SRCTARBALL"

        ln -s ../$ISL_SRCDIR $GCC_SRCDIR/isl
        ln -s ../mpfr-4.2.1 $GCC_SRCDIR/mpfr
        ln -s ../gmp-6.3.0 $GCC_SRCDIR/gmp
        ln -s ../mpc-1.3.1 $GCC_SRCDIR/mpc
        ln -s ../cloog-0.21.1 $GCC_SRCDIR/cloog
        ln -s ../osl-0.9.7 $GCC_SRCDIR/osl

        ln -s ../isl-0.22.1 $BINUTILS_SRCDIR/isl # Point to this isl version for binutils, works almost great
#        ln -s ../$ISL_SRCDIR $BINUTILS_SRCDIR/isl 
        ln -s ../mpfr-4.2.1 $BINUTILS_SRCDIR/mpfr
        ln -s ../gmp-6.3.0 $BINUTILS_SRCDIR/gmp
        ln -s ../mpc-1.3.1 $BINUTILS_SRCDIR/mpc
        ln -s ../cloog-0.21.1 $BINUTILS_SRCDIR/cloog
        ln -s ../osl-0.9.7 $BINUTILS_SRCDIR/osl 

        cd $GCC_SRCDIR && \
        # Applying Alex's patches to cross-compile MinGW-w64
        rm -rf intl/canonicalize.c intl/canonicalize.h intl/relocatex.c intl/relocatex.h

        # 0014-gcc-9-branch-clone_function_name_1-Retain-any-stdcall-suffix.patch => removed
        # 0012-Handle-spaces-in-path-for-default-manifest.patch => removed
        for file in   \
            0002-Relocate-libintl.patch \
            0003-Windows-Follow-Posix-dir-exists-semantics-more-close.patch \
            0004-Windows-Use-not-in-progpath-and-leave-case-as-is.patch \
            0005-Windows-Don-t-ignore-native-system-header-dir.patch \
            0006-Windows-New-feature-to-allow-overriding.patch \
            0007-Build-EXTRA_GNATTOOLS-for-Ada.patch \
            0008-Prettify-linking-no-undefined.patch \
            0010-Fix-using-large-PCH.patch \
            0011-Enable-shared-gnat-implib.patch \
            0020-libgomp-Don-t-hard-code-MS-printf-attributes.patch \
            0140-gcc-8.2.0-diagnostic-color.patch \
            0200-add-m-no-align-vector-insn-option-for-i386.patch \
            0300-override-builtin-printf-format.patch
        do
                echo "###### Applying patches ../MINGW-packages/mingw-w64-gcc/${file} ###############"
                patch -Nbp1 -i ../MINGW-packages/mingw-w64-gcc/${file}
        done


        # Applying additional patches  -- Thanks Gentoo!!!
        #for file in $(ls ../gcc-patches/11.4.0/gentoo/*patch|sort|grep -vE '(06|01|77)_');
        #for file in $(ls ../gcc-patches/11.5.0/gentoo/*patch|sort|grep -vE '(01)')
        for file in $(ls ../gcc-patches/11.5.0/gentoo/*patch|sort)
        # Removed 06_all_ia64_note.GNU-stack.patch...could not be applied
        # Removed 01_all_default-fortify-source.patch...it breaks vkd3d-proton at run-time
        do 
                echo "###### Applying patches ${file} ###############"
                patch -Np1 -i $file;
        done
        
        [ "$(basename ${PWD})" == "gcc-11.4.1-20230605" ] && { patch -Np1 -i ../gcc-patches/11.4.0/gentoo/77_all_all_PR112823_13_libiberty-warning.patch; }

        for file in $(ls ../debian/patches/{gcc,cross,libffi,libstd,pr,libg,a,gdc}*|sort|grep -vE '(gcc-as-needed|cross-install|gcc-ice-apport|libstdc\+\+-nothumb-check|ibstdc++-nothumb-check|distro|pr39491|pr67899|pr81829|ada|gdc-multiarch)')
        do
                echo "###### Applying patches ${file} ###############"
                patch -Np2 -i $file;             
        done

        patch -Np2 -i ../debian/patches/disable-gdc-tests.diff 
        #patch -Np2 -i ../debian/patches/ignore-pie-specs-when-not-enabled.diff # <=============
        patch -Np2 -i ../debian/patches/g++-multiarch-incdir.diff 
        patch -Np2 -i ../debian/patches/libitm-no-fortify-source.diff 
        
        # Added
        patch -Np2 -i ../debian/patches/rename-info-files.diff
        patch -Np2 -i ../debian/patches/skip-bootstrap-multilib.diff 
        patch -Np2 -i ../debian/patches/sparc64-biarch-long-double-128.diff
        patch -Np2 -i ../debian/patches/sparc64-v8plus-default.diff 
        patch -Np2 -i ../debian/patches/sys-auxv-header.diff 
        patch -Np2 -i ../debian/patches/cuda-float128.diff 
        patch -Np2 -i ../debian/patches/gm2.diff
        # Added
        patch -Np2 -i ../debian/patches/canonical-cpppath.diff
        patch -Np2 -i ../debian/patches/config-ml.diff
        # Added
        patch -Np2 -i ../debian/patches/llvm-as.diff
        patch -Np2 -i ../debian/patches/t-libunwind-elf-Wl-z-defs.diff
        patch -Np2 -i ../debian/patches/format-diag.diff
        patch -Np2 -i ../debian/patches/gcc-distro-specs-ubuntu-doc.diff
        patch -Np2 -i ../debian/patches/gcc-distro-specs.diff
        sed -i "s:#include \"distro-defaults\.h\":#ifndef ACCEL_COMPILER\n#endif:g" gcc/gcc.c gcc/cp/lang-specs.h gcc/objc/lang-specs.h gcc/objcp/lang-specs.h gcc/c-family/c-cppbuiltin.c

        wget "https://raw.githubusercontent.com/niXman/mingw-builds/develop/patches/gcc/gcc-11-replace-abort-with-fancy_abort.patch" -O -|patch -Np1

        # Additional patches
        wget "https://raw.githubusercontent.com/msys2/MINGW-packages/master/mingw-w64-gcc/0021-PR14940-Allow-a-PCH-to-be-mapped-to-a-different-addr.patch" -O 0021-PR14940-Allow-a-PCH-to-be-mapped-to-a-different-addr.patch ;
        sed -i "s:\.cc:\.c:g" 0021-PR14940-Allow-a-PCH-to-be-mapped-to-a-different-addr.patch;
        patch -Np1 -i 0021-PR14940-Allow-a-PCH-to-be-mapped-to-a-different-addr.patch;
        rm -vf 0021-PR14940-Allow-a-PCH-to-be-mapped-to-a-different-addr.patch;

         wget "https://raw.githubusercontent.com/msys2/MINGW-packages/5e812a2f380196140cef890d317e0547a6ba9617/mingw-w64-gcc/0001-missing-__thiscall-attribute-on-builtin-declaration-of-__cxa_thread_atexit.patch" -O 0001-missing-__thiscall-attribute-on-builtin-declaration-of-__cxa_thread_atexit.patch;
         sed -i "s:\.cc:\.c:g;s:^+\#undef:\#undef:g;s:^\+\#define:\#define:g" 0001-missing-__thiscall-attribute-on-builtin-declaration-of-__cxa_thread_atexit.patch;
        head -n140 0001-missing-__thiscall-attribute-on-builtin-declaration-of-__cxa_thread_atexit.patch | patch -Np1 -F 9
        tail -n 170 0001-missing-__thiscall-attribute-on-builtin-declaration-of-__cxa_thread_atexit.patch |patch -Np1
        rm -vf 0001-missing-__thiscall-attribute-on-builtin-declaration-of-__cxa_thread_atexit.patch

        wget "https://raw.githubusercontent.com/msys2/MINGW-packages/a60ce782537921a1c1fb43f427c4c6ad064e0fa1/mingw-w64-gcc/1001-libgomp-use-_aligned_free-in-gomp_aligned_free-if-ne.patch" -O - |sed -e "s:\.cc:\.c:g" | patch -Np1

        wget "https://raw.githubusercontent.com/niXman/mingw-builds/refs/heads/develop/patches/gcc/gcc-11.5.0-ktietz-libgomp.patch" -O -|patch -Np1
        wget "https://raw.githubusercontent.com/niXman/mingw-builds/refs/heads/develop/patches/gcc/gcc-13-mcf-sjlj-avoid-infinite-recursion.patch" -O -|patch -Np1

        patch -Np2 -i ../debian/patches/gcc-as-needed.diff 
        patch -Np2 -i ../debian/patches/gcc-as-needed-gold.diff
        patch -Np2 -i ../debian/patches/kfreebsd-decimal-float.diff
        patch -Np2 -i ../debian/patches/testsuite-glibc-warnings.diff 
        patch -Np2 -i ../debian/patches/testsuite-hardening-format.diff 
        patch -Np2 -i ../debian/patches/testsuite-hardening-printf-types.diff
        patch -Np2 -i ../debian/patches/testsuite-hardening-updates.diff 

	    # Added on 06-Jan-20025
	    #patch -Np0 -i ../mingw-builds/patches/gcc/gcc-5.1-iconv.patch
	    patch -Np1 -i ../mingw-builds/patches/gcc/gcc-4.8-libstdc++export.patch
	    patch -Np1 -i ../mingw-builds/patches/gcc/gcc-5.1.0-make-xmmintrin-header-cplusplus-compatible.patch
	    patch -Np1 -i ../mingw-builds/patches/gcc/gcc-5-dwarf-regression.patch
	    patch -Np1 -i ../mingw-builds/patches/gcc/gcc-libgomp-ftime64.patch
	    patch -Np1 -i ../mingw-builds/patches/gcc/gcc-10-libgcc-ldflags.patch

        cd ..

        cd $BINUTILS_SRCDIR 

        # Patch from Linux From Scratch (source https://www.linuxfromscratch.org/lfs/view/11.1-systemd/chapter08/binutils.html)
        #wget https://www.linuxfromscratch.org/patches/lfs/11.1/binutils-2.38-lto_fix-1.patch # Already applied now
        sed -e '/R_386_TLS_LE /i \   || (TYPE) == R_386_TLS_IE \\' -i ./bfd/elfxx-x86.h

        # Applying Alex's patches to cross-compile MinGW-w64 
        # removing  0001-Revert-check-thin-archive-element-file-size.patch from the original list => That one was already applied
        for file in \
            0002-check-for-unusual-file-harder.patch \
            0010-bfd-Increase-_bfd_coff_max_nscns-to-65279.patch \
            0110-binutils-mingw-gnu-print.patch \
            2001-ld-option-to-move-default-bases-under-4GB.patch
        do
                patch -Nbp1 -i ../MINGW-packages/mingw-w64-binutils/${file}
        done

        patch -R -p1 -i ../MINGW-packages/mingw-w64-binutils/2003-Restore-old-behaviour-of-windres-so-that-options-con.patch
        patch -p2 -i ../MINGW-packages/mingw-w64-binutils/reproducible-import-libraries.patch
        patch -p2 -i ../MINGW-packages/mingw-w64-binutils//specify-timestamp.patch

        for file in $(ls /tmp/debian/patches/*patch|grep -vE '(014|127|128|158)_'|sort)
        do
                echo "###### Applying patches ${file} ###############";
                patch -Nbp1 -i ${file};
        done

        wget "https://sourceware.org/git/?p=binutils-gdb.git;a=patch;h=b7eab2a9d4f4e92692daf14b09fc95ca11b72e30" -O -|patch -Np1
        wget "https://raw.githubusercontent.com/niXman/mingw-builds/develop/patches/binutils/0022-libiberty-missing-typedef.patch" -O -|patch -Np1

        # Added on Saturday 25-May-2024
        wget "https://raw.githubusercontent.com/msys2/MINGW-packages/master/mingw-w64-binutils/2003-Restore-old-behaviour-of-windres-so-that-options-con.patch" -O - |patch -Np1

        # Added on 07-Jan-2025
        patch -Np1 -i ../mingw-builds/patches/binutils/0008-fix-libiberty-makefile.mingw.patch
        patch -Np1 -i ../mingw-builds/patches/binutils/0009-fix-libiberty-configure.mingw.patch

        # Added on 03-Mar-2025
        patch -Np1 -i /tmp/debian/patches/164_ld_doc_remove_xref.diff 
        patch -Np1 -i /tmp/debian/patches/aarch64-libpath.diff 
        patch -Np1 -i /tmp/debian/patches/branch-version.diff 
        patch -Np1 -i /tmp/debian/patches/gold-mips.diff 
        patch -Np1 -i /tmp/debian/patches/gold-no-keep-files-mapped.diff
        patch -Np1 -i /tmp/debian/patches/gprof-build.diff 
        patch -Np1 -i /tmp/debian/patches/lto-wrapper-warnings.diff 
        patch -Np1 -i /tmp/debian/patches/mips64-default-n64.diff 
        patch -Np1 -i /tmp/debian/patches/mips-hack.diff

        # Added on 07-Mar-2025
        patch -Np1 -i /tmp/debian/patches/161_gold_dummy_zoption.diff
        patch -Np1 -i /tmp/debian/patches/gold-mips.diff

        # Added on Saturday 29-Mar-2025
        wget https://raw.githubusercontent.com/nxp-imx/meta-nxp-desktop/refs/heads/lf-6.1.55-2.2.0-mickledore/recipes-devtools/binutils/binutils/0001-binutils-crosssdk-Generate-relocatable-SDKs.patch -O - | patch -Np1
#        wget  https://raw.githubusercontent.com/nxp-imx/meta-nxp-desktop/refs/heads/lf-6.1.55-2.2.0-mickledore/recipes-devtools/binutils/binutils/0002-binutils-cross-Do-not-generate-linker-script-directo.patch -O - | patch -Np1
#        wget  https://raw.githubusercontent.com/nxp-imx/meta-nxp-desktop/refs/heads/lf-6.1.55-2.2.0-mickledore/recipes-devtools/binutils/binutils/0004-Point-scripts-location-to-libdir.patch -O - | patch -Np1
        wget https://raw.githubusercontent.com/nxp-imx/meta-nxp-desktop/refs/heads/lf-6.1.55-2.2.0-mickledore/recipes-devtools/binutils/binutils/0005-Only-generate-an-RPATH-entry-if-LD_RUN_PATH-is-not-e.patch -O - | patch -Np1
        wget https://raw.githubusercontent.com/nxp-imx/meta-nxp-desktop/refs/heads/lf-6.1.55-2.2.0-mickledore/recipes-devtools/binutils/binutils/0006-don-t-let-the-distro-compiler-point-to-the-wrong-ins.patch -O - | patch -Np1
        wget https://raw.githubusercontent.com/nxp-imx/meta-nxp-desktop/refs/heads/lf-6.1.55-2.2.0-mickledore/recipes-devtools/binutils/binutils/0007-warn-for-uses-of-system-directories-when-cross-linki.patch -O - | patch -Np1
        wget https://raw.githubusercontent.com/nxp-imx/meta-nxp-desktop/refs/heads/lf-6.1.55-2.2.0-mickledore/recipes-devtools/binutils/binutils/0008-fix-the-incorrect-assembling-for-ppc-wait-mnemonic.patch -O - | patch -Np1
        wget https://raw.githubusercontent.com/nxp-imx/meta-nxp-desktop/refs/heads/lf-6.1.55-2.2.0-mickledore/recipes-devtools/binutils/binutils/0009-Use-libtool-2.4.patch -O - | patch -Np1
        wget https://raw.githubusercontent.com/nxp-imx/meta-nxp-desktop/refs/heads/lf-6.1.55-2.2.0-mickledore/recipes-devtools/binutils/binutils/0010-Fix-rpath-in-libtool-when-sysroot-is-enabled.patch -O - | patch -Np1
        wget https://raw.githubusercontent.com/nxp-imx/meta-nxp-desktop/refs/heads/lf-6.1.55-2.2.0-mickledore/recipes-devtools/binutils/binutils/0011-sync-with-OE-libtool-changes.patch -O - | patch -Np1
        wget https://raw.githubusercontent.com/nxp-imx/meta-nxp-desktop/refs/heads/lf-6.1.55-2.2.0-mickledore/recipes-devtools/binutils/binutils/0012-Check-for-clang-before-checking-gcc-version.patch -O - | patch -Np1
        wget https://raw.githubusercontent.com/nxp-imx/meta-nxp-desktop/refs/heads/lf-6.1.55-2.2.0-mickledore/recipes-devtools/binutils/binutils/0013-Avoid-as-info-race-condition.patch -O - | patch -Np1

        cd ..

    fi

    if [ ! -e "$MINGW_W64_SRCDIR" ]; then
        git clone -b ${MINGW_W64_BRANCH} --recurse-submodules ${MINGW_W64_GITURL} ${MINGW_W64_SRCDIR} 
        cd $MINGW_W64_SRCDIR 
        git checkout ${MINGW_W64_GITVER}
        git submodule update --recursive 
        cd -

        # Re-conf subfolder so it could be built
        cd $MINGW_W64_SRCDIR/mingw-w64-libraries/pseh && autoreconf -fiv && rm -rf autom4te.cache && cd -

        # Additional patches taken from sourceforge: sometimes repo from github is behind/has to catch up/not sync'
        #cd $MINGW_W64_SRCDIR/ 
        #ls  /root/mingw-w64-sf/*.patch | sort | xargs -i patch -Np1 -i {}
        #cd -

        #cd $MINGW_W64_SRCDIR/mingw-w64-tools/widl
        #autoreconf -fiv
        #./wine-import.sh /opt/Sources/Build_Wine/wine-git-2025-03-25 /opt/Sources/Build_Wine/wine-git-2025-03-25
        #rm -rf autom4te.cache
        #cd -

        #cd $MINGW_W64_SRCDIR/mingw-w64-headers
        #./wine-import.sh /opt/Sources/Build_Wine/wine-git-2025-03-25 
        #./configure --with-widl --host=i686-w64-mingw32 && make && make clean distclean
        #cd -        
        
    fi
}

function build_arch {
    BUILD_ARCH=$(gcc -dumpmachine) #machine which is building the compiler
    HOST_ARCH=$1 #machine which will run the compiler
    WIN32_TARGET_ARCH=$2 #machine which we are building for
    NEWPATH=$DST_DIR/bin:$PATH

    if [ ${WIN32_TARGET_ARCH} == "i686-w64-mingw32" ];then
        export CFLAGS="${CFLAGS32}"
    fi
    
    if [ ${WIN32_TARGET_ARCH} == "x86_64-w64-mingw32" ];then
        export CFLAGS="${CFLAGS64}"
    fi   

    export CXXFLAGS="${CFLAGS} -std=c++11 "
    export CPPFLAGS="${CFLAGS}"

    mkdir -p build-$WIN32_TARGET_ARCH/
    pushd build-$WIN32_TARGET_ARCH/

        mkdir -p binutils/
        pushd binutils/
            if [ ! -e Makefile ]; then
                    ../../$BINUTILS_SRCDIR/configure \
                    --prefix=$DST_DIR/ \
                    --build=$BUILD_ARCH \
                    --host=$HOST_ARCH \
                    --target=$WIN32_TARGET_ARCH \
                    --enable-lto \
                    --enable-deterministic-archives \
                    --disable-multilib \
                    --disable-nls \
                    --disable-werror \
                    --with-pkgversion="Red-Rose-MinGW-w64-Posix-Ucrt-${MINGW_W64_PKGVER}" \
                    --with-bugurl="http://techer.pascal.free.fr/Red-Rose_MinGW-w64-Toolchain/" \
                    $BINUTILS_EXTRA_CONFIGURE
            fi
            make $JOBS configure-host
            make $JOBS LDFLAGS=-all-static
            make $JOBS install-strip
        popd

        # For below -with-default-win32-winnt, please refer to this page: https://learn.microsoft.com/fr-fr/cpp/porting/modifying-winver-and-win32-winnt?view=msvc-170

        mkdir -p mingw-w64-headers/
        pushd mingw-w64-headers/
            if [ ! -e Makefile ]; then
                PATH=$NEWPATH:$PATH ../../$MINGW_W64_SRCDIR/mingw-w64-headers/configure \
                    --prefix=$DST_DIR/$WIN32_TARGET_ARCH/ \
                    --host=$WIN32_TARGET_ARCH \
                    --enable-sdk=all \
                    --enable-secure-api \
                    --enable-idl \
                    --enable-crt \
                    --with-default-msvcrt=ucrt \
                    --with-default-win32-winnt="0xa00" \
                    $MINGW_W64_HEADERS_EXTRA_CONFIGURE
            fi
            PATH=$NEWPATH:$PATH make $JOBS install
        popd

        export lt_cv_deplibs_check_method='pass_all'

        mkdir -p gcc/
        pushd gcc/
            if [ ! -e Makefile ]; then
                #arguments mostly taken from Arch AUR mingw-w64-gcc PKGBUILD,
                #except "--disable-dw2-exceptions" swapped for "--disable-sjlj-exceptions --with-dwarf2"
                #for performance reasons on 32-bit
                LDFLAGS=-static PATH=$NEWPATH:$PATH ../../$GCC_SRCDIR/configure \
                    --prefix=$DST_DIR/ \
                    --build=$BUILD_ARCH \
                    --host=$HOST_ARCH \
                    --target=$WIN32_TARGET_ARCH \
                    --with-pkgversion="Red-Rose-MinGW-w64-Posix-Ucrt-${MINGW_W64_PKGVER}" \
                    --with-bugurl="http://techer.pascal.free.fr/Red-Rose_MinGW-w64-Toolchain/" \
                    $GCC_EXTRA_CONFIGURE
            fi
            PATH=$NEWPATH make $JOBS all-gcc
            PATH=$NEWPATH make $JOBS install-strip-gcc
        popd

        mkdir -p mingw-w64-crt/
        pushd mingw-w64-crt/
            if [ ! -e Makefile ]; then
                PATH=$NEWPATH ../../$MINGW_W64_SRCDIR/mingw-w64-crt/configure \
                    --prefix=$DST_DIR/$WIN32_TARGET_ARCH/ \
                    --host=$WIN32_TARGET_ARCH \
                    --enable-wildcard \
                    --enable-private-exports \
                    --enable-delay-import-libs \
                    --enable-experimental=all,dfp,printf128,registeredprintf,softmath \
                    --with-default-msvcrt=ucrt \
                    --enable-tests-unicode \
                    $MINGW_W64_CRT_EXTRA_CONFIGURE
            fi
            PATH=$NEWPATH make $JOBS
            PATH=$NEWPATH make $JOBS install
        popd

        mkdir -p mingw-w64-winpthreads/
        pushd mingw-w64-winpthreads/
            if [ ! -e Makefile ]; then
                PATH=$NEWPATH ../../$MINGW_W64_SRCDIR/mingw-w64-libraries/winpthreads/configure \
                    --prefix=$DST_DIR/$WIN32_TARGET_ARCH/ \
                    --host=$WIN32_TARGET_ARCH \
                    --enable-static \
                    --enable-shared \
                    $MINGW_W64_WINPTHREADS_EXTRA_CONFIGURE
            fi
            PATH=$NEWPATH make $JOBS
            PATH=$NEWPATH make $JOBS install
        popd

        pushd gcc/
            #next step requires libgcc in default library location, but
            #"canadian" build doesn't handle that(?), so install it explicitly
            PATH=$NEWPATH make configure-target-libgcc
            PATH=$NEWPATH make -C $WIN32_TARGET_ARCH/libgcc $JOBS
            PATH=$NEWPATH make -C $WIN32_TARGET_ARCH/libgcc $JOBS install-strip

            #install libstdc++ and other stuff
            PATH=$NEWPATH make $JOBS
            PATH=$NEWPATH make $JOBS install-strip

            #libstdc++ requires that libstdc++ is installed in order to find gettimeofday(???)
            #so, rebuild libstdc++ after installing it above
            PATH=$NEWPATH make $JOBS -C $WIN32_TARGET_ARCH/libstdc++-v3/ distclean
            PATH=$NEWPATH make $JOBS 
            PATH=$NEWPATH make $JOBS install-strip
        popd

        for library in libmangle winstorecompat
        do
            mkdir -p mingw-w64-${library}/
            pushd mingw-w64-${library}/
                if [ ! -e Makefile ]; then
                    PATH=$NEWPATH ../../$MINGW_W64_SRCDIR/mingw-w64-libraries/${library}/configure \
                        --prefix=$DST_DIR/$WIN32_TARGET_ARCH/ \
                        --host=$WIN32_TARGET_ARCH \
                        --enable-static \
                        --enable-shared \
                        --bindir=$DST_DIR/bin --program-prefix="${WIN32_TARGET_ARCH}-" \
                        $MINGW_W64_WINPTHREADS_EXTRA_CONFIGURE
                fi
                PATH=$NEWPATH make $JOBS
                PATH=$NEWPATH make $JOBS install
            popd
        done

        #pseh => Only x86 32-bit Win32 host variants are supported
        if [ ${WIN32_TARGET_ARCH} == "i686-w64-mingw32" ];then 
            library="pseh"
            mkdir -p mingw-w64-${library}/
            pushd mingw-w64-${library}/
                if [ ! -e Makefile ]; then
                    PATH=$NEWPATH ../../$MINGW_W64_SRCDIR/mingw-w64-libraries/${library}/configure \
                        --prefix=$DST_DIR/$WIN32_TARGET_ARCH/ \
                        --host=$WIN32_TARGET_ARCH \
                        --enable-static \
                        --enable-shared \
                        --bindir=$DST_DIR/bin --program-prefix="${WIN32_TARGET_ARCH}-" \
                        $MINGW_W64_WINPTHREADS_EXTRA_CONFIGURE
                fi
                PATH=$NEWPATH make $JOBS LDFLAGS=--static
                PATH=$NEWPATH make $JOBS install
            popd            
        fi

        for tools in gendef  genidl  genpeimg  widl
        do
            mkdir -p mingw-w64-tools/${tools}
            pushd mingw-w64-tools/${tools}/
                if [ ! -e Makefile ]; then
                    PATH=$NEWPATH ../../../$MINGW_W64_SRCDIR/mingw-w64-tools/${tools}/configure \
                        --prefix=$DST_DIR/$WIN32_TARGET_ARCH/ \
                        --target=$WIN32_TARGET_ARCH \
                        --bindir=$DST_DIR/bin --program-prefix="${WIN32_TARGET_ARCH}-"
                fi
                PATH=$NEWPATH make $JOBS LDFLAGS=--static
                PATH=$NEWPATH make $JOBS install
            popd
        done

        mkdir -p mingw-w64-pkgconf-config
        pushd mingw-w64-pkgconf-config
        PATH=$NEWPATH LDFLAGS=--static ../../pkgconf-2.3.0/configure \
            --prefix=$DST_DIR/$WIN32_TARGET_ARCH  \
            --target=$WIN32_TARGET_ARCH \
            --bindir=$DST_DIR/bin \
            --with-pkg-config-dir=$DST_DIR/$WIN32_TARGET_ARCH/lib/pkgconfig \
            --enable-static --disable-shared --with-system-includedir=$DST_DIR/$WIN32_TARGET_ARCH/include \
            --with-system-libdir=$DST_DIR/$WIN32_TARGET_ARCH/lib \
            --program-prefix="${WIN32_TARGET_ARCH}-" --program-suffix="-config"
         PATH=$NEWPATH make $JOBS LDFLAGS=--static
         PATH=$NEWPATH make $JOBS install && { cp $DST_DIR/bin/${WIN32_TARGET_ARCH}-pkgconf-config  $DST_DIR/bin/${WIN32_TARGET_ARCH}-pkg-config; }
         popd

    popd

    # Free up / Clean up 
    ${WIN32_TARGET_ARCH}-strip ${DST_DIR}/${WIN32_TARGET_ARCH}/lib/*.dll
    strip ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-*
    strip ${DST_DIR}/lib/gcc/${WIN32_TARGET_ARCH}/11.5.0/{cc1*,collect2,lto*}
    mv ${DST_DIR}/${WIN32_TARGET_ARCH}/lib/*.dll ${DST_DIR}/${WIN32_TARGET_ARCH}/bin/
    #rm -rf ${DST_DIR}/share/*

cat << _EOF_ > ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-configure

export CFLAGS="${CFLAGS}"
export CXXFLAGS="${CFLAGS} -std=c++11 "
export CPPFLAGS="${CFLAGS}"
export LDFLAGS=" -Wl,-O1,--sort-common,--as-needed -Wl,--file-alignment,4096 -static-libgcc  -static-libstdc++ -fasynchronous-unwind-tables "

export CC="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-gcc"
export CXX="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-g++"
export CPP="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-cpp"

export LD="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-ld"
export NM="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-nm"
export STRIP="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-strip"
export AR="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-ar"
export RANLIB="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-ranlib"
export AS="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-as"
export DLLTOOL="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-dlltool"
export OBJDUMP="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-objdump"
export DLLWRAP="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-dllwrap"

export RESCOMP="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-windres"
export WINDRES="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-windres"
export RC="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-windres"

export PKG_CONFIG="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-pkgconf-config"
export PKG_CONFIG_LIBDIR=${DST_DIR}/bin/${WIN32_TARGET_ARCH}/lib/pkgconfig


PATH="${DST_DIR}/bin/:\${PATH}" ../configure \
  --build=${BUILD_ARCH} --host=${WIN32_TARGET_ARCH} --target=${WIN32_TARGET_ARCH} \
  --prefix=${DST_DIR}/${WIN32_TARGET_ARCH} --libdir=${DST_DIR}/${WIN32_TARGET_ARCH}/lib --includedir=${DST_DIR}/${WIN32_TARGET_ARCH}/include \
  --enable-shared --enable-static "\$@"
_EOF_

chmod 777 ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-configure

[ ! -d "${DST_DIR}/share/mingw" ] && { mkdir -pv "${DST_DIR}/share/mingw"; }

cat << _EOF_ > ${DST_DIR}/share/mingw/toolchain-${WIN32_TARGET_ARCH}.cmake
set (CMAKE_SYSTEM_NAME Windows)
set (CMAKE_SYSTEM_PROCESSOR ${WIN32_TARGET_ARCH})

# specify the cross compiler
set (CMAKE_C_COMPILER ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-gcc)
set (CMAKE_CXX_COMPILER ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-g++)
set (CMAKE_RC_COMPILER ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-windres)

# specify flags
set(CMAKE_C_FLAGS " ${CFLAGS} ")
set(CMAKE_CXX_FLAGS " ${CFLAGS} -std=c++11  ")
set(CMAKE_EXE_LINKER_FLAGS " -Wl,-O1,--sort-common,--as-needed -Wl,--file-alignment,4096 -static-libgcc  -static-libstdc++ -fasynchronous-unwind-tables ")

# where is the target environment
set (CMAKE_FIND_ROOT_PATH ${DST_DIR}/${WIN32_TARGET_ARCH})

# search for programs in the build host directories
set (CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
set (CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set (CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# set the resource compiler (RHBZ #652435)
set (CMAKE_RC_COMPILER ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-windres)
set (CMAKE_MC_COMPILER ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-windmc)

# These are needed for compiling lapack (RHBZ #753906)
set (CMAKE_Fortran_COMPILER ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-gfortran)
set (CMAKE_AR:FILEPATH ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-ar)
set (CMAKE_RANLIB:FILEPATH ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-ranlib)
_EOF_

cat << _EOF_ > ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-cmake

export _MINGW_PREFIX_="${DST_DIR}/bin/${WIN32_TARGET_ARCH}"

export CFLAGS="${CFLAGS}"
export CXXFLAGS="${CFLAGS} -std=c++11 "
export CPPFLAGS="${CFLAGS}"
export LDFLAGS=" -Wl,-O1,--sort-common,--as-needed -Wl,--file-alignment,4096 -static-libgcc  -static-libstdc++ -fasynchronous-unwind-tables "

export CC="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-gcc"
export CXX="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-g++"
export CPP="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-cpp"

export LD="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-ld"
export NM="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-nm"
export STRIP="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-strip"
export AR="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-ar"
export RANLIB="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-ranlib"
export AS="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-as"
export DLLTOOL="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-dlltool"
export OBJDUMP="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-objdump"
export DLLWRAP="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-dllwrap"

export RESCOMP="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-windres"
export WINDRES="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-windres"
export RC="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-windres"

export PKG_CONFIG="${DST_DIR}/bin/${WIN32_TARGET_ARCH}-pkgconf-config"
export PKG_CONFIG_LIBDIR=$DST_DIR/$WIN32_TARGET_ARCH/lib/pkgconfig

PATH=${DST_DIR}/bin:\$PATH cmake \
    -DCMAKE_INSTALL_PREFIX:PATH=${DST_DIR}/${WIN32_TARGET_ARCH} \
    -DCMAKE_INSTALL_LIBDIR:PATH=lib \
    -DCMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES:PATH=${DST_DIR}/${WIN32_TARGET_ARCH}/include \
    -DCMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES:PATH=${DST_DIR}/${WIN32_TARGET_ARCH}/include \
    -DCMAKE_BUILD_TYPE=None \
    -DBUILD_SHARED_LIBS:BOOL=ON \
    -DCMAKE_TOOLCHAIN_FILE=${DST_DIR}/share/mingw/toolchain-${WIN32_TARGET_ARCH}.cmake \
    -DCMAKE_CROSSCOMPILING_EMULATOR=${DST_DIR}/bin/${WIN32_TARGET_ARCH}-wine \
    "\$@"
_EOF_

cat << _EOF_ > ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-wine
WINEDEBUG=-all
WINEPREFIX=/tmp/wineprefix-tmp-${WIN32_TARGET_ARCH}
if test "${WIN32_TARGET_ARCH}" = "x86_64-w64-mingw32"
then
  export WINEARCH=win64
else
  export WINEARCH=win32
fi
wine ""\$@""
_EOF_

chmod 777 ${DST_DIR}/share/mingw/toolchain-${WIN32_TARGET_ARCH}.cmake
chmod 777 ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-cmake
chmod 777 ${DST_DIR}/bin/${WIN32_TARGET_ARCH}-wine

}

setup_src

mkdir -p $DST_DIR

BINUTILS_EXTRA_CONFIGURE=" --enable-libssp --with-static-standard-libraries=yes --enable-host-shared --enable-gold=yes --enable-ld=yes --enable-serial-host-configure --enable-serial-target-configure --enable-serial-build-configure --enable-libada --enable-libssp --enable-plugins --with-system-zlib --enable-relro --enable-threads --with-pic --disable-gdb --enable-targets=i386-efi-pe  --enable-initfini-array " \
GCC_EXTRA_CONFIGURE=" --libexecdir=${DST_DIR}/lib --enable-host-shared --enable-serial-host-configure --enable-serial-target-configure --enable-serial-build-configure --with-default-libstdcxx-abi=new --with-diagnostics-color=auto --with-dwarf2 --with-libiconv --without-cuda-driver --enable-languages=c,c++,lto --enable-shared --enable-static --enable-__cxa_atexit --enable-checking=release --enable-cloog-backend=isl --enable-fully-dynamic-string --enable-libgomp --enable-libada --enable-libatomic --enable-graphite --enable-libquadmath --enable-libquadmath-support --enable-libssp --enable-libstdcxx --enable-libstdcxx-time=yes --enable-libstdcxx-visibility --enable-libstdcxx-threads --enable-libstdcxx-filesystem-ts=yes --enable-lto --enable-pie-tools --enable-threads=posix --enable-install-libiberty --disable-libstdcxx-debug --disable-libstdcxx-pch --disable-multilib --disable-nls --disable-sjlj-exceptions --disable-werror  --with-boot-ldflags=-static-libstdc++ --with-stage1-ldflags=-static-libstdc++ --enable-gold=yes --with-as=${DST_DIR}/bin/i686-w64-mingw32-as --with-ar=${DST_DIR}/bin/i686-w64-mingw32-ar --with-ld=${DST_DIR}/bin/i686-w64-mingw32-ld --with-gnu-as=${DST_DIR}/bin/i686-w64-mingw32-as --with-gnu-ld=${DST_DIR}/bin/i686-w64-mingw32-ld --enable-large-address-aware --enable-default-pie --enable-default-ssp --enable-cet --disable-libunwind-exceptions --enable-mingw-wildcard --with-system-zlib --with-fpmath=sse --enable-linker-build-id --disable-vtable-verify --enable-libmudflap " MINGW_W64_CRT_EXTRA_CONFIGURE="--disable-lib64 --enable-lib32" build_arch x86_64-linux-gnu i686-w64-mingw32

BINUTILS_EXTRA_CONFIGURE=" --enable-libssp --with-static-standard-libraries=yes --enable-host-shared --enable-gold=yes --enable-ld=yes --enable-serial-host-configure --enable-serial-target-configure --enable-serial-build-configure --enable-libada --enable-libssp --enable-plugins --with-system-zlib --enable-relro --enable-threads --with-pic --disable-gdb --enable-64-bit-bfd --enable-targets=x86_64-pep  --enable-initfini-array " \
GCC_EXTRA_CONFIGURE=" --libexecdir=${DST_DIR}/lib --enable-host-shared --enable-serial-host-configure --enable-serial-target-configure --enable-serial-build-configure --with-default-libstdcxx-abi=new --with-diagnostics-color=auto --with-libiconv --without-cuda-driver --enable-languages=c,c++,lto --enable-shared --enable-static --enable-__cxa_atexit --enable-checking=release --enable-cloog-backend=isl --enable-fully-dynamic-string --enable-libgomp --enable-libada --enable-libatomic --enable-graphite --enable-libquadmath --enable-libquadmath-support --enable-libssp --enable-libstdcxx --enable-libstdcxx-time=yes --enable-libstdcxx-visibility --enable-libstdcxx-threads --enable-libstdcxx-filesystem-ts=yes --enable-lto --enable-pie-tools --enable-threads=posix --enable-install-libiberty --disable-libstdcxx-debug --disable-libstdcxx-pch --disable-multilib --disable-nls --disable-sjlj-exceptions --with-dwarf2 --disable-werror --with-as=${DST_DIR}/bin/x86_64-w64-mingw32-as --with-ar=${DST_DIR}/bin/x86_64-w64-mingw32-ar --with-ld=${DST_DIR}/bin/x86_64-w64-mingw32-ld --with-gnu-as=${DST_DIR}/bin/x86_64-w64-mingw32-as --with-gnu-ld=${DST_DIR}/bin/x86_64-w64-mingw32-ld --with-boot-ldflags=-static-libstdc++ --with-stage1-ldflags=-static-libstdc++ --enable-gold=yes  --enable-large-address-aware --enable-default-pie --enable-default-ssp --enable-cet --enable-cld --disable-libunwind-exceptions --enable-mingw-wildcard --with-system-zlib --with-fpmath=sse --enable-linker-build-id --disable-vtable-verify --enable-libmudflap " MINGW_W64_CRT_EXTRA_CONFIGURE="--disable-lib32 --enable-lib64" build_arch x86_64-linux-gnu x86_64-w64-mingw32

echo "Done!"
