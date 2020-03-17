#ifndef ACC_H
#define ACC_H
#include <math.h>
#include <algorithm>
#include <signal.h>
#include <unistd.h>

#include <memory>

#include <memory>
#include <iostream>
#include <queue>

template <typename EventType, typename EventComp>
class EventQueue
{
public:
    EventQueue(const EventComp &comp) : m_event_queue(comp) {}
    void push(const EventType &event) { m_event_queue.push(event); }
    const EventType &top() const { return m_event_queue.top(); }
    void pop() { m_event_queue.pop(); }
    bool empty() const { return m_event_queue.empty(); }
    auto get_next_time() const
    {
        return m_event_queue.top().get_key();
    }
    auto get_next_event() const
    {
        return m_event_queue.top().get_value();
    }
    void clear()
    {
        while (!m_event_queue.empty())
            m_event_queue.pop();
    }

private:
    std::priority_queue<EventType, std::deque<EventType>, EventComp> m_event_queue;
};
enum class EventType
{
    ReadWatcherList,
    ProcessWatcherList,
    FinishAndSendClause,
    ProcessClause,

};
std::ostream &operator<<(std::ostream &os, EventType type)
{
    switch (type)
    {
    case EventType::ReadWatcherList:
        os << "ReadWatcherList";
        break;
    case EventType::ProcessWatcherList:
        os << "ProcessWatcherList";
        break;
    case EventType::FinishAndSendClause:
        os << "FinishAndSendClause";
        break;
    case EventType::ProcessClause:
        os << "ProcessClause";
        break;
    }
    os << std::endl;
    return os;
}
enum class HardwareType
{
    watcherListUnit,
    ClauseUnit
};
template <typename T>
struct EventValue
{
    EventValue(EventType _tp, int _id, int _sz, std::weak_ptr<T> v, HardwareType ht, int hi) : type(_tp), index(_id), size(_sz), value(v), hType(ht), HardwareId(hi) {}
    EventType type;
    int index;
    int size;
    std::weak_ptr<T> value; // the assignment wraper
    HardwareType hType;
    int HardwareId;
};

template <typename T>
class Event
{
public:
    Event(const EventValue<T> &value, int startTime, int endTime) : start_time(startTime), end_time(endTime), mEventValue(value) {}
    int get_key() const { return end_time; }
    const EventValue<T> &get_value() const { return mEventValue; }
    int get_start_time() const { return start_time; }
    int get_end_time() const { return end_time; }

private:
    int start_time;
    int end_time;
    EventValue<T> mEventValue;
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const EventValue<T> &eventValue)
{
    os << "EventValue:type: " << eventValue.type << std::endl;
    os << "EventValue:value: " << eventValue.value.lock()->get_value() << std::endl;
    os << "EventValue:index: " << eventValue.index << std::endl;
    os << "EventValue:size: " << eventValue.size << std::endl;
    return os;
}
template <typename T>
std::ostream &operator<<(std::ostream &os, const Event<T> &event)
{
    os << "Event start time: " << event.get_start_time() << std::endl;
    os << "Event end time: " << event.get_end_time() << std::endl;
    os << event.get_value() << std::endl;
    return os;
}
template <typename T>
auto EventComp = [](const Event<T> &e1, const Event<T> &e2) {
    return e1.get_key() > e2.get_key();
};

template <typename T>
class ACC
{
public:
    ACC() = delete;
    ACC(int watcher_proc_size,
        int watcher_proc_num,
        int clause_proc_num,
        int miss_latency,
        int watcher_process_latency,
        int clause_process_latency) : w_size(watcher_proc_size),
                                      w_num(watcher_proc_num),
                                      c_num(clause_proc_num),
                                      m_ready(false),
                                      miss_latency(miss_latency),
                                      watcher_process_latency(watcher_process_latency),
                                      clause_process_latency(clause_process_latency),
                                      m_using_watcher_unit(0),
                                      m_using_clause_unit(0),
                                      m_event_queue(EventComp<T>) {}
    void print_on(int level)
    {
        print_level = level;
    }
    void print_off()
    {
        print_level = 0;
    }
    bool ready() const
    {
        return m_ready;
    }
    void set_ready()
    {
        m_ready = true;
    }
    void clear()
    {
        m_ready = false;
        value_queue.clear();
        //time_records.clear();
        m_event_queue.clear();
    }

    int start_sim();
    void push_to_trail(std::shared_ptr<T> value)
    {
        value_queue.push_back(value);
    }
    int get_trail_size() const
    {
        return value_queue.size();
    }

