/**
  *  \file u/t_interpreter_vmio_valueloader.cpp
  *  \brief Test for interpreter::vmio::ValueLoader
  */

#include "interpreter/vmio/valueloader.hpp"

#include <memory>
#include <cstdio>               // sprintf
#include "t_interpreter_vmio.hpp"
#include "afl/base/countof.hpp"
#include "afl/charset/utf8charset.hpp"
#include "afl/data/booleanvalue.hpp"
#include "afl/data/floatvalue.hpp"
#include "afl/data/integervalue.hpp"
#include "afl/data/scalarvalue.hpp"
#include "afl/data/stringvalue.hpp"
#include "afl/io/constmemorystream.hpp"
#include "afl/io/internalsink.hpp"
#include "interpreter/savevisitor.hpp"
#include "interpreter/values.hpp"
#include "interpreter/vmio/loadcontext.hpp"
#include "interpreter/vmio/nullloadcontext.hpp"
#include "interpreter/vmio/nullsavecontext.hpp"

namespace {
    struct RealTestCase {
        uint8_t bytes[6];
        const char* value;
    };

    /** Test cases for real conversion.

        These test cases have been generated by a Turbo Pascal program. The first half
        is a set of "simple" real numbers, the second half is a set of randomly-generated
        bytes interpreted as reals. The string representation has been generated by
        Turbo Pascal. We compare that against the string representations generated by
        sprintf. Many of the random numbers fail the check by differing in the last
        few bits, e.g. instead of "167721453090000.0000000000" I'm getting
        "167721453092352.0000000000". Turbo Pascal is limiting the conversion to 11
        decimal digits, so the sprintf results are actually more precise. For this
        particular case, 0xB0,0xFE,0xA1,0xB1,0x8A,0x18 means 0x1.188AB1A1FE * 2**0x2F,
        which is precisely 167721453092352.

        An exact value for {0x11, 0x22, 0x33, 0x44, 0x55, 0x66} can be obtained as
           echo "(2^39+$((0x6655443322))/(2^39))*2^$((0x11-129-39))" | bc -l
        (note that the last byte's MSB is the sign bit!) */
    static const RealTestCase reals[] = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, "0.0000000000" },
        { 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, "1.0000000000" },
        { 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, "2.0000000000" },
        { 0x82, 0x00, 0x00, 0x00, 0x00, 0x40, "3.0000000000" },
        { 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, "4.0000000000" },
        { 0x83, 0x00, 0x00, 0x00, 0x00, 0x20, "5.0000000000" },
        { 0x83, 0x00, 0x00, 0x00, 0x00, 0x40, "6.0000000000" },
        { 0x83, 0x00, 0x00, 0x00, 0x00, 0x60, "7.0000000000" },
        { 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, "8.0000000000" },
        { 0x84, 0x00, 0x00, 0x00, 0x00, 0x10, "9.0000000000" },
        { 0x84, 0x00, 0x00, 0x00, 0x00, 0x20, "10.0000000000" },
        { 0x87, 0x00, 0x00, 0x00, 0x00, 0x48, "100.0000000000" },
        { 0x8A, 0x00, 0x00, 0x00, 0x00, 0x7A, "1000.0000000000" },
        { 0x8E, 0x00, 0x00, 0x00, 0x40, 0x1C, "10000.0000000000" },
        { 0x91, 0x00, 0x00, 0x00, 0x50, 0x43, "100000.0000000000" },
        { 0x94, 0x00, 0x00, 0x00, 0x24, 0x74, "1000000.0000000000" },
        { 0x98, 0x00, 0x00, 0x80, 0x96, 0x18, "10000000.0000000000" },
        { 0x9B, 0x00, 0x00, 0x20, 0xBC, 0x3E, "100000000.0000000000" },
        { 0x9E, 0x00, 0x00, 0x28, 0x6B, 0x6E, "1000000000.0000000000" },
        { 0xA2, 0x00, 0x00, 0xF9, 0x02, 0x15, "10000000000.0000000000" },
        { 0xA5, 0x00, 0x40, 0xB7, 0x43, 0x3A, "100000000000.0000000000" },
        { 0x9B, 0x00, 0xA0, 0xA2, 0x79, 0x6B, "123456789.0000000000" },
        { 0x80, 0x66, 0x66, 0x66, 0x66, 0x66, "0.9000000000" },
        { 0x80, 0xCD, 0xCC, 0xCC, 0xCC, 0x4C, "0.8000000000" },
        { 0x80, 0x33, 0x33, 0x33, 0x33, 0x33, "0.7000000000" },
        { 0x80, 0x9A, 0x99, 0x99, 0x99, 0x19, "0.6000000000" },
        { 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, "0.5000000000" },
        { 0x7F, 0xCD, 0xCC, 0xCC, 0xCC, 0x4C, "0.4000000000" },
        { 0x7F, 0x9A, 0x99, 0x99, 0x99, 0x19, "0.3000000000" },
        { 0x7E, 0xCD, 0xCC, 0xCC, 0xCC, 0x4C, "0.2000000000" },
        { 0x7D, 0xCD, 0xCC, 0xCC, 0xCC, 0x4C, "0.1000000000" },
        { 0x7A, 0x71, 0x3D, 0x0A, 0xD7, 0x23, "0.0100000000" },
        { 0x77, 0x8D, 0x97, 0x6E, 0x12, 0x03, "0.0010000000" },
        { 0x73, 0xE2, 0x58, 0x17, 0xB7, 0x51, "0.0001000000" },
        { 0x70, 0x1B, 0x47, 0xAC, 0xC5, 0x27, "0.0000100000" },
        { 0x6D, 0xAF, 0x05, 0xBD, 0x37, 0x06, "0.0000010000" },
        { 0x69, 0xE5, 0xD5, 0x94, 0xBF, 0x56, "0.0000001000" },
        { 0x66, 0x84, 0x11, 0x77, 0xCC, 0x2B, "0.0000000100" },
        { 0x63, 0x37, 0x41, 0x5F, 0x70, 0x09, "0.0000000010" },
        { 0x5F, 0xBE, 0xCE, 0xFE, 0xE6, 0x5B, "0.0000000001" },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, "0.0000000000" },
        { 0x81, 0x00, 0x00, 0x00, 0x00, 0x80, "-1.0000000000" },
        { 0x82, 0x00, 0x00, 0x00, 0x00, 0x80, "-2.0000000000" },
        { 0x82, 0x00, 0x00, 0x00, 0x00, 0xC0, "-3.0000000000" },
        { 0x83, 0x00, 0x00, 0x00, 0x00, 0x80, "-4.0000000000" },
        { 0x83, 0x00, 0x00, 0x00, 0x00, 0xA0, "-5.0000000000" },
        { 0x83, 0x00, 0x00, 0x00, 0x00, 0xC0, "-6.0000000000" },
        { 0x83, 0x00, 0x00, 0x00, 0x00, 0xE0, "-7.0000000000" },
        { 0x84, 0x00, 0x00, 0x00, 0x00, 0x80, "-8.0000000000" },
        { 0x84, 0x00, 0x00, 0x00, 0x00, 0x90, "-9.0000000000" },
        { 0x84, 0x00, 0x00, 0x00, 0x00, 0xA0, "-10.0000000000" },
        { 0x87, 0x00, 0x00, 0x00, 0x00, 0xC8, "-100.0000000000" },
        { 0x8A, 0x00, 0x00, 0x00, 0x00, 0xFA, "-1000.0000000000" },
        { 0x8E, 0x00, 0x00, 0x00, 0x40, 0x9C, "-10000.0000000000" },
        { 0x91, 0x00, 0x00, 0x00, 0x50, 0xC3, "-100000.0000000000" },
        { 0x94, 0x00, 0x00, 0x00, 0x24, 0xF4, "-1000000.0000000000" },
        { 0x98, 0x00, 0x00, 0x80, 0x96, 0x98, "-10000000.0000000000" },
        { 0x9B, 0x00, 0x00, 0x20, 0xBC, 0xBE, "-100000000.0000000000" },
        { 0x9E, 0x00, 0x00, 0x28, 0x6B, 0xEE, "-1000000000.0000000000" },
        { 0xA2, 0x00, 0x00, 0xF9, 0x02, 0x95, "-10000000000.0000000000" },
        { 0xA5, 0x00, 0x40, 0xB7, 0x43, 0xBA, "-100000000000.0000000000" },
        { 0x9B, 0x00, 0xA0, 0xA2, 0x79, 0xEB, "-123456789.0000000000" },
        { 0x80, 0x66, 0x66, 0x66, 0x66, 0xE6, "-0.9000000000" },
        { 0x80, 0xCD, 0xCC, 0xCC, 0xCC, 0xCC, "-0.8000000000" },
        { 0x80, 0x33, 0x33, 0x33, 0x33, 0xB3, "-0.7000000000" },
        { 0x80, 0x9A, 0x99, 0x99, 0x99, 0x99, "-0.6000000000" },
        { 0x80, 0x00, 0x00, 0x00, 0x00, 0x80, "-0.5000000000" },
        { 0x7F, 0xCD, 0xCC, 0xCC, 0xCC, 0xCC, "-0.4000000000" },
        { 0x7F, 0x9A, 0x99, 0x99, 0x99, 0x99, "-0.3000000000" },
        { 0x7E, 0xCD, 0xCC, 0xCC, 0xCC, 0xCC, "-0.2000000000" },
        { 0x7D, 0xCD, 0xCC, 0xCC, 0xCC, 0xCC, "-0.1000000000" },
        { 0x7A, 0x71, 0x3D, 0x0A, 0xD7, 0xA3, "-0.0100000000" },
        { 0x77, 0x8D, 0x97, 0x6E, 0x12, 0x83, "-0.0010000000" },
        { 0x73, 0xE2, 0x58, 0x17, 0xB7, 0xD1, "-0.0001000000" },
        { 0x70, 0x1B, 0x47, 0xAC, 0xC5, 0xA7, "-0.0000100000" },
        { 0x6D, 0xAF, 0x05, 0xBD, 0x37, 0x86, "-0.0000010000" },
        { 0x69, 0xE5, 0xD5, 0x94, 0xBF, 0xD6, "-0.0000001000" },
        { 0x66, 0x84, 0x11, 0x77, 0xCC, 0xAB, "-0.0000000100" },
        { 0x63, 0x37, 0x41, 0x5F, 0x70, 0x89, "-0.0000000010" },
        { 0x5F, 0xBE, 0xCE, 0xFE, 0xE6, 0xDB, "-0.0000000001" },

        // This one would have been normalized out by the writer:
        // { 0x00, 0x08, 0xDC, 0x33, 0x45, 0xAB, "-0.0000000000" },
        { 0x51, 0x29, 0x5F, 0x6C, 0x14, 0x79, "0.0000000000" },
        { 0x12, 0xD7, 0x0F, 0x4B, 0xEA, 0x5E, "0.0000000000" },
        { 0xC6, 0x53, 0xB2, 0xD8, 0xB7, 0x4E, "953319203389407494144.0000000000" }, // Turbo Pascal: 953319203390000000000.0000000000, exact: 953319203389407494144
        { 0x29, 0x54, 0x77, 0x3F, 0xD3, 0x47, "0.0000000000" },
        { 0x7B, 0x26, 0xDF, 0x49, 0xC5, 0xF9, "-0.0304895823" },
        { 0x7E, 0xE3, 0xD3, 0x05, 0x24, 0x24, "0.1602936659" },
        { 0x80, 0x05, 0x97, 0x02, 0xC6, 0xA6, "-0.6514588946" },
        { 0x15, 0xC5, 0x7E, 0xE1, 0x82, 0x92, "-0.0000000000" },
        { 0x47, 0xE0, 0x29, 0x45, 0x8B, 0xF5, "-0.0000000000" },
        { 0x2C, 0x28, 0x44, 0x2D, 0x93, 0x7F, "0.0000000000" },
        { 0x49, 0x27, 0x9C, 0xBD, 0xBF, 0x14, "0.0000000000" },
        { 0x13, 0x0F, 0x1B, 0xCB, 0xCE, 0x61, "0.0000000000" },
        { 0x9C, 0x6F, 0xBD, 0x7E, 0x57, 0xD1, "-219510763.8395996094" }, // Turbo Pascal: -219510763.8400000000, exact 219510763.839599609375
        { 0x50, 0x50, 0xA8, 0xB1, 0xE7, 0x51, "0.0000000000" },
        { 0x7A, 0x4B, 0xC4, 0xBD, 0x8D, 0xB9, "-0.0113252977" },
        { 0x69, 0x8A, 0x98, 0xE1, 0xAB, 0x1C, "0.0000000730" },
        { 0x78, 0x67, 0x1D, 0x0D, 0x8C, 0xAC, "-0.0026328594" },
        { 0x14, 0xEB, 0xD6, 0xAE, 0xED, 0x46, "0.0000000000" },
        { 0x1A, 0xDD, 0x8D, 0xB2, 0x19, 0xAB, "-0.0000000000" },
        { 0x04, 0x4F, 0xE7, 0x89, 0x53, 0x71, "0.0000000000" },
        { 0x67, 0x35, 0xCE, 0x96, 0x1C, 0x9F, "-0.0000000185" },
        { 0x65, 0x5D, 0x15, 0x74, 0x81, 0x75, "0.0000000071" },
        { 0x5C, 0x5D, 0x9F, 0xC7, 0x74, 0x34, "0.0000000000" },
        { 0x44, 0x4B, 0xB3, 0x74, 0xF6, 0x30, "0.0000000000" },
        { 0x03, 0x39, 0xE6, 0x52, 0xDC, 0xC3, "-0.0000000000" },
        { 0x0D, 0x81, 0x6C, 0x00, 0x26, 0xB5, "-0.0000000000" },
        { 0x4F, 0x65, 0x8C, 0x42, 0x26, 0x4B, "0.0000000000" },
        { 0x13, 0xB4, 0x54, 0xCA, 0xA7, 0x90, "-0.0000000000" },
        { 0xE5, 0x05, 0x41, 0xE2, 0xAF, 0x08, "1353682937867496664104723021824.0000000000" }, // Turbo Pascal: 1353682937900000000000000000000.0000000000, exact: 1353682937867496664104723021824
        { 0x28, 0x27, 0x69, 0x6B, 0x24, 0x00, "0.0000000000" },
    };
}

