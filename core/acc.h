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
#include <cache.h>

//to do list:
/**
 * 1,implement vault, different clause should be assigned to different valut.
 * 2, collect valut_im_balance
 * 3, implement push to other watcher list
 * 4, implement new_assignment confilict.
 * 
 * 
 * 
 * 
**/

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
    missAcce,
    sendToVault

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
    case EventType::missAccess:
        os << "MissAccess";
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
                                      m_event_queue(EventComp<T>),
                                      vault_waiting_queue(clause_proc_num),
                                      valut_cache(clause_proc_num, cache(16, 16384, cache::lru, 256, 256))
                                          m_cache(16, 16384, cache::lru, 256, 256),
                                      vault_busy(clause_proc_num, false) {}
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
        for (auto &queue : vault_waiting_queue)
        {
            queue.clear();
        }
        for (auto &slot : vault_busy)
        {
            slot = false;
        }
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
    void handle_new_watch_list(std::queue<std::pair<int, std::weak_ptr<T>>> &waiting_queue, unsigned long long end_time);

    bool have_watcher_unit_ready() const { return m_using_watcher_unit < w_num; }
    bool have_clause_unit_ready() const { return m_using_watcher_unit < c_num; }
    void value_push(std::shared_ptr<T> value) { value_queue.push_back(value); }
    int get_max_level() { return (*std::prev(value_queue.cend()))->get_level(); }
    //void push_time_records(std::shared_ptr<Time_record> time) { time_records.push_back(time); }
    unsigned long long get_global_blocked_clause() const { return global_blocked_clause; }
    unsigned long long get_global_blocked_times() const { return global_blocked_times; }
    unsigned long long get_waiting_watcher_list() const { return waiting_watcher_list; }
    unsigned long long get_waiting_watcher_times() const { return waiting_watcher_times; }
    unsigned long long get_idle_clause_unit_total() const { return idle_clause_unit_total; }
    unsigned long long get_idle_clause_unit_times() const { return idle_clause_unit_times; }
    unsigned long long get_idel_watcher_total() const { return idel_watcher_total; }
    unsigned long long get_idel_watcher_times() const { return idel_watcher_times; }
    unsigned long long get_m_access() const { return m_access; }
    unsigned long long get_m_miss() const { return m_miss; }
    unsigned long long get_m_hit() const { return m_hit; }
    unsigned long long get_m_hit_res() const { return m_hit_res; }
    void print() const
    {

        std::cout << "global_blocked_clause: " << get_global_blocked_clause() << std::endl;
        std::cout << "global_blocked_times: " << get_global_blocked_times() << std::endl;
        std::cout << "waiting_watcher_list: " << get_waiting_watcher_list() << std::endl;
        std::cout << "waiting_watcher_times: " << get_waiting_watcher_times() << std::endl;
        std::cout << "idle_clause_unit_total: " << get_idle_clause_unit_total() << std::endl;
        std::cout << "idle_clause_unit_times: " << get_idle_clause_unit_times() << std::endl;
        std::cout << "idel_watcher_total: " << get_idel_watcher_total() << std::endl;
        std::cout << "idel_watcher_times: " << get_idel_watcher_times() << std::endl;
        std::cout << "m_access: " << get_m_access() << std::endl;
        std::cout << "m_hit " << get_m_hit() << std::endl;
        std::cout << "m_miss " << get_m_miss() << std::endl;
        std::cout << "m_hit_res " << get_m_hit_res() << std::endl;
        for (int i = 0; i < num_clauses; i++)
        {
            std::cout << vault_waiting_all[i] << " " << vault_waiting_times[i] << std::endl;
            std::cout << vault_idle_all[i] << " " << vault_idle_times[i] << std::endl;
        }
    }

private:
    int w_size;
    int w_num;
    int c_num;
    int assigned_to_valut(std::unique_ptr<T> v) { return 0; }
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
    std::vector<std::queue<std::tuple<int, std::unique_ptr<T>, unsigned long long>>> vault_waiting_queue;
    std::vector<cache> valut_cache;
    //statistics
    cache m_cache;
    std::vector<bool> vault_busy;
    unsigned long long global_blocked_clause = 0;
    unsigned long long global_blocked_times = 0;
    unsigned long long waiting_watcher_list = 0;
    unsigned long long waiting_watcher_times = 0;
    unsigned long long idle_clause_unit_total = 0;
    unsigned long long idle_clause_unit_times = 0;
    unsigned long long idel_watcher_total = 0;
    unsigned long long idel_watcher_times = 0;
    unsigned long long m_access = 0;
    unsigned long long m_miss = 0;
    unsigned long long m_hit = 0;
    unsigned long long m_hit_res = 0;
    std::vector<unsigned long long> vault_waiting_all;
    std::vector<unsigned long long> vault_waiting_times;
    std::vector<unsigned long long> vault_idle_all;
    std::vector<unsigned long long> vault_idle_times;
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

