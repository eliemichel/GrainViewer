#pragma once

#include <string>
#include <chrono>
#include <set>
#include <map>
#include <memory>

/**
 * Global timer is used as a singleton to record render timings using both CPU
 * side timings and GPU timer queries.
 */
class GlobalTimer {
public:
    typedef void* TimerHandle;
    struct Stats {
        double cumulatedTime = 0.0;
        int sampleCount = 0;
    };

public:
    static std::weak_ptr<GlobalTimer> GetInstance() noexcept { return s_instance; }
    static TimerHandle Start(const std::string& message) noexcept { return s_instance->start(message); }
    static void Stop(TimerHandle handle) noexcept { s_instance->stop(handle); }

public:
    GlobalTimer();
    ~GlobalTimer();
    GlobalTimer& operator=(const GlobalTimer&) = delete;
    GlobalTimer(const GlobalTimer&) = delete;

    TimerHandle start(const std::string & message) noexcept;
    void stop(TimerHandle handle) noexcept;
    const std::map<std::string, Stats>& stats() const noexcept { return m_stats; }
    void resetAllStats() noexcept;

private:
    struct Timer {
        std::chrono::steady_clock::time_point startTime;
        std::string message;
    };

private:
    static std::shared_ptr<GlobalTimer> s_instance;

private:
    std::set<Timer*> m_pool; // running timers
    std::map<std::string, Stats> m_stats; // cumulated statistics
};

