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

#include "jsrl_test_helpers.hpp"
#include "../src/jsrl.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <iterator>
#include <sstream>
#include <iosfwd>
#include <map>
#include <vector>
#include <string>
#include <utility>
#include <limits>

// Macro to explicitly mark result as unused
#define UNUSED_RESULT(x) static_cast<void>(x)

namespace {
    using namespace jsrl::literals;
    using jsrl::Json;
    using jsrl::GeneralNumber;
    using jsrl::validate_utf8;
    using std::shared_ptr;
    using std::prev;
    using std::begin;
    using std::end;
    using std::istringstream;
    using std::ostringstream;
    using std::ios;
    using std::map;
    using std::vector;
    using std::string;
    using std::make_pair;
    using std::pair;
    using std::numeric_limits;

    auto is_null( Json const &json ) -> bool
    {
        return json.is_null();
    }
    auto is_bool( Json const &json ) -> bool
    {
        return json.is_bool();
    }
    auto is_number( Json const &json ) -> bool
    {
        return json.is_number();
    }
    auto is_number_general( Json const &json ) -> bool
    {
        return json.is_number_general();
    }
    auto is_number_float( Json const &json ) -> bool
    {
        return json.is_number_float();
    }
    auto is_number_integer( Json const &json ) -> bool
    {
        return json.is_number_integer();
    }
    auto is_number_sint( Json const &json ) -> bool
    {
        return json.is_number_sint();
    }
    auto is_number_uint( Json const &json ) -> bool
    {
        return json.is_number_uint();
    }
    auto is_string( Json const &json ) -> bool
    {
        return json.is_string();
    }
    auto is_array( Json const &json ) -> bool
    {
        return json.is_array();
    }
    auto is_object( Json const &json ) -> bool
    {
        return json.is_object();
    }
    auto isnt_null( Json const &json ) -> bool
    {
        return not is_null(json);
    }
    auto isnt_bool( Json const &json ) -> bool
    {
        return not is_bool(json);
    }
    auto isnt_number( Json const &json ) -> bool
    {
        return not is_number(json);
    }
    auto isnt_number_general( Json const &json ) -> bool
    {
        return not is_number_general(json);
    }
    auto isnt_number_float( Json const &json ) -> bool
    {
        return not is_number_float(json);
    }
    auto isnt_number_integer( Json const &json ) -> bool
    {
        return not is_number_integer(json);
    }
    auto isnt_number_sint( Json const &json ) -> bool
    {
        return not is_number_sint(json);
    }
    auto isnt_number_uint( Json const &json ) -> bool
    {
        return not is_number_uint(json);
    }
    auto isnt_string( Json const &json ) -> bool
    {
        return not is_string(json);
    }
    auto isnt_array( Json const &json ) -> bool
    {
        return not is_array(json);
    }
    auto isnt_object( Json const &json ) -> bool
    {
        return not is_object(json);
    }

    auto has_key( Json const &json, string const &key ) -> bool
    {
        return json.has_key( key );
    }
    auto doesnt_have_key( Json const &json, string const &key ) -> bool
    {
        return not has_key( json, key );
    }
}

TEST(Jsrl,BasicParse) {
    istringstream jsonstr(R"JSON({}[])JSON");
    jsonstr.exceptions(ios::failbit);
    Json json;
    jsonstr >> json;
    EXPECT_PRED1( is_object, json );
    jsonstr >> json;
    EXPECT_PRED1( is_array, json );
    EXPECT_EQ( 0, json.size() );
    EXPECT_THROW(jsonstr >> json, Json::ParseError);
}

TEST(Jsrl,ParseObject) {
    istringstream jsonstr(
        R"JSON({ "Hello" : "World" , "Foo":1234,"Float":-1234.5e-1,)JSON"
        R"JSON("Magic":"/\"\\\b\f\n\r\t\u001b" })JSON");
    Json json;
    jsonstr >> json;
    EXPECT_PRED1( is_object, json );
    EXPECT_EQ( Json::TT_OBJECT, json.get_typetag(false) );
    EXPECT_EQ( string("World"), json["Hello"].as_string() );
    EXPECT_EQ( 1234, json["Foo"].as_number_sint() );
    {
        vector<pair<string,Json> > expected{
                    {"Float",Json::parse("-123.45")},
                    {"Foo",Json(1234)},
                    {"Hello",Json("World")},
                    {"Magic",Json("/\"\\\b\f\n\r\t\x1b")},
                };
        auto xp = expected.cbegin();
        for ( auto const &el : json.as_object() ) {
            ASSERT_EQ( xp->first, el.first );
            auto const typetag = xp->second.get_typetag(true);
            ASSERT_EQ( typetag, el.second.get_typetag(true) );
            switch ( typetag ) {
            case Json::TT_STRING:
                {
                    EXPECT_EQ( xp->second.as_string(), el.second.as_string() );
                    break;
                }
            case Json::TT_NUMBER_INTEGER_UNSIGNED:
                {
                    EXPECT_EQ(
                            xp->second.as_number_uint(),
                            el.second.as_number_uint() );
                    break;
                }
            case Json::TT_NUMBER_INTEGER:
                {
                    EXPECT_EQ(
                            xp->second.as_number_sint(),
                            el.second.as_number_sint() );
                    break;
                }
            case Json::TT_NUMBER_GENERAL:
                {
                    EXPECT_EQ(
                            *xp->second.as_number_general(),
                            *el.second.as_number_general() );
                    break;
                }
            case Json::TT_NUMBER:
                {
                    EXPECT_DOUBLE_EQ(
                            xp->second.as_number_float(),
                            el.second.as_number_float() );
                    break;
                }
            default:
                {
                    FAIL();
                }
            }
            EXPECT_NE( xp, expected.cend() );
            ++xp;
        }
        EXPECT_EQ( xp, expected.cend() );
    }
}

