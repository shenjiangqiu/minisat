#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "assign_wrap.h"
//#include "acc.h"
#include "event_queue.h"
#include "event.h"
#include <iostream>
/*
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
*/
TEST_CASE("EventQueue")
{

    SECTION("fake test")
    {
        struct Task
        {
            int get_key() const { return id; }
            int get_value() const { return value; }
            int id;

            int value;
        };
        auto comp = [](const Task &t1, const Task &t2) { return t1.id < t2.id; };
        auto m_queue = EventQueue<Task, decltype(comp)>(comp);
        SECTION("first test")
        {
            m_queue.push({1, 2});
            m_queue.push({2, 3});
            REQUIRE(m_queue.get_next_event() == 3);
            REQUIRE(m_queue.get_next_time() == 2);
        }
        SECTION("second test")
        {
            m_queue.push({4, 1});
            m_queue.push({3, 2});
            REQUIRE(m_queue.get_next_event() == 1);
            REQUIRE(m_queue.get_next_time() == 4);
        }
        SECTION("Third test")
        {
            m_queue.push({4, 1});
            m_queue.push({4, 2});
            REQUIRE(m_queue.get_next_event() == 1);
            REQUIRE(m_queue.get_next_time() == 4);
        }
    }
    SECTION("True test")
    {
        auto w1 = assign_wrap_factory::create(1, 0, 100,{-1,nullptr});
        using T=decltype(w1)::element_type;
        w1->add_modified_list(10, 1);
        w1->add_modified_list(11, 2);
        w1->add_modified_list(12, 3);
        w1->add_modified_list(13, 4);
        w1->add_modified_list(14, 5);
        EventQueue<Event<T>,decltype(EventComp<T>)> m_queue(EventComp<T>);
        auto evalue=EventValue<T>(EventType::FinishAndSendClause,0,10,w1,HardwareType::ClauseUnit,1);
        auto event=Event<T>(evalue,0,10);
        m_queue.push(event);
        REQUIRE(m_queue.get_next_event().size==10);
        auto evalue2=EventValue<T>(EventType::FinishAndSendClause,11,10,w1,HardwareType::ClauseUnit,1);
        auto event2=Event<T>(evalue2,0,9);
        m_queue.push(event2);
        REQUIRE(m_queue.get_next_event().index==11);

    }
}