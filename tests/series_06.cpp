/* Copyright 2009-2016 Francesco Biscani (bluescarni@gmail.com)

This file is part of the Piranha library.

The Piranha library is free software; you can redistribute it and/or modify
it under the terms of either:

  * the GNU Lesser General Public License as published by the Free
    Software Foundation; either version 3 of the License, or (at your
    option) any later version.

or

  * the GNU General Public License as published by the Free Software
    Foundation; either version 3 of the License, or (at your option) any
    later version.

or both in parallel, as here.

The Piranha library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received copies of the GNU General Public License and the
GNU Lesser General Public License along with the Piranha library.  If not,
see https://www.gnu.org/licenses/. */

#include "../src/series.hpp"

#define BOOST_TEST_MODULE series_06_test
#include <boost/test/unit_test.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/filesystem.hpp>
#include <initializer_list>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../src/config.hpp"
#include "../src/exceptions.hpp"
#include "../src/init.hpp"
#include "../src/is_cf.hpp"
#include "../src/monomial.hpp"
#include "../src/mp_integer.hpp"
#include "../src/poisson_series.hpp"
#include "../src/polynomial.hpp"
#include "../src/pow.hpp"
#include "../src/s11n.hpp"
#include "../src/symbol_set.hpp"

using namespace piranha;

template <typename OArchive, typename IArchive, typename T>
static inline T boost_roundtrip(const T &x)
{
    std::stringstream ss;
    {
        OArchive oa(ss);
        boost_save(oa, x);
    }
    T retval;
    {
        IArchive ia(ss);
        boost_load(ia, retval);
    }
    return retval;
}

namespace bfs = boost::filesystem;

struct tmp_file {
    tmp_file()
    {
        m_path = bfs::temp_directory_path();
        // Concatenate with a unique filename.
        m_path /= bfs::unique_path();
    }
    ~tmp_file()
    {
        bfs::remove(m_path);
    }
    std::string name() const
    {
        return m_path.string();
    }
    bfs::path m_path;
};

template <typename T>
static inline void boost_roundtrip_file(const T &x)
{
    for (auto f : {data_format::boost_portable, data_format::boost_binary}) {
        for (auto c : {compression::none, compression::bzip2, compression::zlib, compression::gzip}) {
#if defined(PIRANHA_WITH_ZLIB)
            tmp_file file;
            save_file(x, file.name(), f, c);
            T retval;
            load_file(retval, file.name(), f, c);
            BOOST_CHECK_EQUAL(x, retval);
#else
            try {
                tmp_file file;
                save_file(x, file.name(), f, c);
                T retval;
                load_file(retval, file.name(), f, c);
                BOOST_CHECK_EQUAL(x, retval);
            } catch (const not_implemented_error &) {
            }
#endif
        }
    }
}

static const int ntrials = 10;

static std::mt19937 rng;

// A mock coefficient with no serialization support.
struct mock_cf3 {
    mock_cf3();
    explicit mock_cf3(const int &);
    mock_cf3(const mock_cf3 &);
    mock_cf3(mock_cf3 &&) noexcept;
    mock_cf3 &operator=(const mock_cf3 &);
    mock_cf3 &operator=(mock_cf3 &&) noexcept;
    friend std::ostream &operator<<(std::ostream &, const mock_cf3 &);
    mock_cf3 operator-() const;
    bool operator==(const mock_cf3 &) const;
    bool operator!=(const mock_cf3 &) const;
    mock_cf3 &operator+=(const mock_cf3 &);
    mock_cf3 &operator-=(const mock_cf3 &);
    mock_cf3 operator+(const mock_cf3 &) const;
    mock_cf3 operator-(const mock_cf3 &) const;
    mock_cf3 &operator*=(const mock_cf3 &);
    mock_cf3 operator*(const mock_cf3 &)const;
};

