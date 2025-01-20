/*!
_ _
__ _(_) |_ _ _ ___ _ _
\ \ / |  _| '_/ -_) ' \
/_\_\_|\__|_| \___|_||_|
* @date 03.03.2024
*/
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>

#define portBYTE_ALIGNMENT 8

#if portBYTE_ALIGNMENT == 32
#    define portBYTE_ALIGNMENT_MASK (0x001f)
#elif portBYTE_ALIGNMENT == 16
#    define portBYTE_ALIGNMENT_MASK (0x000f)
#elif portBYTE_ALIGNMENT == 8
#    define portBYTE_ALIGNMENT_MASK (0x0007)
#elif portBYTE_ALIGNMENT == 4
#    define portBYTE_ALIGNMENT_MASK (0x0003)
#elif portBYTE_ALIGNMENT == 2
#    define portBYTE_ALIGNMENT_MASK (0x0001)
#elif portBYTE_ALIGNMENT == 1
#    define portBYTE_ALIGNMENT_MASK (0x0000)
#else  /* if portBYTE_ALIGNMENT == 32 */
#    error "Invalid portBYTE_ALIGNMENT definition"
#endif /* if portBYTE_ALIGNMENT == 32 */

#ifndef mtCOVERAGE_TEST_MARKER
#    define mtCOVERAGE_TEST_MARKER()
#endif

#ifndef vTaskSuspendAll
#    define vTaskSuspendAll()
#endif

#ifndef xTaskResumeAll
#    define xTaskResumeAll()
#endif

#ifndef configASSERT
#    define configASSERT(N)
#endif
/* Canary value for protecting internal heap pointers. */
static std::size_t xHeapCanary{0x655556UL};

#define portMAX_DELAY 0xffffffffUL

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE ((size_t)(xHeapStructSize << 1))

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE ((size_t)8)

/* Max value that fits in a size_t type. */
#define heapSIZE_MAX (~((size_t)0))

/* Check if multiplying a and b will result in overflow. */
#define heapMULTIPLY_WILL_OVERFLOW(a, b) (((a) > 0) && ((b) > (heapSIZE_MAX / (a))))

/* Check if adding a and b will result in overflow. */
#define heapADD_WILL_OVERFLOW(a, b) ((a) > (heapSIZE_MAX - (b)))

/* Check if the subtraction operation ( a - b ) will result in underflow. */
#define heapSUBTRACT_WILL_UNDERFLOW(a, b) ((a) < (b))

/* MSB of the xBlockSize member of an block_link_t structure is used to track
 * the allocation status of a block.  When MSB of the xBlockSize member of
 * an block_link_t structure is set then the block belongs to the application.
 * When the bit is free the block is still part of the free heap space. */
#define heapBLOCK_ALLOCATED_BITMASK (((size_t)1) << ((sizeof(size_t) * heapBITS_PER_BYTE) - 1))
#define heapBLOCK_SIZE_IS_VALID(xBlockSize) (((xBlockSize)&heapBLOCK_ALLOCATED_BITMASK) == 0)
#define heapBLOCK_IS_ALLOCATED(pxBlock) (((pxBlock->xBlockSize) & heapBLOCK_ALLOCATED_BITMASK) != 0)
#define heapALLOCATE_BLOCK(pxBlock) ((pxBlock->xBlockSize) |= heapBLOCK_ALLOCATED_BITMASK)
#define heapFREE_BLOCK(pxBlock) ((pxBlock->xBlockSize) &= ~heapBLOCK_ALLOCATED_BITMASK)

/* Macro to load/store block_link_t pointers to memory. By XORing the
 * pointers with a random canary value, heap overflows will result
 * in randomly unpredictable pointer values which will be caught by
 * heapVALIDATE_BLOCK_POINTER assert. */
#define heapPROTECT_BLOCK_POINTER(pxBlock) ((block_link_t*)(((std::size_t)(pxBlock)) ^ xHeapCanary))

/* Assert that a heap block pointer is within the heap bounds. */
#define heapVALIDATE_BLOCK_POINTER(pxBlock)                 \
    configASSERT(((std::uint8_t*)(pxBlock) >= &(ucHeap[0])) \
                 && ((std::uint8_t*)(pxBlock) <= &(ucHeap[configTOTAL_HEAP_SIZE - 1])))

/*-----------------------------------------------------------*/

namespace xitren::allocators {

template <std::size_t Size>
class static_heap {
    static const std::size_t config_total_heap_size = Size;

