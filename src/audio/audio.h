//
// Created by Michael Ankele on 2024-10-01.
//

#ifndef AUDIO_AUDIO_H
#define AUDIO_AUDIO_H

#include "../lib/base/base.h"

class vec3;
class quaternion;

namespace audio {

extern float VolumeMusic, VolumeSound;


void init();
void exit();
void iterate(float dt);
void set_listener(const vec3 &pos, const quaternion &ang, const vec3 &vel, float v_sound);
void reset();
void clear_small_cache();



class AudioBuffer {
public:
	int channels, bits, samples, freq;
	bytes buffer;
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

};



#endif //AUDIO_AUDIO_H
