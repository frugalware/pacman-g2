/*
 *  fgraph.c
 *
 *  Copyright (c) 2013 by Michel Hermier <hermier@frugalware.org>
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

#include "fgraph.h"

void f_graph_init (FGraph *graph) {
	if (graph != NULL) {
		f_list_init (&graph->vertices);
		f_list_init (&graph->edges);
	}
}

void f_graph_fini (FGraph *graph, FVisitorFunc fn, void *user_data) {
	if (graph != NULL) {
		f_list_fini (&graph->vertices, NULL, NULL);
		f_list_fini (&graph->edges, NULL, NULL);
	}
}

FGraph *f_graph_new () {
	FGraph *graph = f_zalloc (sizeof(*graph));

	f_graph_init (graph);
	return graph;
}

void f_graph_delete (FGraph *graph, FVisitorFunc fn, void *user_data) {
}

static
void f_graph_add_edge (FGraph *graph, FGraphEdge *edge) {
}

static
void f_graph_add_vertex (FGraph *graph, FGraphVertex *vertex) {
}

void f_graphedge_init (FGraphEdge *graphedge, FGraph *graph) {
}

void f_graphedge_fini (FGraphEdge *graphedge, FVisitorFunc fn, void *user_data) {
}

void f_graphedge_delete (FGraphEdge *graphedge, FVisitorFunc fn, void *user_data) {
}

void f_graphvertex_init (FGraphVertex *graphvertex, FGraph *graph) {
}

void f_graphvertex_fini (FGraphVertex *graphvertex, FVisitorFunc fn, void *user_data) {
}

void f_graphvertex_delete (FGraphVertex *graphvertex, FVisitorFunc fn, void *user_data) {
}

/* vim: set ts=2 sw=2 noet: */