    /* Define the linked list structure.  This is used to link free blocks in order
     * of their memory address. */
    using block_link_t = struct a_block_link {
        struct a_block_link* px_next_free_block; /**< The next free block in the list. */
        size_t               xBlockSize;         /**< The size of the free block. */
    };

    /* Used to pass information about the heap out of vPortGetHeapStats(). */
    using heap_stats_t = struct {
        size_t x_available_heap_space_in_bytes; /* The total heap size currently available - this is the sum of all the
                                              free blocks, not the largest block that can be allocated. */
        size_t x_size_of_largest_free_block_in_bytes;  /* The maximum size, in bytes, of all the free blocks within the
                                                   heap at  the time vPortGetHeapStats() is called. */
        size_t x_size_of_smallest_free_block_in_bytes; /* The minimum size, in bytes, of all the free blocks within the
                                                   heap at the time vPortGetHeapStats() is called. */
        size_t x_number_of_free_blocks;                /* The number of free memory blocks within the heap at the time
                                                      vPortGetHeapStats()                is called. */
        size_t x_minimum_ever_free_bytes_remaining; /* The minimum amount of total free memory (sum of all free blocks)
                                                  there has been in the heap since the system booted. */
        size_t x_number_of_successful_allocations;  /* The number of calls to pvPortMalloc() that have returned a valid
                                                   memory block. */
        size_t x_number_of_successful_frees; /* The number of calls to vPortFree() that has successfully freed a block
                                            of memory. */
    };

    // constexpr block_link_t*
    // heapPROTECT_BLOCK_POINTER(auto pxBlock)
    // {
    //     return (block_link_t*)(((std::size_t)(pxBlock)) ^ xHeapCanary);
    // }

    // constexpr void
    // heapVALIDATE_BLOCK_POINTER(auto pxBlock)
    // {
    //     configASSERT(((std::uint8_t*)(pxBlock) >= &(ucHeap[0]))
    //                  && ((std::uint8_t*)(pxBlock) <= &(ucHeap[configTOTAL_HEAP_SIZE - 1])));
    // }

    /* The size of the structure placed at the beginning of each allocated memory
     * block must by correctly byte aligned. */
    static const size_t xHeapStructSize
        = (sizeof(block_link_t) + ((size_t)(portBYTE_ALIGNMENT - 1))) & ~((size_t)portBYTE_ALIGNMENT_MASK);

