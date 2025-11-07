#include "../include/heap.hpp"
#include "../include/utils.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

namespace {
size_t next_pow2(size_t n) {
    size_t p = 1;
    while (p < n) p <<= 1;
    return p;
}
}

BuddyAllocator::BuddyAllocator()
    : total_size_(0),
      min_block_(0),
      next_alloc_id_(1),
      initialized_(false),
      max_level_(0),
      total_allocated_bytes_(0),
      total_requested_bytes_(0),
      current_allocated_bytes_(0),
      allocation_attempts_(0),
      successful_allocations_(0) {}

size_t BuddyAllocator::block_size(size_t level) const {
    return min_block_ << level;
}

bool BuddyAllocator::init(size_t total_size, size_t min_block) {
    if (total_size == 0 || min_block == 0 || min_block > total_size) {
        std::cout << Color::RED << "[HEAP] Tamaños inválidos"
                  << Color::RESET << std::endl;
        return false;
    }

    min_block_ = next_pow2(min_block);
    total_size_ = next_pow2(total_size);

    if (min_block_ > total_size_) {
        std::cout << Color::RED << "[HEAP] min_block > total_size"
                  << Color::RESET << std::endl;
        return false;
    }

    max_level_ = static_cast<int>(
        std::log2(total_size_ / min_block_));

    // Aquí reiniciamos toda la estructura para garantizar
    // que cada llamada a heap-init comienza con memoria limpia.
    free_sets_.assign(max_level_ + 1, {});
    allocations_.clear();

    build_tree();

    total_allocated_bytes_ = 0;
    total_requested_bytes_ = 0;
    current_allocated_bytes_ = 0;
    allocation_attempts_ = 0;
    successful_allocations_ = 0;

    initialized_ = true;

    std::cout << Color::GREEN << "[HEAP] Buddy allocator inicializado "
              << "(total=" << total_size_ << " bytes, min_block="
              << min_block_ << ")" << Color::RESET << std::endl;
    return true;
}

void BuddyAllocator::build_tree() {
    for (auto& set : free_sets_) set.clear();
    free_sets_[max_level_].insert(0); // bloque completo
}

size_t BuddyAllocator::level_for_size(size_t size) const {
    size_t required = std::max(size, min_block_);
    size_t level = 0;
    while (level <= static_cast<size_t>(max_level_) &&
           block_size(level) < required) {
        level++;
    }
    return std::min(level, static_cast<size_t>(max_level_));
}

bool BuddyAllocator::find_free_block(size_t needed_level,
                                     size_t& level_out,
                                     size_t& offset_out) {
    // Recorremos de menor a mayor nivel hasta encontrar el bloque
    // y vamos partiendo sobre la marcha para evitar listas enlazadas complejas.
    for (size_t level = needed_level; level <= static_cast<size_t>(max_level_); ++level) {
        if (!free_sets_[level].empty()) {
            size_t offset = *free_sets_[level].begin();
            free_sets_[level].erase(free_sets_[level].begin());

            size_t current_level = level;
            size_t current_offset = offset;

            while (current_level > needed_level) {
                current_level--;
                size_t child_size = block_size(current_level);
                size_t buddy_offset = current_offset + child_size;
                free_sets_[current_level].insert(buddy_offset);
            }

            level_out = needed_level;
            offset_out = current_offset;
            return true;
        }
    }
    return false;
}

void BuddyAllocator::insert_block(size_t level, size_t offset) {
    free_sets_[level].insert(offset);
}

void BuddyAllocator::merge_block(size_t level, size_t offset) {
    while (level < static_cast<size_t>(max_level_)) {
        size_t buddy_offset = offset ^ block_size(level);

        auto buddy_it = free_sets_[level].find(buddy_offset);
        auto self_it = free_sets_[level].find(offset);

        if (buddy_it == free_sets_[level].end() ||
            self_it == free_sets_[level].end()) {
            break;
        }

        free_sets_[level].erase(buddy_it);
        free_sets_[level].erase(self_it);

        offset = std::min(offset, buddy_offset);
        level++;
        free_sets_[level].insert(offset);
    }
}

