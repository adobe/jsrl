/**
 * Copyright 2025 Adobe. All rights reserved.
 * This file is licensed to you under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. You may obtain a copy
 * of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under
 * the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS
 * OF ANY KIND, either express or implied. See the License for the specific language
 * governing permissions and limitations under the License.
 */
#ifndef JSRLPP_HPP_4F27FB97E4CD09CA66B36CB4C0C47649
#define JSRLPP_HPP_4F27FB97E4CD09CA66B36CB4C0C47649

#include "jsrl_export.hpp"
#include "jsrl.hpp"
#include <functional>
#include <cassert>
#include <type_traits>
#include <iosfwd>
#include <vector>
#include <string>
#include <utility>

namespace jsrl {
    using std::function;
    using std::is_convertible_v;
    using std::enable_if_t;
    using std::ostream;
    using std::vector;
    using std::string;

    struct BoundJsonPrettyPrint;

    struct JSRL_EXPORT JsonPrettyPrintConfig {
        using KeyOrderer = function<vector<string>(Json::ObjectBody const &)>;

        JsonPrettyPrintConfig()
            : m_add_indent( "  " )
            , m_base_linesep( "\n" )
            , m_comma_separator(",")
            , m_colon_separator(": ")
            , m_object_begin("{")
            , m_empty_object_spacer(" ")
            , m_object_end("}")
            , m_array_begin("[")
            , m_empty_array_spacer(" ")
            , m_array_end("]")
            , m_encode_options( Json::EncodeOptions::TN_EXACT, false, false )
        { }

        /*! @brief  Change the indent string to add to each nesting level.
         */
        auto indent( string add_indent ) && -> JsonPrettyPrintConfig
        {
            m_add_indent = std::move(add_indent);
            return std::move(*this);
        }
        /*! @brief  Change the base indentation used after line breaks.
         */
        auto base( string base_indent ) && -> JsonPrettyPrintConfig
        {
            m_base_linesep = "\n" + std::move(base_indent);
            return std::move(*this);
        }
        /*! @brief  Provide a functor to control key order for printing objects.
         */
        auto order_keys( KeyOrderer key_orderer ) && -> JsonPrettyPrintConfig
        {
            m_key_orderer = std::move(key_orderer);
            return std::move(*this);
        }

        /*! @brief  Simple alternative to @ref order_keys().
         *
         *  This modifier will emit the listed keys first in objects.
         *  Note that this is a simple convenience on top of @ref order_keys(),
         *  which provides more general control over key ordering
         *  (by having an opportunity to examine the object
         *  before emitting the key sequence).
         */
        auto first_keys(
                vector<string> const &early_keys
                ) && -> JsonPrettyPrintConfig
        {
            return std::move(*this).order_keys(
                        [early_keys]( Json::ObjectBody const & ) {
                            return early_keys;
                        } );
        }

        /*! @brief  Orders object keys numerically.
         *
         *  @note   This is a wrapper around @ref order_keys().
         */
        auto numeric_key_order() && -> JsonPrettyPrintConfig;

