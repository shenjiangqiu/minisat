#include"core/cache_wrap.h"
#include<cache.h>
std::pair<CacheWrap::status,CacheWrap::hit_where> CacheWrap::access(unsigned long long addr)
    {
        auto l1_result = l1cache.access(addr);
        if (l1_result == cache::hit)
        {
            return std::make_pair(hit, L1);
        }
        if (l1_result == cache::hit_res)
        {
            return std::make_pair(hit_res, L1);
        }
        else
        {
            l1cache.fill(addr);
            auto l2_result = l2cache.access(addr);
            if (l2_result == cache::hit)
            {
                return std::make_pair(hit, L2);
            }
            if (l2_result == cache::hit_res)
            {
                return std::make_pair(hit_res, L2);
            }
            else
            {
                l2cache.fill(addr);
                auto l3_result = l3cache.access(addr);
                if (l3_result == cache::hit)
                {
                    return std::make_pair(hit, L3);
                }
                if (l3_result == cache::hit_res)
                {
                    return std::make_pair(hit_res, L3);
                }
                else
                {
                    l3cache.fill(addr);
                    return std::make_pair(miss, L3);
                }
            }
        }
    }