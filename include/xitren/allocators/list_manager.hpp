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
#include <functional>

namespace xitren::allocators {

struct vault_list {
    vault_list* next;
    std::size_t size;
};

template <size_t MemorySize>
class list_manager {
    using size_type               = std::size_t;
    using byte_type               = std::uint8_t;
    using pointer_type            = byte_type*;
    using storage_parameters_type = std::aligned_storage_t<sizeof(byte_type), alignof(byte_type)>;
    using storage_type            = std::array<storage_parameters_type, MemorySize>;
    static_assert(MemorySize, "MemorySize can't be 0!");
    static_assert(MemorySize != 1, "MemorySize can't be 1!");

    storage_type data_;
    vault_list*  free_list_{reinterpret_cast<vault_list*>(data_.data())};
    vault_list*  allocated_list_{nullptr};

    template <typename Type>
    static vault_list*
    list_find_by(vault_list*& list, Type value,
                 std::function<bool(vault_list*, vault_list*)> const& action = nullptr) noexcept
    {
        if (!list) [[unlikely]] {
            return nullptr;
        }
        vault_list* prev_it    = nullptr;
        vault_list* current_it = list;

        while (current_it) {
            if constexpr (std::same_as<size_type, Type>) {
                if (current_it->size >= value) [[unlikely]] {
                    if (action) {
                        if (action(prev_it, current_it)) {
                            break;
                        }
                    }
                }
            } else if constexpr (std::same_as<void*, Type>) {
                pointer_type const data_ptr = reinterpret_cast<pointer_type>(current_it) + sizeof(vault_list);
                if (data_ptr == value) [[unlikely]] {
                    if (action) {
                        action(prev_it, current_it);
                    }
                    break;
                }
            }
            prev_it    = current_it;
            current_it = current_it->next;
        }
        return current_it;
    }

    static void
    list_insert_sorted(vault_list*& list, vault_list* item) noexcept
    {
        if (!item) [[unlikely]] {
            return;
        }
        if (!list) [[unlikely]] {
            list       = item;
            item->next = nullptr;
            return;
        }
        vault_list* top_it  = list;
        vault_list* prev_it = nullptr;
        while (top_it) {
            pointer_type const top_data_ptr  = reinterpret_cast<pointer_type>(top_it) + sizeof(vault_list);
            pointer_type const item_data_ptr = reinterpret_cast<pointer_type>(item) + sizeof(vault_list);
            if (top_data_ptr > item_data_ptr) [[unlikely]] {
                break;
            }
            prev_it = top_it;
            top_it  = top_it->next;
        }
        if (prev_it) {
            prev_it->next = item;
            item->next    = top_it;
        } else {
            list       = item;
            item->next = top_it;
        }
    }

    vault_list*
    list_split_item(vault_list* prev_it, vault_list* current_it, size_type size) noexcept
    {
        pointer_type const current_data_ptr = reinterpret_cast<pointer_type>(current_it) + sizeof(vault_list);
        auto const         new_size         = current_it->size - (size + sizeof(vault_list));
        current_it->size                    = size;
        auto* const new_struct_place        = current_data_ptr + current_it->size;
        vault_list* new_free_item{reinterpret_cast<vault_list*>(new_struct_place)};
        new_free_item->size = new_size;
        new_free_item->next = nullptr;
        if (prev_it) {
            prev_it->next       = new_free_item;
            new_free_item->next = current_it->next;
        } else {
            free_list_          = new_free_item;
            new_free_item->next = current_it->next;
        }
        list_insert_sorted(allocated_list_, current_it);
        return new_free_item;
    }

    bool
    list_check_item_collision(vault_list* prev_it, vault_list* current_it) const noexcept
    {
        if (!prev_it || !current_it) [[unlikely]] {
            return false;
        }
        pointer_type const prev_data_ptr     = reinterpret_cast<pointer_type>(prev_it) + sizeof(vault_list);
        auto* const        next_struct_place = prev_data_ptr + prev_it->size;
        return next_struct_place == reinterpret_cast<pointer_type>(current_it);
    }

    void
    list_concat_items(vault_list*& list) noexcept
    {
        if (!list) [[unlikely]] {
            return;
        }
        vault_list* top_it  = list;
        vault_list* prev_it = nullptr;

        while (top_it) {
            if (list_check_item_collision(prev_it, top_it)) [[unlikely]] {
                prev_it->size = prev_it->size + top_it->size + sizeof(vault_list);
                prev_it->next = top_it->next;
            }
            prev_it = top_it;
            top_it  = top_it->next;
        }
    }

    static void
    compute_list_parameters(vault_list* list, size_t& total_size, size_t& list_len)
    {
        while (list) {
            total_size += list->size;
            list_len += 1;
            list = list->next;
        }
    }

public:
    constexpr explicit list_manager() noexcept
    {
        free_list_->size = data_.size() - sizeof(vault_list);
        free_list_->next = nullptr;
    }

    [[nodiscard]] constexpr void*
    allocate(size_type size) noexcept
    {
        vault_list* ret = nullptr;
        list_find_by(free_list_, size, [&](vault_list* prev_it, vault_list* current_it) -> bool {
            if (current_it->size == size) [[unlikely]] {
                if (prev_it) {
                    prev_it->next = current_it->next;
                } else {
                    free_list_ = current_it->next;
                }
                list_insert_sorted(allocated_list_, current_it);
                ret = current_it;
                return true;
            }
            if (current_it->size > (size + sizeof(vault_list))) [[unlikely]] {
                list_split_item(prev_it, current_it, size);
                ret = current_it;
                return true;
            }
            return false;
        });
        if (!ret) {
            return nullptr;
        }
        pointer_type const ret_data_ptr = reinterpret_cast<pointer_type>(ret) + sizeof(vault_list);
        return ret_data_ptr;
    }

    constexpr void
    deallocate(void* ptr, [[maybe_unused]] std::size_t size = 0) noexcept
    {
        list_find_by(allocated_list_, ptr, [&](vault_list* prev_it, vault_list* current_it) -> bool {
            if (prev_it) {
                prev_it->next = current_it->next;
            } else {
                allocated_list_ = current_it->next;
            }
            list_insert_sorted(free_list_, current_it);
            return true;
        });
        list_concat_items(free_list_);
    }

    [[nodiscard]] bool
    check_invariant() const noexcept
    {
        vault_list* free_list       = free_list_;
        size_t      total_free_size = 0;
        size_t      free_list_len   = 0;
        compute_list_parameters(free_list, total_free_size, free_list_len);

        vault_list* alloc_list       = allocated_list_;
        size_t      total_alloc_size = 0;
        size_t      alloc_list_len   = 0;
        compute_list_parameters(alloc_list, total_alloc_size, alloc_list_len);

        return MemorySize == total_alloc_size + total_free_size + (alloc_list_len + free_list_len) * sizeof(vault_list);
    }
};

}    // namespace xitren::allocators
