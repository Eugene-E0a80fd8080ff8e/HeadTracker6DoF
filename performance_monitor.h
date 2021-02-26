#pragma once

class PerformanceMonitor {
	PerformanceMonitor(const PerformanceMonitor&) = delete;
	PerformanceMonitor& operator=(PerformanceMonitor  const&) = delete;

	static int64_t currentTimeMicros()
	{
		std::chrono::time_point<std::chrono::steady_clock> t = std::chrono::steady_clock::now();
		return std::chrono::duration_cast<std::chrono::microseconds>(t.time_since_epoch()).count();
	}

	char title[100];
	char output[1000];

	int64_t beginTime = -1, endTime = -1;
	double avgBeginEnd = 0.0, avgEndBegin = 0.0;

public:
	PerformanceMonitor(const char* title);

	void begin();
	void end();

	char* getStringPercents();
	char* getStringFPS();

};