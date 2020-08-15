/*
 * PerformanceMonitor.h
 *
 *  Created on: 07.01.2020
 *      Author: michi
 */

#ifndef SRC_HELPER_PERFORMANCEMONITOR_H_
#define SRC_HELPER_PERFORMANCEMONITOR_H_

#include <chrono>

enum class PMLabel {
	UNKNOWN,
	PRE,
	OUT,
	END,
	PREPARE_LIGHTS,
	SHADOWS,
	WORLD,
	PARTICLES,
	SKYBOXES,
	GUI,
	ITERATE,
	ANIMATION,
};

class PerformanceMonitor {
public:
	PerformanceMonitor();
	void reset();

	static const int NUM_LOCATIONS = 16;

	int frames = -1;
	bool just_cleared = true;
	std::chrono::high_resolution_clock::time_point prev;
	std::chrono::high_resolution_clock::time_point prev_frame;
	std::chrono::high_resolution_clock::time_point prev_notify;

	struct {
		float frame_time;
		float location[NUM_LOCATIONS];
	} temp;

	struct {
		float frame_time;
		float location[NUM_LOCATIONS];
	} avg;
	float frame_dt = 0;


	void frame();
	void tick(PMLabel label);
};

#endif /* SRC_HELPER_PERFORMANCEMONITOR_H_ */