/** Test loading and saving of real values. */
void
TestInterpreterVmioValueLoader::testReal()
{
    // ex IntValueTestSuite::testReal
    afl::charset::Utf8Charset cs;
    interpreter::vmio::NullLoadContext loadContext;
    interpreter::vmio::NullSaveContext saveContext;

    /*
     *  Deserialize all values. Compare stringified form (that's where the test cases originated). Serialize again.
     */
    for (size_t i = 0; i < sizeof(reals)/sizeof(reals[0]); ++i) {
        // Message for assertions
        const char*const msg = reals[i].value;

        // Build TagNode
        interpreter::TagNode tagIn;
        tagIn.tag   = static_cast<uint16_t>(reals[i].bytes[0] + 256*reals[i].bytes[1]);
        tagIn.value = static_cast<uint32_t>(reals[i].bytes[2] + 256*reals[i].bytes[3] + 65536*reals[i].bytes[4] + 16777216*reals[i].bytes[5]);

        // Load value
        afl::io::ConstMemoryStream auxIn((afl::base::Bytes_t()));
        std::auto_ptr<afl::data::Value> value(interpreter::vmio::ValueLoader(cs, loadContext).loadValue(tagIn, auxIn));
        TSM_ASSERT(msg, value.get() != 0);

        afl::data::FloatValue* fv = dynamic_cast<afl::data::FloatValue*>(value.get());
        TSM_ASSERT(msg, fv != 0);

        // Convert value to string
        char tmpstr[100];
        std::sprintf(tmpstr, "%.10f", fv->getValue());
        TSM_ASSERT_EQUALS(msg, String_t(tmpstr), reals[i].value);

        // Serialize again and compare
        afl::io::InternalSink auxOut;
        interpreter::TagNode tagOut;
        interpreter::SaveVisitor(tagOut, auxOut, cs, saveContext).visit(value.get());

        TSM_ASSERT_EQUALS(msg, auxOut.getContent().size(), 0U);
        TSM_ASSERT_EQUALS(msg, tagOut.tag,   tagIn.tag);
        TSM_ASSERT_EQUALS(msg, tagOut.value, tagIn.value);
    }

    /*
     *  Test some specific values
     */
    afl::io::InternalSink auxOut;
    interpreter::TagNode tagOut;
    {
        // too small, serializes to 0
        afl::data::FloatValue fv(1.0e-100);
        TS_ASSERT_DIFFERS(fv.getValue(), 0.0);
        interpreter::SaveVisitor(tagOut, auxOut, cs, saveContext).visit(&fv);
        TS_ASSERT_EQUALS(tagOut.tag, 0U);
        TS_ASSERT_EQUALS(tagOut.value, 0U);
        // TS_ASSERT_EQUALS(IntFloatValue(itn, ms).getValue(), 0.0);
    }
    {
        // too large, serializes to maximum
        afl::data::FloatValue fv(1.0e+100);
        TS_ASSERT_DIFFERS(fv.getValue(), 0.0);
        interpreter::SaveVisitor(tagOut, auxOut, cs, saveContext).visit(&fv);
        TS_ASSERT_EQUALS(tagOut.tag, 0xFFFFU);
        TS_ASSERT_EQUALS(tagOut.value, 0x7FFFFFFFU);
        // test approximate range:
        // TS_ASSERT_LESS_THAN(IntFloatValue(itn, ms).getValue(), 1.0e+39);
        // TS_ASSERT_LESS_THAN(1.0e+38, IntFloatValue(itn, ms).getValue());
        // // it turns out we can compare with the actual value as well (works with gcc/x86):
        // TS_ASSERT_EQUALS(IntFloatValue(itn, ms).getValue(), 170141183460314489226776631181521715200.0);
    }
    {
        // value with known serialisation
        afl::data::FloatValue fv(7.0);
        TS_ASSERT_DIFFERS(fv.getValue(), 0.0);
        interpreter::SaveVisitor(tagOut, auxOut, cs, saveContext).visit(&fv);
        TS_ASSERT_EQUALS(tagOut.tag, 0x0083U);
        TS_ASSERT_EQUALS(tagOut.value, 0x60000000U);
        // TS_ASSERT_EQUALS(IntFloatValue(itn, ms).getValue(), 7.0);
    }
}

