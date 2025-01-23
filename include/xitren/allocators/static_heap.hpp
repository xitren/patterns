/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 20.01.2025
*/
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>

namespace xitren::allocators {

template <std::size_t Size>
class static_heap {
    static const std::size_t config_total_heap_size = Size;

    using callback_t = std::function<void(void)>;

    /* Define the linked list structure.  This is used to link free blocks in order
     * of their memory address. */
    using block_link_t = struct a_block_link {
        struct a_block_link* px_next_free_block; /**< The next free block in the list. */
        size_t               x_block_size;       /**< The size of the free block. */
    };

    /* Used to pass information about the heap. */
    using heap_stats_t = struct {
        size_t x_available_heap_space_in_bytes;
        size_t x_size_of_largest_free_block_in_bytes;
        size_t x_size_of_smallest_free_block_in_bytes;
        size_t x_number_of_free_blocks;
        size_t x_minimum_ever_free_bytes_remaining;
        size_t x_number_of_successful_allocations;
        size_t x_number_of_successful_frees;
    };

    static constexpr std::size_t port_byte_alignment = 8;

    // static constexpr auto
    // aligment_mask(auto aligment)
    // {
    //     if constexpr (aligment == 32) {
    //         return 0x001f;
    //     } else if constexpr (aligment == 16) {
    //         return 0x000f;
    //     } else if constexpr (aligment == 8) {
    //         return 0x0007;
    //     } else if constexpr (aligment == 4) {
    //         return 0x0003;
    //     } else if constexpr (aligment == 2) {
    //         return 0x0001;
    //     } else if constexpr (aligment == 1) {
    //         return 0x0000;
    //     } else {
    //         static_assert("Invalid portBYTE_ALIGNMENT definition");
    //     }
    // }

    static constexpr std::size_t port_byte_alignment_mask = 0x0007;

    constexpr void
    mt_coverage_test_marker()
    {}

    constexpr void
    v_task_suspend_all()
    {}

    constexpr void
    x_task_resume_all()
    {}

    constexpr void config_assert(auto) {}

    /* Canary value for protecting internal heap pointers. */
    static constexpr std::size_t x_heap_canary{0x655556UL};

    static constexpr std::size_t port_max_delay = 0xffffffffUL;

    /* The size of the structure placed at the beginning of each allocated memory
     * block must by correctly byte aligned. */
    static constexpr std::size_t x_heap_struct_size
        = (sizeof(block_link_t) + ((std::size_t)(port_byte_alignment - 1))) & ~((std::size_t)port_byte_alignment_mask);

    /*-----------------------------------------------------------*/

    /* Block sizes must not get too small. */
    static constexpr std::size_t heap_minimum_block_size = ((std::size_t)(x_heap_struct_size << 1));

    /* Assumes 8bit bytes! */
    static constexpr std::size_t heap_bits_per_byte = ((std::size_t)8);

    /* Max value that fits in a size_t type. */
    static constexpr std::size_t heap_size_max = (~((std::size_t)0));

    /* Check if multiplying a and b will result in overflow. */
    constexpr auto
    heap_multiply_will_overflow(auto a, auto b)
    {
        return (((a) > 0) && ((b) > (heap_size_max / (a))));
    }

    /* Check if adding a and b will result in overflow. */
    constexpr auto
    heap_add_will_overflow(auto a, auto b)
    {
        return ((a) > (heap_size_max - (b)));
    }

    /* Check if the subtraction operation ( a - b ) will result in underflow. */
    constexpr auto
    heap_subtract_will_underflow(auto a, auto b)
    {
        return ((a) < (b));
    }

    static constexpr std::size_t heap_block_allocated_bitmask
        = (((std::size_t)1) << ((sizeof(std::size_t) * heap_bits_per_byte) - 1));