TEST(Jsrl,Constructors) {
    Json const
            json_null,
            json_bool(false),
            json_number_g(GeneralNumber(3.625L)),
            json_number_f(3.625F),
            json_number_d(3.625),
            json_number_ld(3.625L),
            json_number_integer_s(static_cast<short>(0)),
            json_number_integer_su(static_cast<short unsigned>(0)),
            json_number_integer_i(0),
            json_number_integer_iu(0U),
            json_number_integer_l(0L),
            json_number_integer_lu(0LU),
            json_number_integer_ll(0LL),
            json_number_integer_llu(0LLU),
            json_string_c(""),
            json_string_cpp(string{}),
            json_array(vector<Json>{Json()}),
            json_object_m(map<string,Json>{}),
            json_object_v(vector<pair<string,Json> >{});
    EXPECT_PRED1( is_bool, json_bool );
    EXPECT_PRED1( is_bool, json_bool );
    EXPECT_PRED1( is_number, json_number_g );
    EXPECT_PRED1( is_number_general, json_number_g );
    EXPECT_PRED1( is_number, json_number_f );
    EXPECT_PRED1( is_number_float, json_number_f );
    EXPECT_PRED1( is_number, json_number_d );
    EXPECT_PRED1( is_number_float, json_number_d );
    EXPECT_PRED1( is_number, json_number_ld );
    EXPECT_PRED1( is_number_float, json_number_ld );
    EXPECT_PRED1( is_number, json_number_integer_s );
    EXPECT_PRED1( is_number_integer, json_number_integer_s );
    //EXPECT_PRED1( is_number_sint, json_number_integer_s );
    EXPECT_PRED1( is_number, json_number_integer_su );
    EXPECT_PRED1( is_number_integer, json_number_integer_su );
    EXPECT_PRED1( is_number_uint, json_number_integer_su );
    EXPECT_PRED1( is_number, json_number_integer_i );
    EXPECT_PRED1( is_number_integer, json_number_integer_i );
    //EXPECT_PRED1( is_number_sint, json_number_integer_i );
    EXPECT_PRED1( is_number, json_number_integer_iu );
    EXPECT_PRED1( is_number_integer, json_number_integer_iu );
    EXPECT_PRED1( is_number_uint, json_number_integer_iu );
    EXPECT_PRED1( is_number, json_number_integer_l );
    EXPECT_PRED1( is_number_integer, json_number_integer_l );
    //EXPECT_PRED1( is_number_sint, json_number_integer_l );
    EXPECT_PRED1( is_number, json_number_integer_lu );
    EXPECT_PRED1( is_number_integer, json_number_integer_lu );
    EXPECT_PRED1( is_number_uint, json_number_integer_lu );
    EXPECT_PRED1( is_number, json_number_integer_ll );
    EXPECT_PRED1( is_number_integer, json_number_integer_ll );
    //EXPECT_PRED1( is_number_sint, json_number_integer_ll );
    EXPECT_PRED1( is_number, json_number_integer_llu );
    EXPECT_PRED1( is_number_integer, json_number_integer_llu );
    EXPECT_PRED1( is_number_uint, json_number_integer_llu );
    EXPECT_PRED1( is_string, json_string_c );
    EXPECT_PRED1( is_string, json_string_cpp );
    EXPECT_PRED1( is_array, json_array );
    EXPECT_PRED1( is_object, json_object_m );
    EXPECT_PRED1( is_object, json_object_v );
    EXPECT_DOUBLE_EQ( json_number_f.as_number_float(),
            json_number_d.as_number_float() );
    EXPECT_DOUBLE_EQ( json_number_d.as_number_float(),
            json_number_ld.as_number_float() );
    EXPECT_DOUBLE_EQ( json_number_ld.as_number_float(),
            json_number_g.as_number_float() );
#define EQCHAIN( SFX1, SFX2 ) \
    EXPECT_EQ( json_number_integer_ ## SFX1.as_number_sint(), \
            json_number_integer_ ## SFX2.as_number_sint() );
    EQCHAIN( s, su );
    EQCHAIN( i, su );
    EQCHAIN( i, iu );
    EQCHAIN( l, iu );
    EQCHAIN( l, lu );
    EQCHAIN( ll, lu );
    EQCHAIN( ll, llu );
    EQCHAIN( s, llu );
#undef EQCHAIN
}

TEST(Jsrl,ConstructArrayFromArrayBody) {
    auto array_body = Json::ArrayBody{};
    push_back( array_body, Json() );
    push_back( array_body, Json(false) );
    push_back( array_body, Json(0) );
    push_back( array_body, Json(GeneralNumber(0.0L)) );
    push_back( array_body, Json("") );
    Json json( std::move(array_body) );
    EXPECT_EQ( R"JSON([null,false,0,0.0,""])JSON"_Json, json );
}

TEST(Jsrl,ConstructObjectFromObjectBody) {
    auto object_body = Json::ObjectBody{};
    insert( object_body, "n", Json() );
    insert( object_body, "b", Json(false) );
    insert( object_body, "i", Json(0) );
    insert( object_body, "r", Json(0.0) );
    insert( object_body, "s", Json("") );
    Json json( std::move(object_body) );
    EXPECT_EQ( R"JSON({"n":null,"b":false,"i":0,"r":0.0,"s":""})JSON"_Json,
            json );
}

TEST(Jsrl,ConstructObjectFromMap) {
    auto object_body = map<string,Json>{};
    object_body.emplace( "n", Json() );
    object_body.emplace( "b", Json(false) );
    object_body.emplace( "i", Json(0) );
    object_body.emplace( "r", Json(0.0) );
    object_body.emplace( "s", Json("") );
    Json json( std::move(object_body) );
    EXPECT_EQ( R"JSON({"n":null,"b":false,"i":0,"r":0.0,"s":""})JSON"_Json,
            json );
}

TEST(Jsrl,ConstructObjectFromVector) {
    auto object_body = vector<pair<string,Json> >{};
    object_body.emplace_back( "n", Json() );
    object_body.emplace_back( "b", Json(false) );
    object_body.emplace_back( "i", Json(0) );
    object_body.emplace_back( "r", Json(0.0) );
    object_body.emplace_back( "s", Json("") );
    Json json( std::move(object_body) );
    EXPECT_EQ( R"JSON({"n":null,"b":false,"i":0,"r":0.0,"s":""})JSON"_Json,
            json );
}

TEST(Jsrl,ConstructArrayFromIterators) {
    static char const *const teststrings[] = {
                "foo",
                "bar",
                "baz",
                "",
            };
    Json json( begin(teststrings), end(teststrings) );
    EXPECT_EQ( string(R"JSON(["foo","bar","baz",""])JSON"), encode(json) );
    EXPECT_EQ( json, Json( begin(json.as_array()), end(json.as_array()) ) );
}
TEST(Jsrl,ConstructArrayFromIterators_IgnoreBadUnicode) {
    static char const *const teststrings[] = {
                "foo",
                "bar\xFFrab",
                "baz",
                "",
            };
    EXPECT_THROW(Json(begin(teststrings), end(teststrings)), Json::EncodeError);
    Json json( begin(teststrings), end(teststrings), Json::ignore_bad_unicode );
    EXPECT_EQ( string(R"JSON(["foo","bar\ufffdrab","baz",""])JSON"),
            encode(json) );
}

TEST(Jsrl,ConstructObjectFromIterators) {
    static pair<char const*,char const*> const teststrings[] = {
                { "foo", "FOO" },
                { "bar", "BAR" },
                { "baz", "BAZ" },
            };
    Json json( begin(teststrings), end(teststrings) );
    EXPECT_EQ( string(R"JSON({"bar":"BAR","baz":"BAZ","foo":"FOO"})JSON"),
            encode(json) );
    EXPECT_EQ( json, Json( begin(json.as_object()), end(json.as_object()) ) );
}
TEST(Jsrl,ConstructObjectFromIterators_IgnoreBadUnicode) {
    static pair<char const*,char const*> const teststrings[] = {
                { "foo", "FOO" },
                { "bar\xFFrab", "BAR\xFFRAB" },
                { "baz", "BAZ" },
            };
    EXPECT_THROW(Json(begin(teststrings), end(teststrings)), Json::EncodeError);
    Json json( begin(teststrings), end(teststrings), Json::ignore_bad_unicode );
    EXPECT_EQ( string(  R"JSON({"bar\ufffdrab":"BAR\ufffdRAB")JSON"
                        R"JSON(,"baz":"BAZ","foo":"FOO"})JSON"), encode(json) );
}

TEST(Jsrl,ConstructEncode) {
    Json json( map<string,Json>{
                {"SomeArray",Json(vector<Json>{
                        Json{},
                        Json(false),
                        Json(true)
                    })},
                {"SomeInt",Json(12345)},
                {"SomeString",Json("StringValue")},
            } );
    ostringstream oss;
    oss << json;
    EXPECT_EQ( string(
                R"JSON({"SomeArray":[null,false,true],)JSON"
                R"JSON("SomeInt":12345,)JSON"
                R"JSON("SomeString":"StringValue")JSON"
                R"JSON(})JSON"), oss.str() );
}
TEST(Jsrl,ConstructFloat) {
    Json json(-123.456L);
    ASSERT_PRED1( is_number, json );
    ASSERT_PRED1( is_number_float, json );
    ASSERT_PRED1( isnt_number_general, json );
    ASSERT_PRED1( isnt_number_integer, json );
    ASSERT_PRED1( isnt_number_sint, json );
    ASSERT_PRED1( isnt_number_uint, json );
    EXPECT_DOUBLE_EQ( -123.456L, json.as_number_float() );
    {
        ostringstream oss;
        oss << json;
        istringstream iss(std::move(oss).str());
        long double value;
        iss >> value;
        EXPECT_DOUBLE_EQ( -123.456L, value );
    }
}

TEST(Jsrl,LooseFloats) {
    vector<long double> testcases{ 0.05L, 0.01L, -123.456L,
            1.05L, 2.05L, 4.05L, 8.05L, 16.05L, 32.05L,
            1.01L, 2.01L, 4.01L, 8.01L, 16.01L, 32.01L };
    string result = "[0.05,0.01,-123.456,"
            "1.05,2.05,4.05,8.05,16.05,32.05,"
            "1.01,2.01,4.01,8.01,16.01,32.01]";
    if ( test_precision_check<long double>() ) {
        vector<Json> v;
        for ( long double num : testcases )
            v.push_back( Json(num) );
        Json json(std::move(v));
        ostringstream oss;
        oss << loose_long_doubles(json);
        EXPECT_EQ( result, oss.str() );
    }
    if ( test_precision_check<double>() ) {
        vector<Json> v;
        for ( double num : testcases )
            v.push_back( Json(num) );
        Json json(std::move(v));
        ostringstream oss;
        oss << loose_doubles(json);
        EXPECT_EQ( result, oss.str() );
    }
    if ( test_precision_check<float>() ) {
        vector<Json> v;
        for ( float num : testcases )
            v.push_back( Json(num) );
        Json json(std::move(v));
        ostringstream oss;
        oss << loose_floats(json);
        EXPECT_EQ( result, oss.str() );
    }
}

TEST(Jsrl,Unsigned) {
    static auto const llmin = numeric_limits<long long>::min();
    static auto const llmax = numeric_limits<long long>::max();
    static auto const ullmax = numeric_limits<long long unsigned>::max();
    ostringstream oss;
    oss << R"JSON({ "llmin" : )JSON" << llmin
        << R"JSON(, "llmax" : )JSON" << llmax
        << R"JSON(, "ullmax" : )JSON" << ullmax
        << R"JSON( })JSON";
    Json json = Json::parse(oss.str());

    EXPECT_PRED1( is_number, json["llmin"] );
    EXPECT_PRED1( is_number_integer, json["llmin"] );
    EXPECT_PRED1( is_number_sint, json["llmin"] );
    EXPECT_EQ( llmin, json["llmin"].as_number_sint() );
    EXPECT_PRED1( isnt_number_uint, json["llmin"] );
    EXPECT_THROW( json["llmin"].as_number_uint(), Json::TypeError );

    EXPECT_PRED1( is_number, json["llmax"] );
    EXPECT_PRED1( is_number_integer, json["llmax"] );
    //EXPECT_PRED1( is_number_sint, json["llmax"] );
    EXPECT_EQ( llmax, json["llmax"].as_number_sint() );
    EXPECT_PRED1( is_number_uint, json["llmax"] );
    EXPECT_EQ( llmax, json["llmax"].as_number_uint() );

    EXPECT_PRED1( is_number, json["ullmax"] );
    EXPECT_PRED1( is_number_integer, json["ullmax"] );
    EXPECT_PRED1( isnt_number_sint, json["ullmax"] );
    EXPECT_EQ( llmax, json["ullmax"].as_number_sint() );
    EXPECT_PRED1( is_number_uint, json["ullmax"] );
    EXPECT_EQ( ullmax, json["ullmax"].as_number_uint() );
}
TEST(Jsrl,Decode) {
    istringstream iss(
            R"JSON([{},{"Foo":"Bar"},{"Foo":"Bar","Baz":"Mumble"},)JSON"
            R"JSON({"Foo":{"Foo":{}}},)JSON"
            R"JSON([],["Foo"],["Foo","Bar"]])JSON");
    Json json;
    iss >> json;
    ASSERT_PRED1( is_array, json );
    ASSERT_EQ( 7, json.size() );
    ASSERT_PRED1( is_object, json[0] );
    ASSERT_PRED1( is_null, json[0].get("Foo") );
    ASSERT_PRED1( is_null, json[0].get("Baz") );
    ASSERT_PRED1( is_object, json[1] );
    ASSERT_PRED1( isnt_null, json[1]["Foo"] );
    ASSERT_PRED1( is_string, json[1]["Foo"] );
    EXPECT_EQ( string("Bar"), json[1]["Foo"].as_string() );
    ASSERT_PRED1( is_null, json[1].get("Baz") );
    ASSERT_PRED1( is_object, json[2] );
    ASSERT_PRED1( isnt_null, json[2]["Foo"] );
    ASSERT_PRED1( is_string, json[2]["Foo"] );
    EXPECT_EQ( string("Bar"), json[2]["Foo"].as_string() );
    ASSERT_PRED1( isnt_null, json[2]["Baz"] );
    ASSERT_PRED1( is_string, json[2]["Baz"] );
    EXPECT_EQ( string("Mumble"), json[2]["Baz"].as_string() );
    ASSERT_PRED1( is_object, json[3] );
    ASSERT_PRED1( is_object, json[3]["Foo"] );
    ASSERT_PRED1( is_object, json[3]["Foo"]["Foo"] );
    ASSERT_PRED1( is_null, json[3]["Foo"]["Foo"].get("Foo") );
    ASSERT_PRED1( is_array, json[4] );
    ASSERT_EQ( json[4].size(), 0 );
    ASSERT_PRED1( is_array, json[5] );
    ASSERT_EQ( json[5].size(), 1 );
    ASSERT_EQ( string("Foo"), json[5][0].as_string() );
    ASSERT_PRED1( is_array, json[6] );
    ASSERT_EQ( json[6].size(), 2 );
    ASSERT_PRED1( is_string, json[6][0] );
    EXPECT_EQ( string("Foo"), json[6][0].as_string() );
    ASSERT_PRED1( is_string, json[6][1] );
    EXPECT_EQ( string("Bar"), json[6][1].as_string() );
}
TEST(Jsrl,Encode) {
    Json json( vector<Json>{
                Json(vector<Json>{}),
                Json(map<string,Json>{}),
                Json(),
                Json(false),
                Json(true),
                Json(0),
                Json(123),
                Json(-456),
                Json("A String"),
                Json("\x1b\\\"/\b\f\n\r\t"),
            } );
    ostringstream oss;
    oss << json;
    EXPECT_EQ(
            string(
                R"JSON([[],{},null,false,true,0,123,-456,"A String",)JSON"
                R"JSON("\u001b\\\"/\b\f\n\r\t"])JSON" ),
            oss.str() );
}
TEST(Jsrl,Unicode) {
    istringstream iss("[ "
            "\"|\xC2\xA9|\\u00A9|\u00A9|\", "
            "\"|\xC2\xA3|\\u00a3|\u00A3|\", "
            "\"|\xC2\xB6|\\u00B6|\u00B6|\", "
            "\"|\xE8\x87\xBB|\\u81FB|\u81FB|\", "
            "\"|\xF0\x90\x8C\x88|\\uD800\\uDF08|\U00010308|\", "
            "\"|\xF0\xA0\x95\x87|\\uD841\\uDD47|\U00020547|\", "
            "\"|\xF3\xA0\x80\x81     \xF3\xA0\x81\xBF     |"
                "\\uDB40\\uDC01     \\uDB40\\uDC7F     |\", "
            "\"|\xEF\xBF\x80\xEF\xBC\x80|\\uFFC0\\uFF00|\uFFC0\uFF00|\" "
            "]");
    Json json;
    iss >> json;
    ASSERT_PRED1( is_array, json );
    ASSERT_EQ( 8, json.size() );
    EXPECT_EQ(string("|\u00A9|\u00A9|\u00A9|"),json[0].as_string());
    EXPECT_EQ(string("|\u00A3|\u00A3|\u00A3|"),json[1].as_string());
    EXPECT_EQ(string("|\u00B6|\u00B6|\u00B6|"),json[2].as_string());
    EXPECT_EQ(string("|\u81FB|\u81FB|\u81FB|"),json[3].as_string());
    EXPECT_EQ(string("|\U00010308|\U00010308|\U00010308|"),json[4].as_string());
    EXPECT_EQ(string("|\U00020547|\U00020547|\U00020547|"),json[5].as_string());
    EXPECT_EQ(string("|\U000E0001     \U000E007F     |"
                      "\U000E0001     \U000E007F     |"),json[6].as_string());
    EXPECT_EQ(string("|\uFFC0\uFF00|\uFFC0\uFF00|"
                      "\uFFC0\uFF00|"),json[7].as_string());
    ostringstream oss;
    oss << json;
    EXPECT_EQ( string("["
            "\"|\\u00a9|\\u00a9|\\u00a9|\","
            "\"|\\u00a3|\\u00a3|\\u00a3|\","
            "\"|\\u00b6|\\u00b6|\\u00b6|\","
            "\"|\\u81fb|\\u81fb|\\u81fb|\","
            "\"|\\ud800\\udf08|\\ud800\\udf08|\\ud800\\udf08|\","
            "\"|\\ud841\\udd47|\\ud841\\udd47|\\ud841\\udd47|\","
            "\"|\\udb40\\udc01     \\udb40\\udc7f     |"
                "\\udb40\\udc01     \\udb40\\udc7f     |\","
            "\"|\\uffc0\\uff00|\\uffc0\\uff00|\\uffc0\\uff00|\""
            "]"), oss.str() );
}

