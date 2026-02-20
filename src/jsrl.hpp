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
#ifndef JSRL_HPP_B29F03A37D5EAC6B156C3B92A05C1E4F
#define JSRL_HPP_B29F03A37D5EAC6B156C3B92A05C1E4F

#include "jsrl_general_number.hpp"

#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <istream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

/*! @brief  JSON parser library.
 *
 *  This library is a basic JSON parser and emitter.
 *  It is designed around UTF-8 as the representation format.
 *
 *  In this library, underlying JSON objects/entities are essentially immutable.
 *  Elements in JSON arrays and objects can't be added, removed, or changed.
 *  However, @ref jsrl::Json objects can be assigned to,
 *  and essentially function as handles or envelopes around JSON entities.
 *  Copying a @ref jsrl::Json object is as cheap as copying a @b shared_ptr.
 *
 *  To parse JSON text, use the C++ stream extraction operator
 *  and read into a @ref jsrl::Json instance.
 *  A JSON object or array may similarly be printed
 *  using the stream insertion operator.
 *  For example:
 *  @code
 *  #include "jsrl.hpp"
 *  #include <iostream>
 *  #include <sstream>
 *  using namespace jsrl;
 *  using namespace std;
 *  int main() {
 *      istringstream jsonstr("{ \"Key\": [ \"value1\", true ] }");
 *      Json json;
 *      jsonstr >> json;
 *      cout << json << endl;                       // {"Key":["value1",true]}
 *      cout << json["Key"][0] << endl;             // "value1"
 *      cout << json["Key"][0].as_string() << endl; // value1
 *      cout << json["Non-key"] << endl;            // throw ObjectKeyError
 *  }
 *  @endcode
 *
 *  To construct a new Json entity, use one of the constructors.
 *  A @c bool, @c string, @c vector, @c map, or number can be
 *  passed into one of the @ref jsrl::Json constructors
 *  to make a corresponding JSON entity.
 *  A default-constructed object represents @c null.
 *
 *  This library draws a loose distinction between floating point numbers
 *  (stored as @code long double @endcode) and integers
 *  (stored as @code long long @endcode),
 *  because the author feels that in many languages (including C++),
 *  the distinction is a useful one.
 *  However, if an integral value is read or constructed,
 *  it can be queried as a floating point value,
 *  so users that wish to ignore this distinction
 *  should be able to do so transparently.
 *
 *  # Conveniences For Element Access
 *
 *  The @c Json class provides a few convenience functions
 *  for accessing elements of an array or object.
 *  If a @c Json instance is an array, you can use
 *  the @c size() member function to find out
 *  how many elements are in the array,
 *  and @c operator[] to get the individual elements.
 *  If you try to access an element beyond the end of the array,
 *  a @c TypeError exception will be thrown.
 *  To avoid this, you can call @c get() instead,
 *  which will optionally take a default @c Json object to return instead.
 *
 *  Similarly, for @c Json objects, you can use @c operator[]
 *  with a @c string key to access elements of the object,
 *  and @c get() to avoid an exception for a missing key.
 *
 *  If you want to access an element of an array or object,
 *  which should be of a specific type,
 *  and you don't want an exception if the element is
 *  either missing or the wrong type,
 *  you can use one of the @c get_*() functions,
 *  which take a key and a default value of the appropriate type.
 *  For example, to get an element from an object that should be a string,
 *  and provide a default replacement if the element is
 *  missing or the wrong type:
 *  @code
 *  int main() {
 *      Json json;
 *      cin >> json;
 *      if ( json.is_object() ) {
 *          cout << "The value for \"key\" is: "
 *               << json.get_string("key","<no value was given>")
 *               << endl ;
 *      }
 *  }
 *  @endcode
 *
 *  # Conveniences For Setting Elements
 *
 *  The @c Json class also provides convenience functions
 *  for making Json arrays and objects by setting specific values.
 *  The @c Json class serves as a handle to immutable body elements,
 *  so there is no way to "change" the value associated with a key.
 *  However, the @c set() member functions are convenience functions
 *  to accomplish effectively the same result.
 *  The @c set() member functions take a key (integral for arrays,
 *  string for objects) and a value, and replace the body
 *  with a new array or object that has the specified key/value set.
 *  Note that since @c Json body entities are immutable,
 *  copies can be shallow, which means that the hidden copy
 *  is usually not a serious performance concern unless
 *  an array or object itself has a large cardinality.
 *  (With C++11 support compiled in, @c set() can take
 *  any number of key/value pairs, which avoids
 *  having to do multiple intermediate copies.)
 *
 */
namespace jsrl {
    using std::ios_base;
    using std::istream;
    using std::ostream;
    using std::streambuf;
    using std::pair;
    using std::runtime_error;
    using std::map;
    using std::vector;
    using std::string;
    using std::string_view;

