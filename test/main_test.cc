#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

TEST_CASE("tset1", "[test1]")
{
    REQUIRE(1 == 1);
}