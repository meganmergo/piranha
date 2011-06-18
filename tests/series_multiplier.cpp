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

#include "../src/series_multiplier.hpp"

#define BOOST_TEST_MODULE series_multiplier_test
#include <boost/test/unit_test.hpp>

#include <boost/mpl/for_each.hpp>
#include <boost/mpl/vector.hpp>

#include "../src/debug_access.hpp"
#include "../src/echelon_descriptor.hpp"
#include "../src/integer.hpp"
#include "../src/numerical_coefficient.hpp"
#include "../src/polynomial_term.hpp"
#include "../src/settings.hpp"
#include "../src/top_level_series.hpp"

using namespace piranha;

template <typename Cf, typename Expo>
class polynomial:
	public top_level_series<polynomial_term<Cf,Expo>,polynomial<Cf,Expo>>
{
		typedef top_level_series<polynomial_term<Cf,Expo>,polynomial<Cf,Expo>> base;
	public:
		polynomial() = default;
		polynomial(const polynomial &) = default;
		polynomial(polynomial &&) = default;
		explicit polynomial(const char *name):base()
		{
			typedef typename base::term_type term_type;
			// Insert the symbol.
			this->m_ed.template add_symbol<term_type>(symbol(name));
			// Construct and insert the term.
			this->insert(term_type(Cf(1,this->m_ed),typename term_type::key_type{Expo(1)}),this->m_ed);
		}
		~polynomial() = default;
		polynomial &operator=(const polynomial &) = default;
		polynomial &operator=(polynomial &&other) piranha_noexcept_spec(true)
		{
			base::operator=(std::move(other));
			return *this;
		}
};

typedef polynomial<numerical_coefficient<double>,int> p_type1;
typedef polynomial<numerical_coefficient<integer>,int> p_type2;

typedef boost::mpl::vector<p_type1,p_type2> p_types;

struct operator_tag {};

namespace piranha
{
template <>
struct debug_access<operator_tag>
{
	debug_access()
	{
		p_type1 p1("x"), p2("x");
		echelon_descriptor<p_type1::term_type> ed;
		ed.add_symbol<p_type1::term_type>("x");
		p1.m_container.begin()->m_cf.multiply_by(2,ed);
		p2.m_container.begin()->m_cf.multiply_by(3,ed);
		series_multiplier<p_type1,p_type1> sm1(p1,p2);
		auto retval = sm1(ed);
		BOOST_CHECK(retval.size() == 1u);
		BOOST_CHECK(retval.m_container.begin()->m_key.size() == 1u);
		BOOST_CHECK(retval.m_container.begin()->m_key[0] == 2);
		BOOST_CHECK(retval.m_container.begin()->m_cf.get_value() == (double(3) * double(1)) * (double(2) * double(1)));
		p_type2 p3("x");
		p3.m_container.begin()->m_cf.multiply_by(4,ed);
		series_multiplier<p_type1,p_type2> sm2(p1,p3);
		retval = sm2(ed);
		BOOST_CHECK(retval.size() == 1u);
		BOOST_CHECK(retval.m_container.begin()->m_key.size() == 1u);
		BOOST_CHECK(retval.m_container.begin()->m_key[0] == 2);
		BOOST_CHECK(retval.m_container.begin()->m_cf.get_value() == double((double(2) * double(1)) * (integer(1) * 4)));
	}
};
}

typedef debug_access<operator_tag> operator_tester;

BOOST_AUTO_TEST_CASE(series_multiplier_operator_test)
{
	operator_tester o;
}

struct multiplication_tester
{
	// NOTE: this test is going to be exact in case of coefficients cancellations with double
	// precision coefficients only if the platform has ieee 754 format (integer exactly representable
	// as doubles up to 2 ** 53).
	template <typename T>
	void operator()(const T &)
	{
		T x("x"), y("y"), z("z"), t("t"), u("u");
		// Dense case, default setup.
		auto f = 1 + x + y + z + t;
		auto tmp(f);
		for (int i = 1; i < 10; ++i) {
			f *= tmp;
		}
		auto g = f + 1;
		auto retval = f * g;
		BOOST_CHECK_EQUAL(retval.size(),10626u);
		// Dense case, force number of threads.
		for (auto i = 1u; i <= 4u; ++i) {
			settings::set_n_threads(i);
			auto retval = f * g;
			BOOST_CHECK_EQUAL(retval.size(),10626u);
		}
		settings::reset_n_threads();
		// Dense case with cancellations, default setup.
		auto h = 1 - x + y + z + t;
		tmp = h;
		for (int i = 1; i < 10; ++i) {
			h *= tmp;
		}
		retval = f * h;
		BOOST_CHECK_EQUAL(retval.size(),5786u);
		// Dense case with cancellations, force number of threads.
		for (auto i = 1u; i <= 4u; ++i) {
			settings::set_n_threads(i);
			auto retval = f * h;
			BOOST_CHECK_EQUAL(retval.size(),5786u);
		}
		settings::reset_n_threads();
		// Sparse case, default.
		f = (x + y + z*z*2 + t*t*t*3 + u*u*u*u*u*5 + 1);
		auto tmp_f(f);
		g = (u + t + z*z*2 + y*y*y*3 + x*x*x*x*x*5 + 1);
		auto tmp_g(g);
		h = (-u + t + z*z*2 + y*y*y*3 + x*x*x*x*x*5 + 1);
		auto tmp_h(h);
		for (int i = 1; i < 8; ++i) {
			f *= tmp_f;
			g *= tmp_g;
			h *= tmp_h;
		}
		retval = f * g;
		BOOST_CHECK_EQUAL(retval.size(),591235u);
		// Sparse case, force n threads.
		for (auto i = 1u; i <= 4u; ++i) {
			settings::set_n_threads(i);
			auto retval = f * g;
			BOOST_CHECK_EQUAL(retval.size(),591235u);
		}
		settings::reset_n_threads();
		// Sparse case with cancellations, default.
		retval = f * h;
		BOOST_CHECK_EQUAL(retval.size(),591184u);
		// Sparse case with cancellations, force number of threads.
		for (auto i = 1u; i <= 4u; ++i) {
			settings::set_n_threads(i);
			auto retval = f * h;
			BOOST_CHECK_EQUAL(retval.size(),591184u);
		}
	}
};

BOOST_AUTO_TEST_CASE(series_multiplier_multiplication_test)
{
	boost::mpl::for_each<p_types>(multiplication_tester());
}
