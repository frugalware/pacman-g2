ENV_CFLAGS=$CFLAGS

AC_PREREQ(2.59)
AC_INIT([Pacman package manager], 1.0, [bugreport@somemail.org], pacman)
AC_LANG(C)
AM_CONFIG_HEADER(config.h)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE

dnl Define here the libalpm version number
PM_MAJOR_VERSION=0
PM_MINOR_VERSION=1
PM_MICRO_VERSION=0
PM_MICRO_VERSION_SUFFIX=.5
PM_VERSION=$PM_MAJOR_VERSION.$PM_MINOR_VERSION.$PM_MICRO_VERSION$PM_MICRO_VERSION_SUFFIX
dnl Needed for libtool to create proper shared lib version
PM_VERSION_INFO=`expr $PM_MAJOR_VERSION + $PM_MINOR_VERSION`:$PM_MICRO_VERSION:$PM_MINOR_VERSION

AC_SUBST(PM_MAJOR_VERSION)
AC_SUBST(PM_MINOR_VERSION)
AC_SUBST(PM_MICRO_VERSION)
AC_SUBST(PM_VERSION)
AC_SUBST(PM_VERSION_INFO)

dnl Define here the pacman binary program version number
PACMAN_VERSION=2.9.99.5
AC_SUBST(PACMAN_VERSION)

dnl Define the default config file path for pacman.conf
PACCONF=/etc/pacman.conf
AC_DEFINE_UNQUOTED([PACCONF], ["$PACCONF"], [pacman.conf location])

dnl Put out version numbers to config.h
AC_DEFINE_UNQUOTED([PM_VERSION], ["$PM_VERSION"], [libalpm version number])
AC_DEFINE_UNQUOTED([PACMAN_VERSION], ["$PACMAN_VERSION"], [pacman version number])

dnl Configuration files
AC_CONFIG_FILES([etc/makepkg.conf] [etc/pacman.conf])

AC_PROG_CC
AC_HEADER_STDC
AC_PROG_INSTALL
AC_CHECK_FUNCS([strverscmp])
AM_PROG_LIBTOOL

dnl Help line for man2html
AC_ARG_ENABLE(man2html,
    AC_HELP_STRING([--enable-man2html], [Build html docs via man2html]),
    [wantman2html=$enableval], [wantman2html=no])

dnl Check for architecture
case "$host" in
	i686-*) AC_DEFINE_UNQUOTED([CARCH], ["i686"], [Architecture])
		AC_DEFINE_UNQUOTED([CARCHFLAGS], ["i686"], [Architecture])
		CARCH="i686"
		CARCHFLAGS="i686"
		CHOST="${host}"
		;;
	x86_64-*) AC_DEFINE_UNQUOTED([CARCH], ["x86_64"], [Architecture])
		AC_DEFINE_UNQUOTED([CARCHFLAGS], ["k8"], [Architecture])
		CARCH="x86_64"
		CARCHFLAGS="k8"
		CHOST="${host}"
		;;
	ppc-* | powerpc-*)
		AC_DEFINE_UNQUOTED([CARCH], ["ppc"], [Architecture])
		AC_DEFINE_UNQUOTED([CARCHFLAGS], ["750"], [Architecture])
		CARCH="ppc"
		CARCHFLAGS="750"
		CHOST="${host}"
		;;
	*)
		AC_MSG_ERROR([Your architecture is not supported])
		;;
esac

AC_SUBST(CARCH)
AC_SUBST(CARCHFLAGS)
AC_SUBST(CHOST)

dnl Help line for more-warnings
AC_ARG_ENABLE(more-warnings, 
    AC_HELP_STRING([--disable-more-warnings], [Minimum compiler warnings]),
    [set_more_warnings="${enableval}"], [warnings_default=yes])

dnl Help line for debug
AC_ARG_ENABLE(debug, [  --enable-debug    Enable debugging support], debug=yes)

