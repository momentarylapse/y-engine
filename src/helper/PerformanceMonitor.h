/*
 * PerformanceMonitor.h
 *
 *  Created on: 07.01.2020
 *      Author: michi
 */

#pragma once

#include <chrono>
#include "../lib/base/base.h"

/*enum class PMLabel {
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
};*/


struct PerformanceChannel {
	enum class Group {
		RENDER,
		ITERATE,
		INPUT
	};

	string name;
	Group group;
	std::chrono::high_resolution_clock::time_point prev;
	float dt = 0, average = 0;
	int count = 0;
};

class PerformanceMonitor {
public:
	//PerformanceMonitor();

	static Array<PerformanceChannel> channels;
	static int create_channel(const string &name, PerformanceChannel::Group group);
	static void begin(int channel);
	static void end(int channel);

	static void next_frame();
	static void _reset();


	//static void _reset_old();
	//static const int NUM_LOCATIONS = 16;

	static int frames;
	static bool just_cleared;
	static std::chrono::high_resolution_clock::time_point prev_frame;

	static float temp_frame_time;
	static float avg_frame_time;

	/*struct {
		float frame_time;
		float location[NUM_LOCATIONS];
	} static temp;

	struct {
		float frame_time;
		float location[NUM_LOCATIONS];
	} static avg;*/
	static float frame_dt;


	//static void frame();
	//static void tick(PMLabel label);
};

