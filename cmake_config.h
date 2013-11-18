/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
#cmakedefine ENABLE_NLS 1

/* Not specified at configure line */
#cmakedefine HAS_ASCIIDOC TRUE

/* Not specified at configure line */
#cmakedefine HAS_CSHARP TRUE

/* Not specified at configure line */
#cmakedefine HAS_DOXYGEN TRUE

/* Not specified at configure line */
#cmakedefine HAS_JAVA TRUE

/* Not specified at configure line */
#cmakedefine HAS_PERL TRUE

/* Not specified at configure line */
#cmakedefine HAS_PO4A TRUE

/* Not specified at configure line */
#cmakedefine HAS_PYTHON TRUE

/* Not specified at configure line */
#cmakedefine HAS_VALA TRUE

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
#cmakedefine HAVE_DCGETTEXT 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H 1

/* Define if the GNU gettext() function is already present or preinstalled. */
#cmakedefine HAVE_GETTEXT 1

/* Define if you have the iconv() function. */
#cmakedefine HAVE_ICONV 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the `strverscmp' function. */
#cmakedefine HAVE_STRVERSCMP 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* pacman-g2.conf location */
#define PACCONF "/etc/pacman-g2.conf"

/* Name of package */
#define PACKAGE "@CMAKE_PROJECT_NAME_LOWER@"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "frugalware-devel@frugalware.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "@CMAKE_PROJECT_NAME_LOWER@ package manager"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "@CMAKE_PROJECT_NAME_LOWER@ package manager @PACMAN_G2_VERSION@"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "pacman-g2"

/* Define to the home page for this package. */
#define PACKAGE_URL "www.frugalware.org"

/* Define to the version of this package. */
#define PACKAGE_VERSION "@PACMAN_G2_VERSION@"

/* default bytes per block */
#define PM_DEFAULT_BYTES_PER_BLOCK 10240

/* libpacman version number */
#define PM_VERSION "@PM_VERSION@"

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* Version number of package */
#define VERSION "@PACMAN_G2_VERSION@"

/* Define fakeroot protection */
#cmakedefine FAKEROOT_PROOF 1