TEST( Jsrl,Get ) {
    istringstream iss(R"JSON({ "realkey": "realvalue", "k2": 2 } )JSON");
    Json json;
    iss >> json;
    EXPECT_PRED1( isnt_null, json.get( "realkey" ) );
    EXPECT_PRED1( is_string, json.get( "realkey" ) );
    EXPECT_PRED1( isnt_string, json.get( "k2" ) );
    EXPECT_PRED1( is_number, json.get( "k2" ) );
    EXPECT_PRED1( isnt_number_float, json.get( "k2" ) );
    EXPECT_PRED1( is_number_integer, json.get( "k2" ) );
    EXPECT_PRED1( isnt_number_sint, json.get( "k2" ) );
    EXPECT_PRED1( is_number_uint, json.get( "k2" ) );
    EXPECT_PRED1( is_null, json.get( "fakekey" ) );
    EXPECT_PRED1( isnt_null, json.get( "realkey", "replacement" ) );
    EXPECT_PRED1( is_string, json.get( "realkey", "replacement" ) );
    EXPECT_PRED1( isnt_string, json.get( "k2", "replacement" ) );
    EXPECT_PRED1( is_number, json.get( "k2", "replacement" ) );
    EXPECT_PRED1( isnt_number_float, json.get( "k2", "replacement" ) );
    EXPECT_PRED1( is_number_integer, json.get( "k2", "replacement" ) );
    EXPECT_PRED1( isnt_number_sint, json.get( "k2", "replacement" ) );
    EXPECT_PRED1( is_number_uint, json.get( "k2", "replacement" ) );
    EXPECT_PRED1( isnt_null, json.get( "fakekey", "replacement" ) );
    EXPECT_PRED1( is_string, json.get( "fakekey", "replacement" ) );
    EXPECT_EQ( string("realvalue"), json.get( "realkey", "repl" ).as_string() );
    EXPECT_EQ( string("repl"), json.get( "fakekey", "repl" ).as_string() );
}

TEST( Jsrl,GetPtr ) {
    shared_ptr<string const> s;
    shared_ptr<Json::ArrayBody const> a;
    shared_ptr<Json::ObjectBody const> o;
    EXPECT_FALSE( s );
    EXPECT_FALSE( a );
    EXPECT_FALSE( o );
    {
        auto json = R"JSON(["foo",[false,true,null],{"bar":123}])JSON"_Json;
        s = json[0].as_string_ptr();
        a = json[1].as_array_ptr();
        o = json[2].as_object_ptr();
        EXPECT_THROW( json[0].as_array_ptr(), Json::TypeError );
        EXPECT_THROW( json[0].as_object_ptr(), Json::TypeError );
        EXPECT_THROW( json[1].as_string_ptr(), Json::TypeError );
        EXPECT_THROW( json[1].as_object_ptr(), Json::TypeError );
        EXPECT_THROW( json[2].as_string_ptr(), Json::TypeError );
        EXPECT_THROW( json[2].as_array_ptr(), Json::TypeError );
        EXPECT_THROW( json[1][0].as_string_ptr(), Json::TypeError );
        EXPECT_THROW( json[1][0].as_array_ptr(), Json::TypeError );
        EXPECT_THROW( json[1][0].as_object_ptr(), Json::TypeError );
        EXPECT_THROW( json[1][2].as_string_ptr(), Json::TypeError );
        EXPECT_THROW( json[1][2].as_array_ptr(), Json::TypeError );
        EXPECT_THROW( json[1][2].as_object_ptr(), Json::TypeError );
        EXPECT_THROW( json[2]["bar"].as_string_ptr(), Json::TypeError );
        EXPECT_THROW( json[2]["bar"].as_array_ptr(), Json::TypeError );
        EXPECT_THROW( json[2]["bar"].as_object_ptr(), Json::TypeError );
        EXPECT_THROW( json[1][3].as_string_ptr(), Json::ArrayKeyError );
        EXPECT_THROW( json[2]["baz"].as_string_ptr(), Json::ObjectKeyError );
        // Original Json object goes out of scope here.
        // The individual elements should still be live
        // through the saved shared_ptr<>s.
    }
    ASSERT_TRUE( s );
    ASSERT_TRUE( a );
    ASSERT_TRUE( o );
    EXPECT_EQ( string("foo"), *s );
    EXPECT_EQ( 3, a->size() );
    EXPECT_TRUE( (*a)[1].as_bool() );
    EXPECT_THROW( (*a)[2].as_bool(), Json::TypeError );
    EXPECT_EQ( 123, find(*o,"bar")->second.as_number_sint() );
    EXPECT_EQ( end(*o), find(*o,"baz") );
}

