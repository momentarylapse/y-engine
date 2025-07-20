/*
 * Profiler.h
 *
 *  Created on: 07.01.2020
 *      Author: michi
 */

#pragma once

#include <chrono>
#include "../base/base.h"

namespace profiler {
	struct Channel {
		string name;
		int parent = -1;
		std::chrono::high_resolution_clock::time_point prev;
		float dt = 0, average = 0;
		int count = 0;
	};

	struct TimingData {
		int channel;
		float offset;
	};

	struct FrameTimingData {
		Array<TimingData> cpu0;
		Array<TimingData> gpu;
		float total_time;
	};

	class Profiler {
	public:
		//Profiler();
	};

	int create_channel(const string &name, int parent = -1);
	string get_name(int channel);

	void begin(int channel);
	void end(int channel);

	void next_frame();
	void _reset();


	extern int frames;
	extern bool just_cleared;
	extern std::chrono::high_resolution_clock::time_point frame_start;

	extern float temp_frame_time;
	extern float avg_frame_time;

	extern Array<Channel> channels;
	extern FrameTimingData current_frame_timing;
	extern FrameTimingData previous_frame_timing;

	extern float frame_dt;
}

