/*
 *  package_graph.h
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
#ifndef _PACMAN_PACKAGE_GRAPH_H
#define _PACMAN_PACKAGE_GRAPH_H

#include "util/fset.h"

namespace libpacman {

	class package_node
		: public flib::refcounted
	{
	public:
		package_node(const flib::str &name);
		~package_node();
		bool operator < (const package_node &o) const;

		const flib::str &name() const;

	private:
		flib::str m_name;
		libpacman::package_set m_packages;
	};

	typedef flib::refcounted_shared_ptr<libpacman::package_node> package_node_ptr;
	bool operator < (const package_node_ptr &pn1, const package_node_ptr &pn2);

	typedef flib::set<libpacman::package_node_ptr> package_node_set;

	class package_graph
	{
	public:
		package_graph();

		libpacman::package_node_ptr create(const flib::str &pkgname);

	private:
		package_graph(const package_graph &o);
		package_graph &operator = (const package_graph &o);

		package_node_set m_nodes;
	};
} // namespace libpacman

#endif /* _PACMAN_PACKAGE_GRAPH_H */

/* vim: set ts=2 sw=2 noet: */