TEST( Jsrl,GetAs ) {
    Json const json_null;
    Json const json_bool( false );
    Json const json_number( 0.0 );
    Json const json_integer( 0 );
    Json const json_string( "" );
    Json const json_array=R"JSON([null,false,0,0.0,"",[],{}])JSON"_Json;
    Json const json_object
            = R"JSON({
                "n":null,
                "b":false,
                "i":0,
                "r":0.0,
                "s":"",
                "a":[],
                "o":{} })JSON"_Json;
    for ( Json const &json : { json_null, json_bool,
            json_number, json_integer, json_string } ) {
        EXPECT_THROW(json.get_bool(0,false),Json::TypeError);
        EXPECT_THROW(json.get_bool("",false),Json::TypeError);
        EXPECT_THROW(json.get_number_float(0,0),Json::TypeError);
        EXPECT_THROW(json.get_number_float("",0),Json::TypeError);
        EXPECT_THROW(json.get_number_sint(0,0),Json::TypeError);
        EXPECT_THROW(json.get_number_sint("",0),Json::TypeError);
        EXPECT_THROW(json.get_string(0,""),Json::TypeError);
        EXPECT_THROW(json.get_string("",""),Json::TypeError);
        EXPECT_THROW(json.get_array(0,vector<Json>{}),Json::TypeError);
        EXPECT_THROW(json.get_array("",vector<Json>{}),Json::TypeError);
        EXPECT_THROW(json.get_object(0,Json::ObjectBody{}),Json::TypeError);
        EXPECT_THROW(json.get_object("",Json::ObjectBody{}),Json::TypeError);
    }
    EXPECT_THROW(json_array.get_bool("",false),Json::TypeError);
    EXPECT_THROW(json_array.get_number_float("",0),Json::TypeError);
    EXPECT_THROW(json_array.get_number_sint("",0),Json::TypeError);
    EXPECT_THROW(json_array.get_string("",""),Json::TypeError);
    EXPECT_THROW(json_array.get_array("",vector<Json>{}),Json::TypeError);
    EXPECT_THROW(json_array.get_object("",Json::ObjectBody{}),Json::TypeError);
    for ( size_t i=0; i!=json_array.size()+2; ++i ) {
        SCOPED_TRACE( "Element " + string(1,'0'+i) );
        if ( i>=7 ) {
            EXPECT_FALSE( json_array.get_bool(i,false) );
            EXPECT_TRUE( json_array.get_bool(i,true) );
        } else if ( i==1 ) {
            EXPECT_FALSE( json_array.get_bool(i,false) );
            EXPECT_FALSE( json_array.get_bool(i,true) );
        } else {
            EXPECT_THROW( json_array.get_bool(i,false), Json::TypeError );
            EXPECT_THROW( json_array.get_bool(i,true), Json::TypeError );
        }
        if ( i>=7 ) {
            EXPECT_DOUBLE_EQ( 0.0, json_array.get_number_float(i,0.0) );
            EXPECT_DOUBLE_EQ( 1.0, json_array.get_number_float(i,1.0) );
        } else if ( i>=2 and i<=3 ) {
            EXPECT_DOUBLE_EQ( 0.0, json_array.get_number_float(i,0.0) );
            EXPECT_DOUBLE_EQ( 0.0, json_array.get_number_float(i,1.0) );
        } else {
            EXPECT_THROW( json_array.get_number_float(i,0.0), Json::TypeError );
            EXPECT_THROW( json_array.get_number_float(i,1.0), Json::TypeError );
        }
        if ( i>=7 ) {
            EXPECT_EQ( 0, json_array.get_number_sint(i,0) );
            EXPECT_EQ( 1, json_array.get_number_sint(i,1) );
        } else if ( i==2 ) {
            EXPECT_EQ( 0, json_array.get_number_sint(i,0) );
            EXPECT_EQ( 0, json_array.get_number_sint(i,1) );
        } else {
            EXPECT_THROW( json_array.get_number_sint(i,0), Json::TypeError );
            EXPECT_THROW( json_array.get_number_sint(i,1), Json::TypeError );
        }
        if ( i>=7 ) {
            EXPECT_EQ( string(""), json_array.get_string(i,"") );
            EXPECT_EQ( string("-"), json_array.get_string(i,"-") );
        } else if ( i==4 ) {
            EXPECT_EQ( string(""), json_array.get_string(i,"") );
            EXPECT_EQ( string(""), json_array.get_string(i,"-") );
        } else {
            EXPECT_THROW( json_array.get_string(i,""), Json::TypeError );
            EXPECT_THROW( json_array.get_string(i,"-"), Json::TypeError );
        }
        if ( i>=7 ) {
            EXPECT_EQ( 0, json_array.get_array(i,vector<Json>{}).size() );
            EXPECT_EQ( 1, json_array.get_array(i,{Json{}}).size() );
        } else if ( i==5 ) {
            EXPECT_EQ( 0, json_array.get_array(i,vector<Json>{}).size() );
            EXPECT_EQ( 0, json_array.get_array(i,{Json{}}).size() );
        } else {
            EXPECT_THROW( UNUSED_RESULT(json_array.get_array(i,vector<Json>{}).size()),
                    Json::TypeError );
            EXPECT_THROW( UNUSED_RESULT(json_array.get_array(i,{Json{}}).size()),
                    Json::TypeError );
        }
        if ( i>=7 ) {
            EXPECT_EQ( 0, json_array.get_object(i,Json::ObjectBody{}).size() );
            EXPECT_EQ( 1, json_array.get_object(i,{{"",Json{}}}).size() );
        } else if ( i==6 ) {
            EXPECT_EQ( 0, json_array.get_object(i,Json::ObjectBody{}).size() );
            EXPECT_EQ( 0, json_array.get_object(i,{{"",Json{}}}).size() );
        } else {
            EXPECT_THROW( UNUSED_RESULT(json_array.get_object(i,Json::ObjectBody{}).size()),
                    Json::TypeError );
            EXPECT_THROW( UNUSED_RESULT(json_array.get_object(i,{{"",Json{}}}).size()),
                    Json::TypeError );
        }
    }
    EXPECT_THROW(json_object.get_bool(0,false),Json::TypeError);
    EXPECT_THROW(json_object.get_number_float(0,0),Json::TypeError);
    EXPECT_THROW(json_object.get_number_sint(0,0),Json::TypeError);
    EXPECT_THROW(json_object.get_string(0,""),Json::TypeError);
    EXPECT_THROW(json_object.get_array(0,vector<Json>{}),Json::TypeError);
    EXPECT_THROW(json_object.get_object(0,Json::ObjectBody{}),Json::TypeError);
    for ( string const key : { "n", "b", "i", "r", "s", "a", "o", "x" } ) {
        if ( key == "b" ) {
            EXPECT_FALSE( json_object.get_bool(key,false) );
            EXPECT_FALSE( json_object.get_bool(key,true) );
        } else if ( key == "x" ) {
            EXPECT_FALSE( json_object.get_bool(key,false) );
            EXPECT_TRUE( json_object.get_bool(key,true) );
        } else {
            EXPECT_THROW( json_object.get_bool(key,false), Json::TypeError );
            EXPECT_THROW( json_object.get_bool(key,true), Json::TypeError );
        }
        if ( key == "i" ) {
            EXPECT_EQ( 0, json_object.get_number_sint(key,0) );
            EXPECT_EQ( 0, json_object.get_number_sint(key,1) );
        } else if ( key == "x" ) {
            EXPECT_EQ( 0, json_object.get_number_sint(key,0) );
            EXPECT_EQ( 1, json_object.get_number_sint(key,1) );
        } else {
            EXPECT_THROW(json_object.get_number_sint(key,0),Json::TypeError);
            EXPECT_THROW(json_object.get_number_sint(key,1),Json::TypeError);
        }
        if ( key == "i" or key == "r" ) {
            EXPECT_DOUBLE_EQ( 0.0, json_object.get_number_float(key,0.0) );
            EXPECT_DOUBLE_EQ( 0.0, json_object.get_number_float(key,1.0) );
        } else if ( key == "x" ) {
            EXPECT_DOUBLE_EQ( 0.0, json_object.get_number_float(key,0.0) );
            EXPECT_DOUBLE_EQ( 1.0, json_object.get_number_float(key,1.0) );
        } else {
            EXPECT_THROW( json_object.get_number_float(key,0.0), Json::TypeError );
            EXPECT_THROW( json_object.get_number_float(key,1.0), Json::TypeError );
        }
        if ( key == "s" ) {
            EXPECT_EQ( string(""), json_object.get_string(key,"") );
            EXPECT_EQ( string(""), json_object.get_string(key,"-") );
        } else if ( key == "x" ) {
            EXPECT_EQ( string(""), json_object.get_string(key,"") );
            EXPECT_EQ( string("-"), json_object.get_string(key,"-") );
        } else {
            EXPECT_THROW( json_object.get_string(key,""), Json::TypeError );
            EXPECT_THROW( json_object.get_string(key,"-"), Json::TypeError );
        }
        if ( key == "a" ) {
            EXPECT_EQ( 0, json_object.get_array(key,vector<Json>{}).size() );
            EXPECT_EQ( 0, json_object.get_array(key,{Json{}}).size() );
        } else if ( key == "x" ) {
            EXPECT_EQ( 0, json_object.get_array(key,vector<Json>{}).size() );
            EXPECT_EQ( 1, json_object.get_array(key,{Json{}}).size() );
        } else {
            EXPECT_THROW( UNUSED_RESULT(json_object.get_array(key,vector<Json>{}).size()),
                    Json::TypeError );
            EXPECT_THROW( UNUSED_RESULT(json_object.get_array(key,{Json{}}).size()),
                    Json::TypeError );
        }
        if ( key == "o" ) {
            EXPECT_EQ(0, json_object.get_object(key,Json::ObjectBody{}).size());
            EXPECT_EQ(0, json_object.get_object(key,{{"",Json{}}}).size());
        } else if ( key == "x" ) {
            EXPECT_EQ(0, json_object.get_object(key,Json::ObjectBody{}).size());
            EXPECT_EQ(1, json_object.get_object(key,{{"",Json{}}}).size());
        } else {
            EXPECT_THROW( UNUSED_RESULT(json_object.get_object(key,Json::ObjectBody{}).size()),
                    Json::TypeError );
            EXPECT_THROW( UNUSED_RESULT(json_object.get_object(key,{{"",Json{}}}).size()),
                    Json::TypeError );
        }
    }
}

TEST( Jsrl,String_PtrConstruct ) {
    Json const json( "" );
    auto ptr = json.as_string_ptr();
    EXPECT_EQ( string(""), *ptr );
    auto new_json = Json( ptr );
    EXPECT_EQ( json, new_json );
}
TEST( Jsrl,String_ConstructPtrConstruct ) {
    auto manual_ptr = Json::StringPtr(string{""});
    Json const json( manual_ptr );
    auto ptr = json.as_string_ptr();
    EXPECT_EQ( static_cast<shared_ptr<string const> >(manual_ptr),
                static_cast<shared_ptr<string const> >(ptr) );
    EXPECT_EQ( string(""), *ptr );
    auto new_json = Json( ptr );
    EXPECT_EQ( json, new_json );
}
TEST( Jsrl,Array_PtrConstruct ) {
    Json const json = R"JSON([null,false,0,0.0,"",[],{}])JSON"_Json;
    auto ptr = json.as_array_ptr();
    EXPECT_EQ( Json{}, (*ptr)[0] );
    EXPECT_EQ( 0, (*ptr)[2].as_number_sint() );
    auto new_json = Json( ptr );
    EXPECT_EQ( json, new_json );
}
TEST( Jsrl,Array_ConstructPtrConstruct ) {
    auto manual_ptr = Json::ArrayPtr( Json::ArrayBody{
            Json{}, Json(false), Json(0), Json(0.0), Json(""),
            Json::array({}), Json::object({}),
            } );
    Json const json( manual_ptr );
    auto ptr = json.as_array_ptr();
    EXPECT_EQ( static_cast<shared_ptr<Json::ArrayBody const> >(manual_ptr),
                static_cast<shared_ptr<Json::ArrayBody const> >(ptr) );
    EXPECT_EQ( Json{}, (*ptr)[0] );
    EXPECT_EQ( 0, (*ptr)[2].as_number_sint() );
    auto new_json = Json( ptr );
    EXPECT_EQ( json, new_json );
}
TEST( Jsrl,Object_PtrConstruct ) {
    Json const json
            = R"JSON({
                "n":null,
                "b":false,
                "i":0,
                "r":0.0,
                "s":"",
                "a":[],
                "o":{} })JSON"_Json;
    auto ptr = json.as_object_ptr();
    EXPECT_EQ( 0, find(*ptr,"i")->second.as_number_sint() );
    auto new_json = Json( ptr );
    EXPECT_EQ( json, new_json );
}

