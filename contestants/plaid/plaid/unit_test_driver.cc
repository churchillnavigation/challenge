#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <array>

#include "float_surgery.hh"

TEST_CASE("Constant cases for bit_ilv_lower", "[bit_ilv_lower]")
{
    REQUIRE(bit_ilv_lower(0x0000ffff, 0x00000000) == 0x55555555);
}

TEST_CASE("Constant cases for bit_ilv_upper", "[bit_ilv_upper]")
{
    REQUIRE(bit_ilv_upper(0xffff0000, 0x00000000) == 0x55555555);
}

TEST_CASE("Constant cases for bit_dlv_lower", "[bit_dlv_lower]")
{
    std::array<uint32_t, 2> result = {0x0000ffff, 0x00000000};
    REQUIRE(bit_dlv_lower(0x55555555)[0] == result[0]);
    REQUIRE(bit_dlv_lower(0x55555555)[1] == result[1]);
}

TEST_CASE("Constant cases for bit_dlv_upper", "[bit_dlv_upper]")
{
    std::array<uint32_t, 2> result = {0x00000000, 0xffff0000};
    REQUIRE(bit_dlv_upper(0xaaaaaaaa)[0] == result[0]);
    REQUIRE(bit_dlv_upper(0xaaaaaaaa)[1] == result[1]);
}

TEST_CASE(
    "Bit upper interleaving round-trips.",
    "[bit_ilv_upper, bit_dlv_upper]")
{
    auto intermed = bit_ilv_upper(0xffff0000, 0x00000000);
    std::array<uint32_t, 2> expected = {0xffff0000, 0x00000000};
    REQUIRE(bit_dlv_upper(intermed)[0] == expected[0]);
    REQUIRE(bit_dlv_upper(intermed)[1] == expected[1]);

}

TEST_CASE("relaxed_compare handles valid floats.", "[relaxed_compare]")
{
    ieee754_b32 pos_sml = {.flt = 0.2379f};
    ieee754_b32 pos_big = {.flt = 5.1482f};

    ieee754_b32 neg_sml = {.flt = - 0.9985f};
    ieee754_b32 neg_big = {.flt = -10.2812f};

    REQUIRE_FALSE(relaxed_compare(neg_big.bit, neg_big.bit));
    REQUIRE((relaxed_compare(neg_big.bit, neg_sml.bit)));
    REQUIRE((relaxed_compare(neg_big.bit, pos_sml.bit)));
    REQUIRE((relaxed_compare(neg_big.bit, pos_big.bit)));

    REQUIRE_FALSE((relaxed_compare(neg_sml.bit, neg_big.bit)));
    REQUIRE_FALSE((relaxed_compare(neg_sml.bit, neg_sml.bit)));
    REQUIRE((relaxed_compare(neg_sml.bit, pos_sml.bit)));
    REQUIRE((relaxed_compare(neg_sml.bit, pos_big.bit)));

    REQUIRE_FALSE((relaxed_compare(pos_sml.bit, neg_big.bit)));
    REQUIRE_FALSE((relaxed_compare(pos_sml.bit, neg_sml.bit)));
    REQUIRE_FALSE((relaxed_compare(pos_sml.bit, pos_sml.bit)));
    REQUIRE((relaxed_compare(pos_sml.bit, pos_big.bit)));
    
    REQUIRE_FALSE((relaxed_compare(pos_big.bit, neg_big.bit)));
    REQUIRE_FALSE((relaxed_compare(pos_big.bit, neg_sml.bit)));
    REQUIRE_FALSE((relaxed_compare(pos_big.bit, pos_sml.bit)));
    REQUIRE_FALSE((relaxed_compare(pos_big.bit, pos_big.bit)));
}
