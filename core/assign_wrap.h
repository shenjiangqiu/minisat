#ifndef ASSIGN_WRAP_H
#define ASSIGN_WRAP_H
#include <memory>
#include <tuple>
#include <map>
#include <vector>
template <typename T>
class assign_wrap
{
public:
    ~assign_wrap()
    {
    }
    assign_wrap() = delete;
    void add_pushed_list(int index, T value) { pushed_other_list_items.insert(std::make_pair(index, value)); }
    void add_modified_list(int index, unsigned long long value) { modified_clause_list_items.insert(std::make_pair(index, value)); }
    void add_detail(int index, unsigned long long value) { clause_detail[index].push_back(value); }
    std::vector<unsigned long long> get_clause_detail(int index) { return clause_detail[index]; }
    void set_watcher_size(int size) { watcher_size = size; }
    int get_watcher_size() const { return watcher_size; }
    const auto &get_pushed() const { return pushed_other_list_items; }
    const auto &get_modified() const { return modified_clause_list_items; }
    int get_level() const { return level; }
    void set_generated_conf(int conf) { generated_conf = conf; }
    int get_generated_conf() const { return generated_conf; }
    const auto &get_generated_assignments() const { return generated_assignments; }
    void add_generated_assignments(int index, std::weak_ptr<assign_wrap<T>> tgenerated)
    {
        generated_assignments.insert(std::make_pair(index, tgenerated));
    }
    friend class assign_wrap_factory;

    assign_wrap(T value,
                int watcher_size,
                int depend_id,
                std::weak_ptr<assign_wrap<T>> depend_value,
                int level = 0) : value(value),
                                 watcher_size(watcher_size),
                                 depend_id(depend_id),
                                 depend_value(depend_value),
                                 generated_conf(-1),
                                 clause_size(0),
                                 level(level)
    {
    }
    assign_wrap(assign_wrap<T> &other) = default;
    assign_wrap(assign_wrap<T> &&other) = default;
    assign_wrap(const assign_wrap<T> &other) = default;
    const T &get_value() const { return value; }
    void set_addr(unsigned long long t_addr) { addr = t_addr; }                                 //watcher list addr
    unsigned long long get_addr() const { return addr; }                                        //watcher list addr
    unsigned long long get_clause_addr(int index) { return modified_clause_list_items[index]; } //clause  addr

private:
    unsigned long long clause_addr;
    T value;
    int watcher_size;
    int depend_id;
    std::weak_ptr<assign_wrap<T>> depend_value;
    //std::pair<int, std::weak_ptr<assign_wrap<T>>> Depends;
    std::map<int, unsigned long long> modified_clause_list_items;
    std::map<int, std::vector<unsigned long long>> clause_detail;
    std::map<int, T> pushed_other_list_items;
    int generated_conf;

    std::map<int, std::weak_ptr<assign_wrap<T>>> generated_assignments; // must make the ptr weak here to break circular;

    int clause_size;

    int level;
    unsigned long long addr;
};
template <typename SharedPointType>
struct GetType
{
    typedef typename SharedPointType::element_type type;
};
class assign_wrap_factory
{
public:
    template <typename T>
    std::shared_ptr<assign_wrap<T>> create(T value,
                                           int watcher_size,
                                           int depend_id,
                                           std::shared_ptr<assign_wrap<T>> depend_value,
                                           int level = 0)
    {
        auto depend_value_weak = std::weak_ptr<assign_wrap<T>>(depend_value);
        auto rt = std::make_shared<assign_wrap<T>>(value, watcher_size, depend_id, depend_value_weak, level);
        if (depend_id != -1)
        {
            //std::cout<<"added to the assignement"<<std::endl;
            depend_value->add_generated_assignments(depend_id, rt);
        }
        return rt;
    }
};

#endif