dnl Check for man2html binary
AC_MSG_CHECKING(for support man2html)
if test x$wantman2html = xyes ; then
    AC_CHECK_PROGS([MAN2HTML], [man2html] [man2html prog])
    AM_CONDITIONAL(HAS_MAN2HTML, test $MAN2HTML)
    if test $MAN2HTML ; then
        AC_DEFINE([HAS_MAN2HTML], [TRUE], [Enabled HTML generation from man via man2html])
	AC_MSG_RESULT(yes)
    else
        AC_DEFINE([HAS_MAN2HTML], [FALSE], [Disabled HTML generation from man via man2html])
	AC_MSG_RESULT(no, man2html missing)
    fi
else
    AM_CONDITIONAL(HAS_MAN2HTML, test $MAN2HTML)
    AC_DEFINE([HAS_MAN2HTML], [FALSE], [Not specified at configure line])
    AC_MSG_RESULT(not requested by configure)
fi

dnl Enable or disable Warning FLAGS during compile
AC_MSG_CHECKING(for more compiler warnings)
if test "$GCC" = "yes" -a "$set_more_warnings" != "no"; then
    AC_MSG_RESULT(yes)
    CFLAGS="$CFLAGS -Wall -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
    -Wnested-externs -Wsign-compare"
    for option in -Wno-sign-compare; do
	SAVE_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $option"
	AC_MSG_CHECKING([whether gcc understands $option])
	AC_TRY_COMPILE([], [],
	    has_option=yes,
	    has_option=no,)
	    if test $has_option = no; then
		CFLAGS="$SAVE_CFLAGS"
	    fi
        AC_MSG_RESULT($has_option)
        unset has_option
        unset SAVE_CFLAGS
    done
    unset option
else
    AC_MSG_RESULT(no)
fi

dnl Enable or disable debug code
AC_MSG_CHECKING(for debug mode request)
if test x$debug = xyes ; then
    AM_CONDITIONAL(PACMAN_DEBUG, test x$debug = xyes)
	CFLAGS="-DPACMAN_DEBUG -Wall -W -O0 -fno-omit-frame-pointer -ggdb -g3"
    AC_MSG_RESULT(yes)
else
    AM_CONDITIONAL(PACMAN_DEBUG, test x$debug = xno)
	CFLAGS="$CFLAGS"
    AC_MSG_RESULT(no)
fi

dnl Check for bzip2
AC_CHECK_LIB([bz2], [BZ2_bzCompress], [AC_CHECK_HEADER([bzlib.h], [LIBBZ2='-lbz2'])])
if test -n "$LIBBZ2"; then
    LDFLAGS="$LDFLAGS $LIBBZ2"
else
    AC_MSG_ERROR("bzip2 devel stuff needed to compile pacman!");
fi

dnl Check for libarchive
AC_CHECK_LIB([archive], [archive_read_data], [AC_CHECK_HEADER([archive.h], [LIBARCHIVE='-larchive'])])
if test -n "$LIBARCHIVE"; then
    CFLAGS="$CFLAGS"
    LDFLAGS="$LDFLAGS $LIBARCHIVE"
else
    AC_MSG_ERROR("libarchive is needed to compile pacman!");
fi

CFLAGS="$CFLAGS"

dnl ==========================================================================

AC_OUTPUT([
lib/libalpm/Makefile
lib/libftp/Makefile
src/pacman/Makefile
src/util/Makefile
scripts/Makefile
doc/Makefile
doc/makepkg.8
doc/pacman.8
etc/Makefile
Makefile
])

echo "
pacman-$VERSION:

        prefix                   : ${prefix}
        source code location     : ${srcdir}
        compiler                 : ${CC}
        compiler flags           : ${CFLAGS}

	libalpm version          : ${PM_VERSION}
	pacman.conf location     : ${PACCONF}

	Architecture CARCH       : ${CARCH}
	Architecture used cflag  : ${CARCHFLAGS}
	Architecture CHOST       : ${CHOST}

	debug support            : ${debug}
"
