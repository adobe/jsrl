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
#include "jsrl_general_number.hpp"

#include "jsrl_impl_util.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <sstream>
#include <cassert>
#include <cstdio>
#include <cctype>
#include <cstdlib>
#include <iterator>


namespace jsrl {
    using std::ios_base;
    using std::reverse;
    using std::make_shared;
    using std::numeric_limits;
    using std::ostringstream;

    GeneralNumber GeneralNumber::parse(
            char const *start,
            char const *finish
            ) {
        jsrl_streambuf sbuf( start, finish );
        GeneralNumber result = parse( sbuf );
        if ( EOF != sbuf.sgetc() )
            throw GeneralNumber::NumberParseError( "Left-over text" );
        return result;
    }

    GeneralNumber GeneralNumber::parse( string const &s ) {
        char const *cs = s.c_str();
        return parse( cs, cs + s.size() );
    }

    namespace {

        bool peek_negative( streambuf &sbuf ) {
            int const firstchar = sbuf.sgetc();
            if ( firstchar == EOF ) {
                throw GeneralNumber::BadEOFParseError(
                        "Malformed input number (stream is empty)" );
            }
            bool const negative = firstchar=='-';
            if ( negative )
                sbuf.sbumpc();
            return negative;
        }

        void shift_exponent( int16_t &exponent, unsigned shiftval ) {
            if ( int32_t(exponent) > int32_t(INT16_MAX) - int32_t(shiftval) ) {
                throw GeneralNumber::NumberParseError(
                        "Range error in exponent" );
            }
            exponent += shiftval;
        }

    }

    GeneralNumber GeneralNumber::parse( streambuf &sbuf ) {
        bool const negative = peek_negative( sbuf );
        vector<char> digits;
        static unsigned const NO_DECIMAL = unsigned(-1);
        unsigned decimal_digits = NO_DECIMAL;
        enum {
            ES_NONE,
            ES_EMPTY,
            ES_POSITIVE_EMPTY,
            ES_NEGATIVE_EMPTY,
            ES_POSITIVE,
            ES_NEGATIVE
        } exponentseen = ES_NONE;
        int16_t exponent = 0;
        char const *need_digit = "No digits seen"; // Why we expect a digit
        bool eofseen = false;
        for (;;) {
            int const byte = sbuf.sbumpc();
            switch ( byte ) {
            case EOF:
                if ( need_digit ) {
                    throw BadEOFParseError(
                            "Malformed input number (stream end, "
                            + string(need_digit) + ")" );
                }
                eofseen = true;
                break;
            case 'e':
            case 'E':
                if ( exponentseen != ES_NONE || need_digit )
                    break;
                exponentseen = ES_EMPTY;
                if ( decimal_digits == NO_DECIMAL )
                    decimal_digits = digits.size();
                need_digit = "No digits after exponent";
                continue;
            case '+':
            case '-':
                if ( exponentseen != ES_EMPTY )
                    break;
                exponentseen = byte == '+'
                        ? ES_POSITIVE_EMPTY
                        : ES_NEGATIVE_EMPTY
                        ;
                need_digit = "No digits after exponent 'E'";
                continue;
            case '.':
                if ( decimal_digits != NO_DECIMAL || need_digit )
                    break;
                decimal_digits = digits.size();
                need_digit = "No digits after decimal";
                continue;
            default:
                if ( !isdigit( byte ) )
                    break;
                need_digit = nullptr;
                switch ( exponentseen ) {
                case ES_NONE:
                    digits.push_back( byte );
                    continue;
                case ES_EMPTY:
                case ES_POSITIVE_EMPTY:
                    exponentseen = ES_POSITIVE;
                    break;
                case ES_NEGATIVE_EMPTY:
                    exponentseen = ES_NEGATIVE;
                    break;
                default:
                    break;
                }
                {
                    assert( exponent >= 0 );
                    uint32_t new_exponent = exponent * 10 + (byte - '0');
                    if ( new_exponent > uint32_t(INT16_MAX) )
                        throw NumberParseError( "Range error in exponent" );
                    exponent = int16_t(new_exponent);
                }
                continue;
            }
            if ( ! eofseen )
                sbuf.sungetc();
            if ( need_digit ) {
                throw NumberParseError( "Malformed input number ("
                        + string(need_digit) + ")" );
            }
            if ( ( decimal_digits == NO_DECIMAL || decimal_digits > 1 )
                    && digits.size() > 1
                    && digits.front() == '0' ) {
                throw NumberParseError("Malformed input number (leading zero)");
            }
            switch ( exponentseen ) {
            default:
            case ES_EMPTY:
            case ES_POSITIVE_EMPTY:
            case ES_NEGATIVE_EMPTY:
                assert(false);
            case ES_NONE:
            case ES_POSITIVE:
                break;
            case ES_NEGATIVE:
                exponent = -exponent;
                break;
            }
            if ( decimal_digits == NO_DECIMAL ) {
                shift_exponent( exponent, digits.size() );
            } else {
                shift_exponent( exponent, decimal_digits );
            }
            return GeneralNumber( decimal_digits != NO_DECIMAL,
                    negative, exponent, digits );
        }
    }

