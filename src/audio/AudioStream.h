#ifndef AUDIO_AUDIOSTREAM_H
#define AUDIO_AUDIOSTREAM_H

#include <lib/base/base.h>

class Path;

namespace audio {

struct AudioStream {
	unsigned int al_buffer[2] = {0, 0};

	AudioStream();
	virtual ~AudioStream();
	virtual bool stream(unsigned int buf) = 0;
};

AudioStream* load_stream(const Path& filename);

}

#endif //AUDIO_AUDIOSTREAM_H