BOOST_AUTO_TEST_CASE(series_boost_s11n_test_00)
{
    init();
    using pt1 = polynomial<integer, monomial<int>>;
    BOOST_CHECK((has_boost_save<boost::archive::text_oarchive, pt1>::value));
    BOOST_CHECK((has_boost_save<boost::archive::text_oarchive, pt1 &>::value));
    BOOST_CHECK((has_boost_save<boost::archive::text_oarchive &, pt1 &>::value));
    BOOST_CHECK((has_boost_save<boost::archive::text_oarchive, const pt1 &>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::xml_oarchive, pt1>::value));
    BOOST_CHECK((!has_boost_save<const boost::archive::text_oarchive, const pt1 &>::value));
    BOOST_CHECK((has_boost_load<boost::archive::text_iarchive, pt1>::value));
    BOOST_CHECK((has_boost_load<boost::archive::text_iarchive, pt1 &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::text_iarchive, const pt1 &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::text_iarchive, const pt1>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::xml_iarchive, pt1>::value));
    BOOST_CHECK(is_cf<mock_cf3>::value);
    BOOST_CHECK((!has_boost_save<boost::archive::text_oarchive, polynomial<mock_cf3, monomial<int>>>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::text_iarchive, polynomial<mock_cf3, monomial<int>>>::value));
    // A few simple tests.
    BOOST_CHECK_EQUAL(pt1{}, (boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(pt1{})));
    BOOST_CHECK_EQUAL(pt1{},
                      (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(pt1{})));
    boost_roundtrip_file(pt1{});
    BOOST_CHECK_EQUAL(pt1{12},
                      (boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(pt1{12})));
    BOOST_CHECK_EQUAL(pt1{14},
                      (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(pt1{14})));
    boost_roundtrip_file(pt1{14});
    pt1 x{"x"}, y{"y"}, z{"z"};
    const auto p1 = math::pow(3 * x + y, 10);
    BOOST_CHECK_EQUAL(p1, (boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(p1)));
    BOOST_CHECK_EQUAL(p1, (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(p1)));
    boost_roundtrip_file(p1);
    // Some random testing.
    std::uniform_int_distribution<int> mdist(-10, 10);
    std::uniform_int_distribution<int> powdist(0, 10);
    for (int i = 0; i < ntrials; ++i) {
        pt1 tmp;
        tmp += mdist(rng) * x;
        tmp += mdist(rng) * y;
        tmp += mdist(rng) * z;
        tmp = math::pow(tmp, powdist(rng));
        BOOST_CHECK_EQUAL(tmp, (boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(tmp)));
        BOOST_CHECK_EQUAL(tmp,
                          (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(tmp)));
        boost_roundtrip_file(tmp);
    }
    // Some error testing.
    // NOTE: we used to have boost text archives for these tests, but apparently
    // they error out in a buggy way in earlier Boost versions. Valgrind reports
    // uninitalized memory reads in correspondence of the missing data, for instance.
    // Might be related to this:
    // https://github.com/boostorg/serialization/commit/fb559afd0dc540774a41fda1c72cc64addebe075
    std::stringstream ss;
    using ss_size_t = symbol_set::size_type;
    using s_size_t = pt1::size_type;
    {
        boost::archive::binary_oarchive oa(ss);
        boost_save(oa, 0u);
        boost_save(oa, ss_size_t(2));
        boost_save(oa, std::string("x"));
        boost_save(oa, std::string("y"));
        boost_save(oa, s_size_t(1));
        boost_save(oa, 1_z);
        // Monomial incompatible with symbol set.
        monomial<int> k;
        k.boost_save(oa, symbol_set{});
    }
    {
        boost::archive::binary_iarchive ia(ss);
        pt1 tmp;
        BOOST_CHECK_THROW(boost_load(ia, tmp), std::invalid_argument);
    }
    ss.str("");
    ss.clear();
    {
        boost::archive::binary_oarchive oa(ss);
        boost_save(oa, 0u);
        boost_save(oa, ss_size_t(2));
        boost_save(oa, std::string("x"));
        boost_save(oa, std::string("y"));
        boost_save(oa, s_size_t(1));
        boost_save(oa, 1_z);
        // Don't save any monomial.
    }
    {
        boost::archive::binary_iarchive ia(ss);
        pt1 tmp;
        BOOST_CHECK_THROW(boost_load(ia, tmp), boost::archive::archive_exception);
    }
    ss.str("");
    ss.clear();
    {
        boost::archive::binary_oarchive oa(ss);
        boost_save(oa, 0u);
        boost_save(oa, ss_size_t(2));
        boost_save(oa, std::string("x"));
        // Save an int in place of a string.
        boost_save(oa, 1);
        boost_save(oa, s_size_t(0));
    }
    {
        boost::archive::binary_iarchive ia(ss);
        pt1 tmp;
        BOOST_CHECK_THROW(boost_load(ia, tmp), boost::archive::archive_exception);
    }
    ss.str("");
    ss.clear();
    {
        boost::archive::binary_oarchive oa(ss);
        // Save version too high.
        boost_save(oa, 1u);
        boost_save(oa, ss_size_t(2));
        boost_save(oa, std::string("x"));
        boost_save(oa, std::string("y"));
        boost_save(oa, s_size_t(0));
    }
    {
        boost::archive::binary_iarchive ia(ss);
        pt1 tmp;
        auto msg_checker = [](const std::invalid_argument &inva) {
            return boost::contains(inva.what(), "what: the series archive version 1 is greater than the "
                                                "latest archive version 0 supported by this version of Piranha");
        };
        BOOST_CHECK_EXCEPTION(boost_load(ia, tmp), std::invalid_argument, msg_checker);
    }
}

