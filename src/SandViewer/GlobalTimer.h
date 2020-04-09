#pragma once

#include <OpenGL>
#include <glm/glm.hpp>
#include <refl.hpp>

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
    struct StatsUi {
        bool visible = true;
        glm::vec3 color = glm::vec3(0.0);

        StatsUi();
    private:
        static int s_counter;
    };
    struct Stats {
        int sampleCount = 0;
        double cumulatedTime = 0.0;
        double cumulatedFrameOffset = 0.0; // time within a frame at which the timer started
        double cumulatedGpuTime = 0.0;
        double cumulatedGpuFrameOffset = 0.0;

        // for UI -- not reset by reset()
        mutable StatsUi ui;

        void reset() noexcept;
    };

public:
    // static API
    static std::shared_ptr<GlobalTimer> GetInstance() noexcept { if (!s_instance) s_instance = std::make_shared<GlobalTimer>(); return s_instance; }
    static TimerHandle Start(const std::string& message) noexcept { return GetInstance()->start(message); }
    static void Stop(TimerHandle handle) noexcept { GetInstance()->stop(handle); }
    static void StartFrame() noexcept { return GetInstance()->startFrame(); }
    static void StopFrame() noexcept { GetInstance()->stopFrame(); }

public:
    struct Properties {
        bool showDiagram = false;
        float decay = 0.1f;
    };
    Properties& properties() { return m_properties; }
    const Properties& properties() const { return m_properties; }

public:
    GlobalTimer();
    ~GlobalTimer();
    GlobalTimer& operator=(const GlobalTimer&) = delete;
    GlobalTimer(const GlobalTimer&) = delete;

    TimerHandle start(const std::string & message) noexcept;
    void stop(TimerHandle handle) noexcept;
    void startFrame() noexcept;
    void stopFrame() noexcept;
    const std::map<std::string, Stats>& stats() const noexcept { return m_stats; }
    void resetAllStats() noexcept;
    const Stats& frameStats() const noexcept { return m_frameStats; }

private:
    struct Timer {
        std::chrono::steady_clock::time_point startTime;
        std::string message;
        GLuint queries[2]; // GPU timer queries for begin and end

        Timer();
        ~Timer();
        Timer(const Timer&) = delete;
        Timer & operator=(const Timer&) = delete;
    };

    // sampleCount must have been incremented first
    void addSample(double& accumulator, double dt, int sampleCount) noexcept;
    void gatherQueries() noexcept;

private:
    static std::shared_ptr<GlobalTimer> s_instance;

private:
    Properties m_properties;
    Timer m_frameTimer; // special timer global to the frame
    Stats m_frameStats;
    std::set<Timer*> m_pool; // running timers
    std::set<Timer*> m_stopped; // stopped timers of which queries are waiting to get read back
    std::map<std::string, Stats> m_stats; // cumulated statistics
};

REFL_TYPE(GlobalTimer::Properties)
REFL_FIELD(showDiagram)
REFL_FIELD(decay)
REFL_END

class ScopedTimer {
public:
    ScopedTimer(const std::string& message) : m_handle(GlobalTimer::Start(message)) {}
    ~ScopedTimer() { GlobalTimer::Stop(m_handle); }
private:
    GlobalTimer::TimerHandle m_handle;
};
