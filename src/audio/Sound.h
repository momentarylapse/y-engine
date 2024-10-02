#pragma once

#include "audio.h"
#include "../lib/math/vec3.h"
#include "../lib/base/pointer.h"
#include "../y/BaseClass.h"

class Path;

namespace audio {

class Sound : public BaseClass {
public:
	bool loop, suicidal;
	vec3 pos, vel;
	float volume, speed;

	AudioBuffer* buffer;

	unsigned int al_source;

	explicit Sound(AudioBuffer* buffer);
	~Sound() override;

	void play(bool loop);
	void stop();
	void pause(bool pause);
	bool is_playing();
	bool has_ended();
	void set_data(const vec3 &pos, const vec3 &vel, float min_dist, float max_dist, float speed, float volume);
};

// TODO use ECS here!

xfer<Sound> load_sound(const Path &filename);
xfer<Sound> emit_sound(const Path &filename, const vec3 &pos, float min_dist, float max_dist, float speed, float volume, bool loop);

xfer<Sound> create_sound(const Array<float>& buffer);
xfer<Sound> emit_sound_buffer(const Array<float>& buffer, const vec3 &pos, float min_dist, float max_dist, float speed, float volume, bool loop);

}