template <typename T>
void ACC<T>::handle_new_watch_list(std::queue<std::pair<int, std::weak_ptr<T>>> &waiting_queue, unsigned long long end_time)
{
    int start = waiting_queue.front().first;

    int total = waiting_queue.front().second.lock()->get_watcher_size();
    auto addr = waiting_queue.front().second.lock()->get_addr();
    addr += start * 8;
    auto result = m_cache.access(addr);
    auto &&waiting_value = waiting_queue.front().second;

    auto is_hit = result == cache::hit;

    switch (result)
    {
    case cache::hit:
        m_hit++;
        break;
    case cache::miss:
        m_miss++;
        break;
    case cache::hit_res:
        m_hit_res++;
        break;
    default:
        break;
    }
    m_access++;
    if (result == cache::miss) //when miss, push event to fill that cache line
    {
        auto evalue = EventValue<T>(EventType::missAccess, start, w_size, waiting_value, HardwareType::watcherListUnit, 0);
        auto event = Event<T>(evalue, end_time, miss_latency + end_time);
        m_event_queue.push(event);
    }
    if (total - start > w_size)
    {
        auto evalue = EventValue<T>(EventType::ReadWatcherList, start, w_size, waiting_value, HardwareType::watcherListUnit, 0);
        auto event = Event<T>(evalue, end_time, (is_hit ? 6 : miss_latency) + end_time);

        waiting_queue.front().first += w_size;
        if (print_level >= 2)
            std::cout << event << std::endl;
        m_event_queue.push(event);
        m_using_watcher_unit++;
    }
    else
    {
        //only print the last one

        auto evalue = EventValue<T>(EventType::ReadWatcherList, start, total - start, waiting_value, HardwareType::watcherListUnit, 0);
        auto event = Event<T>(evalue, end_time, (is_hit ? 6 : miss_latency) + end_time);
        if (print_level >= 1)
        {
            std::cout << "ACC::INIT:LIT: " << waiting_value.lock()->get_value() << std::endl;
        }
        //waiting_queue.front().first += (total - start);
        if (print_level >= 2)
            std::cout << event << std::endl;
        m_event_queue.push(event);
        waiting_queue.pop(); //finished
        m_using_watcher_unit++;
    }
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

    auto first_value = value_queue.front();
    //auto size = value->get_watcher_size();
    //int process_size = 0;
    std::queue<std::pair<int, std::weak_ptr<T>>> waiting_queue;
    waiting_queue.push(std::make_pair(0, first_value));
    int i = 0;
    while (!waiting_queue.empty() && m_using_watcher_unit < w_num)
    {
        handle_new_watch_list(waiting_queue, i);
        i++;
    }

    //second process the event queue,
    int last_cycle = 0;
    while (!m_event_queue.empty())
    {
        auto end_time = m_event_queue.get_next_time();
        auto event_value = m_event_queue.get_next_event();
        auto value_of_event = event_value.value;
        m_event_queue.pop();
        last_cycle = end_time;
        switch (event_value.type)
        {
        case EventType::missAccess:
        {
            m_cache.fill(value_of_event.lock()->get_addr());
            break;
        }
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
            int iilatency = 0;
            while (lower != upper) //from cpu send to vault.
            {
                //bug here, clause addr is not unique for the whole watcher list;
                //auto clause_addr = value.lock()->get_clause_addr();
                //auto clause_addr = lower->second;
                //auto vault_index = assign_to_vault(clause_addr);
                //vault_waiting_queue[vault_index].push(std::make_tuple(lower->first,value_of_event,clause_addr); // this clause access is generated by this event
                //clause_waiting_queue.push(std::make_pair(lower->first, value)); // this mean later we will need to process "value" 's "first" watch 's clause
                //problem here, push to vault_waiting_queue may not ready;
                //we need different queue here;
                // or another method, here just generate the event, not push to the queue.
                auto evalue = event_value;
                evalue.type = EventType::sendToVault;
                evalue.index = lower->first;
                evalue.size = 1;
                auto event = Event<T>(evalue, end_time + iilatency, end_time + iilatency + cpuToVaultLatency); //todo this is the cpu to vault latency;
                iilatency += 1;
                if (print_level >= 2)
                    std::cout << event << std::endl;
                m_event_queue.push(event);
                //vault_waiting_queue[i].pop(); /*  */

                lower++;
            }

            /* for (int i = 0; i < num_clauses; i++)
            {
                if (!vault_waiting_queue[i].empty() && !vault_busy[i]) //can issue
                {
                    auto clause_waiting_tuple = vault_waiting_queue[i].front();
                    auto evalue = event_value;
                    evalue.type = EventType::ProcessClause;
                    evalue.index = std::get<0>(clause_waiting_tuple);
                    evalue.value = std::get<1>(clause_waiting_tuple);
                    evalue.size = 1;
                    evalue.hType = HardwareType::ClauseUnit;
                    evalue.HardwareId = 0;
                    auto addr = std::get<2>(clause_waiting_tuple);
                    auto result = valut_cache[i].access(addr);
                    auto latency = 0;
                    if (result == cache_hit)
                    {
                        latency = 10;
                    }
                    else
                    {
                        latency = 100;
                    }
                    switch (result)
                    {
                    case cache::hit:
                    {
                    }
                    case cache::miss:
                    {
                    }
                    case cache::hit_res:
                    {
                    }
                    }
                    auto event = Event<T>(evalue, end_time, end_time + latency);

                    if (print_level >= 2)
                        std::cout << event << std::endl;
                    m_event_queue.push(event);
                    vault_waiting_queue[i].pop();
                    vault_busy[i] = true;
                }
                else
                {
                    if (vault_busy[i] && !vault_waiting_queue[i].empty())
                    {
                        vault_waiting_all[i] += vault_waiting_queue[i].size();
                        vault_waiting_times[i]++;
                        //have task but busy.
                    }
                    if (vault_waiting_queue[i].empty() && !vault_busy[i])
                    {
                        vault_idle_all[i]++;
                        //do not busy but no task
                    }
                }
            }
            */
            /*
            if (m_using_clause_unit == c_num && !clause_waiting_queue.empty())
            {
                global_blocked_clause += clause_waiting_queue.size();
                global_blocked_times++;
            }
            */

            //first to find any watcher_list_to process
            //m_using_watcher_unit--;

            if (waiting_queue.empty())
            {
                idel_watcher_times++;
                idel_watcher_total += w_num - m_using_watcher_unit;
            }
            if (!waiting_queue.empty())
            {
                handle_new_watch_list(waiting_queue, end_time);
            }

            break;
        }
        case EventType::sendToVault: //after vault recived the clause request
        {
            auto index = event_value.index;
            assert(event_value.size == 1);
            auto clause_addr = value_of_event.lock()->get_clause_addr(index);
            auto vault_index = assign_to_vault(clause_addr);
            vault_waiting_queue[vault_index].push(std::make_tuple(index, value_of_event, clause_addr));
            if (!vault_busy[vault_index]) //process the clause
            {
                auto clause_waiting_tuple = vault_waiting_queue[i].front();
                auto evalue = std::get<1>(clause_waiting_tuple);
                evalue.type = EventType::ProcessClause;
                auto addr = std::get<2>(clause_waiting_tuple);
                auto result = valut_cache[i].access(addr);
                auto latency = 0;
                //remaind here, now need to detail the latency! 1, the value latency 2, the clause latency,3, the sync and cache coherence latency
                if (result == cache_hit)
                {
                    latency = 10;
                }
                else
                {
                    latency = 100;
                }
                switch (result)
                {
                case cache::hit:
                {
                }
                case cache::miss:
                {
                }
                case cache::hit_res:
                {
                }
                }

                auto event = Event<T>(evalue, end_time, end_time + latency);

                if (print_level >= 2)
                    std::cout << event << std::endl;
                m_event_queue.push(event);
                vault_waiting_queue[i].pop();
                vault_busy[i] = true;
            }

            break;
        }
        case EventType::FinishAndSendClause:
            //in this case,
            break;
        case EventType::ProcessClause: //need to modified. now get the task from the vault_waiting_queue
        {
            m_using_clause_unit--;

            //in this case, we need to generate new event, and let waiting queue to be processed

            //first to check if we generated new assignment or conflict
            if (value.lock()->get_generated_conf() == event_value.index)
            {
                m_using_clause_unit = 0;
                m_using_watcher_unit = 0;
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
                    handle_new_watch_list(waiting_queue, end_time + i);
                    i++;
                }
                if (m_using_watcher_unit >= w_num && !waiting_queue.empty())
                {
                    int total_size = 0;
                    int size = waiting_queue.size();
                    for (int i = 0; i < size; i++)
                    {
                        auto temp = waiting_queue.front();
                        total_size += temp.second.lock()->get_watcher_size() - temp.first;
                        waiting_queue.pop();
                        waiting_queue.push(temp);
                    }
                    waiting_watcher_list += (total_size + w_size - 1) / w_size;
                    waiting_watcher_times++;
                }
            }

            //now, need to check if any one is waiting for using the clause process units
            if (clause_waiting_queue.empty())
            {
                idle_clause_unit_total += c_num - m_using_clause_unit;
                idle_clause_unit_times++;
            }
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