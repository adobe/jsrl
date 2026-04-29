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
#include "jsrlpp.hpp"
#include <iterator>
#include <ostream>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <utility>

namespace jsrl {
    using std::inserter;
    using std::multimap;
    using std::set;
    using std::vector;
    using std::string;
    using std::transform;

    auto JsonPrettyPrintConfig::numeric_key_order() && -> JsonPrettyPrintConfig
    {
        return std::move(*this).order_keys(
                []( Json::ObjectBody const &object ) {
                    using KeySet = multimap<long double, string>;
                    KeySet keyset;
                    transform( begin(object), end(object),
                        inserter( keyset, end(keyset) ),
                        [](Json::ObjectBody::value_type const &element) {
                            auto [key, value] = element;
                            return make_pair(
                                stold( key ),
                                key );
                        } );
                    vector<string> result;
                    result.reserve( keyset.size() );
                    for ( auto &[numeric_key, string_key] : keyset )
                        result.push_back( std::move(string_key) );
                    return result;
                } );
    }

    // Helper function for printing each individual element of the object.
    // By putting this in a helper function, we can use the same logic
    // to print the priority keys (requested by the client code to come first)
    // and the remaining keys,
    // including keeping track of the first brace and recent comma
    // (through the "first" flag variable).
    void JsonPrettyPrintConfig::p_print_key(
            ostream &os,
            Json::ObjectBody::value_type const &entry,
            string const &linesep,
            bool &first
            ) const
    {
        if ( first ) {
            first = false;
        } else {
            os << m_comma_separator;
        }
        os << linesep;
        auto const &[key, value] = entry;
        Json::write_JSON_string( os, key );
        os << m_colon_separator;
        p_print( os, value, linesep );
    }

    // Workhorse helper function to do the actual pretty-printing.
    // This function is recursive, via recursive invocations
    // of operator<< on sub-elements.
    void JsonPrettyPrintConfig::p_print(
            ostream &os,
            Json const &json,
            string const &linesep
            ) const
    {
        switch ( json.get_typetag(true) ) {
        case Json::TT_OBJECT:
            {
                string new_linesep = linesep + m_add_indent;
                bool first = true;
                set<string> printed;
                auto const &object = json.as_object();
                os << m_object_begin;
                if ( m_key_orderer ) {
                    // Get the generated list of priority keys to put first
                    // in the object printing sequence.
                    for ( auto const &key : m_key_orderer( object ) ) {
                        auto found = find( object, key );
                        // Check that the key actually exists in the object,
                        // and that the key hasn't already been printed.
                        // Then print the found element.
                        if ( found != end( object )
                                && printed.insert( key ).second )
                            p_print_key( os, *found, new_linesep, first );
                    }
                }
                for ( auto const &entry : object ) {
                    // Check that the key wasn't previously printed
                    // in the above loop (from m_key_orderer),
                    // and then print it.
                    if ( printed.find(entry.first) == end( printed ) )
                        p_print_key( os, entry, new_linesep, first );
                }
                os << (first?m_empty_object_spacer:linesep) << m_object_end;
            }
            break;
        case Json::TT_ARRAY:
            {
                string new_linesep = linesep + m_add_indent;
                bool first = true;
                os << m_array_begin;
                for ( auto const &element : json.as_array() ) {
                    if ( first ) {
                        first = false;
                    } else {
                        os << m_comma_separator;
                    }
                    os << new_linesep;
                    p_print( os, element, new_linesep );
                }
                os << (first?m_empty_array_spacer:linesep) << m_array_end;
            }
            break;
        case Json::TT_STRING:
        case Json::TT_NUMBER:
            os << Json::OptionedWrite(json,m_encode_options);
            break;
        default:
            os << json;
        }
    }

}
// vi: et ts=4 sts=4 sw=4