        auto one_line() && -> JsonPrettyPrintConfig
        {
            m_add_indent = "";
            m_base_linesep = "";
            m_comma_separator = ",";
            m_colon_separator = ":";
            m_empty_array_spacer = "";
            m_empty_object_spacer = "";
            return std::move(*this);
        }
        auto set_comma_spacing(
                string const &before,
                string const &after
                ) && -> JsonPrettyPrintConfig
        {
            m_comma_separator = before + "," + after;
            return std::move(*this);
        }
        auto set_comma_spacing(
                string const &after
                ) && -> JsonPrettyPrintConfig
        {
            return std::move(*this).set_comma_spacing( "", after );
        }
        auto set_colon_spacing(
                string const &before,
                string const &after
                ) && -> JsonPrettyPrintConfig
        {
            m_colon_separator = before + ":" + after;
            return std::move(*this);
        }
        auto set_colon_spacing(
                string const &after
                ) && -> JsonPrettyPrintConfig
        {
            return std::move(*this).set_colon_spacing( "", after );
        }
        auto exact_numbers() && -> JsonPrettyPrintConfig
        {
            m_encode_options.tightness = Json::EncodeOptions::TN_EXACT;
            return std::move(*this);
        }
        auto loose_long_doubles() && -> JsonPrettyPrintConfig
        {
            m_encode_options.tightness = Json::EncodeOptions::TN_LONG_DOUBLE;
            return std::move(*this);
        }
        auto loose_doubles() && -> JsonPrettyPrintConfig
        {
            m_encode_options.tightness = Json::EncodeOptions::TN_DOUBLE;
            return std::move(*this);
        }
        auto loose_floats() && -> JsonPrettyPrintConfig
        {
            m_encode_options.tightness = Json::EncodeOptions::TN_FLOAT;
            return std::move(*this);
        }
        auto replace_bad_utf() && -> JsonPrettyPrintConfig
        {
            m_encode_options.fail_bad_utf8 = false;
            return std::move(*this);
        }
        auto fail_bad_utf() && -> JsonPrettyPrintConfig
        {
            m_encode_options.fail_bad_utf8 = true;
            return std::move(*this);
        }
        auto write_ASCII_strings() && -> JsonPrettyPrintConfig
        {
            m_encode_options.write_utf = false;
            return std::move(*this);
        }
        auto write_utf_strings() && -> JsonPrettyPrintConfig
        {
            m_encode_options.write_utf = true;
            return std::move(*this);
        }

        auto operator()( Json const &json ) const -> BoundJsonPrettyPrint;

        void operator()( Json const &json, ostream &os ) const;

    private:
        void p_print(
                ostream &os,
                Json const &json,
                string const &linesep
                ) const;
        void p_print_key(
                ostream &os,
                Json::ObjectBody::value_type const &entry,
                string const &linesep,
                bool &first
                ) const;

        string m_add_indent;
        string m_base_linesep;
        string m_comma_separator;
        string m_colon_separator;
        string m_object_begin;
        string m_empty_object_spacer;
        string m_object_end;
        string m_array_begin;
        string m_empty_array_spacer;
        string m_array_end;
        KeyOrderer m_key_orderer;
        Json::EncodeOptions m_encode_options;
    };

    struct BoundJsonPrettyPrint {
        BoundJsonPrettyPrint(
                JsonPrettyPrintConfig const &config,
                Json const &json
                )
            : m_config( &config )
            , m_json( &json )
        { }

        /*! @brief  Action function for this proxy adapter.
         *
         *  This operator actually triggers pretty-printing of the Json object
         *  into the passed-in stream.
         */
        template<typename OSTREAM,
                typename = enable_if_t<is_convertible_v<OSTREAM &,ostream &> >
                >
        friend
        auto operator<<(
                OSTREAM &&os,
                BoundJsonPrettyPrint const &self
                ) -> OSTREAM &&
        {
            assert( self.m_config );
            (*self.m_config)( *self.m_json, os );
            return std::forward<OSTREAM>(os);
        }
    private:
        JsonPrettyPrintConfig const *m_config;
        Json const *m_json;
    };

    inline
    auto JsonPrettyPrintConfig::operator()(
            Json const &json
            ) const -> BoundJsonPrettyPrint
    {
        return { *this, json };
    }
    inline
    void JsonPrettyPrintConfig::operator()(
            Json const &json,
            ostream &os
            ) const
    {
        p_print( os, json, m_base_linesep );
    }

    /*! @brief  Helper class returned by @ref jsrl::pretty_print.
     */
    struct JsonPrettyPrint {
        /*! @brief  Constructor, generally invoked by using @ref jsrl::pretty_print().
         */
        JsonPrettyPrint( Json json )
            : m_json( std::move(json) )
        { }