    /*-----------------------------------------------------------*/
public:
    void*
    allocate(size_t xWantedSize)
    {
        block_link_t* pxBlock;
        block_link_t* pxPreviousBlock;
        block_link_t* pxNewBlockLink;
        void*         pvReturn = NULL;
        size_t        xAdditionalRequiredSize;
        size_t        xAllocatedBlockSize = 0;

        if (xWantedSize > 0) {
            /* The wanted size must be increased so it can contain a block_link_t
             * structure in addition to the requested amount of bytes. */
            if (heapADD_WILL_OVERFLOW(xWantedSize, xHeapStructSize) == 0) {
                xWantedSize += xHeapStructSize;

                /* Ensure that blocks are always aligned to the required number
                 * of bytes. */
                if ((xWantedSize & portBYTE_ALIGNMENT_MASK) != 0x00) {
                    /* Byte alignment required. */
                    xAdditionalRequiredSize = portBYTE_ALIGNMENT - (xWantedSize & portBYTE_ALIGNMENT_MASK);

                    if (heapADD_WILL_OVERFLOW(xWantedSize, xAdditionalRequiredSize) == 0) {
                        xWantedSize += xAdditionalRequiredSize;
                    } else {
                        xWantedSize = 0;
                    }
                } else {
                    mtCOVERAGE_TEST_MARKER();
                }
            } else {
                xWantedSize = 0;
            }
        } else {
            mtCOVERAGE_TEST_MARKER();
        }

        vTaskSuspendAll();
        {
            /* Check the block size we are trying to allocate is not so large that the
             * top bit is set.  The top bit of the block size member of the block_link_t
             * structure is used to determine who owns the block - the application or
             * the kernel, so it must be free. */
            if (heapBLOCK_SIZE_IS_VALID(xWantedSize) != 0) {
                if ((xWantedSize > 0) && (xWantedSize <= x_free_bytes_remaining_)) {
                    /* Traverse the list from the start (lowest address) block until
                     * one of adequate size is found. */
                    pxPreviousBlock = &x_start_;
                    pxBlock         = heapPROTECT_BLOCK_POINTER(x_start_.px_next_free_block);
                    heapVALIDATE_BLOCK_POINTER(pxBlock);

                    while ((pxBlock->xBlockSize < xWantedSize)
                           && (pxBlock->px_next_free_block != heapPROTECT_BLOCK_POINTER(NULL))) {
                        pxPreviousBlock = pxBlock;
                        pxBlock         = heapPROTECT_BLOCK_POINTER(pxBlock->px_next_free_block);
                        heapVALIDATE_BLOCK_POINTER(pxBlock);
                    }

                    /* If the end marker was reached then a block of adequate size
                     * was not found. */
                    if (pxBlock != px_end_) {
                        /* Return the memory space pointed to - jumping over the
                         * block_link_t structure at its start. */
                        pvReturn = (void*)(((uint8_t*)heapPROTECT_BLOCK_POINTER(pxPreviousBlock->px_next_free_block))
                                           + xHeapStructSize);
                        heapVALIDATE_BLOCK_POINTER(pvReturn);

                        /* This block is being returned for use so must be taken out
                         * of the list of free blocks. */
                        pxPreviousBlock->px_next_free_block = pxBlock->px_next_free_block;

                        /* If the block is larger than required it can be split into
                         * two. */
                        configASSERT(heapSUBTRACT_WILL_UNDERFLOW(pxBlock->xBlockSize, xWantedSize) == 0);

                        if ((pxBlock->xBlockSize - xWantedSize) > heapMINIMUM_BLOCK_SIZE) {
                            /* This block is to be split into two.  Create a new
                             * block following the number of bytes requested. The void
                             * cast is used to prevent byte alignment warnings from the
                             * compiler. */
                            pxNewBlockLink = reinterpret_cast<block_link_t*>(((uint8_t*)pxBlock) + xWantedSize);
                            configASSERT((((size_t)pxNewBlockLink) & portBYTE_ALIGNMENT_MASK) == 0);

                            /* Calculate the sizes of two blocks split from the
                             * single block. */
                            pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                            pxBlock->xBlockSize        = xWantedSize;

                            /* Insert the new block into the list of free blocks. */
                            pxNewBlockLink->px_next_free_block  = pxPreviousBlock->px_next_free_block;
                            pxPreviousBlock->px_next_free_block = heapPROTECT_BLOCK_POINTER(pxNewBlockLink);
                        } else {
                            mtCOVERAGE_TEST_MARKER();
                        }

                        x_free_bytes_remaining_ -= pxBlock->xBlockSize;

                        if (x_free_bytes_remaining_ < x_minimum_ever_free_bytes_remaining_) {
                            x_minimum_ever_free_bytes_remaining_ = x_free_bytes_remaining_;
                        } else {
                            mtCOVERAGE_TEST_MARKER();
                        }

                        xAllocatedBlockSize = pxBlock->xBlockSize;

                        /* The block is being returned - it is allocated and owned
                         * by the application and has no "next" block. */
                        heapALLOCATE_BLOCK(pxBlock);
                        pxBlock->px_next_free_block = heapPROTECT_BLOCK_POINTER(NULL);
                        x_number_of_successful_allocations_++;
                    } else {
                        mtCOVERAGE_TEST_MARKER();
                    }
                } else {
                    mtCOVERAGE_TEST_MARKER();
                }
            } else {
                mtCOVERAGE_TEST_MARKER();
            }

            // traceMALLOC(pvReturn, xAllocatedBlockSize);

            /* Prevent compiler warnings when trace macros are not used. */
            (void)xAllocatedBlockSize;
        }
        xTaskResumeAll();

#if (configUSE_MALLOC_FAILED_HOOK == 1)
        {
            if (pvReturn == NULL) {
                vApplicationMallocFailedHook();
            } else {
                mtCOVERAGE_TEST_MARKER();
            }
        }
#endif /* if ( configUSE_MALLOC_FAILED_HOOK == 1 ) */

        configASSERT((((size_t)pvReturn) & (size_t)portBYTE_ALIGNMENT_MASK) == 0);
        return pvReturn;
    }
    /*-----------------------------------------------------------*/