int BuddyAllocator::alloc(size_t size) {
    if (!initialized_) {
        std::cout << Color::RED << "[HEAP] Primero heap-init"
                  << Color::RESET << std::endl;
        return -1;
    }

    allocation_attempts_++;
    total_requested_bytes_ += size;

    size_t needed_level = level_for_size(size);
    size_t level, offset;
    if (!find_free_block(needed_level, level, offset)) {
        std::cout << Color::YELLOW << "[HEAP] No hay bloques disponibles para "
                  << size << " bytes" << Color::RESET << std::endl;
        return -1;
    }

    size_t assigned = block_size(level);
    AllocationInfo info{level, offset, size, assigned};
    allocations_[next_alloc_id_] = info;

    total_allocated_bytes_ += assigned;
    current_allocated_bytes_ += assigned;
    successful_allocations_++;

    std::cout << Color::CYAN << "[HEAP] alloc id=" << next_alloc_id_
              << " offset=" << offset
              << " size=" << assigned
              << " (requested=" << size << ")"
              << Color::RESET << std::endl;

    return next_alloc_id_++;
}

bool BuddyAllocator::free(int alloc_id) {
    if (!initialized_) {
        std::cout << Color::RED << "[HEAP] Primero heap-init"
                  << Color::RESET << std::endl;
        return false;
    }

    auto it = allocations_.find(alloc_id);
    if (it == allocations_.end()) {
        std::cout << Color::RED << "[HEAP] Alloc id no válido"
                  << Color::RESET << std::endl;
        return false;
    }

    size_t level = it->second.level;
    size_t offset = it->second.offset;
    size_t assigned = it->second.assigned;

    current_allocated_bytes_ = (current_allocated_bytes_ >= assigned)
                                   ? current_allocated_bytes_ - assigned
                                   : 0;

    allocations_.erase(it);

    insert_block(level, offset);
    merge_block(level, offset);

    std::cout << Color::GREEN << "[HEAP] free id=" << alloc_id
              << " offset=" << offset
              << " size=" << assigned << Color::RESET << std::endl;
    return true;
}

void BuddyAllocator::display_stats() const {
    if (!initialized_) {
        std::cout << Color::RED << "[HEAP] Primero heap-init"
                  << Color::RESET << std::endl;
        return;
    }

    print_header("ESTADÍSTICAS HEAP (Buddy)");
    std::cout << " Tamaño total:              " << total_size_ << " bytes" << std::endl;
    std::cout << " Tamaño mínimo bloque:      " << min_block_ << " bytes" << std::endl;
    std::cout << " Asignaciones activas:      " << allocations_.size() << std::endl;
    std::cout << " Intentos de alloc:         " << allocation_attempts_ << std::endl;
    std::cout << " Alloc exitosos:            " << successful_allocations_ << std::endl;
    std::cout << " Bytes solicitados totales: " << total_requested_bytes_ << std::endl;
    std::cout << " Bytes asignados reales:    " << total_allocated_bytes_ << std::endl;
    std::cout << " Bytes actualmente en uso:  " << current_allocated_bytes_ << std::endl;

    if (total_allocated_bytes_ > 0) {
        double fragmentation = 1.0 -
            (static_cast<double>(total_requested_bytes_) /
             static_cast<double>(total_allocated_bytes_));
        fragmentation = std::clamp(fragmentation, 0.0, 1.0);
        std::cout << std::fixed << std::setprecision(2)
                  << " Fragmentación interna:    " << fragmentation * 100 << "%"
                  << std::endl;
    } else {
        std::cout << " Fragmentación interna:    -" << std::endl;
    }

    size_t free_bytes = total_size_ - current_allocated_bytes_;
    std::cout << " Memoria libre total:       " << free_bytes << " bytes" << std::endl;
    std::cout << std::endl;
}

void BuddyAllocator::reset() {
    allocations_.clear();
    for (auto& set : free_sets_) set.clear();
    initialized_ = false;
    std::cout << Color::CYAN << "[HEAP] Estado reiniciado; ejecute heap-init"
              << Color::RESET << std::endl;
}