TEST( Jsrl,Object_ConstructPtrConstruct ) {
    auto manual_ptr = Json::ObjectPtr( Json::ObjectBody{
                { "n", Json{} },
                { "b", Json(false) },
                { "i", Json(0), },
                { "r", Json(0.0), },
                { "s", Json(""), },
                { "a", Json::array({}) },
                { "o", Json::object({}) },
            } );
    Json const json( manual_ptr );
    auto ptr = json.as_object_ptr();
    EXPECT_EQ( static_cast<shared_ptr<Json::ObjectBody const> >(manual_ptr),
                static_cast<shared_ptr<Json::ObjectBody const> >(ptr) );
    EXPECT_EQ( 0, find(*ptr,"i")->second.as_number_sint() );
    auto new_json = Json( ptr );
    EXPECT_EQ( json, new_json );
}

TEST( Jsrl,Object_MapConstruct ) {
    Json const json
            = R"JSON({
                "n":null,
                "b":false,
                "i":0,
                "r":0.0,
                "s":"",
                "a":[],
                "o":{} })JSON"_Json;
    auto o = json.as_map_object();
    EXPECT_EQ( 0, o.find("i")->second.as_number_sint() );
    auto new_json = Json( o );
    EXPECT_EQ( json, new_json );
}
TEST( Jsrl,Array_SetElem ) {
    auto json = R"JSON([1,2,3])JSON"_Json;
    EXPECT_EQ( 1, json[0].as_number_uint() );
    EXPECT_EQ( 2, json[1].as_number_uint() );
    EXPECT_EQ( 3, json[2].as_number_uint() );
    EXPECT_THROW( json[3].as_number_uint(), Json::KeyError );
    EXPECT_THROW( json[4].as_number_uint(), Json::KeyError );
    EXPECT_THROW( json[5].as_number_uint(), Json::KeyError );
    json.set(   1, Json(4),
                0, Json(5),
                4, Json(6),
                0, Json(7),
                4, Json(8) );
    EXPECT_EQ( 7, json[0].as_number_uint() );
    EXPECT_EQ( 4, json[1].as_number_uint() );
    EXPECT_EQ( 3, json[2].as_number_uint() );
    EXPECT_THROW( json[3].as_number_uint(), Json::TypeError );
    EXPECT_EQ( 8, json[4].as_number_uint() );
    EXPECT_THROW( json[5].as_number_uint(), Json::KeyError );
}

TEST( Jsrl,Object_SetElem ) {
    auto json = R"JSON({"a":1,"b":2,"c":3})JSON"_Json;
    EXPECT_EQ( 1, json["a"].as_number_uint() );
    EXPECT_EQ( 2, json["b"].as_number_uint() );
    EXPECT_EQ( 3, json["c"].as_number_uint() );
    EXPECT_THROW( json["d"].as_number_uint(), Json::KeyError );
    EXPECT_THROW( json["e"].as_number_uint(), Json::KeyError );
    EXPECT_THROW( json["f"].as_number_uint(), Json::KeyError );
    json.set(   "b", Json(4),
                "a", Json(5),
                "e", Json(6),
                "a", Json(7),
                "e", Json(8) );
    EXPECT_EQ( 7, json["a"].as_number_uint() );
    EXPECT_EQ( 4, json["b"].as_number_uint() );
    EXPECT_EQ( 3, json["c"].as_number_uint() );
    EXPECT_THROW( json["d"].as_number_uint(), Json::KeyError );
    EXPECT_EQ( 8, json["e"].as_number_uint() );
    EXPECT_THROW( json["f"].as_number_uint(), Json::KeyError );
}

TEST( Jsrl,Comments ) {
    istringstream iss(
R"JSON(
    { // Object starting comment
        "1key" /**/://
            [ // Array starting comment /**/ . . .
                "value1" , // A comment
                2/* another comment */ ,
                3.25// {"":[]}
            ]/**Object interior comment*/,
        "2nd" /* "cmt" */ : /*comment**/ null
    }
)JSON");
    Json json;
    iss >> json;
    ostringstream oss;
    oss << json;
    EXPECT_EQ( R"JSON({"1key":["value1",2,3.25],"2nd":null})JSON", oss.str() );
}

TEST( Jsrl,EOFParse ) {
    string contents;
    for ( auto &&precursor: { " /**/ ", "  ", "" } ) {
        for ( auto &&data: {
                    R"JSON(null)JSON",
                    R"JSON(false)JSON",
                    R"JSON(12)JSON",
                    R"JSON(-1)JSON",
                    R"JSON(123.45)JSON",
                    R"JSON("some string")JSON",
                    R"JSON([ [ ], { } ])JSON",
                    R"JSON({ "" : "" })JSON",
                    "",
                } ) {
            if ( not *data and *precursor )
                continue;
            for ( auto &&trailer : { "  ", "" } ) {
                [&]{
                    contents.clear();
                    contents += precursor;
                    contents += data;
                    contents += trailer;
                    SCOPED_TRACE("Contents: R\"JSON("+contents+")JSON\"");
                    istringstream iss(contents);
                    iss.exceptions( ios::failbit | ios::badbit );
                    ASSERT_TRUE( iss );
                    ASSERT_FALSE( iss.eof() );
                    Json json;
                    if ( *data ) {
                        ASSERT_NO_THROW( iss >> json );
                        EXPECT_EQ( json, Json::parse(data) );
                        ASSERT_TRUE( iss );
                        EXPECT_FALSE( iss.eof() );
                    }
                    ASSERT_NO_THROW( iss >> std::ws );
                    ASSERT_TRUE( iss );
                    EXPECT_TRUE( iss.eof() );
                    EXPECT_THROW( iss >> json, Json::StartEOFParseError );
                    ASSERT_FALSE( iss );
                    EXPECT_TRUE( iss.eof() );
                }();
            }
        }
    }
}

