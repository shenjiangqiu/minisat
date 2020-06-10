#include "core/acc.h"
#include <numeric>
#include <memory>
#include <fmt/core.h>
#include <fmt/format.h>
namespace MACC
{
    std::ostream &operator<<(std::ostream &os, const ACC &acc)
    {
        return os << "w_num " << acc.w_num << ",miss latency: " << acc.miss_latency << ",w_size " << acc.w_size << ", w_latency " << acc.watcher_process_latency << ",clause_num " << acc.c_num << ", vault_mem_latency " << acc.vault_memory_access_latency << ", cpu_to_vault_latency " << acc.cpu_to_vault_latency << ", is_mode2 " << acc.mode2 << ",ctl_latency: " << acc.ctr_latency;
    }
    ACC::~ACC()
    {
    }
    ACC::ACC(int watcher_proc_size,
             int watcher_proc_num,
             int clause_proc_num,
             int miss_latency,
             int watcher_process_latency,
             int clause_process_latency,
             int vault_memory_access_latency,
             int cpu_to_vault_latency,
             bool mode2,
             int ctr_latency) : l3_bandwidth_manager(8), dram_bandwith_manager(14), clause_buffer_size(clause_proc_num),
                                w_size(watcher_proc_size),
                                w_num(watcher_proc_num),
                                c_num(clause_proc_num),
                                m_ready(false),
                                miss_latency(miss_latency),
                                watcher_process_latency(watcher_process_latency),
                                clause_process_latency(clause_process_latency),
                                m_using_watcher_unit(0),
                                m_using_clause_unit(0),
                                m_event_queue(),
                                vault_waiting_queue(clause_proc_num),
                                vault_cache(clause_proc_num, cache(1 << 2, 1 << 6, cache::lru, 256, 256)), //16kb
                                m_cache(1 << 4, 1 << 15, cache::lru, 256, 256),                            //l3 cache   32 MB 2^25B 2^19 entry                        //16kb
                                vault_busy(clause_proc_num, false),
                                vault_memory_access_latency(vault_memory_access_latency),
                                cpu_to_vault_latency(cpu_to_vault_latency),
                                mode2(mode2),
                                ctr_latency(ctr_latency),
                                watcher_busy(w_num, false),
                                c_busy(c_num, false),
                                next_c(w_num, 0),
                                clause_read_waiting_queue(c_num)
    {
        //nothing to do
    }
    void ACC::handle_vault_process(int vault_index, int end_time) //keep busy until the clause read back
    {
        throw std::runtime_error("Need modification here, don't come!!!");
        spdlog::debug("handle_vault_process mode1,vault:{},end_time:{}", vault_index, end_time);
        //assert(!vault_busy[vault_index]);
        //vault_busy[vault_index] = true;

        spdlog::debug("in handle vaut: vault: {}", vault_index);
        assert(vault_waiting_queue[vault_index].size() > 0);
        spdlog::debug("vault:{},size:{}", vault_index, vault_waiting_queue[vault_index].size());
        auto value_of_vault_waitng_queue = vault_waiting_queue[vault_index].front();
        auto watcher_index = value_of_vault_waitng_queue.index;

        auto value_of_event = value_of_vault_waitng_queue.value;

        auto clause_addr = value_of_event->get_clause_addr(watcher_index); // the address of the clause
        //auto pushed_to_others = value_of_event->get_pushed(); // modified other's watcher list
        auto clause_detail = value_of_event->get_clause_detail(watcher_index); //vector about detailed read value; content type: unsigned long long
        auto size_of_detail = clause_detail.size();
        auto event_value = EventValue(EventType::ProcessClause, watcher_index, 1, value_of_event, HardwareType::ClauseUnit, 0, clause_addr, vault_index, -1);
        event_value.type = EventType::ProcessClause;
        event_value.index = watcher_index;
        auto result = m_cache.access(clause_addr);
        //
        auto latency = 0;
        //remaind here, now need to detail the latency! 1, the value latency 2, the clause latency,3, the sync and cache coherence latency
        if (result == cache::hit) // the latency of read clause;
        {
            latency += 1;
        }
        else
        {
            latency += vault_memory_access_latency;
        }
        if (result == cache::miss)
        {
            //create miss event
            auto event_value = EventValue(EventType::VaultMissAccess, watcher_index, 1, value_of_event, HardwareType::watcherListUnit, 0, clause_addr, vault_index, -1);
            auto event = Event(event_value, end_time, vault_memory_access_latency + end_time);
            //spdlog::debug(std::string("VaultCacheMiss addr:") + std::to_string(clause_addr) + std::string(", at cycle:") + std::to_string(end_time));
            spdlog::debug("VaultCacheMissDetailed addr:{}, at cycle:{}, on vault:{}", clause_addr, end_time, vault_index);

            m_event_queue.push(event);
            end_time += vault_memory_access_latency;
        }
        spdlog::debug(std::string("detail size: ") + std::to_string(size_of_detail));
        for (unsigned long i = 0; i < size_of_detail; i++)
        {
            auto detail = clause_detail[i];
            auto result = m_cache.access(detail);
            //here need to consider the remote modification
            // but currently , we only consider the original clause miss times
            if (result == cache::hit)
            {
                latency += 1;
            }
            else
            {

                latency += vault_memory_access_latency;
            }
            if (result == cache::miss)
            {
                auto evalue = EventValue(EventType::VaultMissAccess, watcher_index, 1, value_of_event, HardwareType::watcherListUnit, 0, detail, vault_index, -1); //fix bug here to cause fill segmentfaut, used to use clause addr here.
                auto event = Event(evalue, end_time, vault_memory_access_latency + end_time);

                spdlog::debug("VaultCacheMissDetailed addr:{}, at cycle:{}, on vault:{}", detail, end_time, vault_index);
                end_time += vault_memory_access_latency;
                m_event_queue.push(event);
            }
        }
        auto event = Event(event_value, end_time, end_time + 5);

        if (print_level >= 2)
            std::cout << event << std::endl;
        m_event_queue.push(event);
        assert(vault_waiting_queue[vault_index].size() > 0);
        vault_waiting_queue[vault_index].pop();
    }
    std::ostream &operator<<(std::ostream &os, BandWidthManager &bm)
    {
        os << "current usage:" << bm.current_bandwidth << std::endl;
        os << "total usage:" << bm.total_bandwidth << std::endl;
        return os;
    }
    void ACC::handle_vault_process_mode2(int vault_index, int end_time)
    {
        if (vault_waiting_queue[vault_index].empty())
        {
            return;
        }
        // if (clause_buffer_size[vault_index] >= 32)
        // {

        //     //std::cout << fmt::format("in valtu_index:{}, can't read clause because buffer full", vault_index) << std::endl;
        //     //std::cout << "the buffer[" << vault_index << "] size is full:" << clause_buffer_size[vault_index] << std::endl;
        //     return;
        // }
        //std::cout << "the buffer[" << vault_index << "] size not full:" << clause_buffer_size[vault_index] << std::endl;

        spdlog::debug("handle_vault_process mode2,vault:{},end_time:{}", vault_index, end_time);

        spdlog::debug("in handle vaut: vault: {}", vault_index);
        //assert(vault_waiting_queue[vault_index].size() == 1);
        spdlog::debug("vault:{},size:{}", vault_index, vault_waiting_queue[vault_index].size());
        auto value_of_vault_waitng_queue = vault_waiting_queue[vault_index].front();

        auto watcher_index = value_of_vault_waitng_queue.index;

        auto value_of_event = value_of_vault_waitng_queue.value;

        auto clause_addr = value_of_event->get_clause_addr(watcher_index); // the address of the clause
        //auto pushed_to_others = value_of_event->get_pushed(); // modified other's watcher list
        //auto clause_detail = value_of_event->get_clause_detail(watcher_index); //vector about detailed read value; content type: unsigned long long
        //auto size_of_detail = clause_detail.size();
        auto event_value = EventValue(EventType::FinishedReadClause, watcher_index, 1, value_of_event, HardwareType::ClauseUnit, 0, clause_addr, vault_index, -1);
        //event_value.type = EventType::ProcessClause;
        //event_value.index = watcher_index;
        /*if (!dram_bandwith_manager.tryAddUse(1))
        {
            //std::cout << fmt::format("in valut:{}, can't read clause because dram full", vault_index);
            //std::cout << "can't use more bandwidth" << std::endl;
            return;
        }
        */
        //clause_buffer_size[vault_index]++; // fix bug here: bug: add buffer size before test the dram bandwidth, fix: after test the bandwidth.
        //std::cout << dram_bandwith_manager << std::endl;
        vault_waiting_queue[vault_index].pop();
        // send miss request to memory controller
        // No event for process_clause generated here

        //auto event_value = EventValue(EventType::FinishedReadClause, watcher_index, 1, value_of_event, HardwareType::ClauseUnit, 0, clause_addr, vault_index, -1);
        auto event = Event(event_value, end_time, end_time + miss_latency);
        m_event_queue.push(event);
        return;
        //still busy
    }