    /*! @brief  Handle class for a JSON document.
     *
     *  Instances of this class store JSON entities such as @c null,
     *  @c true, @c false, numbers, strings, arrays, and objects (dictionaries).
     *  Constructors are provided for creating each type of object directly
     *  (the default constructor creates a @c null object),
     *  and there are query functions for determining the type
     *  of the held entity and examining the held value(s).
     *  To create or examine array entities, use a @code vector<Json> @endcode,
     *  and to create or examine object entities, use a
     *  @code map<string,Json> @endcode.
     *  Additionally, stream insertion and extraction operators are provided
     *  for parsing JSON and formatting JSON (respectively).
     *
     *  All string values are assumed to be in UTF-8.
     *  Input streams are parsed as UTF-8,
     *  escaped Unicode codepoints are converted to UTF-8
     *  (including surrogate pair handling),
     *  and the encoder converts the UTF-8 codepoints into
     *  Unicode escape sequences on output
     *  (via surrogate pairs for non-BMP characters).
     */
    struct Json {
        Json(Json const &) = default;
        Json &operator=(Json const &) = default;
        Json(Json &&) noexcept;
        Json &operator=(Json &&) noexcept;
        ~Json() noexcept = default;

        /*! @brief  Passed to the string constructor to bybass UTF-8 validation.
         */
        static struct ignore_bad_unicode_t { } const ignore_bad_unicode;

        //! Convenience alias for the vector type used to traverse an array.
        using ArrayBody = vector<Json>;
        //! Convenience alias for the map type used to traverse an object.
        using ObjectBody = vector<pair<string,Json> >;

        /*! @brief  Default constructor, provides a @b null JSON entity.
         */
        Json() noexcept;
        /*! @brief  Specialized overload of @c swap() for this type.
         */
        friend void swap( Json &, Json & ) noexcept;

        /*! @brief  Constructor for @b true / @b false JSON entities.
         */
        Json( bool );
        /*! @brief  Constructor for numeric JSON entities.
         */
        Json( GeneralNumber );
        /*! @brief  Helper overload for numeric JSON entities.
         */
        Json( long double );
        /*! @brief  Constructor for numeric JSON entities with a digit hint.
         */
        explicit Json( long double value, short unsigned sigdigs );
        /*! @brief  Helper overload for numeric JSON entities.
         */
        Json( double );
        /*! @brief  Helper overload for numeric JSON entities.
         */
        Json( float );
        /*! @brief  Constructor for integral numeric JSON entities.
         */
        Json( long long );
        /*! @brief  Helper overload for integral numeric JSON entities.
         *  @overload
         */
        Json( long );
        /*! @brief  Helper overload for integral numeric JSON entities.
         *  @overload
         */
        Json( int );
        /*! @brief  Helper overload for integral numeric JSON entities.
         *  @overload
         */
        Json( short );
        /*! @brief  Helper overload for integral numeric JSON entities.
         *  @overload
         */
        Json( long long unsigned );
        /*! @brief  Helper overload for integral numeric JSON entities.
         *  @overload
         */
        Json( long unsigned );
        /*! @brief  Helper overload for integral numeric JSON entities.
         *  @overload
         */
        Json( int unsigned );
        /*! @brief  Helper overload for integral numeric JSON entities.
         *  @overload
         */
        Json( short unsigned );
        /*! @brief  Constructor for string JSON entities.
         *
         *  This overload checks that the input is valid UTF-8
         *  and throws an exception if it's not.
         */
        Json( string );
        /*! @brief  Constructor for string JSON entities.
         *  @overload
         *
         *  This overload assumes that the input is valid UTF-8.
         */
        explicit Json( string, ignore_bad_unicode_t );
        /*! @brief  Helper overload so @c char* doesn't get treated as @c bool.
         *  @overload
         *
         *  This overload checks that the input is valid UTF-8
         *  and throws an exception if it's not.
         */
        Json( char const * );
        /*! @brief  Helper overload so @c char* doesn't get treated as @c bool.
         *  @overload
         *
         *  This overload assumes that the input is valid UTF-8.
         */
        explicit Json( char const *, ignore_bad_unicode_t );
        /*! @brief  Constructor for array JSON entities.
         */
        Json( ArrayBody );
        /*! @brief  Constructor for object JSON entities.
         */
        explicit Json( map<string,Json> const & );
        /*! @brief  Constructor for string-mapped object JSON entities.
         */
        explicit Json( map<string,string> const & );
        /*! @brief  Constructor for vector-mapped object JSON entities.
         */
        Json( ObjectBody );

        static
        Json array( ArrayBody body ) {
            return Json( std::move(body) );
        }

        static
        Json object( ObjectBody body ) {
            return Json( std::move(body) );
        }

        /*! @brief  Disabled, to prevent brace-init syntax from being used.
         *
         *  By disabling brace-initialization syntax here,
         *  an initializer_list<> constructor can be added later
         *  without breaking client code.
         */
        template<typename T>
        Json( std::initializer_list<T> ) = delete;
    private:
        struct ElementBase;
        using ElementBasePtr = std::shared_ptr<ElementBase const>;

        template<typename I, typename... P>
        struct is_array_ctor_iterptr
                : std::is_constructible< Json,
                    decltype(*std::declval<I>()), P... > { };

        template<typename I, typename... P>
        struct is_convertible_pair : std::false_type { };

        template<typename F, typename S, typename... P>
        struct is_convertible_pair<std::pair<F,S>,P...>
                : std::conditional_t<
                    std::is_constructible_v< string, F >,
                    std::is_constructible< Json, S, P...>,
                    std::false_type > { };

