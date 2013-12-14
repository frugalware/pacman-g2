/*
 *  fstring.h
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
#ifndef F_STRING_H
#define F_STRING_H

#include <string.h>

#include "util/fcallback.h"

char *f_basename(const char *path);
char *f_dirname(const char *path);

static inline
int f_strempty(const char *s)
{
	return s != NULL ? s[0] == '\0' : !0;
}

static inline
size_t f_strlen(const char *s)
{
	return s != NULL ? strlen(s) : 0;
}

char *f_strtoupper(char *str);
char *f_strtrim(char *str);

typedef int (*FStrComparatorFunc)(const char *str, const void *stringcomparator_data);
typedef int (*FStrMatcherFunc)(const char *str, const void *stringmatcher_data);
typedef void (*FStrVisitorFunc)(char *str, void *stringvisitor_data);

typedef struct FStrComparator FStrComparator;
typedef struct FStrMatcher FStrMatcher;
typedef struct FStrVisitor FStrVisitor;

struct FStrComparator {
	FStrComparatorFunc fn;
	void *data;
};

struct FStrMatcher {
	FStrMatcherFunc fn;
	void *data;
};

struct FStrVisitor {
	FStrVisitorFunc fn;
	void *data;
};

static inline
int f_str_compare(const char *str, const FStrComparator *str_comparator) {
	return f_compare(str, (const FComparator *)str_comparator);
}

static inline
int f_str_compare_all(const char *str, const FStrComparator **str_comparators) {
	return f_compare_all(str, (const FComparator **)str_comparators);
}

static inline
int f_str_match(const char *str, const FStrMatcher *str_matcher) {
	return f_match(str, (const FMatcher *)str_matcher);
}

static inline
int f_str_match_all(const char *str, const FStrMatcher **str_matchers) {
	return f_match_all(str, (const FMatcher **)str_matchers);
}

static inline
int f_str_match_any(const char *str, const FStrMatcher **str_matchers) {
	return f_match_any(str, (const FMatcher **)str_matchers);
}

static inline
int f_str_match_comparator(const char *str, const FStrComparator *str_comparator) {
	return f_match_comparator(str, (const FComparator *)str_comparator);
}

static inline
void f_str_visit(char *str, FStrVisitor *str_visitor) {
	f_visit(str, (FVisitor *)str_visitor);
}

#define F_STRMATCHER_IGNORE_CASE (1<<0)
#define F_STRMATCHER_EQUAL       (1<<1)
#define F_STRMATCHER_REGEXP      (1<<2)
#define F_STRMATCHER_SUBSTRING   (1<<3)

int f_strmatcher_init(FStrMatcher *str_matcher, const char *pattern, int flags);
int f_strmatcher_fini(FStrMatcher *str_matcher);

#endif /* F_STRING_H */

/* vim: set ts=2 sw=2 noet: */
