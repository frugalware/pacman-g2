==========================================================================
README:

Overview and internals of the PACMAN library and the PACMAN-G2 frontend.

This document describes the state of the implementation before its CVS
import.
At this stage, the code is in pre-alpha state, but the design should not
change that much.
There's still need for some work to get the current code properly working.
The tag "TODO" was added in various places in the code, each time a point
remains unclear or is not yet implemented.

==========================================================================

The "Second Generation" project
===============================

Pacman-G2 is a fork of the not-yet-released cvs version of the complete rewrite
of pacman by Aurelien Foret (the old monolithic pacman is written by Judd
Vinet).

So it provides a library interface to create real front-ends (not just
wrappers) to this great package management tool.

Some features that pacman-g2 has over pacman: (September 2007)

1) It provides C#, Java, Perl and Python bindings for libpacman.
2) libpacman provides a configuration file parser (for pacman-g2.conf).
3) It provides support for subpackages, i.e. creating more than one binary
package using a single build script.
4) Localized manpages.
5) Stable library API. This means that we care about keeping the API
backwards-compatible. (Changes like
http://www.archlinux.org/pipermail/pacman-dev/2007-November/009886.html[this]
are
http://www.archlinux.org/pipermail/pacman-dev/2007-November/009893.html[applied]
without any problem by the pacman developers and we don't do so.)

Using patches to provide this features modify enough pacman to change its name.
Additionally the maintainers of pacman asked us to do so and they have reason.

Patches to port features or bugfixes from pacman are welcome, but in fact we
differed too much to regularly sync the common codebase between the two
projects.

Of course we would like to thank the original pacman developers for all their
effort till we worked together before the fork.

The current homepage of the fork can be found at:

http://ftp.frugalware.org/pub/other/pacman-g2/

The project uses the "pacman-g2" name instead of the "pacman" one, so that
users won't be confused.

If you are interested in developing pacman-g2, please subscribe to the
frugalware-devel@frugalware.org mailing list.

Also please note that the library shipped with pacman-g2 is called libpacman,
not libalpm. This is because libpacman is no longer meant to be specific
Archlinux, the distribution created by Judd Vinet, the original pacman author.

PACMAN library overview & internals
=================================

Here is a list of the main objects and files from the PACMAN (i.e. Package
Management) library.
This document, whilst not exhaustive, also indicates some limitations
(on purpose, or sometimes due to its poor design) of the library at the
present time.

Note: there is one special file ("pacman.h") which is the public interface
that should be distributed and installed on systems with the library.
Only structures, data and functions declared within this file are made
available to the frontend.
Lots of structures are of an opaque type and their fields are only
accessible in read-only mode, through some clearly defined functions.

Note: several structures and functions have been renamed compared to
pacman 2.9 code.
This was done at first for the sake of naming scheme consistency, and
then primarily because of potential namespace conflicts between library
and frontend spaces.
Indeed, it is not possible to have two different functions with the same
name declared in both spaces.
To avoid such conflicts, some function names have been prepended with
"_pacman_".
In a general manner, public library functions are named
"pacman_<type>_<action>" (examples: pacman_trans_commit(),
pacman_release(), pacman_pkg_getinfo(), ...).
Internal (and thus private) functions should be named "_pacman_XXX" for
instance (examples: _pacman_needbackup(), _pacman_runscriplet(), ...).
As of now, this scheme is only applied to most sensitive functions
(mainly the ones from util.c), which have generic names, and thus, which
are likely to be redefined in the frontend.
One can consider that the frontend should have the priority in function
names choice, and that it is up to the library to hide its symbols to
avoid conflicts with the frontend ones.
Finally, functions defined and used inside a single file should be
defined as "static".


[HANDLE] (see handle.c)

The "handle" object is the heart of the library. It is a global
structure available from almost all other objects (although some very
low level objects should not be aware of the handle object, like chained
list, package or groups structures.

There is only one instance, created by the frontend upon
"pacman_init()" call, and destroyed upon "pacman_release()" call.

pacman_init() is used to initialize library internals and to create
the handle object (handle != NULL).
Before its call, the library can't be used.
pacman_release() just does the opposite (memory used by the library is
freed, and handle is set to NULL).
After its call, the library is no more available.

The aim of the handle is to provide a central place holder for essential
library parameters (filesystem root, pointers to database objects,
configuration parameters, ...)

The handle also allows to register a log callback usable by the frontend
to catch all sort of notifications from the library.
The frontend can choose the level of verbosity (i.e. the mask), or can
simply choose to not use the log callback.
A friendly frontend should care at least for WARNING and ERROR
notifications.
Other notifications can safely be ignored and are mainly available for
troubleshooting purpose.

Last, but not least, the handle holds a _unique_ transaction object.


[TRANSACTION] (see trans.c, and also pacman.c)

The transaction structure permits easy manipulations of several packages
at a time (i.e. adding, upgrade and removal operations).

A transaction can be initiated with a type (ADD, UPGRADE or REMOVE),
and some flags (NODEPS, FORCE, CASCADE, ...).

Note: there can only be one type at a time: a transaction is either
created to add packages to the system, or either created to remove packages.
The frontend can't request for mixed operations: it has to run several
transactions, one at a time, in such a case.

The flags allow to tweak the library behaviour during its resolution.
Note, that some options of the handle can also modify the behaviour of a
transaction (NOUPGRADE, IGNOREPKG, ...).

Note: once a transaction has been initiated, it is not possible anymore
to modify its type or its flags.

One can also add some targets to a transaction (pacman_trans_addtarget()).
These targets represent the list of packages to be handled.

Then, a transaction needs to be prepared (pacman_trans_prepare()). It
means that the various targets added, will be inspected and challenged
against the set of already installed packages (dependency checks,

Last, a callback is associated with each transaction. During the
transaction resolution, each time a new step is started or done (i.e
dependency or conflict checks, package adding or removal, ...), the
callback is called, allowing the frontend to be aware of the progress of
the resolution. Can be useful to implement a progress bar.


[CONFIGURATION/OPTIONS] (see handle.c)

The library does not use any configuration file. The handle holds a
number of configuration options instead (IGNOREPKG, SYSLOG usage,
log file name, registered databases, ...).
It is up to the frontend to set the options of the library.
Options can be manipulated using calls to
pacman_set_option()/pacman_get_option().

Note: the file system root is a special option which can only be defined
when calling pacman_init(). It can't be modified afterwards.


[CACHE] (see cache.c)

Compared to pacman 2.9, there is now one cache object connected to each
database object.
There are both a package and a group cache.
The cache is loaded only on demand (i.e the cache is loaded the first
time data from it should be used).

Note: the cache of a database is always updated by the library after
an operation changing the database content (adding and/or removal of
packages).
Beware frontends ;)


[PACKAGE] (see package.c, and also db.c)

The package structure is using three new fields, namely: origin, data,
infolevel.
The purpose of these fields is to know some extra info about data stored
in package structures.

For instance, where is the package coming from (i.e origin)?
Was it loaded from a file or loaded from the cache?
If it's coming from a file, then the field data holds the full path and
name of the file, and infolevel is set to the highest possible value
(all package fields are reputed to be known).
Otherwise, if the package comes from a database, data is a pointer to
the database structure hosting the package, and infolevel is set
according to the db_read() infolevel parameter (it is possible using
db_read() to only read a part of the package data).

Indeed, to reduce database access, all packages data requested by the
frontend are coming from the cache. As a consequence, the library needs
to know exactly the level of information about packages it holds, and
then decide if more data needs to be fetched from the database.

In file pacman.c, have a look at pacman_pkg_getinfo() function to get an
overview.

Also note that the whole database consists of plain text files. If there is
something wrong, all you need is to delete the corrupted directory and
reinstall the given package using --force. No more messages like:

----
bdb_db_open: Database cannot be opened, err 22. Restore from backup!
----

[ERRORS] (error.c)

The library provides a global variable pm_errno.
It aims at being to the library what errno is for C system calls.

Almost all public library functions are returning an integer value: 0
indicating success, whereas -1 would indicate a failure.
If -1 is returned, the variable pm_errno is set to a meaningful value
(not always yet, but it should improve ;).
Wise frontends should always care for these returned values.

Note: the helper function pacman_strerror() can also be used to translate
the error code into a more friendly sentence.


[LIST] (see list.c, and especially list wrappers in pacman.c)

It is a double chained list structure, use only for the internal needs
of the library.
A frontend should be free to use its own data structures to manipulate
packages.
For instance, consider a graphical frontend using the gtk toolkit (and
as a consequence the glib library). The frontend will make use of the
glib chained lists or trees.
As a consequence, the library only provides a simple and very small
interface to retrieve pointers to its internal data (see functions
pacman_list_first(), pacman_list_next() and pacman_list_getdata()), giving to
the frontend the responsibility to copy and store the data retrieved
from the library in its own data structures.


PACMAN-G2 frontend overview & internals
====================================

Here are some words about the frontend responsibilities.
The library can operate only a small set of well defined operations and
dummy operations.

High level features are left to the frontend ;)

For instance, during a sysupgrade, the library returns the whole list of
packages to be upgraded, without any care for its content.
The frontend can inspect the list and perhaps notice that "pacman-g2"
itself has to be upgraded. In such a case, the frontend can choose to
perform a special action.


[MAIN] (see pacman-g2.c)

Calls for pacman_init(), and pacman_release().
Read the configuration file, and parse command line arguments.
Based on the action requested, it initiates the appropriate transactions
(see pacman_add(), pacman_remove(), pacman_sync() in files add.c,
remove.c and sync.c).


[CONFIGURATION] (see conf.c)

The frontend is using a configuration file, usually "/etc/pacman-g2.conf".
Part of these options are useful for the frontend only (mainly,
the download stuffs, and some options like HOLDPKG).
The rest is used to configure the library.


[ADD/UPGRADE/REMOVE/SYNC]

Nothing new here, excepted some reorganization.

The file pacman.c has been divided into several smaller files, namely
add.c, remove.c, sync.c and query.c, to hold the big parts: pacman_add,
pacman_remove, pacman_sync.
These 3 functions have been splitted too in order to ease the code reading.


[DOWNLOAD] (see download.c)

The library is providing download facilities. As a consequence, it is no longer
up to the frontend to retrieve packages from FTP servers.  To do so, libpacman
is linked against an improved version of libftp supporting both http and ftp
downloads.  As a consequence, the library is responsible for the directory
/var/cache/pacman-g2/pkgs.  One can consider that this cache is a facility
provided by the library.


[LIST] (see list.c)

Single chained list.
A minimalistic chained list implementation to store options from the
configuration file, and targets passed to pacman-g2 on the command line.
