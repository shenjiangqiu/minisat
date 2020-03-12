#ifndef _EVENT_H_
#define _EVENT_H_
#include <memory>
enum class EventType
{
    ReadWatcherList,
    ProcessWatcherList,
    FinishAndSendClause,
    ProcessClause,

};
enum class HardwareType
{
    watcherListUnit,
    ClauseUnit
};
template <typename T>
struct EventValue
{
    EventValue(EventType _tp, int _id, int _sz, std::shared_ptr<T> v, HardwareType ht, int hi) : type(_tp), index(_id), size(_sz), value(v), hType(ht), HardwareId(hi) {}
    EventType type;
    int index;
    int size;
    std::shared_ptr<T> value; // the assignment wraper
    HardwareType hType;
    int HardwareId;
};

template <typename T>
class Event
{
public:
    Event(const EventValue<T> &value,int startTime,int endTime) : mEventValue(value),start_time(startTime),end_time(endTime) {}
    int get_key() const { return end_time; }
    const EventValue<T> &get_value() const { return mEventValue; }

private:
    int start_time;
    int end_time;
    EventValue<T> mEventValue;
};
template <typename T>
auto EventComp = [](const Event<T> &e1, const Event<T> &e2) {
    return e1.get_key() > e2.get_key();
};

#endif /* _EVENT_H_ */