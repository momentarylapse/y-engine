#pragma once

#include "audio.h"
#include "../lib/math/vec3.h"
#include "../lib/base/pointer.h"
#include "../y/Component.h"

class Path;

namespace audio {

class Sound : public Component {
public:
	bool loop, suicidal;
	float volume, speed;
	float min_distance, max_distance;

	AudioBuffer* buffer;

	unsigned int al_source;

	Sound();
	~Sound() override;

	void set_buffer(AudioBuffer* buffer);
	void set_buffer_from_file(const Path& filename);
	void set_buffer_from_samples(const Array<float>& samples);

	void play(bool loop);
	void stop();
	void pause(bool pause);
	bool is_playing() const;
	bool has_ended() const;

	void _apply_data();

	static const kaba::Class *_class;
};

Sound& emit_sound_file(const Path &filename, const vec3 &pos);
Sound& emit_sound_buffer(const Array<float>& buffer, float sample_rate, const vec3 &pos);

}