/** Test loading and saving of integers. */
void
TestInterpreterVmioValueLoader::testInteger()
{
    // ex IntValueTestSuite::testInt
    // Some tag nodes
    static const interpreter::TagNode tags[] = {
        { interpreter::TagNode::Tag_Integer, 4711 },
        { interpreter::TagNode::Tag_Integer, -9999999U },
        { interpreter::TagNode::Tag_Integer, 0 },
        { interpreter::TagNode::Tag_Boolean, 0 },
        { interpreter::TagNode::Tag_Boolean, 1 }
    };
    const size_t N = countof(tags);

    // Load them
    afl::charset::Utf8Charset cs;
    interpreter::vmio::NullLoadContext loadContext;
    afl::io::ConstMemoryStream auxIn((afl::base::Bytes_t()));

    afl::data::Value* ivs[N];
    afl::data::ScalarValue* iivs[N];
    for (size_t i = 0; i < N; ++i) {
        ivs[i] = interpreter::vmio::ValueLoader(cs, loadContext).loadValue(tags[i], auxIn);
        TS_ASSERT(ivs[i] != 0);
        iivs[i] = dynamic_cast<afl::data::ScalarValue*>(ivs[i]);
        TS_ASSERT(iivs[i] != 0);
    }

    // Verify them
    TS_ASSERT_EQUALS(iivs[0]->getValue(), 4711);
    TS_ASSERT_EQUALS(iivs[1]->getValue(), -9999999);
    TS_ASSERT_EQUALS(iivs[2]->getValue(), 0);
    TS_ASSERT_EQUALS(iivs[3]->getValue(), 0);
    TS_ASSERT_EQUALS(iivs[4]->getValue(), 1);

    TS_ASSERT(dynamic_cast<afl::data::IntegerValue*>(iivs[0]) != 0);
    TS_ASSERT(dynamic_cast<afl::data::IntegerValue*>(iivs[1]) != 0);
    TS_ASSERT(dynamic_cast<afl::data::IntegerValue*>(iivs[2]) != 0);
    TS_ASSERT(dynamic_cast<afl::data::BooleanValue*>(iivs[3]) != 0);
    TS_ASSERT(dynamic_cast<afl::data::BooleanValue*>(iivs[4]) != 0);

    // Serialize them again
    interpreter::TagNode tagOut[N];
    afl::io::InternalSink auxOut;
    interpreter::vmio::NullSaveContext saveContext;
    for (size_t i = 0; i < N; ++i) {
        interpreter::SaveVisitor(tagOut[i], auxOut, cs, saveContext).visit(ivs[i]);
    }

    // Verify serialisation is the same as the original source
    TS_ASSERT_EQUALS(auxOut.getContent().size(), 0U);
    for (size_t i = 0; i < N; ++i) {
        TS_ASSERT_EQUALS(tagOut[i].tag,   tags[i].tag);
        TS_ASSERT_EQUALS(tagOut[i].value, tags[i].value);
    }

    // // Verify printed form
    // TS_ASSERT_EQUALS(ivs[0]->toString(false), "4711");
    // TS_ASSERT_EQUALS(ivs[1]->toString(false), "-9999999");
    // TS_ASSERT_EQUALS(ivs[2]->toString(false), "0");
    // TS_ASSERT_EQUALS(ivs[3]->toString(false), "NO");
    // TS_ASSERT_EQUALS(ivs[4]->toString(false), "YES");

    // TS_ASSERT_EQUALS(ivs[0]->toString(true), "4711");
    // TS_ASSERT_EQUALS(ivs[1]->toString(true), "-9999999");
    // TS_ASSERT_EQUALS(ivs[2]->toString(true), "0");
    // TS_ASSERT_EQUALS(ivs[3]->toString(true), "False");
    // TS_ASSERT_EQUALS(ivs[4]->toString(true), "True");

    for (size_t i = 0; i < N; ++i) {
        delete ivs[i];
    }
}