    bool have_watcher_unit_ready() const { return m_using_watcher_unit < w_num; }
    bool have_clause_unit_ready() const { return m_using_watcher_unit < c_num; }
    void value_push(std::shared_ptr<T> value) { value_queue.push_back(value); }
    int get_max_level() { return (*std::prev(value_queue.cend()))->get_level(); }
    //void push_time_records(std::shared_ptr<Time_record> time) { time_records.push_back(time); }

private:
    int w_size;
    int w_num;
    int c_num;

    bool m_ready;
    std::vector<std::shared_ptr<T>> value_queue;
    //std::vector<std::shared_ptr<Time_record>> time_records;
    int miss_latency;
    int watcher_process_latency;
    int clause_process_latency;
    int m_using_watcher_unit;
    int m_using_clause_unit;
    EventQueue<Event<T>, decltype(EventComp<T>)> m_event_queue;
    int print_level = 0;
};

template <typename T>
std::shared_ptr<ACC<T>> create_acc(int watcher_proc_size,
                                   int watcher_proc_num,
                                   int clause_proc_num,
                                   int miss_latency,
                                   int watcher_process_latency,
                                   int clause_process_latency)
{
    return std::make_shared<ACC<T>>(watcher_proc_size, watcher_proc_num, clause_proc_num, miss_latency, watcher_process_latency, clause_process_latency);
}

