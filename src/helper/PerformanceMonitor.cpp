/*
 * PerformanceMonitor.cpp
 *
 *  Created on: 07.01.2020
 *      Author: michi
 */

#include "PerformanceMonitor.h"

int PerformanceMonitor::frames = -1;
bool PerformanceMonitor::just_cleared = true;
std::chrono::high_resolution_clock::time_point PerformanceMonitor::frame_start;
float PerformanceMonitor::temp_frame_time = 0;
float PerformanceMonitor::avg_frame_time = 0;
float PerformanceMonitor::frame_dt = 0;



Array<PerformanceChannel> PerformanceMonitor::channels;

Array<TimingData> PerformanceMonitor::current_frame_timing;
Array<TimingData> PerformanceMonitor::previous_frame_timing;

void PerformanceMonitor::_reset() {
	temp_frame_time = 0;
	for (auto &c: channels) {
		c.dt = 0;
		c.count = 0;
	}
	frames = 0;
}

int PerformanceMonitor::create_channel(const string &name, int parent) {
	channels.add({name, parent});
	return channels.num - 1;
}

string PerformanceMonitor::get_name(int channel) {
	return channels[channel].name;
}

void PerformanceMonitor::begin(int channel) {
	auto now = std::chrono::high_resolution_clock::now();
	channels[channel].prev = now;
	current_frame_timing.add({channel, std::chrono::duration<float, std::chrono::seconds::period>(now - frame_start).count()});
}

void PerformanceMonitor::end(int channel) {
	auto now = std::chrono::high_resolution_clock::now();
	channels[channel].dt += std::chrono::duration<float, std::chrono::seconds::period>(now - channels[channel].prev).count();
	channels[channel].count ++;
	current_frame_timing.add({channel | (int)0x80000000, std::chrono::duration<float, std::chrono::seconds::period>(now - frame_start).count()});
}

void PerformanceMonitor::next_frame() {
	auto now = std::chrono::high_resolution_clock::now();

	// frame finished marker
	current_frame_timing.add({-1, std::chrono::duration<float, std::chrono::seconds::period>(now - frame_start).count()});

	if (!just_cleared) {
		frame_dt = std::chrono::duration<float, std::chrono::seconds::period>(now - frame_start).count();
		temp_frame_time += frame_dt;
	}
	frame_start = now;
	frames ++;
	just_cleared = false;

	if (temp_frame_time > 0.2f) {
		avg_frame_time = temp_frame_time / frames;
		for (auto &c: channels)
			c.average = c.dt / frames;
		_reset();
	}

	previous_frame_timing.exchange(current_frame_timing);
	current_frame_timing.clear();
	current_frame_timing.simple_reserve(256);
}
