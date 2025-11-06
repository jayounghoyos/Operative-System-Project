#include "../include/philosophers.hpp"
#include "../include/utils.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>

Philosopher::Philosopher(int philosopher_id, int left, int right)
    : id(philosopher_id),
      state(PhilosopherState::THINKING),
      times_eaten(0),
      times_thought(0),
      times_blocked(0),
      left_fork_id(left),
      right_fork_id(right)
{
}

std::string Philosopher::state_to_string() const
{
    switch (state)
    {
    case PhilosopherState::THINKING:
        return "THINKING";
    case PhilosopherState::HUNGRY:
        return "HUNGRY";
    case PhilosopherState::EATING:
        return "EATING";
    default:
        return "UNKNOWN";
    }
}

DiningPhilosophers::DiningPhilosophers(int)
    : philosophers_{{
          Philosopher(0, 0, 1),
          Philosopher(1, 1, 2),
          Philosopher(2, 2, 3),
          Philosopher(3, 3, 4),
          Philosopher(4, 4, 0),
      }},
      fork_in_use_{},
      fork_holder_{},
      step_count_(0),
      total_meals_(0),
      deadlock_count_(0),
      consecutive_all_hungry_(0)
{
    fork_in_use_.fill(false);
    fork_holder_.fill(-1);
}

void DiningPhilosophers::simulate_step()
{
    std::lock_guard<std::mutex> lock(simulation_mutex_);

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i)
    {
        philosopher_action(i);
    }

    ++step_count_;

    if (check_deadlock())
    {
        ++deadlock_count_;
        std::cout << Color::RED << "[PHIL] Posible interbloqueo detectado en step "
                  << step_count_ << Color::RESET << std::endl;
    }
}

void DiningPhilosophers::simulate_steps(int n)
{
    if (n <= 0)
    {
        std::cout << Color::RED << "Error: n debe ser > 0" << Color::RESET << std::endl;
        return;
    }

    std::cout << Color::BLUE << Color::BOLD
              << "\n▶ Simulando " << n << " pasos de filósofos...\n"
              << Color::RESET << std::endl;

    for (int i = 0; i < n; ++i)
    {
        simulate_step();
    }

    std::cout << Color::BLUE << Color::BOLD
              << "\n Simulación completada (pasos=" << step_count_ << ")\n"
              << Color::RESET << std::endl;
}

void DiningPhilosophers::reset()
{
    std::lock_guard<std::mutex> lock(simulation_mutex_);

    step_count_ = 0;
    total_meals_ = 0;
    deadlock_count_ = 0;
    consecutive_all_hungry_ = 0;

    fork_in_use_.fill(false);
    fork_holder_.fill(-1);

    for (auto &ph : philosophers_)
    {
        ph.state = PhilosopherState::THINKING;
        ph.times_eaten = 0;
        ph.times_thought = 0;
        ph.times_blocked = 0;
    }

    std::cout << Color::CYAN << "[PHIL] Simulación reiniciada" << Color::RESET << std::endl;
}

void DiningPhilosophers::display_table() const
{
    std::lock_guard<std::mutex> lock(simulation_mutex_);

    std::ostringstream title;
    title << "CENA DE LOS FILÓSOFOS (step=" << step_count_ << ")";
    print_header(title.str());

    std::cout << std::left
              << std::setw(8) << "ID"
              << std::setw(12) << "Estado"
              << std::setw(12) << "Pensó"
              << std::setw(12) << "Comió"
              << std::setw(14) << "Bloqueos"
              << std::setw(14) << "Tenedor izq."
              << std::setw(14) << "Tenedor der."
              << std::endl;
    print_separator(80);

    for (const auto &ph : philosophers_)
    {
        std::string color = Color::WHITE;
        if (ph.state == PhilosopherState::EATING)
            color = Color::GREEN;
        else if (ph.state == PhilosopherState::HUNGRY)
            color = Color::YELLOW;

        std::cout << color
                  << std::setw(8) << ("F" + std::to_string(ph.id))
                  << std::setw(12) << ph.state_to_string()
                  << std::setw(12) << ph.times_thought
                  << std::setw(12) << ph.times_eaten
                  << std::setw(14) << ph.times_blocked
                  << std::setw(14) << ("#" + std::to_string(ph.left_fork_id))
                  << std::setw(14) << ("#" + std::to_string(ph.right_fork_id))
                  << Color::RESET << std::endl;
    }

    std::cout << std::endl;
}