    void ACC::print() const
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
    }

    ACC *create_acc(int watcher_proc_size,
                    int watcher_proc_num,
                    int clause_proc_num,
                    int miss_latency,
                    int watcher_process_latency,
                    int clause_process_latency,
                    int vault_memory_access_latency,
                    int cpu_to_vault_latency,
                    bool mode2,
                    int ctr_latency)
    {
        return new ACC(watcher_proc_size, watcher_proc_num,
                       clause_proc_num, miss_latency,
                       watcher_process_latency, clause_process_latency,
                       vault_memory_access_latency, cpu_to_vault_latency,
                       mode2, ctr_latency);
    }

    void ACC::handle_new_watch_list(std::queue<std::pair<int, assign_wrap *>> &waiting_queue, unsigned long long end_time, int watcher_index)
    {
        int start = waiting_queue.front().first;

        int total = waiting_queue.front().second->get_watcher_size();
        if (total == 0)
        {
            waiting_queue.pop();
            return;
        }
        auto addr = waiting_queue.front().second->get_addr();
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
            auto evalue = EventValue(EventType::missAccess, start, w_size, waiting_value, HardwareType::watcherListUnit, -1, -1, -1, watcher_index);
            auto event = Event(evalue, end_time, miss_latency + end_time);
            spdlog::debug(std::string("CacheMiss addr:") + std::to_string(addr) + std::string(", at cycle:") + std::to_string(end_time));
            m_event_queue.push(event);
        }
        if (total - start > w_size)
        {
            auto evalue = EventValue(EventType::ReadWatcherList, start, w_size, waiting_value, HardwareType::watcherListUnit, -1, -1, -1, watcher_index);
            auto event = Event(evalue, end_time, (is_hit ? 6 : miss_latency) + end_time);

            waiting_queue.front().first += w_size;
            if (print_level >= 2)
                std::cout << event << std::endl;
            m_event_queue.push(event);
            watcher_busy[watcher_index] = true;
            m_using_watcher_unit++;
        }
        else
        {
            //only print the last one

            auto evalue = EventValue(EventType::ReadWatcherList, start, total - start, waiting_value, HardwareType::watcherListUnit, -1, -1, -1, watcher_index);
            auto event = Event(evalue, end_time, (is_hit ? 6 : miss_latency) + end_time);
            if (print_level >= 1)
            {
                std::cout << "ACC::INIT:LIT: " << waiting_value->get_value() << std::endl;
            }
            //waiting_queue.front().first += (total - start);
            if (print_level >= 2)
                std::cout << event << std::endl;
            m_event_queue.push(event);
            waiting_queue.pop(); //finished
            watcher_busy[watcher_index] = true;

            m_using_watcher_unit++;
        }
    }
    void ACC::mem_ctr_process(MACC::Event event, int end_time)
    {
        throw std::runtime_error("Can't be here");
        assert(!m_memory_ctr_busy);
        m_memory_ctr_busy = true;
        auto new_event = event;
        new_event.set_end_time(end_time + vault_memory_access_latency); //
        new_event.set_start_time(end_time);
        new_event.get_value_ref().type = EventType::FinishedInMemCtr;
        m_event_queue.push(new_event);
    }
    const unsigned long long default_time = static_cast<unsigned long long>(-1);

    unsigned long long real_start = default_time;
    unsigned long long should_start = default_time;
    unsigned long long total_latency = 0;

    int ACC::start_sim()
    {
        assert(m_event_queue.empty());
        assert(!m_memory_ctr_busy);
        assert(mem_ctr_queue.empty());
        assert(std::all_of(vault_waiting_queue.begin(), vault_waiting_queue.end(), [](auto &the_queue) { return the_queue.empty(); }));
        assert(m_using_watcher_unit == 0);
        assert(m_using_clause_unit == 0);
        assert(std::all_of(watcher_busy.begin(), watcher_busy.end(), [](auto busy) { return busy == false; }));
        assert(std::all_of(c_busy.begin(), c_busy.end(), [](auto busy) { return busy == false; }));
        assert(std::all_of(clause_read_waiting_queue.begin(), clause_read_waiting_queue.end(), [](auto &queue) { return queue.empty(); }));
        assert(l3_bandwidth_manager.empty());
        assert(dram_bandwith_manager.empty());
        static unsigned long long global_times = 0;
        global_times++;

        if (value_queue.empty())
            return 0;
        //initialize the event queue;
        std::queue<std::pair<int, assign_wrap *>> clause_waiting_queue;

        auto first_value = value_queue.front();
        //auto size = value->get_watcher_size();
        //int process_size = 0;
        sjq::queue<std::pair<int, assign_wrap *>> waiting_queue;
        waiting_queue.push(std::make_pair(0, first_value));
        int i = 0;
        while (!waiting_queue.empty() && m_using_watcher_unit < w_num)
        {
            int watcher_index = -1;
            auto found = std::find(watcher_busy.begin(), watcher_busy.end(), false);
            assert(found != watcher_busy.end());
            watcher_index = std::distance(watcher_busy.begin(), found);
            assert(watcher_index >= 0 and watcher_index < w_num);

            handle_new_watch_list(waiting_queue, i, watcher_index);
            i++;
        }
        current_running_level = first_value->get_level();

        //second process the event queue,
        int last_cycle = 0;
        int global_end_time = 0;
        while (!m_event_queue.empty() or !waiting_queue.empty())
        {
            if (m_event_queue.empty())
            {
                assert(l3_bandwidth_manager.empty());
                assert(dram_bandwith_manager.empty());
                assert(std::all_of(clause_read_waiting_queue.begin(), clause_read_waiting_queue.end(), [](auto &queue) { return queue.empty(); }));
                assert(std::all_of(clause_buffer_size.begin(), clause_buffer_size.end(), [](auto size) { return size == 0; }));
                assert(std::all_of(vault_waiting_queue.begin(), vault_waiting_queue.end(), [](auto &queue) { return queue.empty(); }));
                total_latency += global_end_time - should_start;
                should_start = default_time;
                //std::cout << "latency:" << global_end_time - should_start << std::endl;
                spdlog::debug("sycn!global_sync_time:{}", global_end_time);
                //std::cout << "sync!________________" << std::endl;
                assert(!waiting_queue.empty() and m_using_watcher_unit == 0);
                i = 0;
                current_running_level++;
                assert(waiting_queue.front().second->get_level() == current_running_level);
                while (m_using_watcher_unit < w_num and !waiting_queue.empty()) //bug here, should block the next level
                {
                    assert(waiting_queue.front().second->get_level() == current_running_level);

                    int watcher_index = -1;
                    auto found = std::find(watcher_busy.begin(), watcher_busy.end(), false);
                    assert(found != watcher_busy.end());
                    watcher_index = std::distance(watcher_busy.begin(), found);
                    assert(watcher_index >= 0 and watcher_index < w_num);

                    handle_new_watch_list(waiting_queue, global_end_time + i, watcher_index);
                }
            }
            auto end_time = m_event_queue.get_next_time();
            global_end_time = end_time;

            auto event_value = m_event_queue.get_next_event();
            auto value_of_event = event_value.value;
            auto this_event = m_event_queue.get_next();
            m_event_queue.pop();
            last_cycle = end_time;
            switch (event_value.type)
            {
            case EventType::FinishedModWatcherList:
            {
                break;
            }
            case EventType::FinishedInMemCtr:
            {
                throw std::runtime_error("cant be here right now");
                m_memory_ctr_busy = false;
                spdlog::debug("FinishedInMemCtr:{},value:{},index:{}", end_time, value_of_event->get_value(), event_value.index);
                //need generate process clause event;
                auto finished_event = this_event; //bug here, the origin one have already been poped;
                finished_event.set_start_time(end_time);
                finished_event.set_end_time(end_time + ctr_latency + 10);
                finished_event.get_value_ref().type = EventType::ProcessClause;
                auto fill_event = finished_event;
                fill_event.get_value_ref().type = EventType::VaultMissAccess; //bug here, remember to fill the vault cache
                fill_event.set_end_time(end_time + ctr_latency);
                m_event_queue.push(finished_event);
                m_event_queue.push(fill_event);
                if (!mem_ctr_queue.empty())
                {
                    mem_ctr_process(mem_ctr_queue.front(), end_time);
                    mem_ctr_queue.pop();
                }
                break;
            }
            case EventType::SendToMemCtr:
            {
                throw std::runtime_error("can't be here right now");
                spdlog::debug("SendToMemCtr:{},value:{},index:{}", end_time, value_of_event->get_value(), event_value.index);
                auto event = this_event;
                mem_ctr_queue.push(event);
                if (!m_memory_ctr_busy)
                {
                    //process it
                    mem_ctr_process(mem_ctr_queue.front(), end_time);
                    mem_ctr_queue.pop();
                }
                break;
            }
            case EventType::VaultMissAccess:
            {

                auto vault_index = event_value.vault_index;
                auto vault_addr = event_value.addr;
                //spdlog::debug("fill Vault:addr: " ",at cycle:");
                spdlog::debug("fill Vault: addr :{},cycle :{},vault:{}", vault_addr, end_time, vault_index);

                m_cache.fill(vault_addr);
                break;
            }
            case EventType::missAccess:
            {
                spdlog::debug(std::string("fill:addr: ") + std::to_string(value_of_event->get_addr() + 8 * event_value.index) + std::string(",at cycle:") + std::to_string(end_time));
                m_cache.fill(value_of_event->get_addr() + 8 * event_value.index);
                break;
            }
            case EventType::ReadWatcherList:
            {

                auto evalue = event_value;
                evalue.type = EventType::ProcessWatcherList;
                auto event = Event(evalue, end_time, end_time + watcher_process_latency);
                //waiting_queue.front().first += (total - start);
                if (print_level >= 2)
                    std::cout << event << std::endl;
                m_event_queue.push(event);
                break;
            }
            case EventType::ProcessWatcherList:
            {
                auto watcher_index = event_value.watcher_index;
                assert(watcher_index >= 0 and watcher_index < w_num);
                spdlog::debug("ProcessWatcherList:{}", end_time);
                //finished process watcher_list, start to generate new clause access event
                //fist try to send clause reading request
                auto modified_list = value_of_event->get_modified_by_range(event_value.index, event_value.index + event_value.size);
                // only sorted map support this operation
                auto lower = modified_list.first;
                auto upper = modified_list.second;
                int iilatency = 0;
                while (lower != upper) //from cpu send to vault.
                {
                    //bug here, clause addr is not unique for the whole watcher list;
                    //auto clause_addr = value->get_clause_addr();

                    auto clause_addr = lower->second;
                    auto num_c_p_w = c_num / w_num;
                    assert(c_num % w_num == 0);

                    auto vault_index = next_c[watcher_index];
                    next_c[watcher_index] = (vault_index + 1) % num_c_p_w; //round

                    vault_index += watcher_index * num_c_p_w;

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
                    evalue.addr = clause_addr;
                    evalue.vault_index = vault_index;
                    //spdlog::debug("event queue size:{} , vault queue size:{}", m_event_queue.size(), vault_waiting_queue[vault_index].size());

                    spdlog::debug(std::string("send to vault:") + std::to_string(vault_index) + ",at time:" + std::to_string(end_time));

                    auto event = Event(evalue, end_time + iilatency, end_time + iilatency + cpu_to_vault_latency); //todo this is the cpu to vault latency;
                    iilatency += 1;
                    if (print_level >= 2)
                        std::cout << event << std::endl;
                    m_event_queue.push(event);
                    //vault_waiting_queue[i].pop(); /*  */

                    lower++;
                }

                //first to find any watcher_list_to process
                m_using_watcher_unit--;
                //auto watcher_index=event_value.watcher_index;
                assert(watcher_index != -1 && watcher_index >= 0 && watcher_index < w_num);
                watcher_busy[watcher_index] = false;

                if (waiting_queue.empty())
                {
                    idel_watcher_times++;
                    idel_watcher_total += w_num - m_using_watcher_unit;
                }
                if (!waiting_queue.empty() and waiting_queue.front().second->get_level() == current_running_level) // bug here/ TO-DO need to fix, here one watcher list should inside one watcher unit, but here, it will be seperate!!!! MAYBE, make it a feature??????????????????????????????????!!
                {
                    handle_new_watch_list(waiting_queue, end_time, watcher_index);
                }
                else if (!waiting_queue.empty() and waiting_queue.front().second->get_level() == current_running_level + 1)
                {
                    if (should_start == default_time)
                    { //change at the first time we try to run the next level
                        //std::cout << "set should start time" << std::endl;
                        should_start = end_time;
                    }
                }

                break;
            }
            case EventType::sendToVault: //after vault recived the clause request
            {

                auto index = event_value.index; //
                assert(event_value.size == 1);
                auto clause_addr = value_of_event->get_clause_addr(index); // the address of the clause
                //auto pushed_to_others = value_of_event->get_pushed_to_others(index); // modified other's watcher list
                //auto clause_detail = value_of_event->get_clause_detail(index); //vector about detailed read value; content type: unsigned long long

                auto vault_index = event_value.vault_index;
                assert(vault_index >= 0 and vault_index < c_num);
                //auto value_of_vault_waitng_queue=std::make_tuple(index, value_of_event, clause_addr);
                auto value_of_vault_waitng_queue = vault_waiting_queue_value(index, value_of_event, clause_addr);
                vault_waiting_queue[vault_index].push(value_of_vault_waitng_queue);

                spdlog::debug(std::string("recived in vault:") + std::to_string(vault_index) + ",at time:" + std::to_string(end_time));
                spdlog::debug("event queue size:{} , vault queue size:{}", m_event_queue.size(), vault_waiting_queue[vault_index].size());

                if (!mode2)
                {
                    spdlog::debug(std::string("process in Vault:") + std::to_string(vault_index) + ",at time:" + std::to_string(end_time));

                    handle_vault_process(vault_index, end_time);
                }
                else
                { // mode 2, only one memory controller exists,
                    spdlog::debug(std::string("process in Vault mode2:") + std::to_string(vault_index) + ",at time:" + std::to_string(end_time));

                    handle_vault_process_mode2(vault_index, end_time);
                }

                break;
            }
            case EventType::FinishAndSendClause:
                //in this case,
                break;
            case EventType::FinishedReadClause:
            {
                //dram_bandwith_manager.delUse(1);
                //std::cout << dram_bandwith_manager << std::endl;
                auto vault_index = event_value.vault_index;
                static int temp = 0;
                for (int i = 0; i < c_num; i++)
                {
                    handle_vault_process_mode2((temp + i) % c_num, end_time);
                }
                temp += 1;
                temp %= c_num;
                clause_read_waiting_queue[vault_index].push(std::make_pair(event_value.index, value_of_event));

                if (!vault_busy[vault_index])
                {
                    //clause_buffer_size[vault_index]--;
                    handle_vault_process_mode2(vault_index, end_time);

                    auto next_task = clause_read_waiting_queue[vault_index].front();
                    clause_read_waiting_queue[vault_index].pop();

                    event_value.type = EventType::ProcessClause;
                    event_value.index = next_task.first;
                    event_value.value = next_task.second;

                    auto event = Event(event_value, end_time, end_time + 12);
                    vault_busy[vault_index] = true;
                    m_event_queue.push(event);
                }
                break;
            }
            case EventType::ProcessClause: //need to modified. now get the task from the vault_waiting_queue
            {
                spdlog::debug("ProcessClause finished,cycle:{}", end_time);
                auto vault_index = event_value.vault_index;
                //std::cout<<value_of_event->get_level()<<std::endl;
                vault_busy[vault_index] = false; //correct, now set the busy to false

                //in this case, we need to generate new event, and let waiting queue to be processed

                //first to check if we generated new assignment or conflict
                if (value_of_event->get_generated_conf() == event_value.index)
                {
                    //spdlog::error("CONFILICT!");
                    //BUG/do nothing, this is the last one, just finish all the in-flay event
                }
                //check if generate new assignment
                //auto iter = value_of_event->get_generated_assignments().find(event_value.index);
                auto generated = value_of_event->get_generated(event_value.index);
                if (generated)
                {

                    waiting_queue.push(std::make_pair(0, generated));
                    assert(generated->get_level() == current_running_level + 1);
                    //std::cout << generated->get_level() << std::endl;

                    if (should_start == default_time and m_using_watcher_unit < w_num and generated->get_level() == current_running_level + 1)
                    {
                        //std::cout << "set shoudls tart" << std::endl;
                        should_start = end_time;
                    }
                    //change here, we shouldn't execut it immediatly, we should just waiting the sync point;
                    /*
                    int i = 0;
                    while (!waiting_queue.empty() && m_using_watcher_unit < w_num ) 
                    {
                        auto next=waiting_queue.front();
                        auto level=next.second->get_level();
                        if(level==current_running_level){
                            
                        }else{
                            throw std::runtime_error("should be the next level");
                        }
                        int watcher_index = -1;
                        auto found = std::find(watcher_busy.begin(), watcher_busy.end(), false);
                        assert(found != watcher_busy.end());
                        watcher_index = std::distance(watcher_busy.begin(), found);
                        assert(watcher_index >= 0 and watcher_index < w_num);

                        handle_new_watch_list(waiting_queue, end_time + i, watcher_index);
                        i++;
                    }
                    
                    if (m_using_watcher_unit >= w_num && !waiting_queue.empty())
                    {
                        int total_size = 0;
                        total_size = std::accumulate(waiting_queue.begin(), waiting_queue.end(), 0,
                                                     [](int size, auto &element) {
                                                         return size + element.second->get_watcher_size() - element.first;
                                                     });
                        waiting_watcher_list += (total_size + w_size - 1) / w_size;
                        waiting_watcher_times++;
                    }
                    */
                }

                //assert(vault_waiting_queue[vault_index].empty()); // bug here, need to have seperate queues for the value
                if (!clause_read_waiting_queue[vault_index].empty())
                {
                    //clause_buffer_size[vault_index]--;
                    handle_vault_process_mode2(vault_index, end_time);
                    auto next_task = clause_read_waiting_queue[vault_index].front();

                    clause_read_waiting_queue[vault_index].pop();

                    event_value.type = EventType::ProcessClause;
                    event_value.index = next_task.first;
                    event_value.value = next_task.second;

                    auto event = Event(event_value, end_time, end_time + 12);
                    vault_busy[vault_index] = true;
                    m_event_queue.push(event);
                }
                break;
            }
            } // switch
        }
        return last_cycle;
    }
    std::ostream &operator<<(std::ostream &os, EventType type)
    {
        switch (type)
        {
        case EventType::FinishedModWatcherList:
            os << "FinishedModWatcherList";
            break;
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
        case EventType::VaultMissAccess:
            os << "VaultMissAccess";
            break;
        case EventType::sendToVault:
            os << "sendToVault";
            break;
        case EventType::SendToMemCtr:
            os << "SendToMemCtr";
            break;
        case EventType::FinishedInMemCtr:
            os << "FinishedInMemCtr";
            break;
        case EventType::FinishedReadClause:
            os << "FinishedReadClause";
            break;
        }
        os << std::endl;
        return os;
    }
    std::ostream &operator<<(std::ostream &os, const EventValue &eventValue)
    {
        os << "EventValue:type: " << eventValue.type << std::endl;
        os << "EventValue:value: " << eventValue.value->get_value() << std::endl;
        os << "EventValue:index: " << eventValue.index << std::endl;
        os << "EventValue:size: " << eventValue.size << std::endl;
        return os;
    }

    std::ostream &operator<<(std::ostream &os, const Event &event)
    {
        os << "Event start time: " << event.get_start_time() << std::endl;
        os << "Event end time: " << event.get_end_time() << std::endl;
        os << event.get_value() << std::endl;
        return os;
    }
} // namespace MACC