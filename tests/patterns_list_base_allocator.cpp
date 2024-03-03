#include <xitren/allocators/list_allocator.hpp>

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <deque>
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

TEST(TestManager, SeveralSubsequentAllocations)
{
    list_manager<(sizeof(vault_list) + 1) * 3> manager{};

    auto* ptr1 = manager.allocate(1);
    EXPECT_NE(ptr1, nullptr);
    EXPECT_TRUE(manager.check_invariant());
    auto* ptr2 = manager.allocate(1);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_TRUE(manager.check_invariant());
    auto* ptr3 = manager.allocate(1);
    EXPECT_NE(ptr3, nullptr);
    EXPECT_TRUE(manager.check_invariant());
    auto* ptr4 = manager.allocate(1);
    EXPECT_EQ(ptr4, nullptr);
    EXPECT_TRUE(manager.check_invariant());
}

TEST(TestManager, DeallocationsWithEmptyFreeList)
{
    list_manager<(sizeof(vault_list) + 1) * 3> manager{};

    auto* ptr1 = manager.allocate(1);
    EXPECT_NE(ptr1, nullptr);
    auto* ptr2 = manager.allocate(1);
    EXPECT_NE(ptr2, nullptr);
    auto* ptr3 = manager.allocate(1);
    EXPECT_NE(ptr3, nullptr);
    manager.deallocate(ptr1);
    EXPECT_TRUE(manager.check_invariant());
    manager.deallocate(ptr2);
    EXPECT_TRUE(manager.check_invariant());
}

TEST(TestManager, DeallocationsWithNonEmptyFreeList)
{
    list_manager<(sizeof(vault_list) + 1) * 3> manager{};

    auto* ptr1 = manager.allocate(1);
    EXPECT_NE(ptr1, nullptr);
    auto* ptr2 = manager.allocate(1);
    EXPECT_NE(ptr2, nullptr);
    manager.deallocate(ptr1);
    EXPECT_TRUE(manager.check_invariant());
    manager.deallocate(ptr2);
    EXPECT_TRUE(manager.check_invariant());
}

TEST(TestManager, SuccessfulAllocationAfterDeallocation)
{
    list_manager<(sizeof(vault_list) + 1) * 4> manager{};

    auto* ptr1 = manager.allocate(1);
    EXPECT_NE(ptr1, nullptr);
    auto* ptr2 = manager.allocate(1);
    EXPECT_NE(ptr2, nullptr);
    auto* ptr3 = manager.allocate(1);
    EXPECT_NE(ptr3, nullptr);
    auto* ptr4 = manager.allocate(1);
    EXPECT_NE(ptr4, nullptr);
    manager.deallocate(ptr2);
    EXPECT_TRUE(manager.check_invariant());
    manager.deallocate(ptr4);
    EXPECT_TRUE(manager.check_invariant());
    auto* ptr5 = manager.allocate(1);
    EXPECT_EQ(ptr5, ptr2);
    EXPECT_TRUE(manager.check_invariant());
    auto* ptr6 = manager.allocate(1);
    EXPECT_EQ(ptr6, ptr4);
    EXPECT_TRUE(manager.check_invariant());
}

TEST(TestManager, UnsuccessfulAllocationAfterDeallocation)
{
    list_manager<(sizeof(vault_list) + 8) * 2> manager{};

    auto* ptr1 = manager.allocate(8);
    EXPECT_NE(ptr1, nullptr);
    auto* ptr2 = manager.allocate(8);
    EXPECT_NE(ptr2, nullptr);
    manager.deallocate(ptr1);
    EXPECT_TRUE(manager.check_invariant());
    auto* ptr3 = manager.allocate(7);
    EXPECT_EQ(ptr3, nullptr);
    EXPECT_TRUE(manager.check_invariant());
}

TEST(TestManager, SuccessfulAllocationAfterConcatenation)
{
    list_manager<(sizeof(vault_list) + 8) * 2> manager{};

    auto* ptr1 = manager.allocate(8);
    EXPECT_NE(ptr1, nullptr);
    auto* ptr2 = manager.allocate(8);
    EXPECT_NE(ptr2, nullptr);
    manager.deallocate(ptr1);
    manager.deallocate(ptr2);
    EXPECT_TRUE(manager.check_invariant());
    auto* ptr3 = manager.allocate(7);
    EXPECT_EQ(ptr3, ptr1);
    EXPECT_TRUE(manager.check_invariant());
}

TEST(TestAllocators, SimpleListTest)
{
    list_manager<(sizeof(vault_list) + 1) * 3> manager{};

    auto* ptr1 = manager.allocate(1);
    EXPECT_NE(ptr1, nullptr);
    EXPECT_TRUE(manager.check_invariant());
    auto* ptr2 = manager.allocate(1);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_TRUE(manager.check_invariant());
    auto* ptr3 = manager.allocate(1);
    EXPECT_NE(ptr3, nullptr);
    EXPECT_TRUE(manager.check_invariant());
    auto* ptr4 = manager.allocate(1);
    EXPECT_EQ(ptr4, nullptr);
    EXPECT_TRUE(manager.check_invariant());
    manager.deallocate(ptr2);
    EXPECT_TRUE(manager.check_invariant());
    manager.deallocate(ptr1);
    auto* ptr5 = manager.allocate(1);
    EXPECT_EQ(ptr5, ptr1);
    EXPECT_TRUE(manager.check_invariant());
}

