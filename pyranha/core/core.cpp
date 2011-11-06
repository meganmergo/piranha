/***************************************************************************
 *   Copyright (C) 2009-2011 by Francesco Biscani                          *
 *   bluescarni@gmail.com                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// NOTE: we need to include cmath here because of this issue with pyconfig.h and hypot:
// http://mail.python.org/pipermail/new-bugs-announce/2011-March/010395.html
#include <cmath>
#include <boost/python/class.hpp>
#include <boost/python/module.hpp>

#include "../../src/integer.hpp"

using namespace boost::python;
using namespace piranha;

BOOST_PYTHON_MODULE(_core)
{
	class_<integer>("integer", "Arbitrary precision integer class.", init<>());
}
