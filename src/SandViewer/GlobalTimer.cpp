#include "GlobalTimer.h"
#include "Logger.h"

std::shared_ptr<GlobalTimer> GlobalTimer::s_instance = std::make_shared<GlobalTimer>();

GlobalTimer::GlobalTimer()
{}

GlobalTimer::~GlobalTimer()
{
	if (!m_pool.empty()) {
		WARN_LOG << "Program terminates but some timers are still running";
	}

	while (!m_pool.empty()) {
		auto it = m_pool.begin();
		delete *it;
		m_pool.erase(it);
	}
}

GlobalTimer::TimerHandle GlobalTimer::start(const std::string& message) noexcept
{
	Timer *timer = new Timer();
	m_pool.insert(timer);
	timer->message = message;
	timer->startTime = std::chrono::high_resolution_clock::now();
	return static_cast<void*>(timer);
}

void GlobalTimer::stop(TimerHandle handle) noexcept
{
	auto endTime = std::chrono::high_resolution_clock::now();
	Timer* timer = static_cast<Timer*>(handle);
	if (m_pool.count(timer) == 0) {
		WARN_LOG << "Invalid TimerHandle";
		return;
	}
	m_pool.erase(timer);
	double ellapsed = std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1>>>(endTime - timer->startTime).count();

	Stats & stats = m_stats[timer->message];
	stats.sampleCount++;
	stats.cumulatedTime += ellapsed;

	delete timer;
}

void GlobalTimer::resetAllStats() noexcept
{
	for (auto& s : m_stats) {
		s.second.sampleCount = 0;
		s.second.cumulatedTime = 0.0;
	}
}