BOOST_AUTO_TEST_CASE(series_boost_s11n_test_01)
{
    // Similar to above, but with recursive poly.
    using pt0 = polynomial<integer, monomial<int>>;
    using pt1 = polynomial<pt0, monomial<int>>;
    BOOST_CHECK((has_boost_save<boost::archive::text_oarchive, pt1>::value));
    BOOST_CHECK((has_boost_save<boost::archive::text_oarchive, pt1 &>::value));
    BOOST_CHECK((has_boost_save<boost::archive::text_oarchive &, pt1 &>::value));
    BOOST_CHECK((has_boost_save<boost::archive::text_oarchive, const pt1 &>::value));
    BOOST_CHECK((!has_boost_save<boost::archive::xml_oarchive, pt1>::value));
    BOOST_CHECK((!has_boost_save<const boost::archive::text_oarchive, const pt1 &>::value));
    BOOST_CHECK((has_boost_load<boost::archive::text_iarchive, pt1>::value));
    BOOST_CHECK((has_boost_load<boost::archive::text_iarchive, pt1 &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::text_iarchive, const pt1 &>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::text_iarchive, const pt1>::value));
    BOOST_CHECK((!has_boost_load<boost::archive::xml_iarchive, pt1>::value));
    // A few simple tests.
    BOOST_CHECK_EQUAL(pt1{}, (boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(pt1{})));
    BOOST_CHECK_EQUAL(pt1{},
                      (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(pt1{})));
    boost_roundtrip_file(pt1{});
    BOOST_CHECK_EQUAL(pt1{12},
                      (boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(pt1{12})));
    BOOST_CHECK_EQUAL(pt1{14},
                      (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(pt1{14})));
    boost_roundtrip_file(pt1{14});
    pt0 x{"x"};
    pt1 y{"y"}, z{"z"};
    const auto p1 = math::pow(3 * x + y, 10);
    BOOST_CHECK_EQUAL(p1, (boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(p1)));
    BOOST_CHECK_EQUAL(p1, (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(p1)));
    boost_roundtrip_file(p1);
    // Some random testing.
    std::uniform_int_distribution<int> mdist(-10, 10);
    std::uniform_int_distribution<int> powdist(0, 10);
    for (int i = 0; i < ntrials; ++i) {
        pt1 tmp;
        tmp += mdist(rng) * x;
        tmp += mdist(rng) * y;
        tmp += mdist(rng) * z;
        tmp = math::pow(tmp, powdist(rng));
        BOOST_CHECK_EQUAL(tmp, (boost_roundtrip<boost::archive::text_oarchive, boost::archive::text_iarchive>(tmp)));
        BOOST_CHECK_EQUAL(tmp,
                          (boost_roundtrip<boost::archive::binary_oarchive, boost::archive::binary_iarchive>(tmp)));
        boost_roundtrip_file(tmp);
    }
}

#if defined(PIRANHA_WITH_MSGPACK)

template <typename T>
static inline T msgpack_roundtrip(const T &x, msgpack_format f)
{
    msgpack::sbuffer sbuf;
    msgpack::packer<msgpack::sbuffer> p(sbuf);
    msgpack_pack(p, x, f);
    auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
    T retval;
    msgpack_convert(retval, oh.get(), f);
    return retval;
}

