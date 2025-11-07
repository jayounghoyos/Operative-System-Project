#ifndef DISK_HPP
#define DISK_HPP

#include <vector>
#include <string>
#include <optional>
#include <queue>

enum class DiskAlgorithm {
    FCFS,
    SSTF,
    SCAN_UP,
    SCAN_DOWN
};

// Este planificador nos permite comparar FCFS, SSTF y SCAN
// tal como lo pide el proyecto.
class DiskScheduler {
public:
    DiskScheduler();

    bool init(int tracks);
    bool set_head(int position);
    bool set_queue(const std::vector<int>& requests);
    void add_request(int cylinder);

    bool run(DiskAlgorithm algo);
    void display_stats() const;
    void display_view() const;
    void reset();

private:
    int total_tracks_;
    int head_position_;
    std::vector<int> initial_queue_;
    std::vector<int> execution_order_;
    int total_movement_;
    bool initialized_;

    void run_fcfs();
    void run_sstf();
    void run_scan(bool upwards);
};

#endif // DISK_HPP