TEST(TestAllocators, STLVectorListTest)
{
    list_manager<(sizeof(vault_list) + sizeof(int)) * 4>                          manager{};
    list_allocator<int, (sizeof(vault_list) + sizeof(int)) * 4>                   list{manager};
    std::vector<int, list_allocator<int, (sizeof(vault_list) + sizeof(int)) * 4>> vec{list};
    auto                                                                          cnt{0};
    EXPECT_TRUE(manager.check_invariant());
    for (int i = 0; i < 6; i++) {
        try {
            vec.emplace_back(i);
        } catch (std::bad_alloc& ex) {
            cnt++;
        }
    }
    EXPECT_EQ(cnt, 4);
    EXPECT_TRUE(manager.check_invariant());
    for (int i = 0; i < 2; i++) {
        vec.pop_back();
    }
    for (int i = 0; i < 3; i++) {
        try {
            vec.emplace_back(i);
        } catch (std::bad_alloc& ex) {
            cnt++;
        }
    }
    EXPECT_EQ(cnt, 5);
    EXPECT_TRUE(manager.check_invariant());
}

TEST(TestAllocators, AdvancedSTLVectorTest)
{
    list_manager<(sizeof(vault_list) + sizeof(int)) * 4>                          manager{};
    list_allocator<int, (sizeof(vault_list) + sizeof(int)) * 4>                   list{manager};
    std::vector<int, list_allocator<int, (sizeof(vault_list) + sizeof(int)) * 4>> vec{list};
    vec.reserve(8);
    EXPECT_NO_THROW({
        for (int i = 0; i < 8; ++i) {
            vec.emplace_back(i);
        }
    });
    EXPECT_EQ(vec.capacity(), 8);
    EXPECT_EQ(vec.size(), 8);
    EXPECT_TRUE(manager.check_invariant());
    vec.resize(4);
    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), 4);
    EXPECT_EQ(vec.size(), 4);
    EXPECT_TRUE(manager.check_invariant());
    vec.clear();
    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), 0);
    EXPECT_EQ(vec.size(), 0);
    EXPECT_TRUE(manager.check_invariant());
    vec.reserve(16);
    EXPECT_NO_THROW({
        for (int i = 0; i < 16; ++i) {
            vec.emplace_back(i);
        }
    });
    EXPECT_THROW(vec.emplace_back(0), std::bad_alloc);
    EXPECT_EQ(vec.capacity(), 16);
    EXPECT_EQ(vec.size(), 16);
    EXPECT_TRUE(manager.check_invariant());
}

TEST(TestSTLContainers, TestListPushAndInsert)
{
    int const                                                                       count = 64;
    list_manager<(sizeof(vault_list) + sizeof(int)) * count>                        manager{};
    list_allocator<int, (sizeof(vault_list) + sizeof(int)) * count>                 list_all{manager};
    std::list<int, list_allocator<int, (sizeof(vault_list) + sizeof(int)) * count>> list{list_all};

    EXPECT_NO_THROW({
        for (int i = 17; i < 32; ++i) {
            list.emplace_back(i);
        }
        for (int i = 14; i >= 0; --i) {
            list.emplace_front(i);
        }
        auto pos = list.begin();
        for (int i = 0; i < 15; ++i) {
            ++pos;
        }
        list.insert(pos, 16);
        --pos;
        list.insert(pos, 15);
    });
    int cnt = 0;
    for (auto elem : list) {
        EXPECT_EQ(elem, cnt++);
    }
    EXPECT_TRUE(manager.check_invariant());
    EXPECT_NO_THROW({ list.clear(); });
    EXPECT_TRUE(manager.check_invariant());
}

TEST(TestSTLContainers, TestListPopAndErase)
{
    int const                                                                       count = 64;
    list_manager<(sizeof(vault_list) + sizeof(int)) * count>                        manager{};
    list_allocator<int, (sizeof(vault_list) + sizeof(int)) * count>                 list_all{manager};
    std::list<int, list_allocator<int, (sizeof(vault_list) + sizeof(int)) * count>> list{list_all};
    EXPECT_NO_THROW({
        for (int i = 0; i < 8; ++i) {
            if (i % 2 == 0) {
                list.emplace_back(i);
            } else {
                list.emplace_front(i);
            }
        }
    });
    EXPECT_TRUE(manager.check_invariant());
    auto pos = list.end();
    --(--pos);
    pos = list.erase(pos);
    EXPECT_TRUE(list.erase(pos) == list.end());
    EXPECT_NO_THROW({
        for (int i = 0; i < 6; ++i) {
            if (i < 3) {
                list.pop_front();
            } else {
                list.pop_back();
            }
        }
    });
    EXPECT_TRUE(list.empty());
    EXPECT_TRUE(manager.check_invariant());
}