    constexpr auto
    heap_block_size_is_valid(auto x_block_size)
    {
        return (((x_block_size)&heap_block_allocated_bitmask) == 0);
    }
    constexpr auto
    heap_block_is_allocated(auto px_block)
    {
        return (((px_block->x_block_size) & heap_block_allocated_bitmask) != 0);
    }
    constexpr auto
    heap_allocate_block(auto px_block)
    {
        return ((px_block->x_block_size) |= heap_block_allocated_bitmask);
    }
    constexpr auto
    heap_free_block(auto px_block)
    {
        return ((px_block->x_block_size) &= ~heap_block_allocated_bitmask);
    }

    constexpr auto
    heap_protect_block_pointer(auto px_block)
    {
        return (block_link_t*)(((std::size_t)(px_block)) ^ x_heap_canary);
    }

    constexpr void
    heap_validate_block_pointer(auto px_block)
    {
        config_assert(((std::uint8_t*)(px_block) >= &(uc_heap_[0]))
                      && ((std::uint8_t*)(px_block) <= &(uc_heap_[config_total_heap_size - 1])));
    }

public:
    void
    on_fail(callback_t callback)
    {
        callback_ = callback;
    }

    void*
    allocate(size_t x_wanted_size)
    {
        block_link_t* px_block;
        block_link_t* px_previous_block;
        block_link_t* px_new_block_link;
        void*         pv_return = nullptr;
        std::size_t   x_additional_required_size;
        std::size_t   x_allocated_block_size = 0;

        if (x_wanted_size > 0) {
            /* The wanted size must be increased so it can contain a block_link_t
             * structure in addition to the requested amount of bytes. */
            if (heap_add_will_overflow(x_wanted_size, x_heap_struct_size) == 0) {
                x_wanted_size += x_heap_struct_size;

                /* Ensure that blocks are always aligned to the required number
                 * of bytes. */
                if ((x_wanted_size & port_byte_alignment_mask) != 0x00) {
                    /* Byte alignment required. */
                    x_additional_required_size = port_byte_alignment - (x_wanted_size & port_byte_alignment_mask);

                    if (heap_add_will_overflow(x_wanted_size, x_additional_required_size) == 0) {
                        x_wanted_size += x_additional_required_size;
                    } else {
                        x_wanted_size = 0;
                    }
                } else {
                    mt_coverage_test_marker();
                }
            } else {
                x_wanted_size = 0;
            }
        } else {
            mt_coverage_test_marker();
        }

        v_task_suspend_all();
        {
            /* Check the block size we are trying to allocate is not so large that the
             * top bit is set.  The top bit of the block size member of the block_link_t
             * structure is used to determine who owns the block - the application or
             * the kernel, so it must be free. */
            if (heap_block_size_is_valid(x_wanted_size) != 0) {
                if ((x_wanted_size > 0) && (x_wanted_size <= x_free_bytes_remaining_)) {
                    /* Traverse the list from the start (lowest address) block until
                     * one of adequate size is found. */
                    px_previous_block = &x_start_;
                    px_block          = heap_protect_block_pointer(x_start_.px_next_free_block);
                    heap_validate_block_pointer(px_block);

                    while ((px_block->x_block_size < x_wanted_size)
                           && (px_block->px_next_free_block != heap_protect_block_pointer(NULL))) {
                        px_previous_block = px_block;
                        px_block          = heap_protect_block_pointer(px_block->px_next_free_block);
                        heap_validate_block_pointer(px_block);
                    }

                    /* If the end marker was reached then a block of adequate size
                     * was not found. */
                    if (px_block != px_end_) {
                        /* Return the memory space pointed to - jumping over the
                         * block_link_t structure at its start. */
                        pv_return
                            = (void*)(((uint8_t*)heap_protect_block_pointer(px_previous_block->px_next_free_block))
                                      + x_heap_struct_size);
                        heap_validate_block_pointer(pv_return);

                        /* This block is being returned for use so must be taken out
                         * of the list of free blocks. */
                        px_previous_block->px_next_free_block = px_block->px_next_free_block;

                        /* If the block is larger than required it can be split into
                         * two. */
                        config_assert(heap_subtract_will_underflow(px_block->x_block_size, x_wanted_size) == 0);

                        if ((px_block->x_block_size - x_wanted_size) > heap_minimum_block_size) {
                            /* This block is to be split into two.  Create a new
                             * block following the number of bytes requested. The void
                             * cast is used to prevent byte alignment warnings from the
                             * compiler. */
                            px_new_block_link
                                = reinterpret_cast<block_link_t*>(((std::uint8_t*)px_block) + x_wanted_size);
                            config_assert((((size_t)px_new_block_link) & port_byte_alignment_mask) == 0);

                            /* Calculate the sizes of two blocks split from the
                             * single block. */
                            px_new_block_link->x_block_size = px_block->x_block_size - x_wanted_size;
                            px_block->x_block_size          = x_wanted_size;

                            /* Insert the new block into the list of free blocks. */
                            px_new_block_link->px_next_free_block = px_previous_block->px_next_free_block;
                            px_previous_block->px_next_free_block = heap_protect_block_pointer(px_new_block_link);
                        } else {
                            mt_coverage_test_marker();
                        }

                        x_free_bytes_remaining_ -= px_block->x_block_size;

                        if (x_free_bytes_remaining_ < x_minimum_ever_free_bytes_remaining_) {
                            x_minimum_ever_free_bytes_remaining_ = x_free_bytes_remaining_;
                        } else {
                            mt_coverage_test_marker();
                        }

                        x_allocated_block_size = px_block->x_block_size;

                        /* The block is being returned - it is allocated and owned
                         * by the application and has no "next" block. */
                        heap_allocate_block(px_block);
                        px_block->px_next_free_block = heap_protect_block_pointer(NULL);
                        x_number_of_successful_allocations_++;
                    } else {
                        mt_coverage_test_marker();
                    }
                } else {
                    mt_coverage_test_marker();
                }
            } else {
                mt_coverage_test_marker();
            }

            /* Prevent compiler warnings when trace macros are not used. */
            (void)x_allocated_block_size;
        }
        x_task_resume_all();

        if (pv_return == nullptr && callback_ != nullptr) {
            callback_();
        } else {
            mt_coverage_test_marker();
        }

        config_assert((((size_t)pv_return) & (size_t)port_byte_alignment_mask) == 0);
        return pv_return;
    }
    /*-----------------------------------------------------------*/

