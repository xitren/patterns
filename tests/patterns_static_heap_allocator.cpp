#include <xitren/allocators/static_heap.hpp>
#include <xitren/allocators/static_heap_allocator.hpp>

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <deque>
#include <iostream>
#include <list>
#include <set>
#include <unordered_map>

using namespace xitren::allocators;

template <std::invocable<> Callable>
auto
measure(Callable callback)
{
    using std::chrono::duration;
    using std::chrono::duration_cast;
    using std::chrono::high_resolution_clock;
    using std::chrono::microseconds;
    auto start = high_resolution_clock::now();
    callback();
    auto end    = high_resolution_clock::now();
    auto ms_int = duration_cast<microseconds>(end - start);
    return ms_int;
}

TEST(TestStaticHeap, SeveralSubsequentAllocations)
{
    static_heap<256> manager{};

    auto* ptr1 = manager.allocate(1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_NE(ptr1, nullptr);
    auto* ptr2 = manager.allocate(1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_NE(ptr2, nullptr);
    auto* ptr3 = manager.allocate(1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_NE(ptr3, nullptr);
    auto* ptr4 = manager.allocate(1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_NE(ptr4, nullptr);
}

TEST(TestStaticHeap, DeallocationsWithEmptyFreeList)
{
    static_heap<256> manager{};

    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    auto* ptr1 = manager.allocate(1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_NE(ptr1, nullptr);
    auto* ptr2 = manager.allocate(1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_NE(ptr2, nullptr);
    auto* ptr3 = manager.allocate(1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_NE(ptr3, nullptr);
    manager.deallocate(ptr1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    manager.deallocate(ptr2);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
}

TEST(TestStaticHeap, DeallocationsWithNonEmptyFreeList)
{
    static_heap<256> manager{};

    auto* ptr1 = manager.allocate(1);
    EXPECT_NE(ptr1, nullptr);
    auto* ptr2 = manager.allocate(1);
    EXPECT_NE(ptr2, nullptr);
    manager.deallocate(ptr1);
    manager.deallocate(ptr2);
}

TEST(TestStaticHeap, SuccessfulAllocationAfterDeallocation)
{
    static_heap<256> manager{};

    auto* ptr1 = manager.allocate(1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_NE(ptr1, nullptr);
    auto* ptr2 = manager.allocate(1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_NE(ptr2, nullptr);
    auto* ptr3 = manager.allocate(1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_NE(ptr3, nullptr);
    auto* ptr4 = manager.allocate(1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_NE(ptr4, nullptr);
    manager.deallocate(ptr2);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    manager.deallocate(ptr4);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    auto* ptr5 = manager.allocate(1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_EQ(ptr5, ptr2);
    auto* ptr6 = manager.allocate(1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_EQ(ptr6, ptr4);
}

TEST(TestStaticHeap, SuccessfulAllocationAfterConcatenation)
{
    static_heap<256> manager{};

    auto* ptr1 = manager.allocate(8);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_NE(ptr1, nullptr);
    auto* ptr2 = manager.allocate(8);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_NE(ptr2, nullptr);
    manager.deallocate(ptr1);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    manager.deallocate(ptr2);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    auto* ptr3 = manager.allocate(7);
    std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    EXPECT_EQ(ptr3, ptr1);
}

TEST(TestStaticHeap, FullAllocation)
{
    static_heap<256> manager{};
    int              i{};

    std::cout << "Start free_heap_size " << manager.free_heap_size() << " bytes\n";
    auto* ptr1 = manager.allocate(8);
    i++;
    EXPECT_NE(ptr1, nullptr);
    while (ptr1) {
        std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
        ptr1 = manager.allocate(8);
        i++;
    }
    std::cout << "items " << (i - 1) << " of " << (256 / 24) << " \n";
    EXPECT_EQ(i, (256 / 24));
}

TEST(TestStaticHeap, STLVectorListTest)
{
    constexpr std::size_t                             val = 256;
    static_heap<val>                                  manager{};
    static_heap_allocator<int, val>                   list{manager};
    std::vector<int, static_heap_allocator<int, val>> vec{list};
    auto                                              cnt{0};
    std::cout << "Start free_heap_size " << manager.free_heap_size() << " bytes\n";
    for (int i = 0; i < 6; i++) {
        try {
            vec.emplace_back(i);
            std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
        } catch (std::bad_alloc& ex) {
            cnt++;
        }
    }
    EXPECT_EQ(cnt, 0);
    for (int i = 0; i < 2; i++) {
        vec.pop_back();
        std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    }
    for (int i = 0; i < 3; i++) {
        try {
            vec.emplace_back(i);
            std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
        } catch (std::bad_alloc& ex) {
            cnt++;
        }
    }
    std::cout << "Vector capacity " << vec.capacity() << " \n";
    EXPECT_EQ(vec.capacity(), 8);
}

TEST(TestStaticHeap, AdvancedSTLVectorTest)
{
    constexpr std::size_t                             val = 256;
    static_heap<val>                                  manager{};
    static_heap_allocator<int, val>                   list{manager};
    std::vector<int, static_heap_allocator<int, val>> vec{list};
    vec.reserve(8);
    EXPECT_NO_THROW({
        for (int i = 0; i < 8; ++i) {
            vec.emplace_back(i);
        }
    });
    EXPECT_EQ(vec.capacity(), 8);
    EXPECT_EQ(vec.size(), 8);
    vec.resize(4);
    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), 4);
    EXPECT_EQ(vec.size(), 4);
    vec.clear();
    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), 0);
    EXPECT_EQ(vec.size(), 0);
    vec.reserve(16);
    EXPECT_NO_THROW({
        for (int i = 0; i < 16; ++i) {
            vec.emplace_back(i);
        }
    });
    EXPECT_EQ(vec.capacity(), 16);
    EXPECT_EQ(vec.size(), 16);
}

TEST(TestStaticHeap, TestListPushAndInsert)
{
    constexpr std::size_t val = 4096;
    // int const                                       count = 64;
    static_heap<val>                                manager{};
    static_heap_allocator<int, val>                 list_all{manager};
    std::list<int, static_heap_allocator<int, val>> list{list_all};

    EXPECT_NO_THROW({
        for (int i = 17; i < 32; ++i) {
            list.emplace_back(i);
            std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
        }
        for (int i = 14; i >= 0; --i) {
            list.emplace_front(i);
            std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
        }
        auto pos = list.begin();
        for (int i = 0; i < 15; ++i) {
            ++pos;
        }
        list.insert(pos, 16);
        std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
        --pos;
        list.insert(pos, 15);
        std::cout << "free_heap_size " << manager.free_heap_size() << " bytes\n";
    });
    int cnt = 0;
    for (auto elem : list) {
        EXPECT_EQ(elem, cnt++);
    }
    EXPECT_NO_THROW({ list.clear(); });
}

TEST(TestStaticHeap, TestSet)
{
    constexpr std::size_t val = 4096;
    // int const                                                      count = 64;
    static_heap<val>                                               manager{};
    static_heap_allocator<int, val>                                list_all{manager};
    std::set<int, std::less<int>, static_heap_allocator<int, val>> set{list_all};
    EXPECT_NO_THROW({
        for (int i = 16; i >= 0; --i) {
            set.insert(i);
        }
    });
    auto cnt = 0;
    for (auto elem : set) {
        EXPECT_EQ(elem, cnt++);
    }
    EXPECT_NO_THROW({ set.insert({17, 18, 19, 20}); });
    EXPECT_TRUE(set.contains(0) && set.contains(20));
    EXPECT_FALSE(set.contains(21));
    auto pos = set.find(10);
    EXPECT_FALSE(pos == set.end());
    set.erase(pos);
    EXPECT_TRUE(set.find(10) == set.end());
    set.clear();
    EXPECT_TRUE(set.empty());
}

TEST(TestStaticHeap, TestUnorderedMap)
{
    constexpr std::size_t val = 1024;
    // int const             count = 64;
    using key_type   = int;
    using value_type = int;
    using alloc_type = std::pair<key_type const, value_type>;
    static_heap<val>                       manager{};
    static_heap_allocator<alloc_type, val> list_all{manager};
    std::unordered_map<key_type, value_type, std::hash<key_type>, std::equal_to<key_type>,
                       static_heap_allocator<alloc_type, val>>
        hashTable{list_all};
    EXPECT_NO_THROW({
        for (int i = 0; i < 16; ++i) {
            hashTable[i] = i * i;
        }
        for (int i = 0; i < 20; ++i) {
            hashTable.insert({i, 2 * i});
        }
    });
    EXPECT_EQ(hashTable[10], 100);
    EXPECT_EQ(hashTable.at(16), 32);
    EXPECT_THROW({ hashTable.at(21); }, std::out_of_range);

    for (auto it = hashTable.begin(); it != hashTable.end();) {
        if (it->first % 2 == 0) {
            it = hashTable.erase(it);
        } else {
            ++it;
        }
    }
    EXPECT_EQ(hashTable.size(), 10);
    EXPECT_THROW({ hashTable.at(10); }, std::out_of_range);
    EXPECT_TRUE(hashTable.find(6) == hashTable.end());
    EXPECT_TRUE(hashTable.at(5) == 25);
    hashTable.clear();
    EXPECT_TRUE(hashTable.empty());
}

template <class Allocator, size_t Count>
void
make_count_allocations_deallocations(Allocator allocator)
{
    std::array<typename Allocator::value_type*, Count> pointers;
    auto                                               do_allocations = [&]() {
        for (size_t i = 0; i < Count; ++i) {
            pointers[i] = allocator.allocate(1);
        }
    };
    auto alloc_duration = measure(do_allocations);

    auto do_deallocations = [&]() {
        for (size_t i = 0; i < Count; ++i) {
            allocator.deallocate(pointers[i], 1);
        }
    };
    auto dealloc_duration = measure(do_deallocations);

    int const offset = 20;
    std::cout << alloc_duration.count() << std::setw(offset) << dealloc_duration.count() << std::endl;
}

template <class T, size_t Count>
void
compare_allocators_performance()
{
    constexpr std::size_t         val = 16'777'216;
    static_heap<val>              manager{};
    static_heap_allocator<T, val> list_all{manager};

    int const offset = 20;
    std::cout << std::setw(offset) << "Allocator" << std::setw(offset) << "Size of type" << std::setw(offset) << "Count"
              << std::setw(offset) << " Alloc time (mcs)" << std::setw(offset) << " Dealloc time (mcs)\n";

    std::cout << std::setw(offset) << "Custom" << std::setw(offset) << sizeof(T) << std::setw(offset) << Count
              << std::setw(offset);
    make_count_allocations_deallocations<static_heap_allocator<T, val>, Count>(list_all);
    std::cout << std::setw(offset) << "free_heap_size " << manager.free_heap_size() << " bytes\n";

    std::cout << std::setw(offset) << "Standard" << std::setw(offset) << sizeof(T) << std::setw(offset) << Count
              << std::setw(offset);
    make_count_allocations_deallocations<std::allocator<T>, Count>(std::allocator<T>());
    std::cout << std::setw(offset) << "free_heap_size " << manager.free_heap_size() << " bytes\n";
}

// TEST(TestStaticHeap, SubsequentAllocationsDeallocationsOfChar)
// {
//     compare_allocators_performance<char, 256>();
//     std::cout << std::endl;
//     compare_allocators_performance<char, 1024>();
//     std::cout << std::endl;
//     // compare_allocators_performance<char, 65536>();
//     std::cout << std::endl;
// }

// TEST(TestStaticHeap, SubsequentAllocationsDeallocationsOfInt)
// {
//     compare_allocators_performance<int, 256>();
//     std::cout << std::endl;
//     compare_allocators_performance<int, 1024>();
//     std::cout << std::endl;
//     // compare_allocators_performance<int, 65536>();
//     std::cout << std::endl;
// }
