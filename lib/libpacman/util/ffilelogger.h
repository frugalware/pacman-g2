/*
 *  ffilelogger.h
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
#ifndef F_FILELOGGER_H
#define F_FILELOGGER_H

#include <stdio.h>

#include "util/fabstractlogger.h"

class FFileLogger
	: public FAbstractLogger
{
public:
	FFileLogger(unsigned char mask, FILE *file);
	~FFileLogger();

protected:
	virtual void logs_impl(const char *message) override;
	FILE *m_file;
};

#endif /* F_FILELOGGER_H */

/* vim: set ts=2 sw=2 noet: */