TEST(Jsrl,TypeErrors_null) {
    Json const json_null;
    EXPECT_EQ( Json::TT_NULL, json_null.get_typetag(false) );
    EXPECT_PRED1( is_null, json_null );
    EXPECT_PRED1( isnt_bool, json_null );
    EXPECT_THROW( json_null.as_bool(), Json::TypeError );
    EXPECT_PRED1( isnt_number, json_null );
    EXPECT_PRED1( isnt_number_general, json_null );
    EXPECT_THROW( json_null.as_number_general(), Json::TypeError );
    EXPECT_PRED1( isnt_number_float, json_null );
    EXPECT_THROW( json_null.as_number_float(), Json::TypeError );
    EXPECT_PRED1( isnt_number_integer, json_null );
    EXPECT_PRED1( isnt_number_sint, json_null );
    EXPECT_THROW( json_null.as_number_sint(), Json::TypeError );
    EXPECT_PRED1( isnt_number_uint, json_null );
    EXPECT_THROW( json_null.as_number_uint(), Json::TypeError );
    EXPECT_PRED1( isnt_string, json_null );
    EXPECT_THROW( json_null.as_string(), Json::TypeError );
    EXPECT_PRED1( isnt_array, json_null );
    EXPECT_THROW( json_null.as_array(), Json::TypeError );
    EXPECT_THROW( json_null.size(), Json::TypeError );
    EXPECT_THROW( json_null[0], Json::TypeError );
    EXPECT_PRED1( isnt_object, json_null );
    EXPECT_THROW( json_null.as_object(), Json::TypeError );
    EXPECT_THROW( json_null[""], Json::TypeError );
}
TEST(Jsrl,TypeErrors_bool) {
    Json const json_bool(false);
    EXPECT_EQ( Json::TT_BOOL, json_bool.get_typetag(false) );
    EXPECT_PRED1( isnt_null, json_bool );
    EXPECT_PRED1( is_bool, json_bool );
    EXPECT_FALSE( json_bool.as_bool() );
    EXPECT_PRED1( isnt_number, json_bool );
    EXPECT_PRED1( isnt_number_general, json_bool );
    EXPECT_THROW( json_bool.as_number_general(), Json::TypeError );
    EXPECT_PRED1( isnt_number_float, json_bool );
    EXPECT_THROW( json_bool.as_number_float(), Json::TypeError );
    EXPECT_PRED1( isnt_number_integer, json_bool );
    EXPECT_PRED1( isnt_number_sint, json_bool );
    EXPECT_THROW( json_bool.as_number_sint(), Json::TypeError );
    EXPECT_PRED1( isnt_number_uint, json_bool );
    EXPECT_THROW( json_bool.as_number_uint(), Json::TypeError );
    EXPECT_PRED1( isnt_string, json_bool );
    EXPECT_THROW( json_bool.as_string(), Json::TypeError );
    EXPECT_PRED1( isnt_array, json_bool );
    EXPECT_THROW( json_bool.as_array(), Json::TypeError );
    EXPECT_THROW( json_bool.size(), Json::TypeError );
    EXPECT_THROW( json_bool[0], Json::TypeError );
    EXPECT_PRED1( isnt_object, json_bool );
    EXPECT_THROW( json_bool.as_object(), Json::TypeError );
    EXPECT_THROW( json_bool.has_key(""), Json::TypeError );
    EXPECT_THROW( json_bool[""], Json::TypeError );
}
TEST(Jsrl,TypeErrors_number) {
    Json const json_number(0.0);
    EXPECT_EQ( Json::TT_NUMBER, json_number.get_typetag(false) );
    EXPECT_EQ( Json::TT_NUMBER, json_number.get_typetag(true) );
    EXPECT_PRED1( isnt_null, json_number );
    EXPECT_PRED1( isnt_bool, json_number );
    EXPECT_THROW( json_number.as_bool(), Json::TypeError );
    EXPECT_PRED1( is_number, json_number );
    EXPECT_PRED1( isnt_number_general, json_number );
    EXPECT_DOUBLE_EQ( 0.0, json_number.as_number_general()->as_long_double() );
    EXPECT_PRED1( is_number_float, json_number );
    EXPECT_DOUBLE_EQ( 0.0, json_number.as_number_float() );
    EXPECT_PRED1( isnt_number_integer, json_number );
    EXPECT_PRED1( isnt_number_sint, json_number );
    EXPECT_THROW( json_number.as_number_sint(), Json::TypeError );
    EXPECT_PRED1( isnt_number_uint, json_number );
    EXPECT_THROW( json_number.as_number_uint(), Json::TypeError );
    EXPECT_PRED1( isnt_string, json_number );
    EXPECT_THROW( json_number.as_string(), Json::TypeError );
    EXPECT_PRED1( isnt_array, json_number );
    EXPECT_THROW( json_number.as_array(), Json::TypeError );
    EXPECT_THROW( json_number.size(), Json::TypeError );
    EXPECT_THROW( json_number[0], Json::TypeError );
    EXPECT_PRED1( isnt_object, json_number );
    EXPECT_THROW( json_number.as_object(), Json::TypeError );
    EXPECT_THROW( json_number.has_key(""), Json::TypeError );
    EXPECT_THROW( json_number[""], Json::TypeError );
}
TEST(Jsrl,TypeErrors_number_general) {
    Json const json_number(GeneralNumber(0.0L));
    EXPECT_EQ( Json::TT_NUMBER, json_number.get_typetag(false) );
    EXPECT_EQ( Json::TT_NUMBER_GENERAL, json_number.get_typetag(true) );
    EXPECT_PRED1( isnt_null, json_number );
    EXPECT_PRED1( isnt_bool, json_number );
    EXPECT_THROW( json_number.as_bool(), Json::TypeError );
    EXPECT_PRED1( is_number, json_number );
    EXPECT_PRED1( is_number_general, json_number );
    EXPECT_DOUBLE_EQ( 0.0, json_number.as_number_general()->as_long_double() );
    EXPECT_PRED1( isnt_number_float, json_number );
    EXPECT_DOUBLE_EQ( 0.0, json_number.as_number_float() );
    EXPECT_PRED1( isnt_number_integer, json_number );
    EXPECT_PRED1( isnt_number_sint, json_number );
    EXPECT_THROW( json_number.as_number_sint(), Json::TypeError );
    EXPECT_PRED1( isnt_number_uint, json_number );
    EXPECT_THROW( json_number.as_number_uint(), Json::TypeError );
    EXPECT_PRED1( isnt_string, json_number );
    EXPECT_THROW( json_number.as_string(), Json::TypeError );
    EXPECT_PRED1( isnt_array, json_number );
    EXPECT_THROW( json_number.as_array(), Json::TypeError );
    EXPECT_THROW( json_number.size(), Json::TypeError );
    EXPECT_THROW( json_number[0], Json::TypeError );
    EXPECT_PRED1( isnt_object, json_number );
    EXPECT_THROW( json_number.as_object(), Json::TypeError );
    EXPECT_THROW( json_number.has_key(""), Json::TypeError );
    EXPECT_THROW( json_number[""], Json::TypeError );
}
TEST(Jsrl,TypeErrors_number_sint) {
    Json const json_number_int(-1);
    EXPECT_EQ( Json::TT_NUMBER, json_number_int.get_typetag(false) );
    EXPECT_EQ( Json::TT_NUMBER_INTEGER, json_number_int.get_typetag(true) );
    EXPECT_PRED1( isnt_null, json_number_int );
    EXPECT_PRED1( isnt_bool, json_number_int );
    EXPECT_THROW( json_number_int.as_bool(), Json::TypeError );
    EXPECT_PRED1( is_number, json_number_int );
    EXPECT_PRED1( isnt_number_general, json_number_int );
    EXPECT_DOUBLE_EQ( -1.0,
            json_number_int.as_number_general()->as_long_double() );
    EXPECT_PRED1( isnt_number_float, json_number_int );
    EXPECT_DOUBLE_EQ( -1.0, json_number_int.as_number_float() );
    EXPECT_PRED1( is_number_integer, json_number_int );
    EXPECT_PRED1( is_number_sint, json_number_int );
    EXPECT_EQ( -1, json_number_int.as_number_sint() );
    EXPECT_PRED1( isnt_number_uint, json_number_int );
    EXPECT_THROW(json_number_int.as_number_uint(), Json::TypeError);
    EXPECT_PRED1( isnt_string, json_number_int );
    EXPECT_THROW( json_number_int.as_string(), Json::TypeError );
    EXPECT_PRED1( isnt_array, json_number_int );
    EXPECT_THROW( json_number_int.as_array(), Json::TypeError );
    EXPECT_THROW( json_number_int.size(), Json::TypeError );
    EXPECT_THROW( json_number_int[0], Json::TypeError );
    EXPECT_PRED1( isnt_object, json_number_int );
    EXPECT_THROW( json_number_int.as_object(), Json::TypeError );
    EXPECT_THROW( json_number_int.has_key(""), Json::TypeError );
    EXPECT_THROW( json_number_int[""], Json::TypeError );
}
TEST(Jsrl,TypeErrors_number_uint) {
    Json const json_number_int_u(0);
    EXPECT_EQ( Json::TT_NUMBER, json_number_int_u.get_typetag(false) );
    EXPECT_EQ( Json::TT_NUMBER_INTEGER_UNSIGNED,
            json_number_int_u.get_typetag(true) );
    EXPECT_PRED1( isnt_null, json_number_int_u );
    EXPECT_PRED1( isnt_bool, json_number_int_u );
    EXPECT_THROW( json_number_int_u.as_bool(), Json::TypeError );
    EXPECT_PRED1( is_number, json_number_int_u );
    EXPECT_PRED1( isnt_number_general, json_number_int_u );
    EXPECT_DOUBLE_EQ( 0.0,
            json_number_int_u.as_number_general()->as_long_double() );
    EXPECT_PRED1( isnt_number_float, json_number_int_u );
    EXPECT_DOUBLE_EQ( 0.0, json_number_int_u.as_number_float() );
    EXPECT_PRED1( is_number_integer, json_number_int_u );
    EXPECT_PRED1( isnt_number_sint, json_number_int_u );
    EXPECT_EQ( 0, json_number_int_u.as_number_sint() );
    EXPECT_PRED1( is_number_uint, json_number_int_u );
    EXPECT_EQ( 0, json_number_int_u.as_number_uint() );
    EXPECT_PRED1( isnt_string, json_number_int_u );
    EXPECT_THROW( json_number_int_u.as_string(), Json::TypeError );
    EXPECT_PRED1( isnt_array, json_number_int_u );
    EXPECT_THROW( json_number_int_u.as_array(), Json::TypeError );
    EXPECT_THROW( json_number_int_u.size(), Json::TypeError );
    EXPECT_THROW( json_number_int_u[0], Json::TypeError );
    EXPECT_PRED1( isnt_object, json_number_int_u );
    EXPECT_THROW( json_number_int_u.as_object(), Json::TypeError );
    EXPECT_THROW( json_number_int_u.has_key(""), Json::TypeError );
    EXPECT_THROW( json_number_int_u[""], Json::TypeError );
}
TEST(Jsrl,TypeErrors_string) {
    Json const json_string("");
    EXPECT_EQ( Json::TT_STRING, json_string.get_typetag(false) );
    EXPECT_PRED1( isnt_null, json_string );
    EXPECT_PRED1( isnt_bool, json_string );
    EXPECT_THROW( json_string.as_bool(), Json::TypeError );
    EXPECT_PRED1( isnt_number, json_string );
    EXPECT_PRED1( isnt_number_general, json_string );
    EXPECT_THROW( json_string.as_number_general(), Json::TypeError );
    EXPECT_PRED1( isnt_number_float, json_string );
    EXPECT_THROW( json_string.as_number_float(), Json::TypeError );
    EXPECT_PRED1( isnt_number_integer, json_string );
    EXPECT_PRED1( isnt_number_sint, json_string );
    EXPECT_THROW( json_string.as_number_sint(), Json::TypeError );
    EXPECT_PRED1( isnt_number_uint, json_string );
    EXPECT_THROW( json_string.as_number_uint(), Json::TypeError );
    EXPECT_PRED1( is_string, json_string );
    EXPECT_EQ( string(""), json_string.as_string() );
    EXPECT_PRED1( isnt_array, json_string );
    EXPECT_THROW( json_string.as_array(), Json::TypeError );
    EXPECT_THROW( json_string.size(), Json::TypeError );
    EXPECT_THROW( json_string[0], Json::TypeError );
    EXPECT_PRED1( isnt_object, json_string );
    EXPECT_THROW( json_string.as_object(), Json::TypeError );
    EXPECT_THROW( json_string.has_key(""), Json::TypeError );
    EXPECT_THROW( json_string[""], Json::TypeError );
}
TEST(Jsrl,TypeErrors_array) {
    Json const json_array = Json::array({Json(false)});
    EXPECT_EQ( Json::TT_ARRAY, json_array.get_typetag(false) );
    EXPECT_PRED1( isnt_null, json_array );
    EXPECT_PRED1( isnt_bool, json_array );
    EXPECT_THROW( json_array.as_bool(), Json::TypeError );
    EXPECT_PRED1( isnt_number, json_array );
    EXPECT_PRED1( isnt_number_general, json_array );
    EXPECT_THROW( json_array.as_number_general(), Json::TypeError );
    EXPECT_PRED1( isnt_number_float, json_array );
    EXPECT_THROW( json_array.as_number_float(), Json::TypeError );
    EXPECT_PRED1( isnt_number_integer, json_array );
    EXPECT_PRED1( isnt_number_sint, json_array );
    EXPECT_THROW( json_array.as_number_sint(), Json::TypeError );
    EXPECT_PRED1( isnt_number_uint, json_array );
    EXPECT_THROW( json_array.as_number_uint(), Json::TypeError );
    EXPECT_PRED1( isnt_string, json_array );
    EXPECT_THROW( json_array.as_string(), Json::TypeError );
    EXPECT_PRED1( is_array, json_array );
    EXPECT_NO_THROW( json_array.as_array() );
    EXPECT_EQ( 1, json_array.size() );
    EXPECT_PRED1( is_bool, json_array[0] );
    EXPECT_PRED1( isnt_null, json_array.get(0) );
    EXPECT_PRED1( is_bool, json_array.get(0) );
    EXPECT_PRED1( is_bool, json_array.get(0,Json()) );
    EXPECT_THROW( json_array[1], Json::ArrayKeyError );
    EXPECT_PRED1( is_null, json_array.get(1) );
    EXPECT_PRED1( isnt_null, json_array.get(1,Json(false)) );
    EXPECT_PRED1( is_bool, json_array.get(1,Json(false)) );
    EXPECT_PRED1( isnt_object, json_array );
    EXPECT_THROW( json_array.as_object(), Json::TypeError );
    EXPECT_THROW( json_array.has_key(""), Json::TypeError );
    EXPECT_THROW( json_array[""], Json::TypeError );
    EXPECT_THROW( json_array.get(""), Json::TypeError );
    Json json_copy = json_array;
    json_copy.set(3,Json(),2,Json(""));
    EXPECT_EQ( 1, json_array.size() );
    EXPECT_EQ( 4, json_copy.size() );
    EXPECT_THROW( json_array[2], Json::ArrayKeyError );
    EXPECT_PRED1( is_null, json_array.get(2) );
    EXPECT_PRED1( is_string, json_copy[2] );
    EXPECT_PRED1( isnt_null, json_copy[2] );
    EXPECT_PRED1( is_null, json_copy[1] );
    EXPECT_THROW( json_copy.set("",Json(),"_",Json()), Json::TypeError );
}
TEST(Jsrl,TypeErrors_object) {
    Json const json_object = Json::object({{"_",Json(false)}});
    EXPECT_EQ( Json::TT_OBJECT, json_object.get_typetag(false) );
    EXPECT_PRED1( isnt_null, json_object );
    EXPECT_PRED1( isnt_bool, json_object );
    EXPECT_THROW( json_object.as_bool(), Json::TypeError );
    EXPECT_PRED1( isnt_number, json_object );
    EXPECT_PRED1( isnt_number_general, json_object );
    EXPECT_THROW( json_object.as_number_general(), Json::TypeError );
    EXPECT_PRED1( isnt_number_float, json_object );
    EXPECT_THROW( json_object.as_number_float(), Json::TypeError );
    EXPECT_PRED1( isnt_number_integer, json_object );
    EXPECT_PRED1( isnt_number_sint, json_object );
    EXPECT_THROW( json_object.as_number_sint(), Json::TypeError );
    EXPECT_PRED1( isnt_number_uint, json_object );
    EXPECT_THROW( json_object.as_number_uint(), Json::TypeError );
    EXPECT_PRED1( isnt_string, json_object );
    EXPECT_THROW( json_object.as_string(), Json::TypeError );
    EXPECT_PRED1( isnt_array, json_object );
    EXPECT_THROW( json_object.as_array(), Json::TypeError );
    EXPECT_THROW( json_object.size(), Json::TypeError );
    EXPECT_THROW( json_object[0], Json::TypeError );
    EXPECT_THROW( json_object.get(0), Json::TypeError );
    EXPECT_PRED1( is_object, json_object );
    EXPECT_NO_THROW( json_object.as_object() );
    EXPECT_PRED2( doesnt_have_key, json_object, "" );
    EXPECT_PRED2( has_key, json_object, "_" );
    EXPECT_THROW( json_object[""], Json::ObjectKeyError );
    EXPECT_PRED1( is_null, json_object.get("") );
    EXPECT_PRED1( isnt_null, json_object.get("",Json(false)) );
    EXPECT_PRED1( is_bool, json_object.get("",Json(false)) );
    EXPECT_PRED1( isnt_null, json_object["_"] );
    EXPECT_PRED1( is_bool, json_object["_"] );
    EXPECT_PRED1( isnt_null, json_object.get("_") );
    EXPECT_PRED1( is_bool, json_object.get("_") );
    EXPECT_PRED1( isnt_null, json_object.get("_",Json()) );
    EXPECT_PRED1( is_bool, json_object.get("_",Json()) );
    Json json_copy = json_object;
    json_copy.set("__",Json(""),"",Json());
    EXPECT_THROW( json_object[""], Json::ObjectKeyError );
    EXPECT_PRED1( is_bool, json_object.get("",Json(false)) );
    EXPECT_PRED1( is_bool, json_object["_"] );
    EXPECT_PRED1( is_null, json_copy[""] );
    EXPECT_PRED1( is_bool, json_copy["_"] );
    EXPECT_THROW( json_copy.set(0,Json(),1,Json()), Json::TypeError );
}

