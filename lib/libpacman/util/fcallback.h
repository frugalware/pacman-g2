/*
 *  fcallbacks.h
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
#ifndef F_CALLBACKS_H
#define F_CALLBACKS_H

#include "util.h"

typedef int (*FComparatorFunc)(const void *ptr, const void *comparator_data);
typedef int (*FMatcherFunc)(const void *ptr, const void *matcher_data);
typedef void (*FVisitorFunc)(void *ptr, void *visitor_data);

typedef struct FComparator FComparator;
typedef struct FMatcher FMatcher;
typedef struct FVisitor FVisitor;

struct FComparator {
	FComparatorFunc fn;
	void *data;
};

struct FMatcher {
	FMatcherFunc fn;
	void *data;
};

struct FVisitor {
	FVisitorFunc fn;
	void *data;
};

static inline
int f_compare(const void *ptr, const FComparator *comparator) {
	ASSERT(comparator != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(comparator->fn != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	return comparator->fn(ptr, comparator->data);
}

static inline
int f_compare_all(const void *ptr, const FComparator **comparators) {
	ASSERT(comparators != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	while(*comparators != NULL) {
		int match;

		if((match = f_compare(ptr, *comparators)) != 0) {
			return match;
		}
		comparators++;
	}
	return 0;
}

static inline
int f_match(const void *ptr, const FMatcher *matcher) {
	ASSERT(matcher != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(matcher->fn != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	return matcher->fn(ptr, matcher->data);
}

static inline
int f_match_all(const void *ptr, const FMatcher **matchers) {
	ASSERT(matchers != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	while(*matchers != NULL) {
		int match;

		if((match = f_match(ptr, *matchers)) != 0) {
			return match;
		}
		matchers++;
	}
	return 0;
}

static inline
int f_match_any(const void *ptr, const FMatcher **matchers) {
	ASSERT(matchers != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	while(*matchers != NULL) {
		if(f_match(ptr, *matchers) == 0) {
			return 1;
		}
		matchers++;
	}
	return 0;
}

static inline
int f_match_comparator(const void *ptr, const FComparator *comparator) {
	ASSERT(comparator != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));
	ASSERT(comparator->fn != NULL, RET_ERR(PM_ERR_WRONG_ARGS, -1));

	return comparator->fn(ptr, comparator->data) == 0;
}

static inline
void f_visit(void *ptr, const FVisitor *visitor) {
	ASSERT(visitor != NULL, return);
	ASSERT(visitor->fn != NULL, return);

	visitor->fn(ptr, visitor->data);
}

#endif /* F_CALLBACKS_H */

/* vim: set ts=2 sw=2 noet: */