        template<typename I, typename... P>
        struct is_object_ctor_iterptr
                : is_convertible_pair<
                    std::decay_t< decltype(*std::declval<I>()) >,
                    P... > { };

        static
        size_t s_reserve_probe( ... ) {
            return 0;
        }
        template<
                typename I,
                typename R = decltype( std::declval<I>()-std::declval<I>() )
                >
        static
        R s_reserve_probe( I b, I e ) {
            return e-b;
        }

        template<typename I, typename... P>
        static
        ArrayBody s_make_ArrayBody( I b, I e, P ...p ) {
            ArrayBody a;
            a.reserve( s_reserve_probe( b, e ) );
            while ( b != e )
                a.push_back( Json(*b++, p...) );
            return a;
        }
        static ElementBasePtr s_make_ArrayBodyPtr( ArrayBody a );

        template<
                typename I,
                typename... P
                >
        static
        auto s_make_CompoundBodyPtr(
                I b,
                I e,
                P ...p
                ) -> std::enable_if_t<
                    is_array_ctor_iterptr<I>::value,
                    ElementBasePtr
                    > {
            return s_make_ArrayBodyPtr( s_make_ArrayBody( b, e, p... ) );
        }

        template<typename I, typename... P>
        static
        ObjectBody s_make_ObjectBody( I b, I e, P ...p ) {
            ObjectBody o;
            while ( b != e ) {
                o.emplace_back( string( b->first ), Json( b->second, p... ) );
                ++b;
            }
            return o;
        }
        static ElementBasePtr s_make_ObjectBodyPtr( ObjectBody o );

        template<
                typename I,
                typename... P
                >
        static
        auto s_make_CompoundBodyPtr(
                I b,
                I e,
                P ...p
                ) -> std::enable_if_t<
                    is_object_ctor_iterptr<I>::value,
                    ElementBasePtr
                    > {
            return s_make_ObjectBodyPtr(
                    s_make_ObjectBody(b, e, p...) );
        }

    public:

        template<
                typename I,
                typename = std::enable_if_t<
                        is_array_ctor_iterptr<I>::value
                        != is_object_ctor_iterptr<I>::value,
                        decltype(++std::declval<I&>())
                    >
                >
        explicit Json( I b, I e )
            : m_el( s_make_CompoundBodyPtr( b, e ) )
        { }

        template<
                typename I,
                typename = std::enable_if_t<
                        is_array_ctor_iterptr<I,ignore_bad_unicode_t>::value !=
                        is_object_ctor_iterptr<I,ignore_bad_unicode_t>::value,
                        decltype(++std::declval<I&>())
                    >
                >
        explicit Json( I b, I e, ignore_bad_unicode_t ignore_bad_unicode )
            : m_el( s_make_CompoundBodyPtr( b, e, ignore_bad_unicode ) )
        { }

        /*! @brief  Tag type to query the flavor of a Json entity in one step.
         *
         *  The intended use of this type is to be able to @c switch
         *  on an entity's type without having to use an if-ladder
         *  with the individual @c is_*() probe functions.
         */
        enum TypeTag {
            TT_NULL,
            TT_BOOL,
            TT_NUMBER,
            TT_NUMBER_GENERAL,
            TT_NUMBER_INTEGER,
            TT_NUMBER_INTEGER_UNSIGNED,
            TT_STRING,
            TT_ARRAY,
            TT_OBJECT,
        };
        /*! @brief  Tag query for the entity type.
         */
        TypeTag get_typetag(
                bool split_subtype      //!<[in] Separate numeric subtypes.
                ) const noexcept;

        bool is_null() const noexcept;   /*!< @brief Test for a null. */
        bool is_bool() const noexcept;   /*!< @brief Test for a boolean. */
        bool is_number() const noexcept; /*!< @brief Test for a numeric. */
        bool is_number_float() const noexcept; /*!< @brief Float test. */
        bool is_number_sint() const noexcept; /*!< @brief Signed int test. */
        bool is_number_uint() const noexcept; /*!< @brief Nonneg.*/
        /*! @brief  Any integral type.
         */
        bool is_number_integer() const noexcept {
            return is_number_sint() || is_number_uint();
        }
        bool is_number_general() const noexcept; /*!< @brief Exact.*/
        bool is_string() const noexcept; /*!< @brief Test for a string. */
        bool is_array() const noexcept;  /*!< @brief Test for an array. */
        bool is_object() const noexcept; /*!< @brief Test for an object. */

        bool as_bool() const;   /*!< @brief Retrieve true/false value. */
        std::shared_ptr<GeneralNumber const> as_number_general() const;
                /*!< @brief Exact number representation. */
        long double as_number_float() const;  /*!< @brief Get numeric value. */
        long long as_number_sint()const;/*!<@brief Retrieve integral value.*/
        long long unsigned as_number_uint() const;
                                /*!< @brief Retrieve unsigned integer value. */
        /*! @brief  Get either integeral value, ignoring sign saturation. */
        long long unsigned as_number_xint() const {
            if ( is_number_sint() )
                return static_cast<long long unsigned>(as_number_sint());
            else
                return as_number_uint();
        }
        string const &as_string() const;/*!< @brief Access string value. */
        ArrayBody const &as_array() const;/*!< @brief Access array body. */
        ObjectBody const &as_object() const; /*!< @brief Access object body. */
        map<string,Json> as_map_object() const; /*!< @brief Map from object. */

