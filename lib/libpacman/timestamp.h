/*
 *  timestamp.h
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
#ifndef _PACMAN_TIMESTAMP_H
#define _PACMAN_TIMESTAMP_H

#include "util/time.h"

namespace libpacman {

struct Timestamp
{
  Timestamp()
		: m_value(PM_TIME_INVALID)
	{ }

  Timestamp(const time_t &epoch)
		: m_value(epoch)
	{ }

  bool operator == (const time_t &epoch) const
	{
		return m_value == epoch;
	}

  double operator - (const libpacman::Timestamp &other) const
	{
		return difftime(m_value, other.m_value);
	}

  bool isValid() const
	{
		return m_value != PM_TIME_INVALID;
	}

	time_t toEpoch() const
	{
		return m_value;
	}

	time_t m_value;
};

}

#endif /* _PACMAN_TIMESTAMP_H */

/* vim: set ts=2 sw=2 noet: */
