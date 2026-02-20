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
#include "jsrl.hpp"
#include "jsrl_impl_util.hpp"
#include <iterator>
#include <sstream>
#include <vector>
#include <algorithm>
#include <istream>
#include <utility>
#include <exception>
#include <limits>
#include <cctype>
#include <cassert>
#include <cstdio>
#include <cmath>
#include <stdint.h>

namespace jsrl {
    using std::reverse_iterator;
    using std::is_sorted;
    using std::reverse;
    using std::vector;
    using std::lower_bound;
    using std::numeric_limits;
    using std::ostringstream;
    using std::ios;
    using std::ostream;
    using std::streamsize;
    using std::streambuf;
    using std::unique;
    using std::stable_sort;
    using std::terminate;
    using std::isdigit;
    using std::isnan;
    using std::shared_ptr;
    using std::make_shared;


    Json::Error::~Error() noexcept = default;

    Json::Error::Error( string const &msg )
        : failure( msg )
    { }

    ostream &operator<<( ostream &os, Json::Error const &self ) {
        os << self.v_failtag() << ": ";
        self.v_print( os );
        return os;
    }

    void Json::Error::v_print( ostream &os ) const {
        os << what();
        if ( os and not m_argument.is_null() ) {
            os << " on ";
            try {
                os << m_argument;
                if ( not os ) {
                    throw EncodeByteError(
                            "Jump into catch block if the write failed." );
                }
            } catch ( Error const & ) {
                os << "<Error formatting argument>";
            }
        }
    }

    Json::TypeError::~TypeError() noexcept = default;

    Json::TypeError::TypeError( string op, char const *actual_type )
        : Error( "Operation " + std::move(op)
                + " not legal for " + actual_type )
        , m_actual_type( actual_type )
    { }

    namespace {
        template<typename T>
        string string_convert( T &&t ) {
            ostringstream oss;
            oss << std::forward<T>(t);
            return std::move(oss).str();
        }
    }

    Json::KeyError::~KeyError() noexcept = default;

    Json::KeyError::KeyError( string const &message )
        : Error( message )
    { }
    Json::ArrayKeyError::ArrayKeyError( size_t key, size_t size )
        : KeyError( "Index " + string_convert(key)
                + " out of range [0.." + string_convert(size) + ")" )
    { }
    Json::ObjectKeyError::ObjectKeyError( string key )
        : KeyError( "Key " + std::move(key) + " not present in object" )
    { }

    struct Json::ElementBase {
        virtual ~ElementBase() = default;

        using EncodeOptions = Json::EncodeOptions;

        using TypeTag = Json::TypeTag;
        using ArrayBody = Json::ArrayBody;
        using ObjectBody = Json::ObjectBody;

        TypeTag get_typetag( bool split_subtype ) const noexcept {
            TypeTag result = v_get_typetag();
            if ( split_subtype )
                return result;
            switch ( result ) {
            default:
                return result;
            case TT_NUMBER_GENERAL:
            case TT_NUMBER_INTEGER:
            case TT_NUMBER_INTEGER_UNSIGNED:
                return TT_NUMBER;
            }
        }
        void write( ostream &os, EncodeOptions encode_options ) const {
            v_write( os, encode_options );
        }
        Json const *find_key(
                std::string_view key
                ) const {
            return v_find_key( key );
        }
        int p_compare( ElementBase const &rhs) const noexcept {
            return v_compare( rhs );
        }
    protected:
        static
        void s_write(
                Json const &json,
                ostream &ost,
                EncodeOptions options
                ) {
            json.p_write( ost, options );
        }
        char const *p_real_type() const noexcept {
            TypeTag const tt = v_get_typetag();
            switch ( tt ) {
            default: assert( tt == TT_NULL ); return "null";
            case TT_BOOL: return "bool";
            case TT_NUMBER: return "number(double)";
            case TT_NUMBER_GENERAL: return "number(general)";
            case TT_NUMBER_INTEGER: return "number(integer)";
            case TT_NUMBER_INTEGER_UNSIGNED: return "number(integer unsigned)";
            case TT_STRING: return "string";
            case TT_ARRAY: return "array";
            case TT_OBJECT: return "object";
            }
        }
    private:
        virtual void v_write( ostream &, EncodeOptions ) const = 0;

        virtual
        int v_compare( ElementBase const &rhs ) const noexcept = 0;

