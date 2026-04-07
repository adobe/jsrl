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
#ifndef JSRL_GENERAL_NUMBER_HPP_1C7F88DD1C404E06622C0EBE5B84ABE8
#define JSRL_GENERAL_NUMBER_HPP_1C7F88DD1C404E06622C0EBE5B84ABE8

#include "jsrl_export.hpp"
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace jsrl {
    using std::istream;
    using std::ostream;
    using std::streambuf;
    using std::vector;
    using std::string;
    using std::runtime_error;

    /*! @brief  Thin adapter to provide access to a GeneralNumber's digits.
     */
    struct Digits {

        Digits( ) : m_data(nullptr), m_size() { }
        Digits( char const *data, size_t size ) : m_data(data), m_size(size) { }
        char const *data() const { return m_data; }
        size_t size() const { return m_size; }
        bool empty() const { return size() == 0; }
        using iterator = char const *;
        using const_iterator = char const *;
        const_iterator begin() const { return data(); }
        const_iterator end() const { return begin() + size(); }

    private:
        char const *m_data;
        size_t m_size;
    };

    /*! @brief  Private helper class for GeneralNumber.
     */
    struct GeneralNumberData {
        bool m_is_decimal;
        bool m_negative;
        int16_t m_exponent;
        vector<char> m_digits;

        GeneralNumberData()
            : m_is_decimal(false)
            , m_negative(false)
            , m_exponent(INT16_MIN)
        { }

        GeneralNumberData(
                bool is_decimal,
                bool negative,
                int16_t exponent,
                vector<char> digits
                )
            : m_is_decimal(is_decimal)
            , m_negative(negative)
            , m_exponent(exponent)
            , m_digits( std::move(digits) )
        { }
    };
    /*! @brief  Character-based type for high-fidelity float round-tripping.
     */
    struct JSRL_EXPORT GeneralNumber : private GeneralNumberData {

        /*! @brief  Default constructor. */
        GeneralNumber() = default;

        /*! @brief  Converting constructor for unsigned integeres. */
        GeneralNumber( long long unsigned value );
        /*! @brief  Converting constructor for signed integers. */
        GeneralNumber( long long value );
        /*! @brief  Converting constructor for floading point values. */
        GeneralNumber( long double value );

        /*! @brief  Constructor with prepared values.
         */
        explicit
        GeneralNumber(
                bool is_decimal,    //!<[in] Output unconditionally as decimal.
                bool negative,      //!<[in] Sign bit.
                int16_t exponent,   //!<[in] Power of ten for digits.
                vector<char> digits //!<[in] Fractional decimal digits.
                );

        /*! @brief  Parse a block of character data as a number. */
        static GeneralNumber parse( char const *start, char const *finish );
        /*! @brief  Parse a string as a number. */
        static GeneralNumber parse( string const &s );
        /*! @brief  Extract a number from a streambuf. */
        static GeneralNumber parse( streambuf &sbuf );

        /*! @brief  Number is decimal (not integral). */
        bool is_decimal() const { return m_is_decimal; }

        /*! @brief  Is the number negative? */
        bool is_negative() const { return m_negative; }

        /*! @brief  Where does the decimal point go in the digit sequence.
         *
         *  . 0 means the decimal goes before the first digit.
         *  . 1 means the decimal goes after the first digit.
         *  . -1 means the decimal goes before the first digit with
         *    a leading '0' added, and so forth.
         */
        int16_t exponent() const { return m_exponent; }

        /*! @brief  Get access to the sequence of digits for the number.
         *  @note   The sequence is empty IFF the number is zero.
         */
        Digits digits() const {
            return m_digits.empty()
                    ? Digits()
                    : Digits( &m_digits.front(), m_digits.size() )
                    ;
        }

        long double as_long_double() const; /*!< @brief Convert to float. */
        long long as_long_long() const;     /*!< @brief Convert to int. */
        long long unsigned as_long_long_unsigned() const;
                                            /*!< @brief Convert to uint. */

        bool is_long_long_unsigned() const; /*!< @brief Fits in uint? */
        bool is_long_long() const;          /*!< @brief Fits in int? */

        /*! @brief  Stream insertion. */
        friend JSRL_EXPORT
        istream &operator>>( istream &istr, GeneralNumber &self );

        /*! @brief  Stream extraction. */
        friend JSRL_EXPORT
        ostream &operator<<( ostream &ostr, GeneralNumber const &self );

        /*! @brief  Three-way comparison (used in tricchotomy operators). */
        friend JSRL_EXPORT
        int cmp3way( GeneralNumber const &lhs, GeneralNumber const &rhs );
#define CMPOP( OP ) \
        friend \
        bool operator OP ( GeneralNumber const &l, GeneralNumber const &r ) \
        { \
            return cmp3way( l, r ) OP 0; \
        }
        CMPOP( == )
        CMPOP( != )
        CMPOP( < )
        CMPOP( > )
        CMPOP( <= )
        CMPOP( >= )
#undef CMPOP

        /*! @brief  Parsing failure, data ended early. */
        struct BadEOFParseError : runtime_error {
            BadEOFParseError( string const &message )
                : runtime_error( message )
            { }
        };

        /*! @brief  Parsing failure, malformed data. */
        struct NumberParseError : runtime_error {
            NumberParseError( string const &message )
                : runtime_error( message )
            { }
        };

    };

}
#endif
// vi:et ts=4 sts=4 sw=4
