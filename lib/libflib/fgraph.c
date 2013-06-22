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

#include "config.h"

#include <assert.h>

#include "fgraph.h"

#include "fstdlib.h"

void f_graph_init (FGraph *graph) {
	assert (graph != NULL);

	f_list_init (&graph->vertices);
	f_list_init (&graph->edges);
}

void f_graph_fini (FGraph *graph, FVisitorFunc fn, void *user_data) {
	assert (graph != NULL);

	f_list_fini (&graph->vertices, NULL, NULL);
	f_list_fini (&graph->edges, NULL, NULL);
}

FGraph *f_graph_new () {
	FGraph *graph = f_zalloc (sizeof(*graph));

	if (graph != NULL) {
		f_graph_init (graph);
	}
	return graph;
}

void f_graph_delete (FGraph *graph, FVisitorFunc fn, void *user_data) {
	f_graph_fini (graph, fn, user_data);
	f_free (graph);
}

int f_graph_add_edge (FGraph *graph, FGraphEdge *edge) {
	assert (graph != NULL);
	assert (edge != NULL);
	assert (edge->graph == NULL);

	edge->graph = graph;
	f_list_add (&graph->edges, &edge->base);
	return 0;
}

int f_graph_add_vertex (FGraph *graph, FGraphVertex *vertex) {
	assert (graph != NULL);
	assert (vertex != NULL);
	assert (vertex->graph == NULL);

	vertex->graph = graph;
	f_list_add (&graph->vertices, &vertex->base);
	return 0;
}

void f_graph_fill_edges_color (FGraph *graph, FGraphColor color) {
	FGraphEdge *graphedge;

	assert (graph != NULL);

	f_foreach_entry (graphedge, &graph->edges, base) {
		graphedge->color = color;
	}
}

void f_graph_fill_vertices_color (FGraph *graph, FGraphColor color) {
	FGraphVertex *graphvertex;

	assert (graph != NULL);

	f_foreach_entry (graphvertex, &graph->vertices, base) {
		graphvertex->color = color;
	}
}

void f_graphedge_init (FGraphEdge *graphedge, FGraph *graph) {
	assert (graphedge != NULL);

	f_listitem_init (&graphedge->base);
	graphedge->graph = graph;
	f_list_init (&graphedge->vertices);
}

void f_graphedge_fini (FGraphEdge *graphedge, FVisitorFunc fn, void *user_data) {
	assert (graphedge != NULL);
}

void f_graphedge_delete (FGraphEdge *graphedge, FVisitorFunc fn, void *user_data) {
	f_graphedge_fini (graphedge, fn, user_data);
	f_free (graphedge);
}

void f_graphvertex_init (FGraphVertex *graphvertex, FGraph *graph) {
	assert (graphvertex != NULL);

	f_listitem_init (&graphvertex->base);
	graphvertex->graph = graph;
	f_list_init (&graphvertex->edges);
}

void f_graphvertex_fini (FGraphVertex *graphvertex, FVisitorFunc fn, void *user_data) {
	assert (graphvertex != NULL);
}

void f_graphvertex_delete (FGraphVertex *graphvertex, FVisitorFunc fn, void *user_data) {
	f_graphvertex_fini (graphvertex, fn, user_data);
	f_free (graphvertex);
}

/* vim: set ts=2 sw=2 noet: */
