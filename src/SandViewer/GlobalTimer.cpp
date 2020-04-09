#include "GlobalTimer.h"
#include "Logger.h"

int GlobalTimer::StatsUi::s_counter = 0;

GlobalTimer::StatsUi::StatsUi() {
	// Iterate through these colors
	constexpr glm::vec3 predefinedColors[] = {
		{225.f / 255.f,  25.f / 255.f,  96.f / 255.f},
		{225.f / 255.f,  96.f / 255.f,  25.f / 255.f},
		{ 15.f / 255.f, 167.f / 255.f, 134.f / 255.f},
		{106.f / 255.f,  81.f / 255.f, 200.f / 255.f},
	};
	color = predefinedColors[(s_counter++) % 4];
}

//-----------------------------------------------------------------------------

GlobalTimer::Timer::Timer()
{
	glCreateQueries(GL_TIMESTAMP, 2, queries);
}

GlobalTimer::Timer::~Timer()
{
	glDeleteQueries(2, queries);
}

//-----------------------------------------------------------------------------

std::shared_ptr<GlobalTimer> GlobalTimer::s_instance;

void GlobalTimer::Stats::reset() noexcept
{
	sampleCount = 0;
	cumulatedTime = 0.0;
	cumulatedFrameOffset = 0.0;
	cumulatedGpuTime = 0.0;
	cumulatedGpuFrameOffset = 0.0;
}

GlobalTimer::GlobalTimer()
{}

GlobalTimer::~GlobalTimer()
{
	gatherQueries();

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
	glQueryCounter(timer->queries[0], GL_TIMESTAMP);
	timer->startTime = std::chrono::high_resolution_clock::now();
	return static_cast<void*>(timer);
}

template <typename Duration>
static double milliseconds(Duration dt)
{
	return std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1, 1000>>>(dt).count();
}

void GlobalTimer::stop(TimerHandle handle) noexcept
{
	auto endTime = std::chrono::high_resolution_clock::now();
	Timer* timer = static_cast<Timer*>(handle);
	glQueryCounter(timer->queries[1], GL_TIMESTAMP);
	if (m_pool.count(timer) == 0) {
		WARN_LOG << "Invalid TimerHandle";
		return;
	}
	m_pool.erase(timer);
	m_stopped.insert(timer);
	double ellapsed = milliseconds(endTime - timer->startTime);
	double offset = milliseconds(timer->startTime - m_frameTimer.startTime);

	Stats & stats = m_stats[timer->message];
	stats.sampleCount++;
	stats.cumulatedTime += ellapsed;
	stats.cumulatedFrameOffset += offset;
}

void GlobalTimer::startFrame() noexcept
{
	glQueryCounter(m_frameTimer.queries[0], GL_TIMESTAMP);
	m_frameTimer.startTime = std::chrono::high_resolution_clock::now();
}

void GlobalTimer::stopFrame() noexcept
{
	auto endTime = std::chrono::high_resolution_clock::now();
	glQueryCounter(m_frameTimer.queries[1], GL_TIMESTAMP);
	double ellapsed = milliseconds(endTime - m_frameTimer.startTime);

	m_frameStats.sampleCount++;
	m_frameStats.cumulatedTime += ellapsed;

	gatherQueries();
}

void GlobalTimer::gatherQueries() noexcept
{
	GLuint64 frameStartNs, frameEndNs;
	glGetQueryObjectui64v(m_frameTimer.queries[0], GL_QUERY_RESULT, &frameStartNs);
	glGetQueryObjectui64v(m_frameTimer.queries[1], GL_QUERY_RESULT, &frameEndNs);

	m_frameStats.cumulatedGpuTime += static_cast<double>(frameEndNs - frameStartNs) * 1e-6;
	
	while (!m_stopped.empty()) {
		auto it = m_stopped.begin();
		Timer* timer = *it;
		m_stopped.erase(it);

		GLuint64 startNs, endNs;
		glGetQueryObjectui64v(timer->queries[0], GL_QUERY_RESULT, &startNs);
		glGetQueryObjectui64v(timer->queries[1], GL_QUERY_RESULT, &endNs);

		Stats& stats = m_stats[timer->message];
		stats.cumulatedGpuTime += static_cast<double>(endNs - startNs) * 1e-6;
		stats.cumulatedGpuFrameOffset += static_cast<double>(startNs - frameStartNs) * 1e-6;

		delete timer;
	}
}

void GlobalTimer::resetAllStats() noexcept
{
	m_frameStats.reset();
	for (auto& s : m_stats) {
		s.second.reset();
	}
}
