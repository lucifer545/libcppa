/******************************************************************************\
 *           ___        __                                                    *
 *          /\_ \    __/\ \                                                   *
 *          \//\ \  /\_\ \ \____    ___   _____   _____      __               *
 *            \ \ \ \/\ \ \ '__`\  /'___\/\ '__`\/\ '__`\  /'__`\             *
 *             \_\ \_\ \ \ \ \L\ \/\ \__/\ \ \L\ \ \ \L\ \/\ \L\.\_           *
 *             /\____\\ \_\ \_,__/\ \____\\ \ ,__/\ \ ,__/\ \__/.\_\          *
 *             \/____/ \/_/\/___/  \/____/ \ \ \/  \ \ \/  \/__/\/_/          *
 *                                          \ \_\   \ \_\                     *
 *                                           \/_/    \/_/                     *
 *                                                                            *
 * Copyright (C) 2011-2013                                                    *
 * Dominik Charousset <dominik.charousset@haw-hamburg.de>                     *
 *                                                                            *
 * This file is part of libcppa.                                              *
 * libcppa is free software: you can redistribute it and/or modify it under   *
 * the terms of the GNU Lesser General Public License as published by the     *
 * Free Software Foundation; either version 2.1 of the License,               *
 * or (at your option) any later version.                                     *
 *                                                                            *
 * libcppa is distributed in the hope that it will be useful,                 *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                       *
 * See the GNU Lesser General Public License for more details.                *
 *                                                                            *
 * You should have received a copy of the GNU Lesser General Public License   *
 * along with libcppa. If not, see <http://www.gnu.org/licenses/>.            *
\******************************************************************************/


#include <limits>
#include <string>
#include <cstdint>
#include <sstream>
#include <cstring>
#include <type_traits>

#include "cppa/primitive_variant.hpp"
#include "cppa/binary_serializer.hpp"

using std::enable_if;

namespace cppa {

namespace {

constexpr size_t chunk_size = 512;
constexpr size_t ui32_size = sizeof(std::uint32_t);

using util::grow_if_needed;

class binary_writer {

 public:

    binary_writer(util::buffer* sink) : m_sink(sink) { }

    template<typename T>
    static inline void write_int(util::buffer* sink, const T& value) {
        sink->write(sizeof(T), &value, grow_if_needed);
    }

    static inline void write_string(util::buffer* sink,
                                    const std::string& str) {
        write_int(sink, static_cast<std::uint32_t>(str.size()));
        sink->write(str.size(), str.c_str(), grow_if_needed);
    }

    template<typename T>
    void operator()(const T& value,
                    typename enable_if<std::is_integral<T>::value>::type* = 0) {
        write_int(m_sink, value);
    }

    template<typename T>
    void operator()(const T& value,
                    typename enable_if<std::is_floating_point<T>::value>::type* = 0) {
        // write floating points as strings
        std::ostringstream iss;
        iss.precision(std::numeric_limits<T>::max_digits10);
        iss << value;
        (*this)(iss.str());
    }

    void operator()(const std::string& str) {
        write_string(m_sink, str);
    }

    void operator()(const std::u16string& str) {
        write_int(m_sink, static_cast<std::uint32_t>(str.size()));
        for (char16_t c : str) {
            // force writer to use exactly 16 bit
            write_int(m_sink, static_cast<std::uint16_t>(c));
        }
    }

    void operator()(const std::u32string& str) {
        write_int(m_sink, static_cast<std::uint32_t>(str.size()));
        for (char32_t c : str) {
            // force writer to use exactly 32 bit
            write_int(m_sink, static_cast<std::uint32_t>(c));
        }
    }

 private:

    util::buffer* m_sink;

};

} // namespace <anonymous>

binary_serializer::binary_serializer(util::buffer* buf, actor_addressing* ptr)
: super(ptr), m_sink(buf) { }

void binary_serializer::begin_object(const std::string& tname) {
    binary_writer::write_string(m_sink, tname);
}

void binary_serializer::end_object() { }

void binary_serializer::begin_sequence(size_t list_size) {
    binary_writer::write_int(m_sink, static_cast<std::uint32_t>(list_size));
}

void binary_serializer::end_sequence() { }

void binary_serializer::write_value(const primitive_variant& value) {
    value.apply(binary_writer(m_sink));
}

void binary_serializer::write_raw(size_t num_bytes, const void* data) {
    m_sink->write(num_bytes, data, grow_if_needed);
}

void binary_serializer::write_tuple(size_t size,
                                    const primitive_variant* values) {
    const primitive_variant* end = values + size;
    for ( ; values != end; ++values) {
        write_value(*values);
    }
}

} // namespace cppa
