#include <catch2/catch.hpp>
#include "mtl/Vec.h"
#include "core/Solver.h"
#include "core/SolverTypes.h"
#include <fstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <vector>
#include <iostream>
using namespace Minisat;

TEST_CASE("solver test", "[base]")
{
    Solver s1;
    Solver s2;

    SECTION("empty test")
    {
        {
            std::ofstream ofs("empty.bin");
            boost::archive::binary_oarchive oa(ofs);
            oa << s1;
        }
        {
            std::ifstream ifs("empty.bin");
            boost::archive::binary_iarchive ia(ifs);
            ia >> s2;
        }
        REQUIRE((s1.watches == s1.watches) == true);
        REQUIRE((s1 == s2) == true);
    }
    SECTION("real test")
    {
    }
}