void DiningPhilosophers::display_stats() const
{
    std::lock_guard<std::mutex> lock(simulation_mutex_);

    print_header("ESTADÍSTICAS FILÓSOFOS");

    std::cout << " Pasos simulados:       " << step_count_ << std::endl;
    std::cout << " Comidas totales:       " << total_meals_ << std::endl;
    std::cout << " Deadlocks detectados:  " << deadlock_count_ << std::endl;

    std::cout << "\n Por filósofo:\n";
    print_separator(70);
    std::cout << std::left
              << std::setw(8) << "ID"
              << std::setw(12) << "Estado"
              << std::setw(12) << "Pensó"
              << std::setw(12) << "Comió"
              << std::setw(14) << "Bloqueos"
              << std::endl;
    print_separator(70);

    for (const auto &ph : philosophers_)
    {
        std::string color = Color::WHITE;
        if (ph.state == PhilosopherState::EATING)
            color = Color::GREEN;
        else if (ph.state == PhilosopherState::HUNGRY)
            color = Color::YELLOW;

        std::cout << color
                  << std::setw(8) << ("F" + std::to_string(ph.id))
                  << std::setw(12) << ph.state_to_string()
                  << std::setw(12) << ph.times_thought
                  << std::setw(12) << ph.times_eaten
                  << std::setw(14) << ph.times_blocked
                  << Color::RESET << std::endl;
    }

    std::cout << std::endl;
}

void DiningPhilosophers::display_forks() const
{
    std::lock_guard<std::mutex> lock(simulation_mutex_);

    print_header("TENEDORES");
    std::cout << std::left
              << std::setw(10) << "Tenedor"
              << std::setw(12) << "En uso"
              << std::setw(12) << "Filósofo"
              << std::endl;
    print_separator(40);

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i)
    {
        bool in_use = fork_in_use_[i];
        int holder = fork_holder_[i];
        std::string color = in_use ? Color::GREEN : Color::WHITE;

        std::cout << color
                  << std::setw(10) << ("#" + std::to_string(i))
                  << std::setw(12) << (in_use ? "Sí" : "No")
                  << std::setw(12) << (holder >= 0 ? "F" + std::to_string(holder) : "-")
                  << Color::RESET << std::endl;
    }

    std::cout << std::endl;
}

void DiningPhilosophers::philosopher_action(int id)
{
    auto &ph = philosophers_[id];

    switch (ph.state)
    {
    case PhilosopherState::THINKING:
        ++ph.times_thought;
        ph.state = PhilosopherState::HUNGRY;
        break;
    case PhilosopherState::HUNGRY:
        if (try_acquire_asymmetric(id))
        {
            ph.state = PhilosopherState::EATING;
        }
        else
        {
            ++ph.times_blocked;
        }
        break;
    case PhilosopherState::EATING:
        release_forks(id);
        ph.state = PhilosopherState::THINKING;
        ++ph.times_eaten;
        ++total_meals_;
        break;
    default:
        break;
    }
}

bool DiningPhilosophers::try_acquire_asymmetric(int id)
{
    int first = (id % 2 == 0) ? philosophers_[id].left_fork_id : philosophers_[id].right_fork_id;
    int second = (id % 2 == 0) ? philosophers_[id].right_fork_id : philosophers_[id].left_fork_id;

    if (fork_in_use_[first])
    {
        return false;
    }

    fork_in_use_[first] = true;
    fork_holder_[first] = id;

    if (fork_in_use_[second])
    {
        fork_in_use_[first] = false;
        fork_holder_[first] = -1;
        return false;
    }

    fork_in_use_[second] = true;
    fork_holder_[second] = id;
    return true;
}

void DiningPhilosophers::release_forks(int id)
{
    int left = philosophers_[id].left_fork_id;
    int right = philosophers_[id].right_fork_id;

    if (fork_holder_[left] == id)
    {
        fork_in_use_[left] = false;
        fork_holder_[left] = -1;
    }

    if (fork_holder_[right] == id)
    {
        fork_in_use_[right] = false;
        fork_holder_[right] = -1;
    }
}

bool DiningPhilosophers::check_deadlock()
{
    bool all_hungry = std::all_of(
        philosophers_.begin(), philosophers_.end(),
        [](const Philosopher &ph)
        {
            return ph.state == PhilosopherState::HUNGRY;
        });

    bool any_eating = std::any_of(
        philosophers_.begin(), philosophers_.end(),
        [](const Philosopher &ph)
        {
            return ph.state == PhilosopherState::EATING;
        });

    bool forks_busy = std::any_of(
        fork_in_use_.begin(), fork_in_use_.end(),
        [](bool in_use)
        {
            return in_use;
        });

    if (all_hungry && !any_eating && !forks_busy)
    {
        ++consecutive_all_hungry_;
    }
    else
    {
        consecutive_all_hungry_ = 0;
    }

    return consecutive_all_hungry_ >= 2;
}