        template<typename... P>
        auto indent( P &&...p ) && -> JsonPrettyPrint {
            m_config = std::move(m_config).indent( std::forward<P>(p)... );
            return std::move(*this);
        }

        template<typename... P>
        auto base( P &&...p ) && -> JsonPrettyPrint {
            m_config = std::move(m_config).base( std::forward<P>(p)... );
            return std::move(*this);
        }

        template<typename... P>
        auto order_keys( P &&...p ) && -> JsonPrettyPrint {
            m_config = std::move(m_config).order_keys( std::forward<P>(p)... );
            return std::move(*this);
        }

        auto first_keys(
                vector<string> const &early_keys
                ) && -> JsonPrettyPrint {
            m_config = std::move(m_config).first_keys( early_keys );
            return std::move(*this);
        }

        auto first_keys(
                vector<string> &&early_keys
                ) && -> JsonPrettyPrint {
            m_config = std::move(m_config).first_keys( std::move(early_keys) );
            return std::move(*this);
        }

        auto numeric_key_order() && -> JsonPrettyPrint {
            m_config = std::move(m_config).numeric_key_order();
            return std::move(*this);
        }

        auto one_line() && -> JsonPrettyPrint {
            m_config = std::move(m_config).one_line();
            return std::move(*this);
        }

        template<typename... P>
        auto set_comma_spacing( P &&...p ) && -> JsonPrettyPrint {
            m_config = std::move(m_config).set_comma_spacing(
                    std::forward<P>(p)... );
            return std::move(*this);
        }

        template<typename... P>
        auto set_colon_spacing( P &&...p ) && -> JsonPrettyPrint {
            m_config = std::move(m_config).set_colon_spacing(
                    std::forward<P>(p)... );
            return std::move(*this);
        }

        auto exact_numbers() && -> JsonPrettyPrint
        {
            m_config = std::move(m_config).exact_numbers();
            return std::move(*this);
        }
        auto loose_long_doubles() && -> JsonPrettyPrint
        {
            m_config = std::move(m_config).loose_long_doubles();
            return std::move(*this);
        }
        auto loose_doubles() && -> JsonPrettyPrint
        {
            m_config = std::move(m_config).loose_doubles();
            return std::move(*this);
        }
        auto loose_floats() && -> JsonPrettyPrint
        {
            m_config = std::move(m_config).loose_floats();
            return std::move(*this);
        }
        auto write_utf_strings() && -> JsonPrettyPrint
        {
            m_config = std::move(m_config).write_utf_strings();
            return std::move(*this);
        }
        auto write_ASCII_strings() && -> JsonPrettyPrint
        {
            m_config = std::move(m_config).write_ASCII_strings();
            return std::move(*this);
        }

        /*! @brief  Action function for this proxy adapter.
         *
         *  This operator actually triggers pretty-printing of the Json object
         *  into the passed-in stream.
         */
        template<
                typename OSTREAM,
                typename = enable_if_t<is_convertible_v<OSTREAM &, ostream &> >
                >
        friend
        auto operator<<(
                OSTREAM &&os,
                JsonPrettyPrint const &self
                ) -> OSTREAM &&
        {
            return std::forward<OSTREAM>(os) << self.m_config(self.m_json);
        }

    private:
        Json m_json;
        JsonPrettyPrintConfig m_config;
    };

    /*! @brief  Pretty-printer interface for JSON.
     *
     *  This function returns a pretty-printer (forwarder) object
     *  for a Json entity.
     *  Usage example:
     *  @code
     *  Json json;
     *  cout << pretty_print( json ) << endl;
     *  @endcode
     */
    inline
    JsonPrettyPrint pretty_print(
            Json json   //!<[in] Json object to pretty-print.
            ) {
        return JsonPrettyPrint(std::move(json));
    }
}
#endif
// vi: et ts=4 sts=4 sw=4