        friend bool operator==( Json lhs, Json rhs ) noexcept {
            return s_compare( std::move(lhs), std::move(rhs) ) == 0;
        }
        friend bool operator!=( Json lhs, Json rhs ) noexcept {
            return s_compare( std::move(lhs), std::move(rhs) ) != 0;
        }
        friend bool operator<( Json lhs, Json rhs ) noexcept {
            return s_compare( std::move(lhs), std::move(rhs) ) < 0;
        }
        friend bool operator>( Json lhs, Json rhs ) noexcept {
            return s_compare( std::move(lhs), std::move(rhs) ) > 0;
        }
        friend bool operator<=( Json lhs, Json rhs ) noexcept {
            return s_compare( std::move(lhs), std::move(rhs) ) <= 0;
        }
        friend bool operator>=( Json lhs, Json rhs ) noexcept {
            return s_compare( std::move(lhs), std::move(rhs) ) >= 0;
        }

    private:
        template<typename T>
        struct JsonBodyPtr {
            using Ptr = std::shared_ptr<T const>;

            explicit JsonBodyPtr( T body );

            operator Ptr const & () const { return p_get(); }
            Ptr const &operator->() const { return p_get(); }
            T const &operator*() const { return *p_get(); }
        private:
            friend struct Json;

            Ptr const &p_get() const { return m_ptr; }

            JsonBodyPtr( ElementBasePtr m_el, Ptr m_ptr );

            ElementBasePtr m_el;
            Ptr m_ptr;
        };
        template<typename T>
        static
        std::shared_ptr<T const> p_extract_body_ptr( ElementBasePtr const &el );

    public:
        using StringPtr = JsonBodyPtr<string>;
        using ArrayPtr = JsonBodyPtr<ArrayBody>;
        using ObjectPtr = JsonBodyPtr<ObjectBody>;

        template<typename T>
        explicit Json( JsonBodyPtr<T> body )
            : m_el( std::move(body.m_el) )
        { }

        /*! @brief  Get a shared pointer to the string body.
         *
         *  This member allows you to get a smart pointer to a string element
         *  that shares ownership with the @ref Json value.
         */
        StringPtr as_string_ptr() const;

        /*! @brief  Get a shared pointer to an array's vector body.
         *
         *  This member provides a smart pointer to an array's @b vector body
         *  that shares ownership with the @ref Json value.
         */
        ArrayPtr as_array_ptr() const;

        /*! @brief  Get a shared pointer to an objects's map body.
         *
         *  This member provides a smart pointer to a Json object's @b map body
         *  that shares ownership with the @ref Json value.
         */
        ObjectPtr as_object_ptr() const;

        size_t size() const;    /*!< @brief Query array length. */
        /*! @brief  Test for an array element. */
        Json const *find_key( size_t index ) const;
        /*! @brief Test for a key. */
        bool has_key( std::string_view key ) const { return find_key(key); }
        /*! @brief Test for a key and inspect its value.
         *  @retval NULL    The key was absent.
         *  @return A pointer to the value for this key.
         */
        Json const *find_key( std::string_view key ) const;
        Json const &operator[](size_t index) const;
                                /*!< @brief Access an indexed array element. */
        Json const &operator[](std::string_view key) const;
                                /*!< @brief Access a keyed object element. */

        /*! @brief  Access an array/object element, but don't throw a KeyError.
         */
        template<typename K, typename D>
        Json get( K &&key, D &&default_val ) const;
        /*! @brief  Access an array/object element, but don't throw a KeyError.
         */
        template<typename K>
        Json get( K &&key ) const;

#define JSRL_DECLARE_GET_AS( TYPE, CPPT )                                   \
        template<typename K>                                                \
        CPPT get_##TYPE( K &&key, CPPT default_value ) const
        JSRL_DECLARE_GET_AS( bool, bool );
        JSRL_DECLARE_GET_AS( number_general, std::shared_ptr<GeneralNumber const> );
        JSRL_DECLARE_GET_AS( number_float, long double );
        JSRL_DECLARE_GET_AS( number_sint, long long );
        JSRL_DECLARE_GET_AS( number_uint, long long unsigned );
        JSRL_DECLARE_GET_AS( string, string const & );
        JSRL_DECLARE_GET_AS( array, ArrayBody const & );
        JSRL_DECLARE_GET_AS( object, ObjectBody const & );
#undef JSRL_DECLARE_GET_AS

        /*! @brief  Replace an object with a copy with keys replaced/added.
         */
        template <typename... R>
        void set( string key, Json value, R &&...rest ) {
            ObjectBody as_obj = as_object();
            p_set_elem( as_obj, std::move(key), std::move(value) , ::std::forward<R>(rest)... );
            operator=( Json(std::move(as_obj)) );
        }
        /*! @brief  Replace an array with a copy with entries replaced.
         */
        template <typename... R>
        void set( size_t key, Json value, R &&...rest ) {
            ArrayBody as_arr = as_array();
            p_set_elem( as_arr, std::move(key), std::move(value) , ::std::forward<R>(rest)... );
            operator=( Json(std::move(as_arr)) );
        }

