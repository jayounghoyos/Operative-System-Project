#ifndef IO_HPP
#define IO_HPP

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <utility>

// Nos dimos a la tarea de encapsular toda la lógica de I/O en esta clase
// para aislar las colas de dispositivos del resto de la CLI.
class IOManager {
public:
    IOManager();

    bool add_device(const std::string& name, int spool_capacity);
    bool device_exists(const std::string& name) const;
    bool has_devices() const { return !devices_.empty(); }

    bool submit_request(const std::string& device_name,
                        int pid,
                        int duration,
                        int priority,
                        std::string& error_message);

    std::vector<std::pair<int, std::string>> process_ticks(int ticks);

    void display_status() const;
    void display_device(const std::string& name) const;
    void reset();

private:
    // Cada solicitud conserva lo necesario para que podamos
    // reanudar el proceso y medir tiempos.
    struct IORequest {
        int request_id;
        int pid;
        int duration;
        int remaining_time;
        int priority;
        int arrival_time;
        int start_time;
    };

    // Representación de un dispositivo con spool limitado.
    struct IODevice {
        std::string name;
        int spool_capacity;
        int next_request_id;
        int total_requests;
        int completed_requests;
        int total_wait_time;
        int total_busy_time;
        std::optional<IORequest> current_request;
        std::vector<IORequest> waiting_queue;
    };

    std::map<std::string, IODevice> devices_;
    int current_time_;

    void assign_next_request(IODevice& device);
    IODevice* get_device(const std::string& name);
    const IODevice* get_device(const std::string& name) const;
};

#endif // IO_HPP