TEST(Jsrl,KeyErrors) {
    Json const json_array(vector<Json>{Json()});
    ASSERT_PRED1( is_array, json_array );
    ASSERT_EQ( 1, json_array.size() );
    EXPECT_NO_THROW( json_array[0] );
    EXPECT_THROW( json_array[1], Json::KeyError );
    EXPECT_THROW( json_array[2], Json::KeyError );
}

TEST(Jsrl,ErrorArgument) {
    Json const json = R"JSON({
                "true" : true,
                "one" : 1,
                "array" : [ null, false, -15 ],
                "object" : { "realkey" : null }
            })JSON"_Json;
    try {
        json["object"].as_string();
    } catch ( Json::TypeError const &e ) {
        ostringstream enc;
        enc << e.get_argument();
        EXPECT_EQ( string( R"JSON({"realkey":null})JSON" ), enc.str() );
    }
}

TEST(Jsrl,EncodeUTF8) {
    static char const *const strcases[][2] = {
        {   "\"Aa\\u0080Zz\"",
            "\"Aa\xC2\x80Zz\"" },
        {   "\"Aa\\u07ffZz\"",
            "\"Aa\xDF\xBFZz\"" },
        {   "\"Aa\\u0800Zz\"",
            "\"Aa\xE0\xA0\x80Zz\"" },
        {   "\"Aa\\uffffZz\"",
            "\"Aa\xEF\xBF\xBFZz\"" },
        {   "\"Aa\\ud800\\udc00Zz\"",
            "\"Aa\xF0\x90\x80\x80Zz\"" },
        {   "\"Aa\\udbff\\udfffZz\"",
            "\"Aa\xF4\x8F\xBF\xBFZz\"" },
    };
    for ( auto const &strcase : strcases ) {
        for ( auto &&source_cs : strcase ) {
            SCOPED_TRACE( string("Source string: \"") + source_cs + "\"" );
            EXPECT_NO_THROW( validate_utf8( source_cs ) );
            auto json = Json::parse( source_cs );
            {
                ostringstream oss;
                EXPECT_NO_THROW( oss << write_ASCII_strings(json) )
                        << "   Input: \"" << source_cs << "\"";
                EXPECT_EQ( string(strcase[0]), oss.str() );
            }
            {
                ostringstream oss;
                EXPECT_NO_THROW( oss << write_utf_strings(json) )
                        << "   Input: \"" << source_cs << "\"";
                EXPECT_EQ( string(strcase[1]), oss.str() );
            }
        }
    }
}
TEST(Jsrl,EncodeErrors) {
#define REPLACE "\xEF\xBF\xBD"
    static char const *const encode_error_inputs[][2] = {
        {   "Aa\xF0\x8F",
            "Aa" REPLACE "" }, // String ends in the middle of a UTF-8 byte
        {   "Aa\xF0\x8F\x8FZz",
            "Aa" REPLACE "Zz"}, // Not enough continuation bytes
        {   "Aa\x8F",
            "Aa" REPLACE "" }, // Unexpected UTF-8 continuation byte
        {   "Aa\xF0\x8F\x8F\x8F\x8FZz",
            "Aa\xF0\x8F\x8F\x8F" REPLACE "Zz" }, // Too many continuation bytes
        {   "Aa\xF8\x8F\x8F\x8F\x8F",
            "Aa" REPLACE "" }, // Bad starting byte
        {   "Aa\xF8\x8F\x8F\x8F\x8FZz",
            "Aa" REPLACE "Zz" }, // Bad starting byte
        {   "Aa\xF8Zz",
            "Aa" REPLACE "Zz" }, // Bad starting byte
        {   "Aa\xED\xA0\x80\xED\xB0\x80Zz",
            "Aa" REPLACE REPLACE "Zz" }, // UTF-8 encoded surrogate
        {   "Aa\xF4\x90\x80\x80Zz",
            "Aa" REPLACE "Zz" }, // UTF-8 Outside Unicode range 0x10FFFF
        {   "Aa\xF7\xBF\xBF\xBFZz",
            "Aa" REPLACE "Zz" }, // UTF-8 Outside Unicode range 0x10FFFF
    };
    for ( auto const &errcase : encode_error_inputs ) {
        auto expected_fixed = encode(Json(errcase[1]));
        SCOPED_TRACE( expected_fixed );
        EXPECT_THROW( validate_utf8( errcase[0] ), Json::EncodeError )
                << "   Input: \"" << errcase[0] << "\"";
        Json json(errcase[0], Json::ignore_bad_unicode);
        ostringstream oss;
        EXPECT_NO_THROW( oss << replace_bad_utf( json ) )
                << "   Input: \"" << errcase[0] << "\"";
        EXPECT_EQ( expected_fixed, oss.str() );
        EXPECT_THROW( oss << fail_bad_utf(json), Json::EncodeError )
                << "   Input: \"" << errcase[0] << "\"";
    }
    {
        char const *const edgecase = "\xF2\x80\x80\x80\0\xF0\x8F";
        char const *const edgecase_end = edgecase + 5;
        char const *const edgecase_buf_end = edgecase_end + 2;
        EXPECT_NO_THROW( validate_utf8( edgecase ) )
                << "   Input: \"" << edgecase << "\"";
        EXPECT_NO_THROW( validate_utf8( edgecase, edgecase_end-1 ) )
                << "   Input: \"" << string( edgecase, edgecase_end-1 ) << "\"";
        EXPECT_NO_THROW( validate_utf8( edgecase, edgecase_end ) )
                << "   Input: \"" << string( edgecase, edgecase_end ) << "\"";
        EXPECT_THROW( validate_utf8( edgecase_end ), Json::EncodeError )
                << "   Input: \"" << edgecase_end << "\"";
        EXPECT_THROW(validate_utf8(edgecase,edgecase_buf_end),Json::EncodeError)
                << "   Input: \"" << string(edgecase, edgecase_buf_end) << "\"";
        EXPECT_THROW(validate_utf8(edgecase,edgecase_end-2),Json::EncodeError)
                << "   Input: \"" << string(edgecase, edgecase_end-2) << "\"";
    }
}
TEST(Jsrl,ParseErrors) {
    static char const *const parse_error_inputs[] = {
        "nul",
        "nulll",
        "falser",
        "truest",
        "[tru]",
        "falsa",
        "00",
        "01.0",
        "-01.0",
        "0.",
        "[0.]",
        "0.e0",
        "0.0e",
        "[0.0e]",
        "0.0e+",
        "[0.0e+]",
        "0.0e-",
        "[0.0e-]",
        ".0",
        "\"\\u012",
        "\"\\u012\"",
        "\"\\u012G\"",
        "\"\\uD800 \"",
        "\"\\uD800\\n \"",
        "\"\\uD800\\uD800\"",
        "\"\\uDC00\"",
        "\" ",
        "\"\\z\"",
        "\"\n\"",
        "[",
        "[\"\"",
        "[\"\",",
        "[\"\"}",
        "[\"\",}",
        "[}",
        "{",
        "{null,false}",
        "{\"\"",
        "{\"\" ",
        "{\"\"}",
        "{\"\"[",
        "{\"\":\"\"",
        "{\"\":\"\" ",
        "{\"\":\"\"[",
        "{\"\":\"\",",
        "{\"\":\"\", ",
        "{\"\":\"\",}",
        "",
        " ",
        "hello",
        "0.+e",
        "0.-e",
        "0.-Ee",
        "0..0",
        "-.0",
    };
    for ( auto const errinput : parse_error_inputs ) {
        istringstream iss(errinput);
        iss.exceptions( ios::failbit );
        Json json;
        EXPECT_THROW( iss >> json, Json::ParseError )
                << "   Input: <" << errinput << ">";
        EXPECT_THROW( Json::parse( errinput ), Json::ParseError )
                << "   Input: <" << errinput << ">";
    }
}