        /*! @brief  Convenience function for writing C++ strings in JSON syntax.
         */
        static
        void write_JSON_string(
                ostream &ost,
                string const &value,
                bool fail_bad_utf8 = false,
                bool write_utf = false
                );
        /*! @brief  Convenience function for writing string data in JSON syntax.
         */
        static
        void write_JSON_string(
                ostream &ost,
                string_view const &value,
                bool fail_bad_utf8 = false,
                bool write_utf = false
                );
        /*! @brief  Overload to avoid ambiguity for char const *.
         */
        static
        void write_JSON_string(
                ostream &ost,
                char const *value,
                bool fail_bad_utf8 = false,
                bool write_utf = false
                ) {
            return write_JSON_string( ost, string_view( value ),
                    fail_bad_utf8, write_utf );
        }

        struct EncodeOptions {
            enum Tightness {
                TN_EXACT,
                TN_LONG_DOUBLE,
                TN_DOUBLE,
                TN_FLOAT,
            } tightness;
            bool fail_bad_utf8;
            bool write_utf;

            EncodeOptions( Tightness t, bool f, bool u )
                : tightness(t)
                , fail_bad_utf8(f)
                , write_utf(u)
            { }
        };

        /*! @brief  Proxy class, alters how floating point numbers are output.
         */
        struct OptionedWrite {

            OptionedWrite( Json const &value )
                : m_value( &value )
                , m_encode_options(
                        EncodeOptions(EncodeOptions::TN_EXACT, false, false) )
            { }
            OptionedWrite( Json const &value, EncodeOptions encode_options )
                : m_value( &value )
                , m_encode_options( encode_options )
            { }
            /*! @brief  JSON stream insertion with nicer floating point values.
             */
            friend ostream &operator<<( ostream &os, OptionedWrite const &el ) {
                el.p_write( os );
                return os;
            }
            /*! @brief  Generic stream writer.
             *
             *  This template overload retains full stream type information.
             */
            template<typename OST,
                    typename = std::enable_if_t<
                        std::is_convertible_v<
                            std::remove_reference_t<OST>*,
                            ostream* >>>
            friend OST&& operator<<(
                    OST &&os,
                    OptionedWrite const &el
                    ) {
                el.p_write( os );
                return std::forward<OST>(os);
            }

            friend OptionedWrite loose_floats( OptionedWrite );
            friend OptionedWrite loose_doubles( OptionedWrite );
            friend OptionedWrite loose_long_doubles( OptionedWrite );
            friend OptionedWrite exact_numbers( OptionedWrite );
            friend OptionedWrite replace_bad_utf( OptionedWrite );
            friend OptionedWrite fail_bad_utf( OptionedWrite );
            friend OptionedWrite write_ASCII_strings( OptionedWrite );
            friend OptionedWrite write_utf_strings( OptionedWrite );
        private:
            void p_write( ostream &os ) const {
                m_value->p_write( os, m_encode_options );
            }

            Json const *m_value;
            EncodeOptions m_encode_options;
        };
        /*! @brief  Wrap JSON value in a proxy for nicer floating point output.
         */
        friend OptionedWrite loose_floats( OptionedWrite ow ) {
            ow.m_encode_options.tightness = EncodeOptions::TN_FLOAT;
            return ow;
        }

        /*! @brief  Wrap JSON value in a proxy for nicer floating point output.
         */
        friend OptionedWrite loose_doubles( OptionedWrite ow ) {
            ow.m_encode_options.tightness = EncodeOptions::TN_DOUBLE;
            return ow;
        }

        /*! @brief  Wrap JSON value in a proxy for nicer floating point output.
         */
        friend OptionedWrite loose_long_doubles( OptionedWrite ow ) {
            ow.m_encode_options.tightness = EncodeOptions::TN_LONG_DOUBLE;
            return ow;
        }

        /*! @brief  Wrap JSON value in proxy for accurate floating point output.
         *
         *  @note   This is currently the default output behavior.
         */
        friend OptionedWrite exact_numbers( OptionedWrite ow ) {
            ow.m_encode_options.tightness = EncodeOptions::TN_EXACT;
            return ow;
        }

        /*! @brief  Wrap JSON value in a proxy to absorb Unicode errors.
         *
         *  @note   This is currently the default output behavior.
         */
        friend OptionedWrite replace_bad_utf(OptionedWrite ow) {
            ow.m_encode_options.fail_bad_utf8 = false;
            return ow;
        }

        /*! @brief  Wrap JSON value in a proxy to throw Unicode errors.
         */
        friend OptionedWrite fail_bad_utf(OptionedWrite ow) {
            ow.m_encode_options.fail_bad_utf8 = true;
            return ow;
        }

        friend OptionedWrite write_ASCII_strings(OptionedWrite ow) {
            ow.m_encode_options.write_utf = false;
            return ow;
        }

        friend OptionedWrite write_utf_strings(OptionedWrite ow) {
            ow.m_encode_options.write_utf = true;
            return ow;
        }