    GeneralNumber::GeneralNumber( long long unsigned value )
    {
        while ( value ) {
            m_digits.push_back( '0' + ( value % 10 ) );
            value /= 10;
        }
        if ( ! m_digits.empty() ) {
            reverse( begin(m_digits), end(m_digits) );
            m_exponent = m_digits.size();
            while ( m_digits.back() == '0' )
                m_digits.pop_back();
        }
    }

    GeneralNumber::GeneralNumber( long long value )
    {
        bool const valnegative = value < 0;
        if ( valnegative )
            value = -value;
        operator=( static_cast<long long unsigned>(value) );
        m_negative = valnegative;
    }

    GeneralNumber::GeneralNumber( long double value )
    {
        ostringstream oss;
        oss.precision( numeric_limits<long double>::max_digits10 );
        oss.setf( ios_base::showpoint );
        oss << value;
        operator=( parse( oss.str() ) );
    }

    GeneralNumber::GeneralNumber(
            bool is_decimal_,
            bool negative_,
            int16_t exponent_,
            vector<char> digits_
            )
        : GeneralNumberData( is_decimal_, negative_, exponent_, std::move(digits_) )
    {
        if ( ! m_digits.empty() && m_digits.front() == '0' ) {
            vector<char>::const_iterator loppoint = m_digits.begin();
            while ( ++loppoint != m_digits.end() && '0' == *loppoint ) { }
            {
                unsigned const shiftval = loppoint - m_digits.begin();
                if ( int32_t(m_exponent)
                        < int32_t(INT16_MIN) + int32_t(shiftval) ) {
                    throw GeneralNumber::NumberParseError(
                            "Range error in exponent" );
                }
                m_exponent -= shiftval;
            }
            m_digits.erase( m_digits.begin(), loppoint );
        }
        if ( m_digits.empty() ) {
            m_exponent = INT16_MIN;
            m_negative = false;
        } else {
            if ( m_digits.back() == '0' ) {
                vector<char>::const_iterator loppoint = m_digits.end();
                while ( '0' == (--loppoint)[-1] ) { }
                m_digits.erase( loppoint, m_digits.end() );
            }
            if ( ptrdiff_t(m_digits.size()) > m_exponent )
                m_is_decimal = true;
        }
        for ( vector<char>::const_iterator
                c = m_digits.begin()
                ; c != m_digits.end()
                ; ++c ) {
            assert( *c >= '0' );
            assert( *c <= '9' );
        }
    }

    long double GeneralNumber::as_long_double() const {
        ostringstream oss;
        oss << *this;
        return strtold( oss.str().c_str(), nullptr );
    }

    namespace {

        bool is_in_long_long_range(
                bool negative,
                int16_t exponent,
                vector<char> const &digits
                ) {
            char const MAXLL[] = "9223372036854775808";
            if ( exponent > int16_t(sizeof(MAXLL)-1) )
                return false;
            if ( exponent < int16_t(sizeof(MAXLL)-1) )
                return true;
            char const *cur = MAXLL;
            for ( vector<char>::const_iterator
                    c = digits.begin()
                    ; c != digits.end()
                    ; ++c
                    , ++cur ) {
                if ( *c == *cur )
                    continue;
                return *c < *cur;
            }
            if ( digits.size() < sizeof(MAXLL)-1 )
                return true;
            return negative;
        }

        bool is_in_long_long_unsigned_range(
                int16_t exponent,
                vector<char> const &digits
                ) {
            char const MAXLLU[] = "18446744073709551615";
            if ( exponent > int16_t(sizeof(MAXLLU)-1) )
                return false;
            if ( exponent < int16_t(sizeof(MAXLLU)-1) )
                return true;
            char const *cur = MAXLLU;
            for ( vector<char>::const_iterator
                    c = digits.begin()
                    ; c != digits.end()
                    ; ++c
                    , ++cur ) {
                if ( *c == *cur )
                    continue;
                return *c < *cur;
            }
            return true;
        }

    }

