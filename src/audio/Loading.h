//
// Created by Michael Ankele on 2024-10-01.
//

#ifndef AUDIO_LOADING_H
#define AUDIO_LOADING_H

#include "audio.h"

class Path;

namespace audio {

class AudioBuffer;

AudioBuffer load_buffer(const Path& filename);
AudioStream load_stream_start(const Path& filename);
void load_stream_step(AudioStream* as);
void load_stream_end(AudioStream* as);

}

#endif //AUDIO_LOADING_H
