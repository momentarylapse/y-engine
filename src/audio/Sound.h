#pragma once

#include "../lib/config.h"
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

	unsigned int al_source, al_buffer;

	Sound();
	~Sound() override;

	void _cdecl __delete__() override;
	void _cdecl play(bool loop);
	void _cdecl stop();
	void _cdecl pause(bool pause);
	bool _cdecl is_playing();
	bool _cdecl has_ended();
	void _cdecl set_data(const vec3 &pos, const vec3 &vel, float min_dist, float max_dist, float speed, float volume);


	static xfer<Sound> _cdecl load(const Path &filename);
	static xfer<Sound> _cdecl emit(const Path &filename, const vec3 &pos, float min_dist, float max_dist, float speed, float volume, bool loop);
};

class AudioFile {
public:
	int channels, bits, samples, freq;
	char *buffer;
};

class AudioStream {
public:
	int channels, bits, samples, freq;
	char *buffer;
	int buf_samples;
	void *vf;
	int type;

	enum class State {
		ERROR,
		READY,
		END
	} state;

	bool stream(int buf);
};

class Music {
public:
	float volume, speed;

	unsigned int al_source, al_buffer[2];
	AudioStream stream;

	Music();
	~Music();
	void _cdecl __delete__();
	void _cdecl play(bool loop);
	void _cdecl set_rate(float rate);
	void _cdecl stop();
	void _cdecl pause(bool pause);
	bool _cdecl is_playing();
	bool _cdecl has_ended();

	void iterate();


	static Music* _cdecl load(const Path &filename);
};

extern float VolumeMusic, VolumeSound;


void init();
void exit();
void calc_move();
void _cdecl set_listener(const vec3 &pos, const quaternion &ang, const vec3 &vel, float v_sound);
void reset();
void clear_small_cache();


// writing
//void _cdecl SoundSaveFile(const string &filename, const Array<float> &data_r, const Array<float> &data_l, int freq, int channels, int bits);

}

