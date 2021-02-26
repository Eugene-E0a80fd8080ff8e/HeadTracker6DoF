
#include "stdafx.h"
#include "after_stdafx.h"

#include <chrono>
#include <math.h>
#include <cstdio>

PerformanceMonitor::PerformanceMonitor(const char * title)
{
	strncpy(this->title, title, sizeof(this->title));
	this->title[sizeof(this->title) - 1] = 0;
}

void PerformanceMonitor::begin() 
{
	int64_t cur = currentTimeMicros();
	if (endTime != -1)
	{
		int64_t span = cur - endTime;
		if (avgEndBegin == 0.0)  avgEndBegin = span;
		else {
			double t = ::exp(-(span * 1e-6));   //  1e-6 is 1/1,000,000 microseconds**-1, which is a time constant
			avgEndBegin = avgEndBegin * t + span * (1 - t);
		}
	}
	beginTime = cur;
}

void PerformanceMonitor::end()
{
	int64_t cur = currentTimeMicros();
	if(beginTime != -1)
	{
		int64_t span = cur - beginTime;
		if (avgBeginEnd == 0.0)  avgBeginEnd = span;
		else {
			double t = ::exp(- (span * 1e-6));   //  1e-6 is 1/1,000,000 microseconds**-1, which is a time constant
			avgBeginEnd = avgBeginEnd * t + span * (1 - t);
		}
	}
	endTime = cur;
}

char* PerformanceMonitor::getStringPercents()
{
	if (avgBeginEnd == 0.0 || avgEndBegin == 0.0)
	{
		this->output[0] = 0;
		return this->output;
	}
	snprintf(this->output, 1000, "%s: %.01f ms (%.01f%%)",this->title, this->avgBeginEnd/1000 , this->avgBeginEnd/(this->avgBeginEnd + this->avgEndBegin)*100);
	return this->output;
}

char* PerformanceMonitor::getStringFPS()
{
	if (avgBeginEnd == 0.0 || avgEndBegin == 0.0)
	{
		this->output[0] = 0;
		return this->output;
	}
	snprintf(this->output, 1000, "%s: %.01f FPS (%.01f ms)", this->title, 1e6 / this->avgBeginEnd , this->avgBeginEnd / 1000);
	return this->output;
}
