#ifndef HEAP_HPP
#define HEAP_HPP

#include <vector>
#include <map>
#include <set>
#include <string>

// Optamos por un buddy allocator porque nos permite medir
// fragmentación de forma sencilla para el informe.
class BuddyAllocator {
public:
    BuddyAllocator();

    bool init(size_t total_size, size_t min_block);
    int alloc(size_t size);
    bool free(int alloc_id);
    void display_stats() const;
    void reset();

private:
    enum class BlockStatus { FREE, SPLIT, USED };

    size_t total_size_;
    size_t min_block_;
    int next_alloc_id_;
    bool initialized_;
    int max_level_;

    std::vector<std::set<size_t>> free_sets_;

    struct AllocationInfo {
        size_t level;
        size_t offset;
        size_t requested;
        size_t assigned;
    };

    std::map<int, AllocationInfo> allocations_;

    size_t total_allocated_bytes_;
    size_t total_requested_bytes_;
    size_t current_allocated_bytes_;
    int allocation_attempts_;
    int successful_allocations_;

    size_t block_size(size_t level) const;

    void build_tree();
    size_t level_for_size(size_t size) const;
    bool find_free_block(size_t needed_level, size_t& level_out, size_t& offset_out);
    void insert_block(size_t level, size_t offset);
    void merge_block(size_t level, size_t offset);
};

#endif // HEAP_HPP