        /*! @brief  Stream insertion (encoded output) for JSON.
         *  @note   This is a specific overload to give this function priority
         *      in some cases when another template might introduce
         *      an ambiguity with the template below.
         */
        friend ostream &operator<<( ostream &os, Json const &el ) {
            el.p_write(os, EncodeOptions(EncodeOptions::TN_EXACT,false,false));
            return os;
        }

        /*! @brief  Stream insertion (encoded output) for JSON.
         */
        template<typename OST,
                typename = std::enable_if_t<
                    std::is_convertible_v<
                        std::remove_reference_t<OST>*,
                        ostream* >>>
        friend OST&& operator<<(
                OST &&os,
                Json const &el
                ) {
            el.p_write(os, EncodeOptions(EncodeOptions::TN_EXACT,false,false));
            return std::forward<OST>(os);
        }

        struct ParseOptions {
            bool use_GN_for_floats;

            ParseOptions( bool use_GN_for_floats )
                : use_GN_for_floats(use_GN_for_floats)
            { }
        };

        /*! @brief  Proxy class, alters how floating point numbers are read.
         */
        struct OptionedParse {

            OptionedParse( Json &target )
                : m_target( &target )
                , m_parse_options( false )
            { }

            friend istream &operator>>( istream &is, OptionedParse const &el ) {
                el.p_parse( is );
                return is;
            }
            /*! @brief  Generic stream reader.
             *
             *  This template overload retains full stream type information.
             */
            template<typename OST,
                    typename = std::enable_if_t<
                        std::is_convertible_v<
                            std::remove_reference_t<OST>*,
                            istream* >>>
            friend OST&& operator>>(
                    OST &&os,
                    OptionedParse const &el
                    ) {
                el.p_parse( os );
                return std::forward<OST>(os);
            }

            friend OptionedParse use_GN_for_floats( OptionedParse );

        private:

            void p_parse( istream &is ) const {
                p_stream_extract( is, *m_target,
                        m_parse_options.use_GN_for_floats );
            }

            Json *m_target;
            ParseOptions m_parse_options;

        };
        /*! @brief  Wrap JSON value in a proxy for control over float input.
         */
        friend OptionedParse use_GN_for_floats( OptionedParse op ) {
            op.m_parse_options.use_GN_for_floats = true;
            return op;
        }

        /*! @brief  Stream extraction (parsing) for JSON.
         *
         *  @note   This operator intercepts parsing errors and
         *      sets @b failbit on the stream.
         *      If you want an exception for a bad parse,
         *      be sure to call @c is.exceptions(ios::failbit)
         *      to request exceptions from the stream.
         *
         *  @throw  Json::Error on bad parse if the stream throws on @b failbit.
         */
        template<typename IST>
        friend IST &operator>>( IST &is, Json &el ) {
            return p_stream_extract( is, el, false );
        }

        /*! @brief  Parse the given character range into Json.
         */
        static
        Json parse( char const *start, char const *finish );
        /*! @brief  Parse the given string or string_view into Json.
         */
        static
        Json parse( string_view str );
        /*! @brief  Parse JSON from a streambuf object.
         */
        static
        Json parse( streambuf &sbuf ) {
            return parse( sbuf, false );
        }
        /*! @brief  Parse JSON from a streambuf object.
         */
        static
        Json parse( streambuf &sbuf, bool use_GN_for_floats );

        /*! @brief  Encode the Json object as a string.
         */
        friend
        string encode( Json const &json );

        struct Error;
        struct TypeError;
        struct CastTypeError;
        struct CompoundTypeError;
        struct KeyError;
        struct ArrayKeyError;
        struct ObjectKeyError;
        struct ParseError;
        struct NumberParseError;
        struct UTFParseError;
        struct EOFParseError;
        struct StartEOFParseError;
        struct BadEOFParseError;
        struct TrailingCommaParseError;
        struct UnexpectedByteParseError;
        struct TrailingBytesParseError;
        struct EncodeError;
        struct EncodeByteError;
        struct EncodeCodepointError;

    private:
        ElementBasePtr m_el;

        void p_write(
                ostream &ost,
                EncodeOptions encode_options
                ) const;

        // Internal helper class in .cpp file:
        friend struct internal_grant;

        template<typename IST>
        static
        IST &p_stream_extract( IST &is, Json &el, bool use_GN_for_floats );

        static
        void p_set_elem( ObjectBody &as_obj, string key, Json value ) {
            as_obj.emplace_back( std::move( key ), std::move( value ) );
        }
        static
        void p_set_elem( ArrayBody &as_arr, size_t key, Json value ) {
            if ( as_arr.size() <= key )
                as_arr.resize( key+1 );
            as_arr[key] = std::move(value);
        }
        template<typename B, typename K1, typename K2, typename... R>
        static
        void p_set_elem(B &body,K1 &&k1,Json v1,K2 &&k2,Json v2,R &&...rest) {
            p_set_elem( body, ::std::forward<K1>(k1), std::move(v1) );
            p_set_elem( body, ::std::forward<K2>(k2), std::move(v2),
                    ::std::forward<R>(rest)... );
        }

        static
        int s_compare( Json lhs, Json rhs ) noexcept;
    };

