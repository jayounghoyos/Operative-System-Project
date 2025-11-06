#ifndef PHILOSOPHERS_HPP
#define PHILOSOPHERS_HPP

#include <mutex>
#include <array>
#include <string>

// Estados de un filósofo
enum class PhilosopherState {
    THINKING,
    HUNGRY,
    EATING
};

// Representación de un filósofo
struct Philosopher {
    int id;
    PhilosopherState state;
    int times_eaten;           // Número de veces que ha comido
    int times_thought;         // Número de veces que ha pensado
    int times_blocked;         // Veces que no pudo obtener tenedores
    int left_fork_id;          // ID del tenedor izquierdo
    int right_fork_id;         // ID del tenedor derecho

    Philosopher(int philosopher_id, int left, int right);
    std::string state_to_string() const;
};

// Simulación de la Cena de los Filósofos
class DiningPhilosophers {
public:
    explicit DiningPhilosophers(int num_philosophers = 5);

    // Operaciones principales
    void simulate_step();              // Simular un paso (todos los filósofos actúan)
    void simulate_steps(int n);        // Simular n pasos
    void reset();                      // Reiniciar simulación

    // Visualización
    void display_table() const;        // Mostrar mesa con ASCII art
    void display_stats() const;        // Estadísticas de cada filósofo
    void display_forks() const;        // Estado de tenedores

    // Getters
    int get_step_count() const { return step_count_; }
    int get_deadlock_count() const { return deadlock_count_; }

private:
    static constexpr int NUM_PHILOSOPHERS = 5;

    std::array<Philosopher, NUM_PHILOSOPHERS> philosophers_;
    std::array<bool, NUM_PHILOSOPHERS> fork_in_use_;     // Fork está en uso?
    std::array<int, NUM_PHILOSOPHERS> fork_holder_;      // Quién tiene el fork (-1 si libre)

    int step_count_;                   // Contador de pasos de simulación
    int total_meals_;                  // Total de comidas
    int deadlock_count_;               // Veces que se detectó deadlock potencial
    int consecutive_all_hungry_;       // Contador de pasos consecutivos con todos hambrientos

    mutable std::mutex simulation_mutex_;  // Mutex para la simulación

    // Helpers internos
    void philosopher_action(int id);
    void release_forks(int id);
    bool check_deadlock();

    // Estrategia asimétrica para prevenir deadlock
    // Filósofos pares: izquierda primero
    // Filósofos impares: derecha primero
    bool try_acquire_asymmetric(int id);
};

#endif // PHILOSOPHERS_HPP