TEST(TestSTLContainers, TestDeque)
{
    int const                                                                        count = 64;
    list_manager<(sizeof(vault_list) + sizeof(int)) * count>                         manager{};
    list_allocator<int, (sizeof(vault_list) + sizeof(int)) * count>                  list_all{manager};
    std::deque<int, list_allocator<int, (sizeof(vault_list) + sizeof(int)) * count>> deque{list_all};
    EXPECT_NO_THROW({
        for (int i = 15; i >= 0; --i) {
            deque.emplace_front(i);
        }
        for (int i = 16; i < 32; ++i) {
            deque.emplace_back(i);
        }
    });
    EXPECT_EQ(deque[15], 15);
    auto cnt = 0;
    for (auto elem : deque) {
        EXPECT_EQ(elem, cnt++);
    }
    EXPECT_NO_THROW({
        for (int _ = 0; _ < 16; ++_) {
            deque.pop_back();
        }
    });
    EXPECT_TRUE(manager.check_invariant());
    deque.clear();
    EXPECT_TRUE(deque.empty());
}

TEST(TestSTLContainers, TestSet)
{
    int const                                                                                      count = 64;
    list_manager<(sizeof(vault_list) + sizeof(int)) * count>                                       manager{};
    list_allocator<int, (sizeof(vault_list) + sizeof(int)) * count>                                list_all{manager};
    std::set<int, std::less<int>, list_allocator<int, (sizeof(vault_list) + sizeof(int)) * count>> set{list_all};
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
    EXPECT_TRUE(manager.check_invariant());
    set.clear();
    EXPECT_TRUE(set.empty());
    EXPECT_TRUE(manager.check_invariant());
}

TEST(TestSTLContainers, TestUnorderedMap)
{
    int const count  = 64;
    using key_type   = int;
    using value_type = int;
    using alloc_type = std::pair<key_type const, value_type>;
    list_manager<(sizeof(vault_list) + sizeof(alloc_type)) * count>               manager{};
    list_allocator<alloc_type, (sizeof(vault_list) + sizeof(alloc_type)) * count> list_all{manager};
    std::unordered_map<key_type, value_type, std::hash<key_type>, std::equal_to<key_type>,
                       list_allocator<alloc_type, (sizeof(vault_list) + sizeof(alloc_type)) * count>>
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
    EXPECT_TRUE(manager.check_invariant());
    hashTable.clear();
    EXPECT_TRUE(hashTable.empty());
    EXPECT_TRUE(manager.check_invariant());
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
    list_manager<(sizeof(vault_list) + sizeof(T)) * Count>      manager{};
    list_allocator<T, (sizeof(vault_list) + sizeof(T)) * Count> list_all{manager};

    int const offset = 20;
    std::cout << std::setw(offset) << "Allocator" << std::setw(offset) << "Size of type" << std::setw(offset) << "Count"
              << std::setw(offset) << " Alloc time (mcs)" << std::setw(offset) << " Dealloc time (mcs)\n";

    std::cout << std::setw(offset) << "Custom" << std::setw(offset) << sizeof(T) << std::setw(offset) << Count
              << std::setw(offset);
    make_count_allocations_deallocations<list_allocator<T, (sizeof(vault_list) + sizeof(T)) * Count>, Count>(list_all);

    std::cout << std::setw(offset) << "Standard" << std::setw(offset) << sizeof(T) << std::setw(offset) << Count
              << std::setw(offset);
    make_count_allocations_deallocations<std::allocator<T>, Count>(std::allocator<T>());
}

TEST(TestPerformance, SubsequentAllocationsDeallocationsOfChar)
{
    compare_allocators_performance<char, 256>();
    std::cout << std::endl;
    compare_allocators_performance<char, 1024>();
    std::cout << std::endl;
    compare_allocators_performance<char, 65536>();
    std::cout << std::endl;
}

TEST(TestPerformance, SubsequentAllocationsDeallocationsOfInt)
{
    compare_allocators_performance<int, 256>();
    std::cout << std::endl;
    compare_allocators_performance<int, 1024>();
    std::cout << std::endl;
    compare_allocators_performance<int, 65536>();
    std::cout << std::endl;
}

TEST(TestPerformance, SubsequentAllocationsDeallocationsOfStruct)
{
    GTEST_SKIP();
    struct t_struct {
        int64_t first;
        int64_t second;
        int64_t third;
        int64_t fourth;
    };

    compare_allocators_performance<t_struct, 256>();
    std::cout << std::endl;
    compare_allocators_performance<t_struct, 1024>();
    std::cout << std::endl;
    compare_allocators_performance<t_struct, 65536>();
    std::cout << std::endl;
}
