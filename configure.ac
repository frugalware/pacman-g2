ENV_CFLAGS=$CFLAGS

AC_PREREQ(2.59)
dnl Update it right before the release since $pkgver_foo are all _post_ release snapshots
AC_INIT([pacman-g2 package manager], 3.8.5, [frugalware-devel@frugalware.org], pacman-g2)
AC_CONFIG_MACRO_DIR([m4])
AC_LANG(C)
AM_CONFIG_HEADER(config.h)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([foreign])
AC_C_BIGENDIAN

dnl Host dependant flags
case "${host}" in
	*-*-cygwin*)
		ENV_CFLAGS="$ENV_CFLAGS -DCYGWIN"
		;;
esac

dnl Define here the libpacman version number
PM_MAJOR_VERSION=0
PM_MINOR_VERSION=3
PM_MICRO_VERSION=8
PM_MICRO_VERSION_SUFFIX=.5
PM_VERSION=$PM_MAJOR_VERSION.$PM_MINOR_VERSION.$PM_MICRO_VERSION$PM_MICRO_VERSION_SUFFIX
dnl Needed for libtool to create proper shared lib version
PM_VERSION_INFO=`expr $PM_MAJOR_VERSION + $PM_MINOR_VERSION`:$PM_MICRO_VERSION:$PM_MINOR_VERSION

AC_SUBST(PM_MAJOR_VERSION)
AC_SUBST(PM_MINOR_VERSION)
AC_SUBST(PM_MICRO_VERSION)
AC_SUBST(PM_VERSION)
AC_SUBST(PM_VERSION_INFO)

dnl Define the default config file path for pacman-g2.conf
PACCONF=/etc/pacman-g2.conf
AC_DEFINE_UNQUOTED([PACCONF], ["$PACCONF"], [pacman-g2.conf location])

dnl Put out version numbers to config.h
AC_DEFINE_UNQUOTED([PM_VERSION], ["$PM_VERSION"], [libpacman version number])

dnl Configuration files
AC_CONFIG_FILES([etc/makepkg.conf] [etc/pacman-g2.conf])

AC_PROG_CC
AC_HEADER_STDC
AC_PROG_INSTALL
AC_CHECK_FUNCS([strverscmp])
AM_PROG_LIBTOOL
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.13.1)
AM_CONDITIONAL(LINKSTATIC, test "$enable_static" = "yes")

dnl Humor lowers blood pressure
AC_MSG_CHECKING(your blood pressure)
AC_MSG_RESULT([a bit high, but we can proceed])

dnl Help line for po4a
AC_ARG_ENABLE(po4a,
	AC_HELP_STRING([--disable-po4a], [Do not build localized manpages via po4a]),
	[wantpo4a=$enableval], [wantpo4a=yes])

dnl Help line for asciidoc
AC_ARG_ENABLE(asciidoc,
	AC_HELP_STRING([--disable-asciidoc], [Do not build manpages via asciidoc]),
	[wantasciidoc=$enableval], [wantasciidoc=yes])

dnl Help line for doxygen
AC_ARG_ENABLE(doxygen,
	AC_HELP_STRING([--disable-doxygen], [Do not build API docs via Doxygen]),
	[wantdoxygen=$enableval], [wantdoxygen=yes])

dnl Help line for python
AC_ARG_ENABLE(python,
	AC_HELP_STRING([--disable-python], [Do not build Python bindings]),
	[wantpython=$enableval], [wantpython=yes])

dnl Help line for perl
AC_ARG_ENABLE(perl,
	AC_HELP_STRING([--disable-perl], [Do not build Perl bindings]),
	[wantperl=$enableval], [wantperl=yes])

dnl Help line for java
AC_ARG_ENABLE(java,
	AC_HELP_STRING([--disable-java], [Do not build Java bindings]),
	[wantjava=$enableval], [wantjava=yes])

dnl Help line for csharp
AC_ARG_ENABLE(csharp,
	AC_HELP_STRING([--disable-csharp], [Do not build csharp bindings]),
	[wantcsharp=$enableval], [wantcsharp=yes])

dnl Help line for vala
AC_ARG_ENABLE(vala,
	AC_HELP_STRING([--disable-vala], [Do not build vala bindings]),
	[wantvala=$enableval], [wantvala=yes])

dnl Help line for chost
AC_ARG_WITH(chost, AC_HELP_STRING([--with-chost], [Build with a specific CHOST (default: CARCH-frugalware-linux)]))

