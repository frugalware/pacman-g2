/*
 *  package_properties.h
 *
 *  Copyright (c) 2014 by Michel Hermier <hermier@frugalware.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 *  USA.
 */

LIBPACMAN_PACKAGE_PROPERTY(FStringList &, backup,     BACKUP    )
LIBPACMAN_PACKAGE_PROPERTY(FStringList &, conflicts,  CONFLICTS )
LIBPACMAN_PACKAGE_PROPERTY(FStringList &, depends,    DEPENDS   )
LIBPACMAN_PACKAGE_PROPERTY(const char *,  description, DESCRIPTION)
LIBPACMAN_PACKAGE_PROPERTY(FStringList *, files,      FILES     )
LIBPACMAN_PACKAGE_PROPERTY(unsigned char, force,      FORCE     )
LIBPACMAN_PACKAGE_PROPERTY(FStringList *, groups,     GROUPS    )
LIBPACMAN_PACKAGE_PROPERTY(const char *,  name,       NAME      )
LIBPACMAN_PACKAGE_PROPERTY(FStringList &, provides,   PROVIDES  )
LIBPACMAN_PACKAGE_PROPERTY(unsigned char, reason,     REASON    )
LIBPACMAN_PACKAGE_PROPERTY(FStringList &, removes,    REMOVES   )
LIBPACMAN_PACKAGE_PROPERTY(FStringList *, replaces,   REPLACES  )
LIBPACMAN_PACKAGE_PROPERTY(FStringList &, requiredby, REQUIREDBY)
LIBPACMAN_PACKAGE_PROPERTY(unsigned char, stick,      STICKY    )
LIBPACMAN_PACKAGE_PROPERTY(FStringList *, triggers,   TRIGGERS  )
LIBPACMAN_PACKAGE_PROPERTY(const char *,  url,        URL       )
LIBPACMAN_PACKAGE_PROPERTY(const char *,  version,    VERSION   )

