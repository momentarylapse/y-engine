/*
 * PerformanceMonitor.cpp
 *
 *  Created on: 07.01.2020
 *      Author: michi
 */

#include "PerformanceMonitor.h"


int PerformanceMonitor::frames = -1;
bool PerformanceMonitor::just_cleared = true;
std::chrono::high_resolution_clock::time_point PerformanceMonitor::prev_frame;
float PerformanceMonitor::temp_frame_time = 0;
float PerformanceMonitor::avg_frame_time = 0;
float PerformanceMonitor::frame_dt = 0;



Array<PerformanceChannel> PerformanceMonitor::channels;

void PerformanceMonitor::_reset() {
	temp_frame_time = 0;
	for (auto &c: channels) {
		c.dt = 0;
		c.count = 0;
	}
	frames = 0;
}

int PerformanceMonitor::create_channel(const string &name, PerformanceChannel::Group group) {
	channels.add({name, group});
	return channels.num - 1;
}

void PerformanceMonitor::begin(int channel) {
	channels[channel].prev = std::chrono::high_resolution_clock::now();
}

void PerformanceMonitor::end(int channel) {
	auto now = std::chrono::high_resolution_clock::now();
	channels[channel].dt += std::chrono::duration<float, std::chrono::seconds::period>(now - channels[channel].prev).count();
	channels[channel].count ++;
}

void PerformanceMonitor::next_frame() {
	auto now = std::chrono::high_resolution_clock::now();
	if (!just_cleared) {
		frame_dt = std::chrono::duration<float, std::chrono::seconds::period>(now - prev_frame).count();
		temp_frame_time += frame_dt;
	}
	prev_frame = now;
	frames ++;
	just_cleared = false;

	if (temp_frame_time > 0.2f) {
		avg_frame_time = temp_frame_time / frames;
		for (auto &c: channels)
			c.average = c.dt / frames;
		_reset();
	}
}