    void
    deallocate(void* pv)
    {
        auto*         puc = (std::uint8_t*)pv;
        block_link_t* px_link;

        if (pv != nullptr) {
            /* The memory being freed will have an block_link_t structure immediately
             * before it. */
            puc -= x_heap_struct_size;

            /* This casting is to keep the compiler from issuing warnings. */
            px_link = reinterpret_cast<block_link_t*>(puc);

            heap_validate_block_pointer(px_link);
            config_assert(heap_block_is_allocated(px_link) != 0);
            config_assert(px_link->px_next_free_block == heap_protect_block_pointer(NULL));

            if (heap_block_is_allocated(px_link) != 0) {
                if (px_link->px_next_free_block == heap_protect_block_pointer(NULL)) {
                    /* The block is being returned to the heap - it is no longer
                     * allocated. */
                    heap_free_block(px_link);

                    v_task_suspend_all();
                    {
                        /* Add this block to the list of free blocks. */
                        x_free_bytes_remaining_ += px_link->x_block_size;
                        prv_insert_block_into_free_list(((block_link_t*)px_link));
                        x_number_of_successful_frees_++;
                    }
                    x_task_resume_all();
                } else {
                    mt_coverage_test_marker();
                }
            } else {
                mt_coverage_test_marker();
            }
        }
    }
    /*-----------------------------------------------------------*/

    std::size_t
    free_heap_size() const
    {
        return x_free_bytes_remaining_;
    }
    /*-----------------------------------------------------------*/

