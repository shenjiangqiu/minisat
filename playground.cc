#include <queue>
#include <iostream>
#include <memory>
using namespace std;
struct element
{
    int key;
    int value;
};

ostream &operator<<(ostream &os, const element &e)
{
    os << e.key << "=" << e.value;
    return os;
}
auto compare = [](const element &lhs, const element &rhs) { return lhs.key < rhs.key; };
int main(int argc, char **argv)
{
    std::priority_queue<element, std::deque<element>, decltype(compare)> queue(compare);
    for (element i : {element{1, 1}, element{2, 2}, element{3, 3}, element{1, 4}})
    {
        queue.push(i);
    }
    while (!queue.empty())
    {
        cout << queue.top() << endl;
        queue.pop();
    }
    //std::pair<int, float> a = {1, 2.9};

    auto a = std::shared_ptr<int>(new int(10));

    if (a)
    {
        std::cout << "yes!" << std::endl;
        std::cout << *a << std::endl;
    }
    else
    {
        std::cout << "no!" << std::endl;
    }


    if((int b=0)==0){
        
    }
}