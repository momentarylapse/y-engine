#include "AudioStream.h"
#include "Loading.h"

#if HAS_LIB_OPENAL

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <al.h>
#include <alc.h>

#endif

namespace audio {


AudioStream::AudioStream() {
#if HAS_LIB_OPENAL
	alGenBuffers(2, al_buffer);
#endif
}

AudioStream::~AudioStream() {
#if HAS_LIB_OPENAL
	alDeleteBuffers(2, al_buffer);
#endif
}

bool AudioStreamFile::stream(unsigned int buf) {
	if (state != AudioStreamFile::State::READY)
		return false;
	step();
	if (channels == 2) {
		if (bits == 8)
			alBufferData(buf, AL_FORMAT_STEREO8, &buffer[0], buf_samples * 2, freq);
		else if (bits == 16)
			alBufferData(buf, AL_FORMAT_STEREO16, &buffer[0], buf_samples * 4, freq);
	} else {
		if (bits == 8)
			alBufferData(buf, AL_FORMAT_MONO8, &buffer[0], buf_samples, freq);
		else if (bits == 16)
			alBufferData(buf, AL_FORMAT_MONO16, &buffer[0], buf_samples * 2, freq);
	}
	return true;
}

AudioStream* load_stream(const Path& filename) {
	return load_stream_start(filename);
}

}
