#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <array>

#include "float_surgery.hh"

TEST_CASE("Constant cases for delta_swap", "[delta_swap]")
{
    REQUIRE(delta_swap(0x000f, 8, 0x00ff) == 0x0f00);
    REQUIRE(delta_swap(0x000f, 12, 0x00ff) == 0xf000);

    REQUIRE(delta_swap(0x000f, 1, 0x5555) == 0x000f);
    REQUIRE(delta_swap(0x0005, 1, 0x5555) == 0x000a);

    REQUIRE(delta_swap(0x0000000012345678ul, 16, 0xffff0000ffff0000ul) == 0x0000123400005678ul);
    REQUIRE(delta_swap(0x0000123400005678ul,  8, 0x0000ff000000ff00ul) == 0x0012003400560078ul);
    REQUIRE(delta_swap(0x0012003400560078ul,  4, 0x00f000f000f000f0ul) == 0x0102030405060708ul);
    REQUIRE(delta_swap(0x0102030405060708ul,  2, 0x0c0c0c0c0c0c0c0cul) == 0x0102031011121320ul);
    REQUIRE(delta_swap(0x0102031011121320ul,  1, 0x2222222222222222ul) == 0x0104051011141540ul);
}

TEST_CASE("Constant cases for bit_zip", "[bit_zip]")
{
    REQUIRE(bit_zip(0x00000000fffffffful) == 0x5555555555555555ul);
}

TEST_CASE("Constant cases for bit_uzp", "[bit_uzp]")
{
    REQUIRE(bit_uzp(0x5555555555555555ul) == 0x00000000fffffffful);
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