    void
    deallocate(void* pv)
    {
        uint8_t*      puc = (uint8_t*)pv;
        block_link_t* pxLink;

        if (pv != NULL) {
            /* The memory being freed will have an block_link_t structure immediately
             * before it. */
            puc -= xHeapStructSize;

            /* This casting is to keep the compiler from issuing warnings. */
            pxLink = reinterpret_cast<block_link_t*>(puc);

            heapVALIDATE_BLOCK_POINTER(pxLink);
            configASSERT(heapBLOCK_IS_ALLOCATED(pxLink) != 0);
            configASSERT(pxLink->px_next_free_block == heapPROTECT_BLOCK_POINTER(NULL));

            if (heapBLOCK_IS_ALLOCATED(pxLink) != 0) {
                if (pxLink->px_next_free_block == heapPROTECT_BLOCK_POINTER(NULL)) {
                    /* The block is being returned to the heap - it is no longer
                     * allocated. */
                    heapFREE_BLOCK(pxLink);
#if (configHEAP_CLEAR_MEMORY_ON_FREE == 1)
                    {
                        /* Check for underflow as this can occur if xBlockSize is
                         * overwritten in a heap block. */
                        if (heapSUBTRACT_WILL_UNDERFLOW(pxLink->xBlockSize, xHeapStructSize) == 0) {
                            (void)memset(puc + xHeapStructSize, 0, pxLink->xBlockSize - xHeapStructSize);
                        }
                    }
#endif

                    vTaskSuspendAll();
                    {
                        /* Add this block to the list of free blocks. */
                        x_free_bytes_remaining_ += pxLink->xBlockSize;
                        // traceFREE(pv, pxLink->xBlockSize);
                        prvInsertBlockIntoFreeList(((block_link_t*)pxLink));
                        x_number_of_successful_frees_++;
                    }
                    xTaskResumeAll();
                } else {
                    mtCOVERAGE_TEST_MARKER();
                }
            } else {
                mtCOVERAGE_TEST_MARKER();
            }
        }
    }
    /*-----------------------------------------------------------*/

    size_t
    free_heap_size() const
    {
        return x_free_bytes_remaining_;
    }
    /*-----------------------------------------------------------*/

    size_t
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
    callocate(size_t xNum, size_t xSize)
    {
        void* pv = nullptr;

        if (heapMULTIPLY_WILL_OVERFLOW(xNum, xSize) == 0) {
            pv = allocate(xNum * xSize);

            if (pv != nullptr) {
                memset(pv, 0, xNum * xSize);
            }
        }

        return pv;
    }
    /*-----------------------------------------------------------*/

    static_heap()
    {
        block_link_t* pxFirstFreeBlock;
        std::size_t   uxStartAddress, uxEndAddress;
        size_t        xTotalHeapSize = config_total_heap_size;

        /* Ensure the heap starts on a correctly aligned boundary. */
        uxStartAddress = (std::size_t)uc_heap_;

        if ((uxStartAddress & portBYTE_ALIGNMENT_MASK) != 0) {
            uxStartAddress += (portBYTE_ALIGNMENT - 1);
            uxStartAddress &= ~((std::size_t)portBYTE_ALIGNMENT_MASK);
            xTotalHeapSize -= (size_t)(uxStartAddress - (std::size_t)uc_heap_);
        }

        /* xStart is used to hold a pointer to the first item in the list of free
         * blocks.  The void cast is used to prevent compiler warnings. */
        x_start_.px_next_free_block = (block_link_t*)heapPROTECT_BLOCK_POINTER(uxStartAddress);
        x_start_.xBlockSize         = (size_t)0;

        /* pxEnd is used to mark the end of the list of free blocks and is inserted
         * at the end of the heap space. */
        uxEndAddress = uxStartAddress + (std::size_t)xTotalHeapSize;
        uxEndAddress -= (std::size_t)xHeapStructSize;
        uxEndAddress &= ~((std::size_t)portBYTE_ALIGNMENT_MASK);
        px_end_                     = (block_link_t*)uxEndAddress;
        px_end_->xBlockSize         = 0;
        px_end_->px_next_free_block = heapPROTECT_BLOCK_POINTER(NULL);

        /* To start with there is a single free block that is sized to take up the
         * entire heap space, minus the space taken by pxEnd. */
        pxFirstFreeBlock                     = (block_link_t*)uxStartAddress;
        pxFirstFreeBlock->xBlockSize         = (size_t)(uxEndAddress - (std::size_t)pxFirstFreeBlock);
        pxFirstFreeBlock->px_next_free_block = heapPROTECT_BLOCK_POINTER(px_end_);

        /* Only one block exists - and it covers the entire usable heap space. */
        x_minimum_ever_free_bytes_remaining_ = pxFirstFreeBlock->xBlockSize;
        x_free_bytes_remaining_              = pxFirstFreeBlock->xBlockSize;
    }
    /*-----------------------------------------------------------*/