dnl Check for architecture
case "$host" in
	i686-*) CARCH="i686"
		CARCHFLAGS="-march=i686 -mtune=generic"
		;;
	x86_64-*) CARCH="x86_64"
		CARCHFLAGS="-march=x86-64 -mtune=generic"
		;;
	ia64-*) CARCH="ia64"
		CARCHFLAGS="-march=ia64 -mtune=generic"
		;;
	sparc-*) CARCH="sparc"
		CARCHFLAGS="-mcpu=v9"
		;;
	ppc-* | powerpc-*)
		CARCH="ppc"
		CARCHFLAGS="-mcpu=powerpc -mtune=powerpc"
		;;
	i386-*) CARCH="i386"
		CARCHFLAGS="-march=i386 -mtune=generic"
		;;
	arm*)
		CARCH="arm"
		CARCHFLAGS="-march=armv5te"
		;;
	*)
		AC_MSG_ERROR([Your architecture is not supported])
		;;
esac

dnl Check for chost
if test "z$with_chost" != "z"; then
	CHOST="$with_chost"
else
	CHOST="${CARCH}-frugalware-linux"
fi

AC_SUBST(CARCH)
AC_SUBST(CARCHFLAGS)
AC_SUBST(CHOST)

dnl Default package file extension
AC_ARG_WITH([package-extension],
	[AS_HELP_STRING([--with-package-extension=<extension>], [define the default package extension (default: fpm)])],
	[PKG_EXT="$withval"], [PKG_EXT="fpm"])
AC_SUBST(PKG_EXT)

dnl Default package file compression
AC_ARG_WITH([package-compression],
	[AS_HELP_STRING([--with-package-compression=<compression>], [define the default package compression (default: xz)])],
	[PKG_COMP="$withval"], [PKG_COMP="xz"])
AC_SUBST(PKG_COMP)

dnl Build with Werror
AC_ARG_ENABLE(werror,
	AC_HELP_STRING([--enable-werror], [Enable building with Werror]),
	[werror=$enableval], [werror=no])

dnl Build with strict :) (bunch of flags :P)
AC_ARG_ENABLE(strict,
	AC_HELP_STRING([--enable-strict], [Enable building with 'Strict' flags]),
	[strict=$enableval], [strict=no])

dnl Help line for debug
AC_ARG_ENABLE(debug,
	AC_HELP_STRING([--enable-debug], [Enable debugging support]),
	[debug=$enableval], [debug=yes])

dnl Help line for fakeroot
AC_ARG_ENABLE(fakeroot,
	AC_HELP_STRING([--enable-fakeroot], [Enable fakeroot proof support]),
	[fakeroot=$enableval], [fakeroot=no])