/** Test load(Segment). */
void
TestInterpreterVmioValueLoader::testLoadSegment()
{
    // ex IntDataTestSuite::testLoad
    static const uint8_t data[] = {
        0, 0, 0, 0, 0, 0,        // real 0.0
        0, 2, 5, 4, 0, 0,        // int 1029
        0, 1, 0, 0, 0, 0,        // null
        0, 4, 0, 0, 0, 0,        // blank string
        0, 3, 1, 0, 0, 0,        // bool true
        0, 6, 7, 0, 0, 0,        // long string, 7 chars
        0, 4, 0, 0, 1, 0,        // string, not empty,
        0x83, 0, 0, 0, 0, 0x20,  // real 5.0
        0, 5, 0, 0, 0x40, 0x40,  // float 3.0
        // = 9 entries, 54 bytes

        'a', 'b', 'c', 'd', 'e', 'f', 'g',
        3, 'X', 'Y', 'Z',
        // +11 bytes, 65 bytes total

        '1', '2', '3'
    };
    afl::io::ConstMemoryStream mem(data);
    afl::data::Segment seg;

    // Load it into a segment
    afl::charset::Utf8Charset cs;
    interpreter::vmio::NullLoadContext loadContext;
    interpreter::vmio::ValueLoader(cs, loadContext).load(seg, mem, 0, 9);

    afl::data::StringValue* sv;
    afl::data::FloatValue* fv;
    afl::data::IntegerValue* iv;
    afl::data::BooleanValue* bv;

    // Make sure 65 bytes consumed
    TS_ASSERT_EQUALS(mem.getPos(), 65U);

    // First entry
    TS_ASSERT(seg[0] != 0);
    fv = dynamic_cast<afl::data::FloatValue*>(seg[0]);
    TS_ASSERT(fv != 0);
    TS_ASSERT_EQUALS(fv->getValue(), 0.0);

    // Second entry
    TS_ASSERT(seg[1] != 0);
    iv = dynamic_cast<afl::data::IntegerValue*>(seg[1]);
    TS_ASSERT(iv != 0);
    TS_ASSERT_EQUALS(iv->getValue(), 1029);

    // Third entry
    TS_ASSERT(seg[2] == 0);

    // Fourth entry
    TS_ASSERT(seg[3] != 0);
    sv = dynamic_cast<afl::data::StringValue*>(seg[3]);
    TS_ASSERT(sv != 0);
    TS_ASSERT_EQUALS(sv->getValue(), "");

    // Fifth entry
    TS_ASSERT(seg[4] != 0);
    bv = dynamic_cast<afl::data::BooleanValue*>(seg[4]);
    TS_ASSERT(bv != 0);
    TS_ASSERT(bv->getValue());

    // Sixth entry
    TS_ASSERT(seg[5] != 0);
    sv = dynamic_cast<afl::data::StringValue*>(seg[5]);
    TS_ASSERT(sv != 0);
    TS_ASSERT_EQUALS(sv->getValue(), "abcdefg");

    // Seventh entry
    TS_ASSERT(seg[6] != 0);
    sv = dynamic_cast<afl::data::StringValue*>(seg[6]);
    TS_ASSERT(sv != 0);
    TS_ASSERT_EQUALS(sv->getValue(), "XYZ");

    // Eighth entry
    TS_ASSERT(seg[7] != 0);
    fv = dynamic_cast<afl::data::FloatValue*>(seg[7]);
    TS_ASSERT(fv != 0);
    TS_ASSERT_EQUALS(fv->getValue(), 5.0);

    // Ninth entry
    TS_ASSERT(seg[8] != 0);
    fv = dynamic_cast<afl::data::FloatValue*>(seg[8]);
    TS_ASSERT(fv != 0);
    TS_ASSERT_EQUALS(fv->getValue(), 3.0);

    // Tenth and following entries (not deserialized)
    TS_ASSERT(seg[9] == 0);
    TS_ASSERT(seg[10] == 0);
    TS_ASSERT(seg[11] == 0);
}

