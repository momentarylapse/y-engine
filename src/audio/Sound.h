#pragma once

#include "../lib/config.h"
#include "../lib/file/file.h"
#include "../lib/math/vector.h"
#include "../y/Entity.h"

namespace audio {

class Sound : public Entity {
public:
	bool loop, suicidal;
	vector pos, vel;
	float volume, speed;

	unsigned int al_source, al_buffer;

	Sound();
	~Sound() override;

	void _cdecl __delete__();
	void _cdecl play(bool loop);
	void _cdecl stop();
	void _cdecl pause(bool pause);
	bool _cdecl is_playing();
	bool _cdecl has_ended();
	void _cdecl set_data(const vector &pos, const vector &vel, float min_dist, float max_dist, float speed, float volume);


	static Sound *_cdecl load(const Path &filename);
	static Sound *_cdecl emit(const Path &filename, const vector &pos, float min_dist, float max_dist, float speed, float volume, bool loop);
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
void _cdecl set_listener(const vector &pos, const quaternion &ang, const vector &vel, float v_sound);
void reset();
void clear_small_cache();


// writing
//void _cdecl SoundSaveFile(const string &filename, const Array<float> &data_r, const Array<float> &data_l, int freq, int channels, int bits);

}