        virtual
        TypeTag v_get_typetag() const noexcept = 0;
#define SETUP_TYPE( TYPENAME, CPPTYPE )                                     \
    public:                                                                 \
        CPPTYPE as_##TYPENAME() const {                                     \
            return v_as_##TYPENAME();                                       \
        }                                                                   \
    private:                                                                \
        virtual CPPTYPE v_as_##TYPENAME() const {                           \
            throw CastTypeError( "as_" #TYPENAME, p_real_type() );          \
        }
        SETUP_TYPE( bool, bool )
        SETUP_TYPE( number_float, long double )
        SETUP_TYPE( number_sint, long long )
        SETUP_TYPE( number_uint, long long unsigned )
        SETUP_TYPE( string, string const & )
        SETUP_TYPE( array, ArrayBody const & )
        SETUP_TYPE( object, ObjectBody const & )
#undef SETUP_TYPE
    public:
        shared_ptr<GeneralNumber const> as_number_general(
                shared_ptr<ElementBase const> const &self
                ) const {
            return v_as_number_general( self );
        }
        GeneralNumber const &downcast_as_number_general() const;
    private:
        virtual shared_ptr<GeneralNumber const> v_as_number_general(
                shared_ptr<ElementBase const> const &
                ) const {
            throw CastTypeError( "as_number_general", p_real_type() );
        }
        virtual Json const *v_find_key(
                std::string_view
                ) const {
            throw CompoundTypeError( "has_key", p_real_type() );
        }
    };
    struct internal_grant {
        using ElementBase = Json::ElementBase;
        static
        int compare( Json lhs, Json rhs ) noexcept {
            return Json::s_compare( std::move(lhs), std::move(rhs) );
        }
    };
    using ElementBase = internal_grant::ElementBase;


    struct JSONElementNull : ElementBase {
        static shared_ptr<JSONElementNull const> const &instance() noexcept {
            static JSONElementNull const s_elem = JSONElementNull();
            static shared_ptr<JSONElementNull const> const
                    sv(shared_ptr<void>(), &s_elem);
            return sv;
        }
    private:
        TypeTag v_get_typetag() const noexcept override {
            return Json::TT_NULL;
        }
        void v_write( ostream &ost, EncodeOptions ) const override {
            ost << "null";
        }

        int v_compare( ElementBase const & ) const noexcept override {
            return 0;
        }
    };

    struct JSONElementBool : ElementBase {
        JSONElementBool( bool value ) : m_value( value ) { }
    private:
        TypeTag v_get_typetag() const noexcept override {
            return Json::TT_BOOL;
        }
        bool v_as_bool() const override { return m_value; }
        void v_write( ostream &ost, EncodeOptions ) const override {
            ost << ( m_value ? "true" : "false" );
        }

        int v_compare( ElementBase const &rhs ) const noexcept override {
            assert( dynamic_cast<JSONElementBool const *>(&rhs) );
            bool const that_value
                    = static_cast<JSONElementBool const*>(&rhs)->m_value;
            return (m_value?1:0) - (that_value?1:0);
        }

        bool m_value;
    };

    namespace {
        inline
        int json_number_compare(
                long long unsigned const lhs,
                long long unsigned const rhs
                ) {
            return lhs < rhs
                    ? -1
                    : lhs > rhs
                    ? 1
                    : 0
                    ;
        }
        inline
        int json_number_compare(
                long long const lhs,
                long long const rhs
                ) {
            return lhs < rhs
                    ? -1
                    : lhs > rhs
                    ? 1
                    : 0
                    ;
        }
        inline
        int json_number_compare(
                long double const lhs,
                long double const rhs
                ) {
            return isnan(lhs)
                    ? isnan(rhs)
                    ? 0
                    : 1
                    : isnan(rhs)
                    ? -1
                    : lhs < rhs
                    ? -1
                    : lhs > rhs
                    ? 1
                    : 0
                    ;
        }
        inline
        int json_number_compare(
                long long unsigned const lhs,
                long long const rhs
                ) {
            return rhs < 0
                    ? 1
                    : json_number_compare( lhs,
                            static_cast<long long unsigned>(rhs) )
                    ;
        }
        inline
        int json_number_compare(
                long long const lhs,
                long long unsigned const rhs
                ) {
            return - json_number_compare( rhs, lhs );
        }
        inline
        int json_number_compare(
                long double const lhs,
                long long unsigned const rhs
                ) {
            long double const rhs_ld = rhs;
            long long unsigned const lhs_llu
                    = static_cast<long long unsigned>(lhs);
            if ( lhs != rhs_ld ) {
                if ( lhs < rhs_ld )
                    return -1;
                else
                    return 1;
            } else if ( lhs_llu != rhs ) {
                if ( lhs_llu < rhs )
                    return -1;
                else
                    return 1;
            } else {
                return 1; // double compares after equal int
            }
        }
        inline
        int json_number_compare(
                long long unsigned const lhs,
                long double const rhs
                ) {
            return - json_number_compare( rhs, lhs );
        }
        inline
        int json_number_compare(
                long double const lhs,
                long long const rhs
                ) {
            long double const rhs_ld = rhs;
            long long const lhs_ll = static_cast<long long>(lhs);
            if ( lhs != rhs_ld ) {
                if ( lhs < rhs_ld )
                    return -1;
                else
                    return 1;
            } else if ( lhs_ll != rhs ) {
                if ( lhs_ll < rhs ) {
                    return -1;
                } else {
                    return 1;
                }
            } else {
                return 1; // double compares after equal int
            }
        }
        inline
        int json_number_compare(
                long long const lhs,
                long double const rhs
                ) {
            return - json_number_compare( rhs, lhs );
        }
        inline
        int json_number_compare(
                GeneralNumber const &lhs,
                GeneralNumber const &rhs
                ) {
            return cmp3way( lhs, rhs );
        }
#define JSON_NUMBER_COMPARE_WITH_GN( TYPE )                                 \
        static inline                                                       \
        int json_number_compare(                                            \
                GeneralNumber const &lhs,                                   \
                TYPE const rhs                                              \
                )                                                           \
        {                                                                   \
            return json_number_compare( lhs, GeneralNumber( rhs ) );        \
        }                                                                   \
        static inline                                                       \
        int json_number_compare(                                            \
                TYPE const lhs,                                             \
                GeneralNumber const &rhs                                    \
                )                                                           \
        {                                                                   \
            return - json_number_compare( rhs, lhs );                       \
        }
        JSON_NUMBER_COMPARE_WITH_GN( long double )
        JSON_NUMBER_COMPARE_WITH_GN( long long )
        JSON_NUMBER_COMPARE_WITH_GN( long long unsigned )
#undef JSON_NUMBER_COMPARE_WITH_GN
    }

    struct JSONElementNumberCommon : ElementBase {
        template<typename N>
        static
        int s_compare_helper( N const &n, ElementBase const &rhs ) {
            switch ( rhs.get_typetag( true ) ) {
            case Json::TT_NUMBER_INTEGER_UNSIGNED:
                return json_number_compare(n, rhs.as_number_uint());
            case Json::TT_NUMBER_INTEGER:
                return json_number_compare(n, rhs.as_number_sint());
            case Json::TT_NUMBER_GENERAL:
                return json_number_compare(n, rhs.downcast_as_number_general());
            case Json::TT_NUMBER:
                return json_number_compare(n, rhs.as_number_float());
            default:
                assert(false);
                terminate();
            }
        }
    };

    struct JSONElementNumberIntegerUint : JSONElementNumberCommon {
        JSONElementNumberIntegerUint( long long unsigned value )
            : m_value( value )
        { }
    private:
        TypeTag v_get_typetag() const noexcept override {
            return Json::TT_NUMBER_INTEGER_UNSIGNED;
        }
        long double v_as_number_float() const override { return m_value; }
        shared_ptr<GeneralNumber const> v_as_number_general(
                shared_ptr<ElementBase const> const &
                ) const override {
            return make_shared<GeneralNumber const>(
                    GeneralNumber( m_value ) );
        }
        long long v_as_number_sint() const override {
            static long long unsigned const limval
                    = numeric_limits<long long>::max();
            if ( m_value > limval )
                return limval;
            return m_value;
        }
        long long unsigned v_as_number_uint() const override {
            return m_value;
        }
        void v_write( ostream &ost, EncodeOptions ) const override {
            ost << m_value;
        }

        int v_compare( ElementBase const &rhs ) const noexcept override {
            return s_compare_helper( m_value, rhs );
        }

        long long unsigned m_value;
    };

    struct JSONElementNumberInteger : JSONElementNumberCommon {
        JSONElementNumberInteger( long long value ) : m_value( value ) { }
    private:
        TypeTag v_get_typetag() const noexcept override {
            return Json::TT_NUMBER_INTEGER;
        }
        long double v_as_number_float() const override { return m_value; }
        shared_ptr<GeneralNumber const> v_as_number_general(
                shared_ptr<ElementBase const> const &
                ) const override {
            return make_shared<GeneralNumber const>(
                    GeneralNumber( m_value ) );
        }
        long long v_as_number_sint() const override {
            return m_value;
        }
        void v_write( ostream &ost, EncodeOptions ) const override {
            ost << m_value;
        }

        int v_compare( ElementBase const &rhs ) const noexcept override {
            return s_compare_helper( m_value, rhs );
        }

        long long m_value;
    };

    namespace {
        struct FormatFloatStream {
            FormatFloatStream( ostream &ost, size_t precision )
                : m_st( &ost )
                , m_oldprecision( m_st->precision( precision ) )
            {
            }
            ~FormatFloatStream() {
                m_st->precision( m_oldprecision );
            }
        private:
            ostream *m_st;
            int m_oldprecision;

            FormatFloatStream(FormatFloatStream const &);
            FormatFloatStream &operator=(FormatFloatStream const &);
        };
    }

    struct JSONElementNumberDouble : JSONElementNumberCommon {
        JSONElementNumberDouble( long double value, short unsigned sigdigs )
            : m_value( value )
            , m_sigdigs( sigdigs )
        { }
    private:
        TypeTag v_get_typetag() const noexcept override {
            return Json::TT_NUMBER;
        }
        long double v_as_number_float() const override { return m_value; }
        shared_ptr<GeneralNumber const> v_as_number_general(
                shared_ptr<ElementBase const> const &
                ) const override {
            ostringstream repr;
            v_write( repr, EncodeOptions(EncodeOptions::TN_EXACT,false,false) );
            return make_shared<GeneralNumber const>(
                    GeneralNumber::parse( repr.str() ) );
        }

        static
        int s_encode_options_digits10(
                EncodeOptions const &encode_options
                ) {
            switch ( encode_options.tightness ) {
            case EncodeOptions::TN_FLOAT:
                return numeric_limits<float>::digits10;
            case EncodeOptions::TN_DOUBLE:
                return numeric_limits<double>::digits10;
            case EncodeOptions::TN_LONG_DOUBLE:
                return numeric_limits<long double>::digits10;
            default:
                return numeric_limits<long double>::max_digits10;
            }
        }
        void v_write(
                ostream &ost,
                EncodeOptions encode_options
                ) const override {
            int const digits10 = m_sigdigs
                    ? m_sigdigs
                    : s_encode_options_digits10( encode_options )
                    ;
            FormatFloatStream ffp( ost, digits10 );
            ost << m_value;
        }

        int v_compare( ElementBase const &rhs ) const noexcept override {
            return s_compare_helper( m_value, rhs );
        }

        long double m_value;
        // Number of significant digits read.  (0 means unavailable.)
        short unsigned m_sigdigs;
    };

    struct JSONElementNumberGeneral : JSONElementNumberCommon {
        JSONElementNumberGeneral( GeneralNumber value )
            : m_value( std::move(value) )
        { }
        using ElementBase::as_number_general;
        GeneralNumber const &as_number_general() const {
            return m_value;
        }
    private:
        TypeTag v_get_typetag() const noexcept override {
            return Json::TT_NUMBER_GENERAL;
        }
        long double v_as_number_float() const override {
            return m_value.as_long_double();
        }
        shared_ptr<GeneralNumber const> v_as_number_general(
                shared_ptr<ElementBase const> const &self
                ) const override {
            assert( self );
            assert( this == self.get() );
            return shared_ptr<GeneralNumber const>(
                    self, &as_number_general() );
        }
        long long v_as_number_sint() const override {
            if ( m_value.is_decimal() ) {
                throw Json::CastTypeError(
                        "as_number_sint(NON-INT)", p_real_type() );
            }
            return m_value.as_long_long();
        }
        long long unsigned v_as_number_uint() const override {
            if ( m_value.is_decimal() ) {
                throw Json::CastTypeError(
                        "as_number_uint(NON-INT)", p_real_type() );
            }
            return m_value.as_long_long_unsigned();
        }
        void v_write( ostream &ost, EncodeOptions ) const override {
            ost << m_value;
        }

        int v_compare( ElementBase const &rhs ) const noexcept override {
            return s_compare_helper( m_value, rhs );
        }

        GeneralNumber m_value;
    };
    GeneralNumber const &ElementBase::downcast_as_number_general() const {
        assert( dynamic_cast<JSONElementNumberGeneral const*>(this) );
        return static_cast<JSONElementNumberGeneral const*>(this)
                ->as_number_general();
    }

    Json::EncodeError::~EncodeError() noexcept = default;

    Json::EncodeError::EncodeError( string const &message )
        : Error(message)
    { }

    Json::EncodeByteError::EncodeByteError( string const &message )
        : EncodeError(message)
    { }

    Json::EncodeCodepointError::EncodeCodepointError( string const &message )
        : EncodeError(message)
    { }

    namespace {
        struct FormatHexStream {
            FormatHexStream( ostream &ost, size_t width )
                : m_st( &ost )
                , m_oldflags( ost.setf( ios::hex, ios::basefield ) )
                , m_oldfill( ost.fill( '0' ) )
                , m_oldwidth( ost.width( width ) )
            {
            }
            ~FormatHexStream() {
                m_st->width( m_oldwidth );
                m_st->fill( m_oldfill );
                m_st->setf( m_oldflags, ios::basefield );
            }
        private:
            ostream *m_st;
            ios::fmtflags m_oldflags;
            char m_oldfill;
            int m_oldwidth;

            FormatHexStream(FormatHexStream const &);
            FormatHexStream &operator=(FormatHexStream const &);
        };
        void write_codeunit( ostream &ost, uint16_t codeunit) {
            ost << "\\u";
            FormatHexStream fhs( ost, 4 );
            ost << codeunit;
        }
        inline
        bool byte_is_utf8_continuation( uint8_t byte ) {
            return (byte & 0xC0) == 0x80;
        }
        uint32_t scan_continuation_bytes(
                uint32_t primer,
                unsigned count,
                char const *&cur,
                char const *const end
                ) {
            while ( count-- ) {
                if ( cur == end )
                    throw Json::EncodeByteError(
                            "String ends in the middle of a UTF-8 sequence" );
                uint8_t const byte = *cur;
                if ( not byte_is_utf8_continuation(byte) )
                    throw Json::EncodeByteError(
                            "Continuation byte missing in UTF-8 sequence" );
                ++cur;
                primer <<= 6;
                primer or_eq byte & 0x3F;
            }
            return primer;
        }
        uint32_t scan_utf8_codepoint(
                char const *&cur,
                char const *const end
                ) {
            assert( cur != end );
            uint8_t byte = *cur++;
            if ( byte < 0x80 ) {
                return byte;
            } else if ( byte < 0xC0 ) {
                throw Json::EncodeByteError(
                        "Unexpected UTF-8 continuation byte" );
            } else if ( byte < 0xE0 ) {
                return scan_continuation_bytes( byte & 0x1F, 1, cur, end );
            } else if ( byte < 0xF0 ) {
                return scan_continuation_bytes( byte & 0x0F, 2, cur, end );
            } else if ( byte < 0xF8 ) {
                return scan_continuation_bytes( byte & 0x07, 3, cur, end );
            } else {
                throw Json::EncodeByteError( "Bad UTF-8 start byte" );
            }
        }
        uint32_t scan_valid_utf8_codepoint(
                char const *&cur,
                char const *const end
                ) {
            uint32_t codepoint = scan_utf8_codepoint( cur, end );
            if ( codepoint >= 0xD800 ) {
                if ( codepoint > 0x0010FFFF )
                    throw Json::EncodeCodepointError(
                            "Codepoint out of 0x10FFFF range" );
                if ( codepoint <= 0xDFFF )
                    throw Json::EncodeCodepointError(
                            "UTF-8-encoded surrogate half" );
            }
            return codepoint;
        }
        uint32_t scan_replace_valid_utf8_codepoint(
                char const *&cur,
                char const *const end,
                bool fail_bad_utf8
                ) {
            try {
                return scan_valid_utf8_codepoint( cur, end );
            } catch ( Json::EncodeByteError const & ) {
                if ( fail_bad_utf8 )
                    throw;
                while ( cur != end and byte_is_utf8_continuation( *cur ) )
                    ++cur;
                return 0xFFFD;
            } catch ( Json::EncodeCodepointError const & ) {
                if ( fail_bad_utf8 )
                    throw;
                return 0xFFFD;
            }
        }
        template<bool WRITE_UTF, typename STRINGISH>
        void write_JSON_string_UTF(
                ostream &ost,
                STRINGISH const &value,
                bool fail_bad_utf8
                ) {
            ost << "\"";
            char const
                    *cur = value.data(),
                    *const end = cur + value.size();
            while ( cur != end ) {
                switch ( *cur ) {
                case '\\': ++cur; ost << "\\\\"; break;
                case '\"': ++cur; ost << "\\\""; break;
                case '\b': ++cur; ost << "\\b"; break;
                case '\f': ++cur; ost << "\\f"; break;
                case '\n': ++cur; ost << "\\n"; break;
                case '\r': ++cur; ost << "\\r"; break;
                case '\t': ++cur; ost << "\\t"; break;
                default:
                    {
                        if ( WRITE_UTF ) {
                            auto start = cur;
                            uint32_t codepoint
                                    = scan_replace_valid_utf8_codepoint(
                                        cur, end, fail_bad_utf8 );
                            if ( codepoint < 0x20 ) {
                                write_codeunit( ost, codepoint );
                            } else {
                                do {
                                    assert( start < cur );
                                    ost << char( *start );
                                } while ( ++start != cur );
                            }
                        } else {
                            uint32_t codepoint
                                    = scan_replace_valid_utf8_codepoint(
                                        cur, end, fail_bad_utf8 );
                            if ( codepoint < 0x20 ) {
                                write_codeunit( ost, codepoint );
                            } else if ( codepoint < 0x80 ) {
                                ost << char(codepoint);
                            } else {
                                if ( codepoint <= 0xFFFF ) {
                                    assert( (codepoint & ~0x7FF) != 0xD800 );
                                    write_codeunit( ost, codepoint );
                                } else {
                                    assert( codepoint <= 0x0010FFFF );
                                    uint32_t const mod_codepoint
                                            = codepoint - 0x00010000;
                                    write_codeunit( ost,
                                            ((mod_codepoint>>10) & 0x3FF)
                                                | 0xD800 );
                                    write_codeunit( ost,
                                            (mod_codepoint & 0x3FF)
                                                | 0xDC00 );
                                }
                            }
                        }
                    }
                }
            }
            ost << "\"";
        }
    }
    void validate_utf8( char const *c, char const *const e ) {
        bool const nullterminated = not e;
        while ( nullterminated ? *c : c != e )
            scan_valid_utf8_codepoint( c, e );
    }

    void Json::write_JSON_string(
            ostream &ost,
            string const &value,
            bool fail_bad_utf8,
            bool write_utf
            ) {
        if ( write_utf )
            return write_JSON_string_UTF<true>( ost, value, fail_bad_utf8 );
        else
            return write_JSON_string_UTF<false>( ost, value, fail_bad_utf8 );
    }

    void Json::write_JSON_string(
            ostream &ost,
            string_view const &value,
            bool fail_bad_utf8,
            bool write_utf
            ) {
        if ( write_utf )
            return write_JSON_string_UTF<true>( ost, value, fail_bad_utf8 );
        else
            return write_JSON_string_UTF<false>( ost, value, fail_bad_utf8 );
    }

    struct JSONElementString : ElementBase {
        JSONElementString( string value )
            : m_value( std::move(value) )
        {
            validate_utf8( m_value );
        }
        JSONElementString( string value, Json::ignore_bad_unicode_t )
            : m_value( std::move(value) )
        { }
    private:
        TypeTag v_get_typetag() const noexcept override {
            return Json::TT_STRING;
        }
        string const &v_as_string() const override { return m_value; }
        void v_write(
                ostream &ost,
                EncodeOptions encode_options
                ) const override {
            Json::write_JSON_string( ost, m_value,
                    encode_options.fail_bad_utf8,
                    encode_options.write_utf );
        }

        int v_compare( ElementBase const &rhs ) const noexcept override {
            assert( dynamic_cast<JSONElementString const *>(&rhs) );
            string const &that_value
                    = static_cast<JSONElementString const*>(&rhs)->m_value;
            string::const_iterator const
                    lhend = m_value.end(),
                    rhend = that_value.end();
            string::const_iterator
                    lhch = m_value.begin(),
                    rhch = that_value.begin();
            for (;;) {
                if ( lhch == lhend ) {
                    if ( rhch == rhend ) {
                        return 0;
                    } else {
                        return -1;
                    }
                } else if ( rhch == rhend ) {
                    return 1;
                }
                if ( *lhch != *rhch ) {
                    return *lhch < *rhch
                           ? -1
                           : 1;
                }
                ++lhch;
                ++rhch;
            }
        }

        string m_value;
    };

    struct JSONElementArray : ElementBase {
        JSONElementArray( ArrayBody value ) : m_value(std::move(value)) { }
    private:
        TypeTag v_get_typetag() const noexcept override {
            return Json::TT_ARRAY;
        }
        ArrayBody const &v_as_array() const override { return m_value; }

        void v_write(
                ostream &ost,
                EncodeOptions encode_options
                ) const override {
            ost << "[";
            bool first = true;
            ArrayBody::const_iterator const value_end = m_value.end();
            for ( ArrayBody::const_iterator it = m_value.begin()
                    ; it!=value_end
                    ; ++it ) {
                if ( first )
                    first = false;
                else
                    ost << ",";
                s_write( *it, ost, encode_options );
            }
            ost << "]";
        }

        int v_compare( ElementBase const &rhs ) const noexcept override {
            assert( dynamic_cast<JSONElementArray const *>(&rhs) );
            ArrayBody const &that_value
                    = static_cast<JSONElementArray const*>(&rhs)->m_value;
            ArrayBody::const_iterator const
                    lhend = m_value.end(),
                    rhend = that_value.end();
            ArrayBody::const_iterator
                    lhch = m_value.begin(),
                    rhch = that_value.begin();
            for (;;) {
                if ( lhch == lhend ) {
                    if ( rhch == rhend )
                        return 0;
                    else
                        return -1;
                } else if ( rhch == rhend ) {
                    return 1;
                }
                int result = internal_grant::compare( *lhch, *rhch );
                if ( result )
                    return result;
                ++lhch;
                ++rhch;
            }
        }

        ArrayBody m_value;
    };

    namespace {
        struct CmpLt {
            bool operator()(
                    pair<string,Json> const &lhs,
                    std::string_view rhs
                    ) const {
                return lhs.first < rhs;
            }
            bool operator()(
                    std::string_view lhs,
                    pair<string,Json> const &rhs
                    ) const {
                return lhs < rhs.first;
            }
            bool operator()(
                    pair<string,Json> const &lhs,
                    pair<string,Json> const &rhs
                    ) const {
                return lhs.first < rhs.first;
            }
        };
        struct CmpEq {
            bool operator()(
                    pair<string,Json> const &lhs,
                    pair<string,Json> const &rhs
                    ) const {
                return lhs.first == rhs.first;
            }
        };
        template<typename Cmp>
        struct CmpNot : private Cmp {
            template<typename P1, typename P2>
            bool operator()( P1 const &p1, P2 const &p2 ) {
                return not Cmp::operator()( p1, p2 );
            }
        };
        template<typename Comp, typename Iter>
        bool is_strictly_ascending( Iter const &r_begin, Iter const &r_end ) {
            using RI = reverse_iterator<Iter>;
            return is_sorted( RI(r_end), RI(r_begin), CmpNot<Comp>() );
        }
    }
    void resort( Json::ObjectBody &body ) {
        Json::ObjectBody::iterator mbegin = body.begin(), mend = body.end();
        if ( not is_strictly_ascending<CmpLt>(mbegin, mend) ) {
            reverse( mbegin, mend );
            stable_sort( mbegin, mend, CmpLt() );
            body.erase( unique( mbegin, mend, CmpEq() ), mend );
        }
    }

    struct JSONElementObject : ElementBase {
        JSONElementObject(ObjectBody value)
            : m_value(std::move(value))
        {
            resort( m_value );
        }
    private:
        TypeTag v_get_typetag() const noexcept override {
            return Json::TT_OBJECT;
        }
        ObjectBody const &v_as_object() const override {
            return m_value;
        }

        Json const *v_find_key( std::string_view key ) const override {
            ObjectBody::const_iterator found = find( m_value, key );
            if ( found == m_value.end() )
                return nullptr;
            return &found->second;
        }

        void v_write(
                ostream &ost,
                EncodeOptions encode_options
                ) const override {
            ost << "{";
            bool first = true;
            ObjectBody::const_iterator const value_end = m_value.end();
            for ( ObjectBody::const_iterator it = m_value.begin()
                    ; it!=value_end
                    ; ++it ) {
                if ( first )
                    first = false;
                else
                    ost << ",";
                Json::write_JSON_string( ost, it->first,
                        encode_options.fail_bad_utf8,
                        encode_options.write_utf );
                ost << ":";
                s_write( it->second, ost, encode_options );
            }
            ost << "}";
        }

        int v_compare( ElementBase const &rhs ) const noexcept override {
            assert( dynamic_cast<JSONElementObject const *>(&rhs) );
            ObjectBody const &that_value
                    = static_cast<JSONElementObject const*>(&rhs)->m_value;
            ObjectBody::const_iterator const
                    lhend = m_value.end(),
                    rhend = that_value.end();
            ObjectBody::const_iterator
                    lhch = m_value.begin(),
                    rhch = that_value.begin();
            for (;;) {
                if ( lhch == lhend ) {
                    if ( rhch == rhend )
                        return 0;
                    else
                        return -1;
                } else if ( rhch == rhend ) {
                    return 1;
                }
                if ( lhch->first != rhch->first ) {
                    if ( lhch->first < rhch->first )
                        return -1;
                    else
                        return 1;
                }
                int result = internal_grant::compare(lhch->second,rhch->second);
                if ( result )
                    return result;
                ++lhch;
                ++rhch;
            }
        }

        ObjectBody m_value;
    };

    Json::ignore_bad_unicode_t const Json::ignore_bad_unicode = {};

    Json::Json( Json &&that ) noexcept
        : m_el(JSONElementNull::instance())
    {
        swap(*this, that);
    }
    Json &Json::operator=( Json &&that ) noexcept {
        swap(*this, that);
        return *this;
    }

    Json::Json() noexcept
        : m_el(JSONElementNull::instance())
    {
    }

    void swap( Json &lhs, Json &rhs ) noexcept {
        using std::swap;
        swap( lhs.m_el, rhs.m_el );
    }

    namespace {
        template<typename T>
        shared_ptr<ElementBase> make_signed_integer_element( T num ) {
            if ( num < 0 )
                return make_shared<JSONElementNumberInteger>(num);
            else
                return make_shared<JSONElementNumberIntegerUint>(num);
        }
    }

    Json::Json( bool value )
        : m_el( make_shared<JSONElementBool>( value ) ) { }
    Json::Json( GeneralNumber value )
        : m_el( make_shared<JSONElementNumberGeneral>( std::move(value) ) ) { }
    Json::Json( long double value )
        : m_el( make_shared<JSONElementNumberDouble>( value, 0 ) ) { }
    Json::Json( long double value, short unsigned sigdigs )
        : m_el( make_shared<JSONElementNumberDouble>( value, sigdigs ) ) {}
    Json::Json( double value )
        : m_el( make_shared<JSONElementNumberDouble>( value, 0 ) ) { }
    Json::Json( float value )
        : m_el( make_shared<JSONElementNumberDouble>( value, 0 ) ) { }
    Json::Json( long long value )
        : m_el( make_signed_integer_element( value ) ) { }
    Json::Json( long value )
        : m_el( make_signed_integer_element( value ) ) { }
    Json::Json( int value )
        : m_el( make_signed_integer_element( value ) ) { }
    Json::Json( short value )
        : m_el( make_signed_integer_element( value ) ) { }
    Json::Json( long long unsigned value )
        : m_el( make_shared<JSONElementNumberIntegerUint>( value ) ) { }
    Json::Json( long unsigned value )
        : m_el( make_shared<JSONElementNumberIntegerUint>( value ) ) { }
    Json::Json( int unsigned value )
        : m_el( make_shared<JSONElementNumberIntegerUint>( value ) ) { }
    Json::Json( short unsigned value )
        : m_el( make_shared<JSONElementNumberIntegerUint>( value ) ) { }
    Json::Json( char const *value )
        : m_el( make_shared<JSONElementString>( string(value) ) ) { }
    Json::Json( string value )
        : m_el( make_shared<JSONElementString>( std::move(value) ) ) { }
    Json::Json( char const *value, ignore_bad_unicode_t )
        : m_el( make_shared<JSONElementString>( string(value),
                    ignore_bad_unicode ) )
    { }
    Json::Json( string value, ignore_bad_unicode_t )
        : m_el( make_shared<JSONElementString>( std::move(value),
                    ignore_bad_unicode ) ) { }
    Json::Json( ArrayBody value )
        : m_el( make_shared<JSONElementArray>( std::move(value) ) ) { }
    Json::Json( map<string,Json> const &value )
        : m_el( make_shared<JSONElementObject>(
                    ObjectBody( value.begin(), value.end() ) ) )
    { }
    Json::Json( map<string,string> const &value )
            : m_el( make_shared<JSONElementObject>(
                    ObjectBody( value.begin(), value.end() ) ) )
    { }
    Json::Json( ObjectBody value )
        : m_el( make_shared<JSONElementObject>( std::move(value) ) ) { }

    Json::ElementBasePtr Json::s_make_ArrayBodyPtr( ArrayBody a ) {
        return make_shared<JSONElementArray>( std::move(a) );
    }
    Json::ElementBasePtr Json::s_make_ObjectBodyPtr( ObjectBody o ) {
        return make_shared<JSONElementObject>( std::move(o) );
    }

    Json::TypeTag Json::get_typetag( bool split_subtype ) const noexcept {
        return m_el->get_typetag( split_subtype );
    }

    bool Json::as_bool() const {
        try {
            return m_el->as_bool();
        } catch ( Error &e ) {
            e.set_argument( *this );
            throw;
        }
    }
    long double Json::as_number_float() const {
        try {
            return m_el->as_number_float();
        } catch ( Error &e ) {
            e.set_argument( *this );
            throw;
        }
    }
    shared_ptr<GeneralNumber const> Json::as_number_general() const {
        try {
            return m_el->as_number_general( m_el );
        } catch ( Error &e ) {
            e.set_argument( *this );
            throw;
        }
    }
    long long Json::as_number_sint() const {
        try {
            return m_el->as_number_sint();
        } catch ( Error &e ) {
            e.set_argument( *this );
            throw;
        }
    }
    long long unsigned Json::as_number_uint() const {
        try {
            return m_el->as_number_uint();
        } catch ( Error &e ) {
            e.set_argument( *this );
            throw;
        }
    }
    string const &Json::as_string() const {
        try {
            return m_el->as_string();
        } catch ( Error &e ) {
            e.set_argument( *this );
            throw;
        }
    }
    Json::ArrayBody const &Json::as_array() const {
        try {
            return m_el->as_array();
        } catch ( Error &e ) {
            e.set_argument( *this );
            throw;
        }
    }
    Json::ObjectBody const &Json::as_object() const {
        try {
            return m_el->as_object();
        } catch ( Error &e ) {
            e.set_argument( *this );
            throw;
        }
    }
    map<string,Json> Json::as_map_object() const {
        Json::ObjectBody const &o = as_object();
        return map<string,Json>(o.begin(), o.end());
    }

    Json const *Json::find_key( std::string_view key ) const {
        return m_el->find_key( key );
    }
    Json const &Json::operator[](std::string_view key) const {
        try {
            Json const *element = m_el->find_key( key );
            if ( not element )
                throw Json::ObjectKeyError( string(key) );
            return *element;
        } catch ( TypeError const &e ) {
            throw CompoundTypeError( "operator[](string)", e.actual_type() );
        }
    }


    template<>
    shared_ptr<string const>
            Json::p_extract_body_ptr<string>(
                    ElementBasePtr const &el
                    ) {
        using StringSharedPtr = shared_ptr<string const>;
        return StringSharedPtr( el, &el->as_string() );
    }
    template<>
    shared_ptr<Json::ArrayBody const>
            Json::p_extract_body_ptr<Json::ArrayBody>(
                ElementBasePtr const &el
                ) {
        using ArraySharedPtr = shared_ptr<ArrayBody const>;
        return ArraySharedPtr( el, &el->as_array() );
    }
    template<>
    shared_ptr<Json::ObjectBody const>
            Json::p_extract_body_ptr<Json::ObjectBody>(
                ElementBasePtr const &el
                ) {
        using ObjectSharedPtr = shared_ptr<ObjectBody const>;
        return ObjectSharedPtr( el, &el->as_object() );
    }

    template<typename T>
    Json::JsonBodyPtr<T>::JsonBodyPtr( T body )
        : m_el( Json(std::move(body)).m_el )
        , m_ptr( p_extract_body_ptr<T>(m_el) )
    { }
    template<typename T>
    Json::JsonBodyPtr<T>::JsonBodyPtr( ElementBasePtr ptr, Ptr m_ptr )
        : m_el( std::move(ptr) )
        , m_ptr( std::move(m_ptr) )
    { }
    template struct Json::JsonBodyPtr<string>;
    template struct Json::JsonBodyPtr<Json::ArrayBody>;
    template struct Json::JsonBodyPtr<Json::ObjectBody>;

    Json::StringPtr Json::as_string_ptr() const {
        try {
            return StringPtr( m_el, p_extract_body_ptr<string>(m_el) );
        } catch ( Error &e ) {
            e.set_argument( *this );
            throw;
        }
    }

    Json::ArrayPtr Json::as_array_ptr() const {
        try {
            return ArrayPtr( m_el, p_extract_body_ptr<ArrayBody>(m_el) );
        } catch ( Error &e ) {
            e.set_argument( *this );
            throw;
        }
    }

    Json::ObjectPtr Json::as_object_ptr() const {
        try {
            return ObjectPtr( m_el, p_extract_body_ptr<ObjectBody>(m_el) );
        } catch ( Error &e ) {
            e.set_argument( *this );
            throw;
        }
    }

    void Json::p_write(
            ostream &ost,
            EncodeOptions encode_options
            ) const {
        try {
            m_el->write( ost, encode_options );
        } catch ( Error &e ) {
            e.set_argument( *this );
            throw;
        }
    }

    int Json::s_compare( Json lhs, Json rhs ) noexcept {
        if ( lhs.m_el == rhs.m_el )
            return 0;
        TypeTag lhs_tt = lhs.get_typetag(false);
        TypeTag rhs_tt = rhs.get_typetag(false);
        if ( lhs_tt == rhs_tt )
            return lhs.m_el->p_compare( *rhs.m_el );
        return lhs_tt < rhs_tt ? -1 : 1;
    }

    Json::ParseError::~ParseError() noexcept = default;

    Json::ParseError::ParseError( string const &message )
        : Error( message )
    { }

    void Json::ParseError::v_print( ostream &os ) const {
        Error::v_print( os );
        if ( not m_context.empty() )
            write_JSON_string( os << " before ", m_context );
    }
    void Json::ParseError::add_context( streambuf &sbuf, size_t max_bytes ) {
        while ( max_bytes-- ) {
            int byte = sbuf.sbumpc();
            if ( byte == EOF )
                break;
            m_context.append( 1, char(byte) );
        }
    }
    void Json::ParseError::add_context( istream &is, size_t max_bytes ) {
        add_context( *is.rdbuf(), max_bytes );
    }

    Json::NumberParseError::NumberParseError(
            string const &message
            )
        : ParseError( message )
    { }
    using NumberParseError = Json::NumberParseError;

    Json::TrailingCommaParseError::TrailingCommaParseError(
            string const &container_type
            )
        : ParseError( "Trailing comma in " + container_type )
    { }
    using TrailingCommaParseError = Json::TrailingCommaParseError;

    void Json::UnexpectedByteParseError::v_print( ostream &os ) const {
        ParseError::v_print( os );
        write_JSON_string( os << " while reading byte ", string(1,get_got()) );
    }
    using UnexpectedByteParseError = Json::UnexpectedByteParseError;

    using TrailingBytesParseError = Json::TrailingBytesParseError;

    Json::EOFParseError::~EOFParseError() noexcept = default;


    Json::EOFParseError::EOFParseError(
            string const &message
            )
        : ParseError( message )
    { }
    Json::StartEOFParseError::StartEOFParseError(
            string const &message
            )
        : EOFParseError( message )
    { }
    using StartEOFParseError = Json::StartEOFParseError;
    Json::BadEOFParseError::BadEOFParseError(
            string const &message
            )
        : EOFParseError( message )
    { }
    using BadEOFParseError = Json::BadEOFParseError;

    namespace {

        Json read_internal_json(
                streambuf &sbuf,
                StringPackager &sp,
                bool use_GN_for_floats
                );

        char get_separator_byte( streambuf &sbuf, bool eof_okay ) {
            int byte = jsrl_get_nonspace_byte( sbuf );
            switch ( byte ) {
            case EOF:
                if ( eof_okay )
                    throw StartEOFParseError( "Premature end of input" );
                else
                    throw BadEOFParseError( "Premature end of input" );
            default:
                return byte;
            }
        }
        char peek_separator_byte( streambuf &sbuf ) {
            char byte = get_separator_byte( sbuf, false );
            sbuf.sungetc();
            return byte;
        }

        void eat_word_rmdr( streambuf &sbuf, char const *word ) {
            assert( *word );
            char const *remainder = word+1;
            assert( *remainder );
            do {
                int const byte = sbuf.sbumpc();
                if ( byte == EOF ) {
                    throw BadEOFParseError( string()
                            + "Input ended while looking for \""
                            + word + "\"" );
                }
                if ( byte != *remainder ) {
                    sbuf.sungetc();
                    throw UnexpectedByteParseError( "Unexpected character"
                            " while looking for \"" + string(word) + "\"",
                            char(byte) );
                }
            } while ( *++remainder );
            int const byte = sbuf.sgetc();
            if ( isalnum(byte) ) {
                sbuf.sungetc();
                throw UnexpectedByteParseError( string()
                        + "Trailing character in keyword \""
                        + word +"\"", byte );
            }
        }
        Json read_number_element(
                streambuf &sbuf,
                char firstchar,
                bool use_GN_for_floats
                ) {
            try {
                sbuf.sputbackc(firstchar);
                GeneralNumber n = GeneralNumber::parse( sbuf );
                if ( n.is_long_long_unsigned() ) {
                    return Json( n.as_long_long_unsigned() );
                } else if ( n.is_long_long() ) {
                    return Json( n.as_long_long() );
                } else {
                    if ( use_GN_for_floats ) {
                        return Json( n );
                    } else {
                        size_t const sigdigs = n.digits().size();
                        return Json( n.as_long_double(), sigdigs ? sigdigs : 1 );
                    }
                }
            } catch ( GeneralNumber::BadEOFParseError const &e ) {
                throw BadEOFParseError( e.what() );
            } catch ( GeneralNumber::NumberParseError const &e ) {
                throw NumberParseError( e.what() );
            }
        }
        Json read_string_element( streambuf &sbuf, StringPackager &sp ) {
            return Json( read_json_string_value( sbuf, sp ),
                    Json::ignore_bad_unicode );
        }
        Json read_array(
                streambuf &sbuf,
                StringPackager &sp,
                bool use_GN_for_floats
                ) {
            Json::ArrayBody items;

            if ( get_separator_byte(sbuf, false) != ']' ) {
                sbuf.sungetc();
                for (;;) {
                    Json element
                            = read_internal_json( sbuf, sp, use_GN_for_floats );
                    items.push_back( element );
                    char c = get_separator_byte(sbuf, false);
                    switch (c) {
                    case ',':
                        if ( peek_separator_byte(sbuf) == ']' )
                            throw TrailingCommaParseError( "array" );
                        continue;
                    case ']':
                        break;
                    default:
                        sbuf.sungetc();
                        throw UnexpectedByteParseError(
                                "Unexpected byte in array", c );
                    }
                    break;
                }
            }
            return Json( std::move(items) );
        }
        string read_object_key( streambuf &sbuf, StringPackager &sp ) {
            char byte = get_separator_byte(sbuf, false);
            if ( '"' == byte )
                return read_json_string_value( sbuf, sp );
            sbuf.sungetc();
            if ( byte == '}' )
                throw TrailingCommaParseError( "object" );
            else
                throw UnexpectedByteParseError( "Unexpected byte"
                        " while looking for an object key string", byte );
        }
        Json read_object(
                streambuf &sbuf,
                StringPackager &sp,
                bool use_GN_for_floats
                ) {
            Json::ObjectBody object;
            if ( get_separator_byte(sbuf, false) != '}' ) {
                sbuf.sungetc();
                for (;;) {
                    string key = read_object_key( sbuf, sp );
                    char c = get_separator_byte(sbuf, false);
                    if ( ':' != c ) {
                        sbuf.sungetc();
                        throw UnexpectedByteParseError(
                                "Missing separator for object key", c );
                    }
                    Json element
                            = read_internal_json( sbuf, sp, use_GN_for_floats );
                    insert( object, std::move(key), std::move(element) );

                    switch ( c = get_separator_byte(sbuf, false) ) {
                    case ',':
                        continue;
                    case '}':
                        break;
                    default:
                        sbuf.sungetc();
                        throw UnexpectedByteParseError(
                                "Unexpected byte in object", c );
                    }
                    break;
                }
            }
            return Json( std::move(object) );
        }
        Json read_json(
                streambuf &sbuf,
                StringPackager &sp,
                bool use_GN_for_floats
                ) {
            char byte = get_separator_byte(sbuf, true);
            switch ( byte ) {
            case '"': return read_string_element( sbuf, sp );
            case '[': return read_array( sbuf, sp, use_GN_for_floats );
            case '{': return read_object( sbuf, sp, use_GN_for_floats );
            case 'n': eat_word_rmdr(sbuf, "null" ); return Json();
            case 'f': eat_word_rmdr(sbuf, "false"); return Json(false);
            case 't': eat_word_rmdr(sbuf, "true" ); return Json(true);
            case '-': return read_number_element(sbuf, byte, use_GN_for_floats);
            default:
                if ( isdigit( byte ) ) {
                    return read_number_element( sbuf, byte, use_GN_for_floats );
                } else {
                    sbuf.sungetc();
                    throw UnexpectedByteParseError(
                            "Unexpected character while looking for element",
                            byte );
                }
            }
        }
        Json read_internal_json(
                streambuf &sbuf,
                StringPackager &sp,
                bool use_GN_for_floats
                ) {
            try {
                return read_json( sbuf, sp, use_GN_for_floats );
            } catch ( StartEOFParseError const &e ) {
                throw BadEOFParseError( e );
            }
        }
    }

    Json Json::parse( streambuf &sbuf, bool use_GN_for_floats ) {
        StringPackager sp;
        return read_json( sbuf, sp, use_GN_for_floats );
    }

    namespace {
        Json context_parse( jsrl_streambuf &sbuf ) {
            try {
                auto result = Json::parse( sbuf );
                int byte = jsrl_get_nonspace_byte( sbuf );
                if ( byte != EOF ) {
                    sbuf.sungetc();
                    throw TrailingBytesParseError();
                }
                return result;
            } catch ( Json::ParseError &e ) {
                try {
                    e.add_context( sbuf, 64 );
                } catch (...) {
                    // Any new errors should be absorbed
                    // and the original one propagated.
                }
                throw;
            }
        }
    }
    Json Json::parse( char const *start, char const *finish ) {
        jsrl_streambuf sbuf( start, finish );
        return context_parse( sbuf );
    }
    Json Json::parse( string_view str ) {
        char const *const s = str.data();
        return parse( s, s+str.size() );
    }

    string encode( Json const &json ) {
        return string_convert( json );
    }

    Json::ObjectBody::const_iterator find(
            Json::ObjectBody const &self,
            std::string_view key
            ) {
        using const_iterator = Json::ObjectBody::const_iterator;
        const_iterator const
                s_begin = self.begin(),
                s_end = self.end();
        const_iterator found = lower_bound( s_begin, s_end, key, CmpLt() );
        if ( found == s_end or found->first != key )
            return s_end;
        return found;
    }
}
// vi: et ts=4 sts=4 sw=4