dnl Check for pkg-config binary
AC_PATH_PROG(PKG_CONFIG, pkg-config)
if test ! -x "$PKG_CONFIG" ; then
   AC_MSG_ERROR([
*** You need pkg-config. Get it from
*** <http://pkg-config.freedesktop.org/>.])
fi
AC_SUBST(PKG_CONFIG)

dnl Check for po4a binary
AC_MSG_CHECKING(for support po4a)
if test x$wantpo4a = xyes ; then
	AC_CHECK_PROGS([PO4A], [po4a])
	AM_CONDITIONAL(HAS_PO4A, test $PO4A)
	if test $PO4A ; then
		AC_DEFINE([HAS_PO4A], [TRUE], [Enabled PO4A (localization support for manpages)])
	AC_MSG_RESULT(yes)
	POSTATUS="yes, path: `which $PO4A`"
	else
		AC_DEFINE([HAS_PO4A], [FALSE], [Disabled PO4A (localization support for manpages)])
	AC_MSG_RESULT(no, po4a binary missing)
	POSTATUS="no, missing po4a binary"
	fi
else
	AM_CONDITIONAL(HAS_PO4A, test $PO4A)
	AC_DEFINE([HAS_PO4A], [FALSE], [Not specified at configure line])
	AC_MSG_RESULT(not requested by configure)
	POSTATUS="no, disabled by configure"
fi

dnl Check for asciidoc
AC_MSG_CHECKING(for support asciidoc)
if test x$wantasciidoc = xyes ; then
	AC_CHECK_PROGS([ASCIIDOC], [asciidoc])
	AM_CONDITIONAL(HAS_ASCIIDOC, test $ASCIIDOC)
	if test $ASCIIDOC ; then
		AC_DEFINE([HAS_ASCIIDOC], [TRUE], [Enabled ASCIIDOC (needed to generate manpages)])
	AC_MSG_RESULT(yes)
	ASCIISTATUS="yes, path: `which $ASCIIDOC`"
	else
		AC_DEFINE([HAS_ASCIIDOC], [FALSE], [Disabled ASCIIDOC (needed to generate manpages)])
	AC_MSG_RESULT(no, asciidoc binary missing)
	ASCIISTATUS="no, missing asciidoc binary"
	fi
else
	AM_CONDITIONAL(HAS_ASCIIDOC, test $ASCIIDOC)
	AC_DEFINE([HAS_ASCIIDOC], [FALSE], [Not specified at configure line])
	AC_MSG_RESULT(not requested by configure)
	ASCIISTATUS="no, disabled by configure"
fi

dnl Check for doxygen support
AC_MSG_CHECKING(for support Doxygen)
if test x$wantdoxygen = xyes ; then
	AC_CHECK_PROGS([DOXYGEN], [doxygen])
	AM_CONDITIONAL(HAS_DOXYGEN, test $DOXYGEN)
	if test $DOXYGEN ; then
		AC_DEFINE([HAS_DOXYGEN], [TRUE], [Enabled Doxygen Support])
	AC_MSG_RESULT(yes)
	DOXYSTATUS="yes, path: `which $DOXYGEN`"
	else
		AC_DEFINE([HAS_DOXYGEN], [FALSE], [Disabled Doxygen support])
	AC_MSG_RESULT(no, doxygen missing)
	DOXYSTATUS="doxygen binary not found"
	fi
else
	AM_CONDITIONAL(HAS_DOXYGEN, test $DOXYGEN)
	AC_DEFINE([HAS_DOXYGEN], [FALSE], [Not specified at configure line])
	AC_MSG_RESULT(not requested by configure)
	DOXYSTATUS="no, disabled by configure"
fi

dnl Check for python support
AC_MSG_CHECKING(if Python bindings should be generated)
if test x$wantpython = xyes ; then
	AC_CHECK_PROGS([SWIG], [swig])
	AM_CONDITIONAL(HAS_PYTHON, test $SWIG)
	if test $SWIG ; then
		AC_DEFINE([HAS_PYTHON], [TRUE], [Enabled Python Support])
	AC_MSG_RESULT(yes)
	PYTHONSTATUS="yes, path: `which $SWIG`"
	else
		AC_DEFINE([HAS_PYTHON], [FALSE], [Disabled Python support])
	AC_MSG_RESULT(no, swig missing)
	PYTHONSTATUS="swig binary not found"
	fi
else
	AM_CONDITIONAL(HAS_PYTHON, test $PYTHON)
	AC_DEFINE([HAS_PYTHON], [FALSE], [Not specified at configure line])
	AC_MSG_RESULT(not requested by configure)
	PYTHONSTATUS="no, disabled by configure"
fi

dnl Check for perl support
AC_MSG_CHECKING(if Perl bindings should be generated)
if test x$wantperl = xyes ; then
	AC_CHECK_PROGS([SWIG], [swig])
	AM_CONDITIONAL(HAS_PERL, test $SWIG)
	if test $SWIG ; then
		AC_DEFINE([HAS_PERL], [TRUE], [Enabled Perl Support])
	AC_MSG_RESULT(yes)
	PERLSTATUS="yes, path: `which $SWIG`"
	else
		AC_DEFINE([HAS_PERL], [FALSE], [Disabled Perl support])
	AC_MSG_RESULT(no, swig missing)
	PERLSTATUS="swig binary not found"
	fi
else
	AM_CONDITIONAL(HAS_PERL, test $PERL)
	AC_DEFINE([HAS_PERL], [FALSE], [Not specified at configure line])
	AC_MSG_RESULT(not requested by configure)
	PERLSTATUS="no, disabled by configure"
fi

dnl Check for java support
AC_MSG_CHECKING(if Java bindings should be generated)
if test x$wantjava = xyes ; then
 	AC_CHECK_PROGS([SWIG], [swig])
 	AC_CHECK_PROGS([JAVAC], [javac])
 	AM_CONDITIONAL(HAS_JAVA, test $SWIG)
 	AM_CONDITIONAL(HAS_JAVA, test $JAVAC)
 	if test $JAVAC ; then
 		if test $SWIG ; then
 			AC_DEFINE([HAS_JAVA], [TRUE], [Enabled Java Support (javac=ok swig=ok)])
 			AC_MSG_RESULT(yes)
 			JAVASTATUS="yes, path: `which $JAVAC`"
 		else
 			AC_DEFINE([HAS_JAVA], [FALSE], [Java support disabled because swig not found (javac=ok swig=missing)])
 			AC_MSG_RESULT(no, swig not found and that is need for java)
 			JAVASTATUS="java compiler found, swig not found"
 		fi
 	else
 		if test $SWIG ; then
 			AC_DEFINE([HAS_JAVA], [FALSE], [Disabled Java support (javac=missing swig=ok])
 			AC_MSG_RESULT(no, javac missing, swig ok)
 			JAVASTATUS="java compiler not found, swig found"
 		else
 			AC_DEFINE([HAS_JAVA], [FALSE], [Disabled Java support (javac=missing swig=missing])
 			AC_MSG_RESULT(no, javac missing, swig missing)
 			JAVASTATUS="java compiler not found, swig not found"
 		fi
 	fi
else
	AM_CONDITIONAL(HAS_JAVA, test $JAVAC)
	AC_DEFINE([HAS_JAVA], [FALSE], [Not specified at configure line])
	AC_MSG_RESULT(not requested by configure)
	JAVASTATUS="no, disabled by configure"
fi

dnl Check for csharp support
AC_MSG_CHECKING(if csharp bindings should be generated)
if test x$wantcsharp = xyes ; then
 	AC_CHECK_PROGS([SWIG], [swig])
 	AC_CHECK_PROGS([MCS], [mcs])
 	AM_CONDITIONAL(HAS_CSHARP, test $SWIG)
 	AM_CONDITIONAL(HAS_CSHARP, test $MCS)
 	if test $MCS ; then
 		if test $SWIG ; then
 			AC_DEFINE([HAS_CSHARP], [TRUE], [Enabled csharp Support (mcs=ok swig=ok)])
 			AC_MSG_RESULT(yes)
 			CSHARPSTATUS="yes, path: `which $MCS`"
 		else
 			AC_DEFINE([HAS_CSHARP], [FALSE], [csharp support disabled because swig not found (mcs=ok swig=missing)])
 			AC_MSG_RESULT(no, swig not found and that is need for csharp)
 			CSHARPSTATUS="mcs found, swig not found"
 		fi
 	else
 		if test $SWIG ; then
 			AC_DEFINE([HAS_CSHARP], [FALSE], [Disabled csharp support (mcs=missing swig=ok])
 			AC_MSG_RESULT(no, mcs missing, swig ok)
 			CSHARPSTATUS="mcs not found, swig found"
 		else
 			AC_DEFINE([HAS_CSHARP], [FALSE], [Disabled csharp support (mcs=missing swig=missing])
 			AC_MSG_RESULT(no, mcs missing, swig missing)
 			CSHARPSTATUS="mcs not found, swig not found"
 		fi
 	fi
else
	AM_CONDITIONAL(HAS_CSHARP, test $MCS)
	AC_DEFINE([HAS_CSHARP], [FALSE], [Not specified at configure line])
	AC_MSG_RESULT(not requested by configure)
	CSHARPSTATUS="no, disabled by configure"
fi

dnl Check for vala support
AC_MSG_CHECKING(if Vala bindings should be generated)
if test x$wantvala = xyes ; then
	AC_CHECK_PROGS([VALAC], [valac])
	AM_CONDITIONAL(HAS_VALA, test $VALAC)
	if test $VALAC ; then
		AC_DEFINE([HAS_VALA], [TRUE], [Enabled Vala Support])
	AC_MSG_RESULT(yes)
	VALASTATUS="yes"
	else
		AC_DEFINE([HAS_VALA], [FALSE], [Disabled Vala support])
	AC_MSG_RESULT(no, valac missing)
	VALASTATUS="valac binary not found"
	fi
else
	AM_CONDITIONAL(HAS_VALA, test $VALAC)
	AC_DEFINE([HAS_VALA], [FALSE], [Not specified at configure line])
	AC_MSG_RESULT(not requested by configure)
	VALASTATUS="no, disabled by configure"
fi

dnl Check for bzip2
AC_CHECK_LIB([bz2], [BZ2_bzCompress], [AC_CHECK_HEADER([bzlib.h], [LIBBZ2='-lbz2'])])
if test -n "$LIBBZ2"; then
	LDFLAGS="$LDFLAGS $LIBBZ2"
else
	AC_MSG_ERROR("bzip2 library and its include files are needed to build pacman-g2!");
fi

dnl Check for xz
AC_CHECK_LIB([lzma], [lzma_stream_decoder], [AC_CHECK_HEADER([lzma.h], [LIBLZMA='-llzma'])])
if test -n "$LIBLZMA"; then
	LDFLAGS="$LDFLAGS $LIBLZMA"
else
	AC_MSG_ERROR("lzma library and its include files are needed to build pacman-g2!");
fi

dnl Check for math
AC_CHECK_LIB([m], [log10], [AC_CHECK_HEADER([math.h], [LIBM='-lm'])])
if test -n "$LIBM"; then
	LDFLAGS="$LDFLAGS $LIBM"
else
	AC_MSG_ERROR("math library not found!");
fi

dnl Check for libarchive
AC_CHECK_LIB([archive], [archive_read_data], [AC_CHECK_HEADER([archive.h], [LIBARCHIVE='-larchive'])])
if test -n "$LIBARCHIVE"; then
	CFLAGS="$CFLAGS"
	LDFLAGS="$LDFLAGS $LIBARCHIVE"
else
	AC_MSG_ERROR("libarchive is needed to compile pacman-g2!");
fi

dnl Enable or disable debug code
AC_MSG_CHECKING(for debug mode request)
if test x$debug = xyes ; then
	AM_CONDITIONAL(PACMAN_DEBUG, test x$debug = xyes)
	CFLAGS="-ggdb3 -Wall -std=c99 -DPACMAN_DEBUG"
	AC_MSG_RESULT(yes)
else
	AM_CONDITIONAL(PACMAN_DEBUG, test x$debug = xno)
	CFLAGS="$CFLAGS -std=c99"
	AC_MSG_RESULT(no)
fi

dnl Enable or disable building with Werror
AC_MSG_CHECKING(for building with Werror)
if test x$werror = xyes ; then
	CFLAGS="$CFLAGS -Werror"
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi

dnl Enable or disable building with 'strict' flags
AC_MSG_CHECKING(for building with 'strict' flags)
if test x$strict = xyes ; then
	CFLAGS="$CFLAGS -Wextra -Wall -pedantic -Wshadow \
		-Wmissing-declarations -Wpointer-arith -Wcast-align \
		-Wfloat-equal -Wstrict-prototypes -Wmissing-prototypes \
		-Wredundant-decls -Wunused-variable -Wunused-parameter \
		-Wparentheses -Wno-long-long -Wwrite-strings -Wimplicit \
		-Wnested-externs -Wno-unused -Wundef -Wpacked \
		-Wbad-function-cast -Wunused-function -Wunused-label \
		-Wunused-value -Wfloat-equal -Wchar-subscripts -Wswitch \
		-Wmissing-braces -Wsequence-point -Wunknown-pragmas \
		-Wdeclaration-after-statement -Wsign-compare"
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no)
fi

dnl Enable or disable fakeroot code
AC_MSG_CHECKING(for fakeroot proof support)
if test x$fakeroot = xyes ; then
	AC_MSG_RESULT(yes)
else
	CFLAGS="$CFLAGS -DFAKEROOT"
	AC_MSG_RESULT(no)
fi

CFLAGS="$CFLAGS"

dnl ==========================================================================

AC_OUTPUT([
lib/libpacman/Makefile
lib/libpacman/po/Makefile.in
lib/libpacman/pacman.pc
lib/libftp/Makefile
src/pacman-g2/Makefile
src/pacman-g2/po/Makefile.in
src/vercmp/Makefile
src/versort/Makefile
scripts/Makefile
doc/Makefile
etc/Makefile
etc/bash_completion.d/Makefile
etc/pacman-g2/Makefile
etc/pacman-g2/hooks/Makefile
etc/pacman-g2/repos/Makefile
etc/pacman-g2/repos/frugalware
etc/pacman-g2/repos/frugalware-current
bindings/perl/Makefile
bindings/python/Makefile
bindings/java/Makefile
bindings/csharp/Makefile
bindings/vala/Makefile
Makefile
])

echo "
pacman-g2-$VERSION:

	prefix				: ${prefix}
	source code location		: ${srcdir}
	compiler			: ${CC}
	compiler flags			: ${CFLAGS}

	libpacman version		: ${PM_VERSION}
	pacman-g2.conf location		: ${PACCONF}

	Architecture CARCH		: ${CARCH}
	Architecture used cflag		: ${CARCHFLAGS}
	Architecture CHOST		: ${CHOST}

	Default package extension	: ${PKG_EXT}
	Default package compression	: ${PKG_COMP}

	Doxygen support			: ${DOXYSTATUS}
	Manpages			: ${ASCIISTATUS}
	Manpage localization		: ${POSTATUS}
	Perl bindings			: ${PERLSTATUS}
	Python bindings			: ${PYTHONSTATUS}
	Java bindings			: ${JAVASTATUS}
	C# bindings			: ${CSHARPSTATUS}
	Vala bindings			: ${VALASTATUS}

	debug support			: ${debug}
	Werror build			: ${werror}
	Strict flags			: ${strict}
"