    /*! @brief  Abstract base of JSON-related errors thrown by Json operations.
     */
    struct Json::Error : ios_base::failure {
        ~Error() noexcept override = 0;

        Json get_argument() const {
            return m_argument;
        }
        void set_argument( Json argument ) {
            m_argument = std::move(argument);
        }

        friend
        ostream &operator<<( ostream &os, Error const &self );
    protected:
        Error( string const &msg );
        virtual void v_print( ostream &os ) const;
        virtual char const *v_failtag() const { return "JSON Error"; }

        Json m_argument;
    };
    /*! @brief  Error thrown for an operation inappropriate for a type.
     */
    struct Json::TypeError : Error {
        ~TypeError() noexcept override = 0;

        TypeError( string op, char const *actual_type );
        char const *actual_type() const { return m_actual_type; }
    protected:
        char const *v_failtag() const override { return "JSON Type Error"; }
    private:
        char const *m_actual_type;
    };
    /*! @brief  Error thrown for a coercion operation on an incorrect type.
     */
    struct Json::CastTypeError : TypeError {
        CastTypeError( string op, char const *actual_type )
            : TypeError( std::move(op), actual_type )
        { }
    };
    /*! @brief  Error thrown for an array/object operation on invalid type.
     */
    struct Json::CompoundTypeError : TypeError {
        CompoundTypeError( string op, char const *actual_type )
            : TypeError( std::move(op), actual_type )
        { }
    };
    /*! @brief  Error thrown for a misuse of operator[].
     */
    struct Json::KeyError : Error {
        ~KeyError() noexcept override = 0;
    protected:
        KeyError( string const &msg );
        char const *v_failtag() const override { return "JSON Key Error"; }
    };
    /*! @brief  Error thrown for an out-of-bounds array index.
     */
    struct Json::ArrayKeyError : KeyError {
        ArrayKeyError( size_t key, size_t size );
    };
    /*! @brief  Error thrown for accessing an unset key in an object.
     */
    struct Json::ObjectKeyError : KeyError {
        ObjectKeyError( string key );
    };
    /*! @brief  Error thrown for bad JSON during parsing.
     */
    struct Json::ParseError : Error {
        ~ParseError() noexcept override = 0;
        ParseError( string const &message );

        string const &get_context() const { return m_context; }
        void add_context( streambuf &sbuf, size_t max_bytes );
        void add_context( istream &is, size_t max_bytes );
    protected:
        void v_print( ostream &os ) const override;
        char const *v_failtag() const override { return "JSON Parsing Error"; }
    private:
        string m_context;
    };
    /*! @brief  Specific error for a number parse failure.
     */
    struct Json::NumberParseError : ParseError {
        NumberParseError( string const &message );
    };
    /*! @brief  Specific error for a parse failure from bad Unicode.
     */
    struct Json::UTFParseError : ParseError {
        UTFParseError( string const &message );
    };
    /*! @brief  Specific error for end-of-file.
     */
    struct Json::EOFParseError : ParseError {
        ~EOFParseError() noexcept override = 0;
        EOFParseError( string const &message );
    };
    /*! @brief  Specific error for end-of-file at input start.
     */
    struct Json::StartEOFParseError : EOFParseError {
        StartEOFParseError( string const &message );
    };
    /*! @brief  Specific error for unexpected end-of-file.
     */
    struct Json::BadEOFParseError : EOFParseError {
        BadEOFParseError( string const &message );
        BadEOFParseError( StartEOFParseError const &err )
            : EOFParseError( err )
        { }
    };
    /*! @brief  Specific error for trailing comma, parsing an array or object.
     */
    struct Json::TrailingCommaParseError : ParseError {
        TrailingCommaParseError( string const &container_type );
    };
    /*! @brief  Error for unexpected bytes while reading a JSON entity.
     */
    struct Json::UnexpectedByteParseError : ParseError {
        UnexpectedByteParseError( string const &message, char got )
            : ParseError( message )
            , m_got( got )
        { }
        char get_got() const { return m_got; }
    protected:
        void v_print( ostream &os ) const override;
    private:
        char m_got;
    };
    /*! @brief  Error for bytes after reading a JSON from a bounded input range.
     */
    struct Json::TrailingBytesParseError : ParseError {
        TrailingBytesParseError( )
            : ParseError( "Trailing bytes" )
        { }
    };
    /*! @brief  Error thrown for bad JSON during output encoding.
     */
    struct Json::EncodeError : Error {
        ~EncodeError() noexcept override = 0;
        EncodeError( string const &message );
    protected:
        char const *v_failtag() const override { return "JSON Encoding Error"; }
    };
    /*! @brief  Error thrown for bad UTF-8 bytes during output encoding.
     */
    struct Json::EncodeByteError : EncodeError {
        EncodeByteError( string const &message );
    };
    /*! @brief  Error thrown for bad Unicode codepoint during output encoding.
     */
    struct Json::EncodeCodepointError : EncodeError {
        EncodeCodepointError( string const &message );
    };