template <typename T>
static inline void msgpack_roundtrip_file(const T &x)
{
    for (auto f : {data_format::msgpack_portable, data_format::msgpack_binary}) {
        for (auto c : {compression::none, compression::bzip2, compression::zlib, compression::gzip}) {
#if defined(PIRANHA_WITH_ZLIB)
            tmp_file file;
            save_file(x, file.name(), f, c);
            T retval;
            load_file(retval, file.name(), f, c);
            BOOST_CHECK_EQUAL(x, retval);
#else
            try {
                tmp_file file;
                save_file(x, file.name(), f, c);
                T retval;
                load_file(retval, file.name(), f, c);
                BOOST_CHECK_EQUAL(x, retval);
            } catch (const not_implemented_error &) {
            }
#endif
        }
    }
}

BOOST_AUTO_TEST_CASE(series_msgpack_s11n_test_00)
{
    using pt1 = polynomial<integer, monomial<int>>;
    BOOST_CHECK((has_msgpack_pack<std::stringstream, pt1>::value));
    BOOST_CHECK((has_msgpack_pack<std::stringstream, pt1 &>::value));
    BOOST_CHECK((has_msgpack_pack<std::stringstream, const pt1 &>::value));
    BOOST_CHECK((has_msgpack_pack<std::stringstream, const pt1>::value));
    BOOST_CHECK((!has_msgpack_pack<std::stringstream &, const pt1>::value));
    BOOST_CHECK((!has_msgpack_pack<const std::stringstream, const pt1>::value));
    BOOST_CHECK((has_msgpack_convert<pt1>::value));
    BOOST_CHECK((has_msgpack_convert<pt1 &>::value));
    BOOST_CHECK((!has_msgpack_convert<const pt1 &>::value));
    BOOST_CHECK((!has_msgpack_convert<const pt1>::value));
    BOOST_CHECK((!has_msgpack_pack<std::stringstream, polynomial<mock_cf3, monomial<int>>>::value));
    BOOST_CHECK((!has_msgpack_convert<polynomial<mock_cf3, monomial<int>>>::value));
    // A few simple checks.
    for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
        BOOST_CHECK_EQUAL(pt1{}, (msgpack_roundtrip(pt1{}, f)));
        BOOST_CHECK_EQUAL(pt1{"x"}, (msgpack_roundtrip(pt1{"x"}, f)));
        BOOST_CHECK_EQUAL(math::pow(2 * pt1{"x"} - 3 * pt1{"y"}, 10),
                          (msgpack_roundtrip(math::pow(2 * pt1{"x"} - 3 * pt1{"y"}, 10), f)));
    }
    // Some random testing.
    pt1 x{"x"}, y{"y"}, z{"z"};
    std::uniform_int_distribution<int> mdist(-10, 10);
    std::uniform_int_distribution<int> powdist(0, 10);
    for (int i = 0; i < ntrials; ++i) {
        for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
            pt1 tmp;
            tmp += mdist(rng) * x;
            tmp += mdist(rng) * y;
            tmp += mdist(rng) * z;
            tmp = math::pow(tmp, powdist(rng));
            BOOST_CHECK_EQUAL(tmp, (msgpack_roundtrip(tmp, f)));
            msgpack_roundtrip_file(tmp);
        }
    }
    // Error testing.
    msgpack::sbuffer sbuf;
    {
        msgpack::packer<msgpack::sbuffer> p(sbuf);
        p.pack_array(1);
        msgpack_pack(p, 42, msgpack_format::portable);
        auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
        auto msg_checker = [](const std::invalid_argument &inva) {
            return boost::contains(inva.what(), "error converting series from msgpack object: "
                                                "the format is portable and the size of the object is 1"
                                                " instead of 3");
        };
        pt1 tmp;
        BOOST_CHECK_EXCEPTION(msgpack_convert(tmp, oh.get(), msgpack_format::portable), std::invalid_argument,
                              msg_checker);
    }
    sbuf.clear();
    {
        msgpack::packer<msgpack::sbuffer> p(sbuf);
        p.pack_array(1);
        msgpack_pack(p, 42, msgpack_format::binary);
        auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
        auto msg_checker = [](const std::invalid_argument &inva) {
            return boost::contains(inva.what(), "error converting series from msgpack object: "
                                                "the format is binary and the size of the object is 1"
                                                " instead of 2");
        };
        pt1 tmp;
        BOOST_CHECK_EXCEPTION(msgpack_convert(tmp, oh.get(), msgpack_format::binary), std::invalid_argument,
                              msg_checker);
    }
    sbuf.clear();
    {
        msgpack::packer<msgpack::sbuffer> p(sbuf);
        p.pack_array(3);
        msgpack_pack(p, 1u, msgpack_format::portable);
        msgpack_pack(p, 1u, msgpack_format::portable);
        msgpack_pack(p, 1u, msgpack_format::portable);
        auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
        auto msg_checker = [](const std::invalid_argument &inva) {
            return boost::contains(inva.what(), "what: the series archive version 1 is greater than the "
                                                "latest archive version 0 supported by this version of Piranha");
        };
        pt1 tmp;
        BOOST_CHECK_EXCEPTION(msgpack_convert(tmp, oh.get(), msgpack_format::portable), std::invalid_argument,
                              msg_checker);
    }
    sbuf.clear();
    {
        msgpack::packer<msgpack::sbuffer> p(sbuf);
        p.pack_array(3);
        msgpack_pack(p, 0u, msgpack_format::portable);
        p.pack_array(2);
        msgpack_pack(p, std::string("x"), msgpack_format::portable);
        msgpack_pack(p, std::string("y"), msgpack_format::portable);
        p.pack_array(1);
        p.pack_array(1);
        msgpack_pack(p, 1_z, msgpack_format::portable);
        auto oh = msgpack::unpack(sbuf.data(), sbuf.size());
        pt1 tmp;
        BOOST_CHECK_THROW(msgpack_convert(tmp, oh.get(), msgpack_format::portable), msgpack::type_error);
    }
}

