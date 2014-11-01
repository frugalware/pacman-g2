/*
 *  package.c
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

#include "config.h"

/* pacman-g2 */
#include "package.h"

#include "db/localdb_files.h"
#include "util.h"
#include "error.h"
#include "db.h"
#include "deps.h"
#include "handle.h"
#include "pacman.h"
#include "versioncmp.h"

#include "io/archive.h"
#include "util/log.h"
#include "fstdlib.h"
#include "fstring.h"

#include <sys/utsname.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>

using namespace flib;
using namespace libpacman;

package_node::package_node(const str &name)
	: m_name(name) // PKG_NAME_LEN
{ }

package_node::~package_node()
{ }

bool package_node::operator < (const package_node &o) const
{
	return m_name < o.m_name;
}

const str &package_node::name() const
{
	return m_name;
}

bool libpacman::operator < (const libpacman::package_node_ptr &pn1, const libpacman::package_node_ptr &pn2)
{
	return pn1->name() < pn2->name();
}

package_graph::package_graph()
{ }

package_node_ptr package_graph::create(const str &pkgname)
{
	package_node_ptr node;

	return node;
}

/* vim: set ts=2 sw=2 noet: */