    template<typename IST>
    IST &Json::p_stream_extract( IST &is, Json &el, bool use_GN_for_floats ) {
        try {
            el = parse( *is.rdbuf(), use_GN_for_floats );
        } catch ( Error const & ) {
            try {
                is.setstate( ios_base::failbit );
                return is;
            } catch ( ... ) {
                // Ignore the generated failure exception
                // and throw the original error.
            }
            throw;
        }
        return is;
    }

#define JSRL_DEFINE_INLINE_IS_TYPE( TYPENAME, TOKEN, FLAG )                 \
    inline bool Json::is_##TYPENAME() const noexcept {                      \
        return get_typetag(FLAG) == TT_##TOKEN;                             \
    }
    JSRL_DEFINE_INLINE_IS_TYPE( null,           NULL,                   false )
    JSRL_DEFINE_INLINE_IS_TYPE( bool,           BOOL,                   false )
    JSRL_DEFINE_INLINE_IS_TYPE( number,         NUMBER,                 false )
    JSRL_DEFINE_INLINE_IS_TYPE( number_general, NUMBER_GENERAL,         true  )
    JSRL_DEFINE_INLINE_IS_TYPE( number_float,   NUMBER,                 true  )
    JSRL_DEFINE_INLINE_IS_TYPE( number_sint,    NUMBER_INTEGER,         true  )
    JSRL_DEFINE_INLINE_IS_TYPE( number_uint,    NUMBER_INTEGER_UNSIGNED,true  )
    JSRL_DEFINE_INLINE_IS_TYPE( string,         STRING,                 false )
    JSRL_DEFINE_INLINE_IS_TYPE( array,          ARRAY,                  false )
    JSRL_DEFINE_INLINE_IS_TYPE( object,         OBJECT,                 false )
#undef JSRL_DEFINE_INLINE_IS_TYPE

#define JSRL_DEFINE_GET_AS( TYPE, CPPT )                                    \
    template<typename K>                                                    \
    CPPT Json::get_##TYPE(K &&key, CPPT default_value) const {             \
        Json const *element = find_key( std::forward<K>(key) );             \
        if ( not element )                                                  \
            return default_value;                                           \
        return element->as_##TYPE();                                        \
    }
    JSRL_DEFINE_GET_AS( bool, bool )
    JSRL_DEFINE_GET_AS( number_general, std::shared_ptr<GeneralNumber const> )
    JSRL_DEFINE_GET_AS( number_float, long double )
    JSRL_DEFINE_GET_AS( number_sint, long long )
    JSRL_DEFINE_GET_AS( number_uint, long long unsigned )
    JSRL_DEFINE_GET_AS( string, string const & )
    JSRL_DEFINE_GET_AS( array, Json::ArrayBody const & )
    JSRL_DEFINE_GET_AS( object, Json::ObjectBody const & )
#undef JSRL_DEFINE_GET_AS

    inline
    size_t Json::size() const {
        return as_array().size();
    }

    inline
    Json const *Json::find_key( size_t index ) const {
        ArrayBody const &body = as_array();
        if ( index >= body.size() )
            return nullptr;
        return &body[index];
    }
    inline
    Json const &Json::operator[]( size_t index ) const {
        try {
            Json const *element = find_key( index );
            if ( not element )
                throw Json::ArrayKeyError( index, size() );
            return *element;
        } catch ( TypeError const &e ) {
            throw CompoundTypeError( "operator[](size_t)", e.actual_type() );
        }
    }

    template<typename K, typename D>
    Json Json::get( K &&key, D &&default_val ) const {
        Json const *element = find_key( std::forward<K>(key) );
        if ( not element )
            return Json( std::forward<D>(default_val) );
        return *element;
    }
    template<typename K>
    Json Json::get( K &&key ) const {
        return get( std::forward<K>(key), Json() );
    }

    /*! @brief  Validate a block of bytes as valid UTF-8.
     *
     *  @throw Json::EncodeError    The block is invalid UTF-8.
     *
     *  @param[in] b    Beginning of the block to validate as UTF-8.
     *  @param[in] e    End of the block to validate as UTF-8.
     *                  If @c ( e == nullptr ) (or not given),
     *                  @c b is taken as a null-terminated string.
     */
    void validate_utf8( char const *b, char const *e = nullptr );

    /*! @overload
     *  @brief  Validate a string as valid UTF-8.
     */
    inline
    void validate_utf8( string const &s ) {
        char const *const b = s.c_str(), *const e = b + s.size();
        validate_utf8( b, e );
    }

    inline
    void push_back( Json::ArrayBody &self, Json element ) {
        self.push_back( std::move(element) );
    }
    inline
    void insert( Json::ObjectBody &self, string key, Json value ) {
        self.emplace_back( std::move( key ), std::move( value ) );
    }

    void resort( Json::ObjectBody &body );

    Json::ObjectBody::const_iterator find(
            Json::ObjectBody const &self,
            std::string_view key
            );
    inline
    Json::ObjectBody::iterator find(
            Json::ObjectBody &self,
            std::string_view key
            ) {
        Json::ObjectBody const &me = self;
        Json::ObjectBody::iterator self_begin = self.begin();
        return self_begin + (find( me, key )-self_begin);
    }

    namespace literals {
        inline
        auto operator ""_Json ( char const *s, size_t n ) -> Json {
            return Json::parse( s, s+n );
        }
    }

}
#endif
// vi: et ts=4 sts=4 sw=4