/** Test load(Segment) with nonzero offset. */
void
TestInterpreterVmioValueLoader::testLoadSegment2()
{
    static const uint8_t data[] = {
        0, 2, 5, 4, 0, 0,        // int 1029
        0, 1, 0, 0, 0, 0,        // null
        // = 2 entries, 12 bytes
    };
    afl::io::ConstMemoryStream mem(data);

    // Set up segment as [null,null,42,23]
    afl::data::Segment seg;
    seg.setNew(2, new afl::data::IntegerValue(42));
    seg.setNew(3, new afl::data::IntegerValue(23));
    TS_ASSERT(seg[2] != 0);
    TS_ASSERT(seg[3] != 0);

    // Load it into a segment as [null,1029,null,23]
    afl::charset::Utf8Charset cs;
    interpreter::vmio::NullLoadContext loadContext;
    interpreter::vmio::ValueLoader(cs, loadContext).load(seg, mem, 1, 2);

    afl::data::IntegerValue* iv;

    // Make sure 65 bytes consumed
    TS_ASSERT_EQUALS(mem.getPos(), 12U);

    // First entry
    TS_ASSERT(seg[0] == 0);

    // Second entry
    TS_ASSERT(seg[1] != 0);
    iv = dynamic_cast<afl::data::IntegerValue*>(seg[1]);
    TS_ASSERT(iv != 0);
    TS_ASSERT_EQUALS(iv->getValue(), 1029);

    // Third entry
    TS_ASSERT(seg[2] == 0);

    // Fourth entry
    TS_ASSERT(seg[3] != 0);
    iv = dynamic_cast<afl::data::IntegerValue*>(seg[3]);
    TS_ASSERT(iv != 0);
    TS_ASSERT_EQUALS(iv->getValue(), 23);
}
