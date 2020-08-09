/*
 * PerformanceMonitor.cpp
 *
 *  Created on: 07.01.2020
 *      Author: michi
 */

#include "PerformanceMonitor.h"

PerformanceMonitor::PerformanceMonitor() {
	reset();
	just_cleared = true;
	frame_dt = 0;
}

void PerformanceMonitor::reset() {
	temp.frame_time = 0;
	for (int i=0; i<NUM_LOCATIONS; i++)
		temp.location[i] = 0;
	frames = 0;
}

void PerformanceMonitor::tick(PMLabel label) {
	auto now = std::chrono::high_resolution_clock::now();
	float dt = std::chrono::duration<float, std::chrono::seconds::period>(now - prev).count();
	temp.location[(int)label] += dt;
	prev = now;
}

void PerformanceMonitor::frame() {
	auto now = std::chrono::high_resolution_clock::now();
	if (!just_cleared) {
		frame_dt = std::chrono::duration<float, std::chrono::seconds::period>(now - prev_frame).count();
		temp.frame_time += frame_dt;
	}
	prev_frame = now;
	frames ++;
	just_cleared = false;

	if (temp.frame_time > 0.2f) {
		avg.frame_time = temp.frame_time / frames;
		for (int i=0; i<NUM_LOCATIONS; i++)
			avg.location[i] = temp.location[i] / frames;
		reset();
	}
}