    std::size_t
    minimum_ever_free_heap_size() const
    {
        return x_minimum_ever_free_bytes_remaining_;
    }
    /*-----------------------------------------------------------*/

    void
    reset_minimum_ever_free_heap_size()
    {
        x_minimum_ever_free_bytes_remaining_ = x_free_bytes_remaining_;
    }
    /*-----------------------------------------------------------*/

    void*
    callocate(std::size_t x_num, std::size_t x_size)
    {
        void* pv = nullptr;

        if (heap_multiply_will_overflow(x_num, x_size) == 0) {
            pv = allocate(x_num * x_size);

            if (pv != nullptr) {
                memset(pv, 0, x_num * x_size);
            }
        }

        return pv;
    }
    /*-----------------------------------------------------------*/

    static_heap()
    {
        block_link_t* px_first_free_block;
        std::size_t   ux_start_address, ux_end_address;
        std::size_t   x_total_heap_size = config_total_heap_size;

        /* Ensure the heap starts on a correctly aligned boundary. */
        ux_start_address = (std::size_t)uc_heap_;

        if ((ux_start_address & port_byte_alignment_mask) != 0) {
            ux_start_address += (port_byte_alignment - 1);
            ux_start_address &= ~((std::size_t)port_byte_alignment_mask);
            x_total_heap_size -= (size_t)(ux_start_address - (std::size_t)uc_heap_);
        }

        /* xStart is used to hold a pointer to the first item in the list of free
         * blocks.  The void cast is used to prevent compiler warnings. */
        x_start_.px_next_free_block = (block_link_t*)heap_protect_block_pointer(ux_start_address);
        x_start_.x_block_size       = (size_t)0;

        /* pxEnd is used to mark the end of the list of free blocks and is inserted
         * at the end of the heap space. */
        ux_end_address = ux_start_address + (std::size_t)x_total_heap_size;
        ux_end_address -= (std::size_t)x_heap_struct_size;
        ux_end_address &= ~((std::size_t)port_byte_alignment_mask);
        px_end_                     = (block_link_t*)ux_end_address;
        px_end_->x_block_size       = 0;
        px_end_->px_next_free_block = heap_protect_block_pointer(NULL);

        /* To start with there is a single free block that is sized to take up the
         * entire heap space, minus the space taken by pxEnd. */
        px_first_free_block                     = (block_link_t*)ux_start_address;
        px_first_free_block->x_block_size       = (std::size_t)(ux_end_address - (std::size_t)px_first_free_block);
        px_first_free_block->px_next_free_block = heap_protect_block_pointer(px_end_);

        /* Only one block exists - and it covers the entire usable heap space. */
        x_minimum_ever_free_bytes_remaining_ = px_first_free_block->x_block_size;
        x_free_bytes_remaining_              = px_first_free_block->x_block_size;
    }
    /*-----------------------------------------------------------*/

