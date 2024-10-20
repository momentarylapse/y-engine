//
// Created by Michael Ankele on 2024-10-01.
//

#ifndef AUDIO_AUDIO_H
#define AUDIO_AUDIO_H

#include "../lib/base/base.h"

class vec3;
class quaternion;
class Path;
class Entity;

namespace audio {

struct AudioBuffer;

extern float VolumeMusic, VolumeSound;


void init();
void exit();
void attach_listener(Entity* e);
void iterate(float dt);
void reset();




struct  RawAudioStream {
	int channels, bits, samples, freq;
	bytes buffer;
	int buf_samples;
	void *vf;
	int type;

	enum class State {
		ERROR,
		READY,
		END
	} state;

	bool stream(unsigned int buf);
};

class AudioStream {
public:
	unsigned int al_buffer[2] = {0, 0};
	RawAudioStream raw;

	AudioStream();
	~AudioStream();
};

AudioStream* load_stream(const Path& filename);

class SoundSource;

// TODO move to World?
SoundSource& emit_sound(AudioBuffer* buffer, const vec3 &pos, float radius1);
SoundSource& emit_sound_file(const Path &filename, const vec3 &pos, float radius1);
SoundSource& emit_sound_stream(AudioStream* stream, const vec3 &pos, float radius1);

};



#endif //AUDIO_AUDIO_H
