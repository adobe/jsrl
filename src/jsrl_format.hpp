/**
 * Copyright 2026 Adobe. All rights reserved.
 * This file is licensed to you under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. You may obtain a copy
 * of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under
 * the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
 * OF ANY KIND, either express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 */
#ifndef JSRL_FORMAT_HPP_9A4E7C3B2D1F8E6A5B9C0D3F4E7A2B1C
#define JSRL_FORMAT_HPP_9A4E7C3B2D1F8E6A5B9C0D3F4E7A2B1C

#include "jsrl.hpp"
#include "jsrlpp.hpp"
#include "jsrl_general_number.hpp"

#include <sstream>

/*! @file jsrl_format.hpp
 *  @brief std::formatter specializations for jsrl types
 *
 *  This header provides std::formatter specializations for jsrl JSON types,
 *  enabling them to be used with std::format() and related formatting functions.
 *  All formatters produce output identical to their corresponding operator<< implementations.
 *
 *  Requires C++20 or later for std::format support.
 */

#if __cplusplus < 202002L
#  error "jsrl_format.hpp requires C++20 or later"
#elif !__has_include(<format>)
#  error "jsrl_format.hpp requires a standard library that provides <format>"
#else
#include <format>
#endif

namespace std {

    /*! @brief std::formatter specialization for jsrl::Json
     *
     *  Formats Json values using their operator<< implementation,
     *  producing compact JSON output.
     *
     *  Inheriting from formatter<string> satisfies the std::formattable concept
     *  as required by libc++'s consteval format-string validation in
     *  std::basic_format_string.
     */
    template <>
    struct formatter<jsrl::Json> : formatter<string> {
        template <typename FormatContext>
        auto format(jsrl::Json const& val, FormatContext& ctx) const {
            std::ostringstream oss;
            oss << val;
            return formatter<string>::format(oss.str(), ctx);
        }
    };

    /*! @brief std::formatter specialization for jsrl::Json::OptionedWrite
     *
     *  Formats Json values with custom encoding options (loose floats, UTF-8 handling, etc.).
     */
    template <>
    struct formatter<jsrl::Json::OptionedWrite> : formatter<string> {
        template <typename FormatContext>
        auto format(jsrl::Json::OptionedWrite const& val, FormatContext& ctx) const {
            std::ostringstream oss;
            oss << val;
            return formatter<string>::format(oss.str(), ctx);
        }
    };

    /*! @brief std::formatter specialization for jsrl::JsonPrettyPrint
     *
     *  Formats Json values with pretty-printing (indentation, newlines).
     */
    template <>
    struct formatter<jsrl::JsonPrettyPrint> : formatter<string> {
        template <typename FormatContext>
        auto format(jsrl::JsonPrettyPrint const& val, FormatContext& ctx) const {
            std::ostringstream oss;
            oss << val;
            return formatter<string>::format(oss.str(), ctx);
        }
    };

    /*! @brief std::formatter specialization for jsrl::BoundJsonPrettyPrint
     *
     *  Formats Json values bound to a specific pretty-print configuration.
     */
    template <>
    struct formatter<jsrl::BoundJsonPrettyPrint> : formatter<string> {
        template <typename FormatContext>
        auto format(jsrl::BoundJsonPrettyPrint const& val, FormatContext& ctx) const {
            std::ostringstream oss;
            oss << val;
            return formatter<string>::format(oss.str(), ctx);
        }
    };

    /*! @brief std::formatter specialization for jsrl::Json::Error
     *
     *  Formats Json error objects with their error tag and message.
     */
    template <>
    struct formatter<jsrl::Json::Error> : formatter<string> {
        template <typename FormatContext>
        auto format(jsrl::Json::Error const& val, FormatContext& ctx) const {
            std::ostringstream oss;
            oss << val;
            return formatter<string>::format(oss.str(), ctx);
        }
    };

    /*! @brief std::formatter specialization for jsrl::GeneralNumber
     *
     *  Formats GeneralNumber values using their operator<< implementation.
     */
    template <>
    struct formatter<jsrl::GeneralNumber> : formatter<string> {
        template <typename FormatContext>
        auto format(jsrl::GeneralNumber const& val, FormatContext& ctx) const {
            std::ostringstream oss;
            oss << val;
            return formatter<string>::format(oss.str(), ctx);
        }
    };

} // namespace std

#endif