BOOST_AUTO_TEST_CASE(series_msgpack_s11n_test_01)
{
    using pt0 = polynomial<integer, monomial<int>>;
    using pt1 = polynomial<pt0, monomial<int>>;
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, pt1>::value));
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, pt1 &>::value));
    BOOST_CHECK((has_msgpack_pack<msgpack::sbuffer, const pt1 &>::value));
    BOOST_CHECK((!has_msgpack_pack<msgpack::sbuffer &, const pt1 &>::value));
    BOOST_CHECK((has_msgpack_convert<pt1>::value));
    BOOST_CHECK((has_msgpack_convert<pt1 &>::value));
    BOOST_CHECK((!has_msgpack_convert<const pt1 &>::value));
    BOOST_CHECK((!has_msgpack_convert<const pt1>::value));
    // A few simple tests.
    pt0 x{"x"};
    pt1 y{"y"}, z{"z"};
    for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
        BOOST_CHECK_EQUAL(pt1{}, (msgpack_roundtrip(pt1{}, f)));
        BOOST_CHECK_EQUAL(pt1{}, (msgpack_roundtrip(pt1{}, f)));
        msgpack_roundtrip_file(pt1{});
        BOOST_CHECK_EQUAL(pt1{12}, (msgpack_roundtrip(pt1{12}, f)));
        BOOST_CHECK_EQUAL(pt1{14}, (msgpack_roundtrip(pt1{14}, f)));
        msgpack_roundtrip_file(pt1{14});
        const auto p1 = math::pow(3 * x + y, 10);
        BOOST_CHECK_EQUAL(p1, (msgpack_roundtrip(p1, f)));
        BOOST_CHECK_EQUAL(p1, (msgpack_roundtrip(p1, f)));
        msgpack_roundtrip_file(p1);
    }
    // Some random testing.
    std::uniform_int_distribution<int> mdist(-10, 10);
    std::uniform_int_distribution<int> powdist(0, 10);
    for (int i = 0; i < ntrials; ++i) {
        for (auto f : {msgpack_format::portable, msgpack_format::binary}) {
            pt1 tmp;
            tmp += mdist(rng) * x;
            tmp += mdist(rng) * y;
            tmp += mdist(rng) * z;
            tmp = math::pow(tmp, powdist(rng));
            BOOST_CHECK_EQUAL(tmp, (msgpack_roundtrip(tmp, f)));
            msgpack_roundtrip_file(tmp);
        }
    }
}

#endif