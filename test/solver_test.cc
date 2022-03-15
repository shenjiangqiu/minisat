#include <catch2/catch_test_macros.hpp>
#include "mtl/Vec.h"
#include "core/Solver.h"
#include "core/SolverTypes.h"
#include <fstream>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <vector>
#include <iostream>
#include "core/Dimacs.h"
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
        gzFile in = gzopen("/home/sjq/cnfs/cnfs/ASG_96_len112_known_last12_2.cnf", "rb");
        //std::ofstream ifs("/home/sjq/cnfs/cnfs/ASG_96_len112_known_last12_2.cnf");
        Solver s;
        parse_DIMACS(in, s);

        {
            std::ofstream ofs("real.bin");
            boost::archive::binary_oarchive oa(ofs);
            oa << s;
        }
        Solver s_empty;
        {
            std::ifstream ifs("real.bin");
            boost::archive::binary_iarchive ia(ifs);
            ia >> s_empty;
        }
        REQUIRE((s == s_empty) == true);
    }
}