// #ifndef CACHE_WRAP_H
// #define CACHE_WRAP_H
// #include <tuple>
// class CacheWrap {
// public:
//   CacheWrap()
//       : l1cache(1 << 3, 1 << (14 - 6 - 3), sjq::cache::lru, 512, 512,
//                 "l1cache"),
//         l2cache(1 << 3, 1 << (19 - 6 - 3), sjq::cache::lru, 512, 512,
//                 "l2cache"),
//         l3cache(1 << 3, 1 << (25 - 6 - 3), sjq::cache::lru, 512, 512,
//                 "l3cache") {}
//   enum status { hit, miss, hit_res };
//   enum hit_where {
//     L1,
//     L2,
//     L3,
//   };
//   std::pair<status, hit_where> access(unsigned long long addr,
//                                       sjq::cache::access_type type);

// private:
//   sjq::cache l1cache;
//   sjq::cache l2cache;
//   sjq::cache l3cache;
// };

// #endif
