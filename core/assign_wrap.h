#ifndef ASSIGN_WRAP_H
#define ASSIGN_WRAP_H
#include <memory>
#include <tuple>
template <typename T, typename Time_t = int, typename Size_t = int, typename Clause_t = int>
class assign_wrap
{
public:
    assign_wrap() = delete;
    assign_wrap(T value, Time_t time,
                Size_t watcher_size,
                std::shared_ptr<assign_wrap<T, Time_t, Size_t>> depend = nullptr,
                int level = 0) : value(value),
                                 start_time(time),
                                 watcher_size(watcher_size),
                                 Depends(depend),
                                 level(level),
                                 clause_size(0) {}
    Time_t get_time() const { return end_time - start_time; }
    void add_pushed_list(int index, T *value) { pushed_other_list_items.push_back(std::make_pair(index, value)); }
    void add_modified_list(int index, Clause_t value) { modified_clause_list_items.push_back(std::make_pair(index, value)); }
    void setstart_time(Time_t time) { start_time = time; }
    void setend_time(Time_t time) { end_time = time; }
    Time_t get_start_time() const { return start_time; }
    Time_t get_end_time() const { return end_time; }
    void set_watcher_size(Size_t size) { watcher_size = size; }
    Size_t get_watcher_size() const { return watcher_size; }
    const std::vector<std::pair<int, T *>> &get_pushed() const { return pushed_other_list_items; }
    const std::vector<std::pair<int, Clause_t>> &get_modified() const { return modified_clause_list_items; }
    int get_level() { return level; }

private:
    T value;
    Time_t start_time;
    Time_t end_time;
    Time_t wather_list_finish_time;
    Time_t clause_finish_time;
    Size_t watcher_size;
    std::shared_ptr<assign_wrap<T, Time_t, Size_t>> Depends;
    std::vector<std::pair<int, T *>> pushed_other_list_items;
    std::vector<std::pair<int, Clause_t>> modified_clause_list_items;
    Size_t clause_size;
    int level;
};

class assign_wrap_factory
{
public:
    template <typename T, typename Time_t, typename Size_t>
    static std::shared_ptr<assign_wrap<T, Time_t, Size_t>> create(T value,
                                                                  Time_t start_time,
                                                                  Size_t watcher_size,
                                                                  std::shared_ptr<assign_wrap<T, Time_t, Size_t>> depend = nullptr,
                                                                  int level = 0)
    {
        return std::make_shared<assign_wrap<T, Time_t, Size_t>>(value, start_time, watcher_size, depend, level);
    }
};

#endif