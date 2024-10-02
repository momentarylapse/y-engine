//
// Created by Michael Ankele on 2024-10-01.
//

#ifndef AUDIO_AUDIO_H
#define AUDIO_AUDIO_H

#include "../lib/base/base.h"

class vec3;
class quaternion;
class Path;

namespace audio {

extern float VolumeMusic, VolumeSound;


void init();
void exit();
void iterate(float dt);
void set_listener(const vec3 &pos, const quaternion &ang, const vec3 &vel, float v_sound);
void reset();



struct RawAudioBuffer {
	int channels, bits, samples, freq;
	bytes buffer;
};

struct AudioBuffer {
	unsigned int al_buffer = 0;
	int ref_count = 0;
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

AudioBuffer* load_buffer(const Path& filename);
AudioBuffer* create_buffer(const Array<float>& samples);

};



#endif //AUDIO_AUDIO_H
