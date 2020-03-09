#define CATCH_CONFIG_MAIN
#include "catch.hpp" 
#include "assign_wrap.h"  
#include "acc.h"
#include <iostream>
TEST_CASE("main test"){
        auto w1=assign_wrap_factory::create(1,0,100);
        w1->add_modified_list(10,1);
        w1->add_modified_list(11,2);
        w1->add_modified_list(12,3);
        w1->add_modified_list(13,4);
        w1->add_modified_list(14,5);

        const auto &list=w1->get_modified();
        int i=10;
        int j=1;
        for(const auto &elem:list){
            REQUIRE(elem==std::make_pair(i++,j++));
        }
        SECTION("acc"){
            auto acc=create_acc<std::remove_reference<decltype(*w1)>::type>();
            acc->push_to_trail(w1);
            auto w2=assign_wrap_factory::create(2,0,100,w1,w1->get_level()+1);
            REQUIRE(w2->get_level()==1);
            auto w3=assign_wrap_factory::create(2,0,100,w2,w2->get_level()+1);
            REQUIRE(w3->get_level()==2);
            acc->push_to_trail(w2);
            acc->push_to_trail(w3);
            REQUIRE(acc->get_max_level()==2);


        }

}