    void
    prv_insert_block_into_free_list(block_link_t* px_block_to_insert)
    {
        block_link_t* px_iterator;
        uint8_t*      puc;

        /* Iterate through the list until a block is found that has a higher address
         * than the block being inserted. */
        for (px_iterator = &x_start_; heap_protect_block_pointer(px_iterator->px_next_free_block) < px_block_to_insert;
             px_iterator = heap_protect_block_pointer(px_iterator->px_next_free_block)) {
            /* Nothing to do here, just iterate to the right position. */
        }

        if (px_iterator != &x_start_) {
            heap_validate_block_pointer(px_iterator);
        }

        /* Do the block being inserted, and the block it is being inserted after
         * make a contiguous block of memory? */
        puc = (uint8_t*)px_iterator;

        if ((puc + px_iterator->x_block_size) == (uint8_t*)px_block_to_insert) {
            px_iterator->x_block_size += px_block_to_insert->x_block_size;
            px_block_to_insert = px_iterator;
        } else {
            mt_coverage_test_marker();
        }

        /* Do the block being inserted, and the block it is being inserted before
         * make a contiguous block of memory? */
        puc = (uint8_t*)px_block_to_insert;

        if ((puc + px_block_to_insert->x_block_size)
            == (uint8_t*)heap_protect_block_pointer(px_iterator->px_next_free_block)) {
            if (heap_protect_block_pointer(px_iterator->px_next_free_block) != px_end_) {
                /* Form one big block from the two blocks. */
                px_block_to_insert->x_block_size
                    += heap_protect_block_pointer(px_iterator->px_next_free_block)->x_block_size;
                px_block_to_insert->px_next_free_block
                    = heap_protect_block_pointer(px_iterator->px_next_free_block)->px_next_free_block;
            } else {
                px_block_to_insert->px_next_free_block = heap_protect_block_pointer(px_end_);
            }
        } else {
            px_block_to_insert->px_next_free_block = px_iterator->px_next_free_block;
        }

        /* If the block being inserted plugged a gap, so was merged with the block
         * before and the block after, then it's px_next_free_block pointer will have
         * already been set, and should not be set here as that would make it point
         * to itself. */
        if (px_iterator != px_block_to_insert) {
            px_iterator->px_next_free_block = heap_protect_block_pointer(px_block_to_insert);
        } else {
            mt_coverage_test_marker();
        }
    }
    /*-----------------------------------------------------------*/

    void
    v_port_get_heap_stats(heap_stats_t* px_heap_stats)
    {
        block_link_t* px_block;
        std::size_t   x_blocks = 0, x_max_size = 0, x_min_size = port_max_delay;

        v_task_suspend_all();
        {
            px_block = heapPROTECT_BLOCK_POINTER(x_start_.px_next_free_block);

            /* pxBlock will be NULL if the heap has not been initialised.  The heap
             * is initialised automatically when the first allocation is made. */
            if (px_block != NULL) {
                while (px_block != px_end_) {
                    /* Increment the number of blocks and record the largest block seen
                     * so far. */
                    x_blocks++;

                    if (px_block->x_block_size > x_max_size) {
                        x_max_size = px_block->x_block_size;
                    }

                    if (px_block->x_block_size < x_min_size) {
                        x_min_size = px_block->x_block_size;
                    }

                    /* Move to the next block in the chain until the last block is
                     * reached. */
                    px_block = heapPROTECT_BLOCK_POINTER(px_block->px_next_free_block);
                }
            }
        }
        x_task_resume_all();

        px_heap_stats->x_size_of_largest_free_block_in_bytes  = x_max_size;
        px_heap_stats->x_size_of_smallest_free_block_in_bytes = x_min_size;
        px_heap_stats->x_number_of_free_blocks                = x_blocks;

        // taskENTER_CRITICAL();
        {
            px_heap_stats->x_available_heap_space_in_bytes     = x_free_bytes_remaining_;
            px_heap_stats->x_number_of_successful_allocations  = x_number_of_successful_allocations_;
            px_heap_stats->x_number_of_successful_frees        = x_number_of_successful_frees_;
            px_heap_stats->x_minimum_ever_free_bytes_remaining = x_minimum_ever_free_bytes_remaining_;
        }
        // taskEXIT_CRITICAL();
    }
    /*-----------------------------------------------------------*/

private:
    callback_t   callback_{nullptr};
    std::uint8_t uc_heap_[config_total_heap_size]{};

    /* Create a couple of list links to mark the start and end of the list. */
    block_link_t  x_start_{};
    block_link_t* px_end_{};

    /* Keeps track of the number of calls to allocate and free memory as well as the
     * number of free bytes remaining, but says nothing about fragmentation. */
    std::size_t x_free_bytes_remaining_{};
    std::size_t x_minimum_ever_free_bytes_remaining_{};
    std::size_t x_number_of_successful_allocations_{};
    std::size_t x_number_of_successful_frees_{};
};
}    // namespace xitren::allocators