    void
    prvInsertBlockIntoFreeList(block_link_t* pxBlockToInsert) /* PRIVILEGED_FUNCTION */
    {
        block_link_t* pxIterator;
        uint8_t*      puc;

        /* Iterate through the list until a block is found that has a higher address
         * than the block being inserted. */
        for (pxIterator = &x_start_; heapPROTECT_BLOCK_POINTER(pxIterator->px_next_free_block) < pxBlockToInsert;
             pxIterator = heapPROTECT_BLOCK_POINTER(pxIterator->px_next_free_block)) {
            /* Nothing to do here, just iterate to the right position. */
        }

        if (pxIterator != &x_start_) {
            heapVALIDATE_BLOCK_POINTER(pxIterator);
        }

        /* Do the block being inserted, and the block it is being inserted after
         * make a contiguous block of memory? */
        puc = (uint8_t*)pxIterator;

        if ((puc + pxIterator->xBlockSize) == (uint8_t*)pxBlockToInsert) {
            pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
            pxBlockToInsert = pxIterator;
        } else {
            mtCOVERAGE_TEST_MARKER();
        }

        /* Do the block being inserted, and the block it is being inserted before
         * make a contiguous block of memory? */
        puc = (uint8_t*)pxBlockToInsert;

        if ((puc + pxBlockToInsert->xBlockSize)
            == (uint8_t*)heapPROTECT_BLOCK_POINTER(pxIterator->px_next_free_block)) {
            if (heapPROTECT_BLOCK_POINTER(pxIterator->px_next_free_block) != px_end_) {
                /* Form one big block from the two blocks. */
                pxBlockToInsert->xBlockSize += heapPROTECT_BLOCK_POINTER(pxIterator->px_next_free_block)->xBlockSize;
                pxBlockToInsert->px_next_free_block
                    = heapPROTECT_BLOCK_POINTER(pxIterator->px_next_free_block)->px_next_free_block;
            } else {
                pxBlockToInsert->px_next_free_block = heapPROTECT_BLOCK_POINTER(px_end_);
            }
        } else {
            pxBlockToInsert->px_next_free_block = pxIterator->px_next_free_block;
        }

        /* If the block being inserted plugged a gap, so was merged with the block
         * before and the block after, then it's px_next_free_block pointer will have
         * already been set, and should not be set here as that would make it point
         * to itself. */
        if (pxIterator != pxBlockToInsert) {
            pxIterator->px_next_free_block = heapPROTECT_BLOCK_POINTER(pxBlockToInsert);
        } else {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    /*-----------------------------------------------------------*/

    void
    vPortGetHeapStats(heap_stats_t* pxHeapStats)
    {
        block_link_t* pxBlock;
        size_t        xBlocks = 0, xMaxSize = 0,
               xMinSize = portMAX_DELAY; /* portMAX_DELAY used as a portable way of getting the maximum value. */

        vTaskSuspendAll();
        {
            pxBlock = heapPROTECT_BLOCK_POINTER(x_start_.px_next_free_block);

            /* pxBlock will be NULL if the heap has not been initialised.  The heap
             * is initialised automatically when the first allocation is made. */
            if (pxBlock != NULL) {
                while (pxBlock != px_end_) {
                    /* Increment the number of blocks and record the largest block seen
                     * so far. */
                    xBlocks++;

                    if (pxBlock->xBlockSize > xMaxSize) {
                        xMaxSize = pxBlock->xBlockSize;
                    }

                    if (pxBlock->xBlockSize < xMinSize) {
                        xMinSize = pxBlock->xBlockSize;
                    }

                    /* Move to the next block in the chain until the last block is
                     * reached. */
                    pxBlock = heapPROTECT_BLOCK_POINTER(pxBlock->px_next_free_block);
                }
            }
        }
        xTaskResumeAll();

        pxHeapStats->x_size_of_largest_free_block_in_bytes  = xMaxSize;
        pxHeapStats->x_size_of_smallest_free_block_in_bytes = xMinSize;
        pxHeapStats->x_number_of_free_blocks                = xBlocks;

        // taskENTER_CRITICAL();
        {
            pxHeapStats->x_available_heap_space_in_bytes     = x_free_bytes_remaining_;
            pxHeapStats->x_number_of_successful_allocations  = x_number_of_successful_allocations_;
            pxHeapStats->x_number_of_successful_frees        = x_number_of_successful_frees_;
            pxHeapStats->x_minimum_ever_free_bytes_remaining = x_minimum_ever_free_bytes_remaining_;
        }
        // taskEXIT_CRITICAL();
    }
    /*-----------------------------------------------------------*/

private:
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
