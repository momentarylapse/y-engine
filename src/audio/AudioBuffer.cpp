#include "AudioBuffer.h"
#include "Loading.h"
#include "../lib/base/map.h"
#include "../lib/os/path.h"

#if HAS_LIB_OPENAL

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <al.h>
#include <alc.h>

#endif

namespace audio {

Array<AudioBuffer*> created_audio_buffers;
base::map<Path, AudioBuffer*> loaded_audio_buffers;

AudioBuffer::AudioBuffer() {
#if HAS_LIB_OPENAL
	alGenBuffers(1, &al_buffer);
#endif
}

AudioBuffer::~AudioBuffer() {
#if HAS_LIB_OPENAL
	alDeleteBuffers(1, &al_buffer);
#endif
}

void AudioBuffer::fill(const RawAudioBuffer& buf) {
#if HAS_LIB_OPENAL
	if (buf.bits == 8)
		alBufferData(al_buffer, AL_FORMAT_MONO8, buf.buffer.data, buf.samples, buf.freq);
	else if (buf.bits == 16)
		alBufferData(al_buffer, AL_FORMAT_MONO16, buf.buffer.data, buf.samples * 2, buf.freq);
#endif
}


AudioBuffer* create_buffer(const Array<float>& samples, float sample_rate) {
	auto buffer = new AudioBuffer;

#if HAS_LIB_OPENAL
	Array<short> buf16;
	buf16.resize(samples.num);
	for (int i=0; i<samples.num; i++)
		buf16[i] = (int)(samples[i] * 32768.0f);
	alBufferData(buffer->al_buffer, AL_FORMAT_MONO16, &buf16[0], samples.num * 2, (int)sample_rate);
#endif

	created_audio_buffers.add(buffer);
	return buffer;
}

AudioBuffer* load_buffer(const Path& filename) {
	int i = loaded_audio_buffers.find(filename);
	if (i >= 0)
		return loaded_audio_buffers.by_index(i);

	auto buffer = new AudioBuffer;
	buffer->fill(load_raw_buffer(filename));

	loaded_audio_buffers.set(filename, buffer);
	return buffer;
}
}
