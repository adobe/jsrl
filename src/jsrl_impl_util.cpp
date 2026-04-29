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
#include "jsrl_impl_util.hpp"
#include "jsrl.hpp"
#include <cctype>

namespace jsrl {
    using std::isspace;

    using UnexpectedByteParseError = Json::UnexpectedByteParseError;
    using BadEOFParseError = Json::BadEOFParseError;
    using UTFParseError = Json::UTFParseError;

    Json::UTFParseError::UTFParseError(
            string const &message
            )
        : ParseError( message )
    { }

    namespace {
        unsigned read_hex_digit( streambuf &sbuf ) {
            int byte = sbuf.sbumpc();
            if ( byte == EOF )
                throw BadEOFParseError( "Input ended in unicode escape" );
            if ( byte >= '0' && byte <= '9' )
                return byte - '0';
            if ( byte >= 'a' && byte <= 'f' )
                return byte - 'a' + 10;
            if ( byte >= 'A' && byte <= 'F' )
                return byte - 'A' + 10;
            sbuf.sungetc();
            throw UnexpectedByteParseError(
                    "Bad hex digit in unicode escape", byte );
        }
        uint16_t read_escaped_codepoint( streambuf &sbuf ) {
            uint16_t codepoint = 0;
            for ( unsigned n = 4; n--; ) {
                codepoint <<= 4;
                codepoint |= read_hex_digit( sbuf );
            }
            return codepoint;
        }
        void add_utf8_seq(
                uint32_t codepoint,
                StringPackager::Make &value,
                uint8_t mask,
                unsigned cont_bytes
                ) {
            value.add_byte( char( uint8_t(codepoint>>(cont_bytes*6)) | mask ) );
            while ( cont_bytes-- ) {
                uint8_t byte = ((codepoint>>(cont_bytes*6)) & 0x3F ) | 0x80;
                value.add_byte( char( byte ) );
            }
        }
        void add_unicode_codepoint(
                streambuf &sbuf,
                StringPackager::Make &value
                ) {
            uint32_t codepoint = read_escaped_codepoint( sbuf );
            if ( ( codepoint & ~0x3FF ) == 0xD800 ) {
                int byte = sbuf.sbumpc();
                if ( byte != '\\' ) {
                    sbuf.sungetc();
                    throw UnexpectedByteParseError( "first half of surrogate",
                            byte );
                }
                byte = sbuf.sbumpc();
                if ( byte != 'u' ) {
                    sbuf.sungetc();
                    throw UnexpectedByteParseError( "first half of surrogate",
                            byte );
                }
                uint16_t second_surrogate = read_escaped_codepoint( sbuf );
                if ( ( second_surrogate & ~0x3FF ) != 0xDC00 ) {
                    throw UTFParseError("bad second half of surrogate");
                }
                codepoint &= 0x03FF;
                codepoint <<= 10;
                codepoint |= (second_surrogate & 0x03FF);
                codepoint += 0x00010000;
            } else if ( ( codepoint & ~0x3FF ) == 0xDC00 ) {
                throw UTFParseError("orphaned second half of surrogate");
            }
            if ( codepoint < 0x00000080 ) {
                value.add_byte( char( uint8_t(codepoint) ) );
            } else if ( codepoint < 0x00000800 ) {
                add_utf8_seq( codepoint, value, 0xC0, 1 );
            } else if ( codepoint < 0x00010000 ) {
                add_utf8_seq( codepoint, value, 0xE0, 2 );
            } else {
                assert( codepoint < 0x00110000 );
                add_utf8_seq( codepoint, value, 0xF0, 3 );
            }
        }
    }

    string read_json_string_value( streambuf &sbuf, StringPackager &sp ) {
        StringPackager::Make value(sp);
        for (;;) {
            int byte = sbuf.sbumpc();
            switch ( byte ) {
            case EOF:
                throw BadEOFParseError( "Input ended within string" );
            case '"':
                return value.package();
            case '\\':
                byte = sbuf.sbumpc();
                switch ( byte ) {
                case '\\':
                case '/':
                case '"': value.add_byte( byte ); break;
                case 'b': value.add_byte( '\b' ); break;
                case 'f': value.add_byte( '\f' ); break;
                case 'n': value.add_byte( '\n' ); break;
                case 'r': value.add_byte( '\r' ); break;
                case 't': value.add_byte( '\t' ); break;
                case 'u': add_unicode_codepoint( sbuf, value ); break;
                default: throw UnexpectedByteParseError(
                                 "Bad escape sequence, \"\\" + ( byte
                                     ? string(1, byte)
                                     : string("<0x00>")
                                     ) + "\"",
                                 char(byte) );
                }
                break;
            default:
                if ( uint8_t(byte) < 0x20 ) {
                    sbuf.sungetc();
                    throw UnexpectedByteParseError(
                            "Control byte within string", char(byte) );
                }
                value.add_byte( byte );
            }
        }
    }

    int jsrl_get_nonspace_byte( streambuf &sbuf ) {
        for (;;) {
            int byte = sbuf.sbumpc();
            if ( byte == EOF )
                return byte;
            if ( isspace( byte ) )
                continue;
            if ( byte != '/' )
                return byte;
            // We just read a '/', so we're at the start of a JSON comment;
            // figure out if it's a block comment or a line comment:
            byte = sbuf.sbumpc();
            switch ( byte ) {
            case EOF:
                throw BadEOFParseError(
                        "Malformed comment start at end of input" );
            case '/':
                do {
                    byte = sbuf.sbumpc();
                    if ( byte == EOF )
                        throw BadEOFParseError( "Premature end of input"
                                " (while reading line-comment)" );
                } while ( byte != '\n' );
                continue;
            case '*':
                byte = '\0';
                for (;;) {
                    int const lastbyte = byte;
                    byte = sbuf.sbumpc();
                    if ( byte == EOF )
                        throw BadEOFParseError( "Incomplete block-comment"
                                " at end of input" );
                    if ( byte == '/' && lastbyte == '*' )
                        break;
                }
                continue;
            default:
                sbuf.sungetc();
                throw UnexpectedByteParseError( "Malformed comment start",
                        char(byte) );
            }
        }
    }

}
// vi: et ts=4 sts=4 sw=4