template <class T>
int ACC<T>::start_sim()
{
    if (!(m_using_watcher_unit == 0 && m_using_clause_unit == 0))
    {
        throw std::runtime_error("when start to sim, there should be no running task");
    }
    if (value_queue.empty())
        return 0;
    //initialize the event queue;
    std::queue<std::pair<int, std::weak_ptr<T>>> clause_waiting_queue;

    auto value = value_queue.front();
    //auto size = value->get_watcher_size();
    //int process_size = 0;
    std::queue<std::pair<int, std::weak_ptr<T>>> waiting_queue;
    waiting_queue.push(std::make_pair(0, value));
    int i = 0;
    while (!waiting_queue.empty() && m_using_watcher_unit < w_num)
    {
        int start = waiting_queue.front().first;
        int total = waiting_queue.front().second.lock()->get_watcher_size();
        if (total - start > w_size)
        {
            auto evalue = EventValue<T>(EventType::ReadWatcherList, start, w_size, value, HardwareType::watcherListUnit, 0);
            auto event = Event<T>(evalue, i, miss_latency + i);

            i++;
            waiting_queue.front().first += w_size;
            if (print_level >= 2)
                std::cout << event << std::endl;
            m_event_queue.push(event);
            m_using_watcher_unit++;
        }
        else
        {
            //only print the last one

            auto evalue = EventValue<T>(EventType::ReadWatcherList, start, total - start, value, HardwareType::watcherListUnit, 0);
            auto event = Event<T>(evalue, i, miss_latency + i);
            if (print_level >= 1)
            {
                std::cout << "ACC::INIT:LIT: " << value->get_value() << std::endl;
            }
            i++;
            //waiting_queue.front().first += (total - start);
            if (print_level >= 2)
                std::cout << event << std::endl;
            m_event_queue.push(event);
            waiting_queue.pop(); //finished
            m_using_watcher_unit++;
        }
    }

    //second process the event queue,
    int last_cycle = 0;
    while (!m_event_queue.empty())
    {
        auto end_time = m_event_queue.get_next_time();
        auto event_value = m_event_queue.get_next_event();
        auto value = event_value.value;
        m_event_queue.pop();
        last_cycle = end_time;
        switch (event_value.type)
        {
        case EventType::ReadWatcherList:
        {
            auto evalue = event_value;
            evalue.type = EventType::ProcessWatcherList;
            auto event = Event<T>(evalue, end_time, end_time + watcher_process_latency);
            //waiting_queue.front().first += (total - start);
            if (print_level >= 2)
                std::cout << event << std::endl;
            m_event_queue.push(event);
            break;
        }
        case EventType::ProcessWatcherList:
        {
            //finished process watcher_list, start to generate new clause access event
            //fist try to send clause reading request
            auto modified_list = value.lock()->get_modified();
            // only sorted map support this operation
            auto lower = modified_list.lower_bound(event_value.index);
            auto upper = modified_list.upper_bound(event_value.index + event_value.size);

            while (lower != upper)
            {                                                                   // this clause access is generated by this event
                clause_waiting_queue.push(std::make_pair(lower->first, value)); // this mean later we will need to process "value" 's "first" watch 's clause
                lower++;
            }
            i = 0;
            while (!clause_waiting_queue.empty() && m_using_clause_unit < c_num)
            {
                auto clause_waiting = clause_waiting_queue.front();

                auto evalue = event_value;
                evalue.type = EventType::ProcessClause;
                evalue.index = clause_waiting.first;
                evalue.value = clause_waiting.second;
                evalue.size = 1;
                evalue.hType = HardwareType::ClauseUnit;
                evalue.HardwareId = 0;
                auto event = Event<T>(evalue, i + end_time, i + end_time + clause_process_latency);
                i++;
                if (print_level >= 2)
                    std::cout << event << std::endl;
                m_event_queue.push(event);
                clause_waiting_queue.pop();
                m_using_clause_unit++;
            }

            //first to find any watcher_list_to process
            m_using_watcher_unit--;
            if (!waiting_queue.empty())
            {
                int start = waiting_queue.front().first;
                int total = waiting_queue.front().second.lock()->get_watcher_size();
                auto &&waiting_value = waiting_queue.front().second;
                if (total - start > w_size)
                {
                    auto evalue = EventValue<T>(EventType::ReadWatcherList, start, w_size, waiting_value, HardwareType::watcherListUnit, 0); //fix bug, here not event value, but the value of waiting queue
                    auto event = Event<T>(evalue, end_time, miss_latency + end_time);
                    waiting_queue.front().first += w_size;
                    if (print_level >= 2)
                        std::cout << event << std::endl;
                    m_event_queue.push(event);
                    m_using_watcher_unit++;
                }
                else
                {

                    auto evalue = EventValue<T>(EventType::ReadWatcherList, start, total - start, waiting_value, HardwareType::watcherListUnit, 0); //fix bug, here not event value, but the value of waiting queue
                    auto event = Event<T>(evalue, end_time, miss_latency + end_time);
                    //waiting_queue.front().first += (total - start);
                    if (print_level >= 2)
                        std::cout << event << std::endl;
                    if (print_level >= 1)
                        std::cout << "ACC::PROCESS:LIT: " << waiting_value.lock()->get_value() << std::endl;
                    m_event_queue.push(event);
                    waiting_queue.pop(); //finished
                    m_using_watcher_unit++;
                }
            }

            break;
        }
        case EventType::FinishAndSendClause:
            //in this case,
            break;
        case EventType::ProcessClause:
        {
            m_using_clause_unit--;

            //in this case, we need to generate new event, and let waiting queue to be processed

            //first to check if we generated new assignment or conflict
            if (value.lock()->get_generated_conf() == event_value.index)
            {
                m_using_clause_unit=0;
                m_using_watcher_unit=0;
                return end_time; //we already finished!
            }
            //check if generate new assignment
            auto iter = value.lock()->get_generated_assignments().find(event_value.index);
            if (iter != value.lock()->get_generated_assignments().end())
            {

                auto generated_assignments = (*iter).second;
                waiting_queue.push(std::make_pair(0, generated_assignments));
                int i = 0;
                while (!waiting_queue.empty() && m_using_watcher_unit < w_num)
                {
                    int start = waiting_queue.front().first;
                    int total = waiting_queue.front().second.lock()->get_watcher_size();
                    auto waiting_value = waiting_queue.front().second;
                    if (total - start > w_size)
                    {
                        auto evalue = EventValue<T>(EventType::ReadWatcherList, start, w_size, waiting_value, HardwareType::watcherListUnit, 0);
                        auto event = Event<T>(evalue, end_time + i, end_time + miss_latency + i);

                        i++;
                        waiting_queue.front().first += w_size;
                        if (print_level >= 2)
                            std::cout << event << std::endl;

                        m_event_queue.push(event);
                    }
                    else
                    {
                        auto evalue = EventValue<T>(EventType::ReadWatcherList, start, total - start, waiting_value, HardwareType::watcherListUnit, 0);
                        auto event = Event<T>(evalue, end_time + i, end_time + miss_latency + i);
                        i++;
                        //waiting_queue.front().first += (total - start);
                        if (print_level >= 2)
                            std::cout << event << std::endl;
                        if (print_level >= 1)
                            std::cout << "ACC::PROCESS:LIT: " << waiting_value.lock()->get_value() << std::endl;
                        m_event_queue.push(event);
                        waiting_queue.pop(); //finished
                    }
                    m_using_watcher_unit++;
                }
            }

            //now, need to check if any one is waiting for using the clause process units
            while (!clause_waiting_queue.empty() && m_using_clause_unit < c_num)
            {
                auto clause_waiting = clause_waiting_queue.front();

                auto evalue = event_value;
                evalue.type = EventType::ProcessClause;
                evalue.index = clause_waiting.first;
                evalue.value = clause_waiting.second;
                evalue.size = 1;
                evalue.hType = HardwareType::ClauseUnit;
                evalue.HardwareId = 0;
                auto event = Event<T>(evalue, i + end_time, i + end_time + clause_process_latency);
                i++;
                if (print_level >= 2)
                    std::cout << event << std::endl;
                m_event_queue.push(event);
                clause_waiting_queue.pop();
                m_using_clause_unit++;
            }
            break;
        }
        } // switch
    }
    return last_cycle;
}
#endif