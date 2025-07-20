/*
 * Profiler.cpp
 *
 *  Created on: 07.01.2020
 *      Author: michi
 */

#include "Profiler.h"

namespace profiler {
	int frames = -1;
	bool just_cleared = true;
	std::chrono::high_resolution_clock::time_point frame_start;
	float temp_frame_time = 0;
	float avg_frame_time = 0;
	float frame_dt = 0;



	Array<Channel> channels;

	FrameTimingData current_frame_timing;
	FrameTimingData previous_frame_timing;

	void _reset() {
		temp_frame_time = 0;
		for (auto &c: channels) {
			c.dt = 0;
			c.count = 0;
		}
		frames = 0;
	}

	int create_channel(const string &name, int parent) {
		channels.add({name, parent});
		return channels.num - 1;
	}

	string get_name(int channel) {
		return channels[channel].name;
	}

	void begin(int channel) {
		auto now = std::chrono::high_resolution_clock::now();
		channels[channel].prev = now;
		current_frame_timing.cpu0.add({channel, std::chrono::duration<float, std::chrono::seconds::period>(now - frame_start).count()});
	}

	void end(int channel) {
		auto now = std::chrono::high_resolution_clock::now();
		channels[channel].dt += std::chrono::duration<float, std::chrono::seconds::period>(now - channels[channel].prev).count();
		channels[channel].count ++;
		current_frame_timing.cpu0.add({channel | (int)0x80000000, std::chrono::duration<float, std::chrono::seconds::period>(now - frame_start).count()});
	}

	/*void begin_gpu(int channel, float t) {
		current_frame_timing.gpu.add({channel | (int)0x80000000, t});
	}
	void end_gpu(int channel, float t) {
		current_frame_timing.gpu.add({channel, t});
	}*/

	void next_frame() {
		auto now = std::chrono::high_resolution_clock::now();

		current_frame_timing.total_time = std::chrono::duration<float, std::chrono::seconds::period>(now - frame_start).count();

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

		previous_frame_timing = current_frame_timing;
		current_frame_timing.cpu0.clear();
		current_frame_timing.cpu0.simple_reserve(256);
		current_frame_timing.gpu.clear();
		current_frame_timing.gpu.simple_reserve(256);
	}
}
