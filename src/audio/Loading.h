//
// Created by Michael Ankele on 2024-10-01.
//

#ifndef AUDIO_LOADING_H
#define AUDIO_LOADING_H

#include "audio.h"

class Path;

namespace audio {

struct RawAudioBuffer;
struct  RawAudioStream;


RawAudioBuffer load_raw_buffer(const Path& filename);
RawAudioStream load_stream_start(const Path& filename);
void load_stream_step(RawAudioStream* as);
void load_stream_end(RawAudioStream* as);


// writing
//void _cdecl SoundSaveFile(const string &filename, const Array<float> &data_r, const Array<float> &data_l, int freq, int channels, int bits);


}

#endif //AUDIO_LOADING_H
