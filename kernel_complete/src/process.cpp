#include "../include/process.hpp"
#include "../include/utils.hpp"
#include <iostream>

Process::Process(int pid, int burst_time) 
    : pid_(pid), 
      state_(ProcessState::NEW),
      burst_time_(burst_time),
      remaining_time_(burst_time),
      wait_time_(0),
      turnaround_time_(0),
      arrival_time_(0) {
}

void Process::execute() {
    if (remaining_time_ > 0) {
        remaining_time_--;
        if (remaining_time_ == 0) {
            state_ = ProcessState::TERMINATED;
        }
    }
}

void Process::calculate_turnaround(int current_time) {
    turnaround_time_ = current_time - arrival_time_;
}

std::string Process::state_to_string() const {
    switch (state_) {
        case ProcessState::NEW: return "NEW";
        case ProcessState::READY: return "READY";
        case ProcessState::RUNNING: return "RUNNING";
        case ProcessState::BLOCKED: return "BLOCKED";
        case ProcessState::TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

void Process::print_info() const {
    std::string color;
    switch (state_) {
        case ProcessState::RUNNING: color = Color::GREEN; break;
        case ProcessState::READY: color = Color::YELLOW; break;
        case ProcessState::BLOCKED: color = Color::BLUE; break;
        case ProcessState::TERMINATED: color = Color::RED; break;
        default: color = Color::RESET;
    }
    
    std::cout << color
              << std::setw(6) << pid_
              << std::setw(12) << state_to_string()
              << std::setw(8) << burst_time_
              << std::setw(10) << remaining_time_
              << std::setw(10) << wait_time_
              << std::setw(12) << turnaround_time_
              << Color::RESET << std::endl;
}