    long long GeneralNumber::as_long_long() const {
        if ( m_exponent < 0 )
            return 0;
        if ( ! is_in_long_long_range( m_negative, m_exponent, m_digits ) ) {
            return m_negative
                    ? numeric_limits<long long>::min()
                    : numeric_limits<long long>::max()
                    ;
        }
        long long result = 0;
        for ( unsigned i = 0 ; i != unsigned(m_exponent) ; ++i ) {
            result *= 10;
            if ( i < m_digits.size() )
                result += m_digits[i]-'0';
        }
        return m_negative ? -result : result;
    }

    long long unsigned GeneralNumber::as_long_long_unsigned() const {
        if ( m_exponent < 0 )
            return 0;
        if ( m_negative )
            return 0;
        if ( ! is_in_long_long_unsigned_range( m_exponent, m_digits ) )
            return numeric_limits<long long unsigned>::max();
        long long unsigned result = 0;
        for ( unsigned i = 0 ; i != unsigned(m_exponent) ; ++i ) {
            result *= 10;
            if ( i < m_digits.size() )
                result += m_digits[i]-'0';
        }
        return result;
    }

    bool GeneralNumber::is_long_long_unsigned() const {
        if ( m_is_decimal )
            return false;
        if ( m_negative )
            return false;
        if ( m_digits.empty() )
            return true;
        if ( ptrdiff_t(m_exponent) < ptrdiff_t(m_digits.size()) )
            return false;
        return is_in_long_long_unsigned_range( m_exponent, m_digits );
    }

    bool GeneralNumber::is_long_long() const {
        if ( m_is_decimal )
            return false;
        if ( m_digits.empty() )
            return true;
        if ( ptrdiff_t(m_exponent) < ptrdiff_t(m_digits.size()) )
            return false;
        return is_in_long_long_range( m_negative, m_exponent, m_digits );
    }

    ostream &operator<<( ostream &ostr, GeneralNumber const &self ) {
        if ( self.m_digits.empty() ) {
            return ostr << ( self.m_is_decimal ? "0.0" : "0" );
        }
        if ( self.m_negative )
            ostr << '-';
        if ( self.m_is_decimal
                || ptrdiff_t(self.m_exponent)
                    < ptrdiff_t(self.m_digits.size()) ) {
            ostr << self.m_digits.front() << ".";
            if ( self.m_digits.size() == 1 ) {
                ostr << "0";
            } else {
                for ( unsigned i = 1 ; i != self.m_digits.size() ; ++i )
                    ostr << self.m_digits[i];
            }
            if ( self.m_exponent != 1 )
                ostr << "e" << ( self.m_exponent - 1 );
        } else {
            for ( unsigned i = 0 ; i != unsigned(self.m_exponent) ; ++i )
                ostr << ( i < self.m_digits.size() ? self.m_digits[i] : '0' );
        }
        return ostr;
    }

    namespace {
        int cmp3wayMag(
                GeneralNumberData const &lhs,
                GeneralNumberData const &rhs
                ) {
            if ( lhs.m_exponent != rhs.m_exponent ) {
                return lhs.m_exponent < rhs.m_exponent ? -1 : 1 ;
            }
            vector<char>::const_iterator lhs_cur = lhs.m_digits.begin();
            vector<char>::const_iterator const lhs_end = lhs.m_digits.end();
            vector<char>::const_iterator rhs_cur = rhs.m_digits.begin();
            vector<char>::const_iterator const rhs_end = rhs.m_digits.end();
            for (;;) {
                if ( lhs_cur == lhs_end ) {
                    if ( rhs_cur == rhs_end )
                        return 0;
                    return -1;
                } else if ( rhs_cur == rhs_end ) {
                    return 1;
                } else if ( *lhs_cur != *rhs_cur ) {
                    return *lhs_cur < *rhs_cur ? -1 : 1 ;
                } else {
                    ++lhs_cur;
                    ++rhs_cur;
                }
            }
        }
    }

    int cmp3way( GeneralNumber const &lhs, GeneralNumber const &rhs ) {
        if ( lhs.m_negative != rhs.m_negative ) {
            return lhs.m_negative ? -1 : 1;
        }
        int const result = ( lhs.m_negative ? -1 : 1 ) * cmp3wayMag( lhs, rhs );
        if ( 0 == result ) {
            if ( lhs.m_is_decimal != rhs.m_is_decimal )
                return lhs.m_is_decimal ? 1 : -1;
        }
        return result;
    }

    istream &operator>>( istream &istr, GeneralNumber &self ) {
        self = GeneralNumber::parse( *istr.rdbuf() );
        return istr;
    }
}
// vi: et ts=4 sts=4 sw=4