TEST(Jsrl,TrailingCommaParseErrors) {
    for ( auto const errinput : {
            R"JSON([null,])JSON",
            R"JSON([[],])JSON",
            R"JSON([{},])JSON",
            R"JSON([{"a":[]},])JSON",
            R"JSON([{"a":[false,]}])JSON",
            R"JSON([{"a":[null,],}])JSON",
            R"JSON([{"a":[null,1,2,]}])JSON",
            R"JSON({"a":[{},]})JSON",
            R"JSON({"null":null,})JSON",
            R"JSON({"a":[null],})JSON",
            R"JSON({"a":[null],"o":{},})JSON",
            R"JSON([{"a":[],}])JSON",
            R"JSON([{"o":{"null":null,},},])JSON",
            R"JSON({"a":[{"null":null,},]})JSON",
            R"JSON({"a":[{"false":false,}]})JSON",
            } ) {
        istringstream iss( errinput );
        iss.exceptions( ios::failbit );
        Json json;
        EXPECT_THROW( iss >> json, Json::TrailingCommaParseError )
                << "   Input: <" << errinput << ">";
        EXPECT_THROW( Json::parse( errinput ), Json::TrailingCommaParseError )
                << "   Input: <" << errinput << ">";
    }
}

string string_encode( Json const &json ) {
    ostringstream oss;
    oss << json;
    return oss.str();
}

TEST(Jsrl,ParseFunction) {
    string parse_string = R"JSON({"":[null,false,true,0],"_":1.25e1})JSON";
    Json json_ss;
    {
        istringstream iss( parse_string );
        iss >> json_ss;
    }
    Json json_sp = Json::parse( std::move( parse_string ) );
    EXPECT_EQ( string_encode(json_ss), string_encode(json_sp) );
}

TEST(Jsrl,ParseFunctionStringView) {
    std::string_view parse_sv = R"JSON({"key":"value","array":[1,2,3]})JSON";
    Json json_sv = Json::parse( parse_sv );
    EXPECT_TRUE( json_sv.is_object() );
    EXPECT_EQ( string("value"), json_sv["key"].as_string() );
    EXPECT_TRUE( json_sv["array"].is_array() );
    EXPECT_EQ( 3u, json_sv["array"].size() );
}

TEST(Jsrl,ParseFunctionStringViewEmpty) {
    std::string_view parse_sv = R"JSON({})JSON";
    Json json_sv = Json::parse( parse_sv );
    EXPECT_TRUE( json_sv.is_object() );
    EXPECT_EQ( 0u, json_sv.as_object().size() );
}

TEST(Jsrl,ParseFunctionStringViewComplex) {
    std::string_view parse_sv = R"JSON({"bool":true,"null":null,"num":42.5,"str":"test"})JSON";
    Json json_sv = Json::parse( parse_sv );
    EXPECT_TRUE( json_sv.is_object() );
    EXPECT_TRUE( json_sv["bool"].as_bool() );
    EXPECT_TRUE( json_sv["null"].is_null() );
    EXPECT_EQ( 42.5L, json_sv["num"].as_number_float() );
    EXPECT_EQ( string("test"), json_sv["str"].as_string() );
}

TEST(Jsrl,ParseFunctionStringViewSubstring) {
    string full_json = R"JSON({"a":1}{"b":2})JSON";
    std::string_view first_part(full_json.data(), 7);  // {"a":1} is 7 chars
    Json json_sv = Json::parse( first_part );
    EXPECT_TRUE( json_sv.is_object() );
    EXPECT_EQ( 1, json_sv["a"].as_number_sint() );
    EXPECT_FALSE( json_sv.has_key("b") );
}

struct ComparatorTester {
    void add( unsigned lineno, bool is_new, string const &json_value ) {
        Json new_value = Json::parse(json_value);
        if ( is_new ) {
            auto inserted = m_values.insert(
                    make_pair(new_value,vector<pair<Json,unsigned> >{
                        make_pair(new_value,lineno) } ) );
            if ( not inserted.second ) {
                FAIL() << "Value at line " << lineno
                        << " is already inserted\nValue "
                        << new_value ;
            }
        } else {
            auto found = m_values.find( new_value );
            if ( found == m_values.end() ) {
                FAIL() << "Value at line " << lineno
                        << " hasn't been inserted\nValue: "
                        << new_value ;
            }
            found->second.emplace_back( new_value, lineno );
        }
        auto last = prev(end(prev(end(m_values))->second));
        EXPECT_EQ( last->first, new_value )
                << ( is_new ? "(New) " : "(Nonnew) " )
                << "Value at line " << lineno
                << " compares less than value at line " << last->second ;
    }
    void operator()() {
        unsigned index1 = 0;
        for ( auto const &entry1 : m_values ) {
            ++index1;
            unsigned index2 = 0;
            for ( auto const &entry2 : m_values ) {
                ++index2;
                for ( auto const &element1 : entry1.second ) {
                    for ( auto const &element2 : entry2.second ) {
                        if ( index1<index2 ) {
                            EXPECT_LT( element1.first, element2.first )
                                    << "El[" << element1.second
                                    << "]<El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_TRUE( element1.first < element2.first )
                                    << "El[" << element1.second
                                    << "]<El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_TRUE( element1.first <= element2.first )
                                    << "El[" << element1.second
                                    << "]<=El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_TRUE( element1.first != element2.first )
                                    << "El[" << element1.second
                                    << "]!=El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_FALSE( element1.first >= element2.first )
                                    << "El[" << element1.second
                                    << "]>=El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_FALSE( element1.first > element2.first )
                                    << "El[" << element1.second
                                    << "]>El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_FALSE( element1.first == element2.first )
                                    << "El[" << element1.second
                                    << "]==El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                        } else if ( index1 > index2 ) {
                            EXPECT_GT( element1.first, element2.first )
                                    << "El[" << element1.second
                                    << "]>El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_TRUE( element1.first > element2.first )
                                    << "El[" << element1.second
                                    << "]>El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_TRUE( element1.first >= element2.first )
                                    << "El[" << element1.second
                                    << "]>=El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_TRUE( element1.first != element2.first )
                                    << "El[" << element1.second
                                    << "]!=El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_FALSE( element1.first <= element2.first )
                                    << "El[" << element1.second
                                    << "]<=El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_FALSE( element1.first < element2.first )
                                    << "El[" << element1.second
                                    << "]<El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_FALSE( element1.first == element2.first )
                                    << "El[" << element1.second
                                    << "]==El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                        } else {
                            EXPECT_EQ( element1.first, element2.first )
                                    << "El[" << element1.second
                                    << "]==El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_TRUE( element1.first == element2.first )
                                    << "El[" << element1.second
                                    << "]==El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_TRUE( element1.first <= element2.first )
                                    << "El[" << element1.second
                                    << "]<=El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_TRUE( element1.first >= element2.first )
                                    << "El[" << element1.second
                                    << "]>=El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            EXPECT_FALSE( element1.first != element2.first )
                                    << "El[" << element1.second
                                    << "]!=El[" << element2.second
                                    << "]\nValue 1: " << element1.first
                                    << "\nValue 2: " << element2.first;
                            auto [element1_value, element1_index] = element1;
                            auto [element2_value, element2_index] = element2;
                            EXPECT_FALSE( element1_value > element2_value )
                                    << "El[" << element1_index
                                    << "]>El[" << element2_index
                                    << "]\nValue 1: " << element1_value
                                    << "\nValue 2: " << element2_value;
                            EXPECT_FALSE( element1_value < element2_value )
                                    << "El[" << element1_index
                                    << "]<El[" << element2_index
                                    << "]\nValue 1: " << element1_value
                                    << "\nValue 2: " << element2_value;
                        }
                    }
                }
            }
        }
    }
private:

    using ValueMap = map<Json,vector<pair<Json,unsigned> > >;
    ValueMap m_values;
};

TEST(Jsrl,Comparator) {
    ComparatorTester ct;
    ct.add(__LINE__,true,"null");
    ct.add(__LINE__,true,"false");
    ct.add(__LINE__,true,"true");
    ct.add(__LINE__,true,"-5");
    ct.add(__LINE__,true,"-5.0");
    ct.add(__LINE__,true,"1");
    ct.add(__LINE__,true,"1.0");
    ct.add(__LINE__,true,"2.0");
    ct.add(__LINE__,true,"5");
    ct.add(__LINE__,true,"10");
    ct.add(__LINE__,true,"10.0");
    ct.add(__LINE__,true,"\"\"");
    ct.add(__LINE__,false,"\"\"");
    ct.add(__LINE__,true,"\"\\n\"");
    ct.add(__LINE__,false,"\"\\u000a\"");
    ct.add(__LINE__,true,"\" \"");
    ct.add(__LINE__,false,"\"\\u0020\"");
    ct.add(__LINE__,true,"[]");
    ct.add(__LINE__,false,"[]");
    ct.add(__LINE__,true,"[null]");
    ct.add(__LINE__,false,"[null]");
    ct.add(__LINE__,true,"[false,true]");
    ct.add(__LINE__,true,"[true,false]");
    ct.add(__LINE__,true,"{}");
    ct.add(__LINE__,false,"{}");
    ct.add(__LINE__,true,"{\"foo\":null,\"bar\":null}");
    ct.add(__LINE__,false,"{\"bar\":null,\"foo\":null}");
    ct.add(__LINE__,true,"{\"foo\":\"bar\"}");
    ct.add(__LINE__,false,"{\"foo\":\"bar\"}");
    ct.add(__LINE__,true,"{\"foo\":[null]}");
    ct.add(__LINE__,false,"{\"foo\":[null]}");
    ct();
}
// vi: et ts=4 sts=4 sw=4
