/*----------------------------------------------------------------------------*\
| Nix sound                                                                    |
| -> sound emitting and music playback                                         |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.11.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

// TODO: cache small buffers...

#include "Sound.h"
#include "audio.h"
#include "Loading.h"
#include "../y/EngineData.h"
#include "../lib/math/vec3.h"
#include "../lib/math/quaternion.h"
#include "../lib/os/file.h"
#include "../lib/os/msg.h"
 
#if HAS_LIB_OPENAL

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <al.h>
//#include <AL/alut.h>
#include <alc.h>

namespace audio {

//extern Array<Sound*> Sounds;
//extern Array<Music*> Musics;


struct SmallAudio {
	unsigned int al_buffer;
	Path filename;
	int ref_count;
};
Array<SmallAudio> small_audio_cache;


xfer<Sound> Sound::load(const Path &filename) {
	// cached?
	int cached = -1;
	for (int i=0;i<small_audio_cache.num;i++)
		if (small_audio_cache[i].filename == filename){
			small_audio_cache[i].ref_count ++;
			cached = i;
			break;
		}

	// no -> load from file
	AudioBuffer af = {};
	if (cached < 0)
		af = load_buffer(engine.sound_dir | filename);

	Sound *s = new Sound;
	
	if (((af.channels == 1) and af.buffer) or (cached >= 0)){
		
		alGenSources(1, &s->al_source);
		if (cached >= 0){
			s->al_buffer = small_audio_cache[cached].al_buffer;
		}else{

			// fill data into al-buffer
			alGenBuffers(1, &s->al_buffer);
			if (af.bits == 8)
				alBufferData(s->al_buffer, AL_FORMAT_MONO8, af.buffer, af.samples, af.freq);
			else if (af.bits == 16)
				alBufferData(s->al_buffer, AL_FORMAT_MONO16, af.buffer, af.samples * 2, af.freq);

			// put into small audio cache
			SmallAudio sa;
			sa.filename = filename;
			sa.al_buffer = s->al_buffer;
			sa.ref_count = 1;
			small_audio_cache.add(sa);
		}

		// set up al-source
		alSourcei (s->al_source, AL_BUFFER,   s->al_buffer);
		alSourcef (s->al_source, AL_PITCH,    s->speed);
		alSourcef (s->al_source, AL_GAIN,     s->volume * VolumeSound);
		alSource3f(s->al_source, AL_POSITION, s->pos.x, s->pos.y, s->pos.z);
		alSource3f(s->al_source, AL_VELOCITY, s->vel.x, s->vel.y, s->vel.z);
		alSourcei (s->al_source, AL_LOOPING,  false);
	}
	if (af.buffer and (cached < 0))
	    delete[](af.buffer);
	return s;
}


xfer<Sound> Sound::emit(const Path &filename, const vec3 &pos, float min_dist, float max_dist, float speed, float volume, bool loop) {
	Sound *s = Sound::load(filename);
	s->suicidal = true;
	s->set_data(pos, v_0, min_dist, max_dist, speed, volume);
	s->play(loop);
	return s;
}

Sound::Sound() : BaseClass(BaseClass::Type::SOUND) {
	suicidal = false;
	pos = v_0;
	vel = v_0;
	volume = 1;
	speed = 1;
	al_source = 0;
	al_buffer = 0;
	loop = false;
}

Sound::~Sound() {
	stop();
	for (int i=0;i<small_audio_cache.num;i++)
		if (al_buffer == small_audio_cache[i].al_buffer)
			small_audio_cache[i].ref_count --;
	//alDeleteBuffers(1, &al_buffer);
	alDeleteSources(1, &al_source);
}

void Sound::__delete__() {
	this->~Sound();
}

void clear_small_cache() {
	for (int i=0; i<small_audio_cache.num; i++)
		alDeleteBuffers(1, &small_audio_cache[i].al_buffer);
	small_audio_cache.clear();
}

void Sound::play(bool loop) {
	alSourcei(al_source, AL_LOOPING, loop);
	alSourcePlay(al_source);
}

void Sound::stop() {
	alSourceStop(al_source);
}

void Sound::pause(bool pause) {
	int state;
	alGetSourcei(al_source, AL_SOURCE_STATE, &state);
	if (pause and (state == AL_PLAYING))
		alSourcePause(al_source);
	else if (!pause and (state == AL_PAUSED))
		alSourcePlay(al_source);
}

bool Sound::is_playing() {
	int state;
	alGetSourcei(al_source, AL_SOURCE_STATE, &state);
	return (state == AL_PLAYING);
}

bool Sound::has_ended() {
	return !is_playing(); // TODO... (paused...)
}

void Sound::set_data(const vec3 &_pos, const vec3 &_vel, float min_dist, float max_dist, float _speed, float _volume) {
	pos = _pos;
	vel = _vel;
	volume = _volume;
	speed = _speed;
	alSourcef (al_source, AL_PITCH,    speed);
	alSourcef (al_source, AL_GAIN,     volume * VolumeSound);
	alSource3f(al_source, AL_POSITION, pos.x, pos.y, pos.z);
	alSource3f(al_source, AL_VELOCITY, vel.x, vel.y, vel.z);
	//alSourcei (al_source, AL_LOOPING,  false);
	alSourcef (al_source, AL_REFERENCE_DISTANCE, min_dist);
	alSourcef (al_source, AL_MAX_DISTANCE, max_dist);
}

bool AudioStream::stream(int buf) {
	if (state != AudioStream::State::READY)
		return false;
	load_stream_step(this);
	if (channels == 2) {
		if (bits == 8)
			alBufferData(buf, AL_FORMAT_STEREO8, buffer, buf_samples * 2, freq);
		else if (bits == 16)
			alBufferData(buf, AL_FORMAT_STEREO16, buffer, buf_samples * 4, freq);
	} else {
		if (bits == 8)
			alBufferData(buf, AL_FORMAT_MONO8, buffer, buf_samples, freq);
		else if (bits == 16)
			alBufferData(buf, AL_FORMAT_MONO16, buffer, buf_samples * 2, freq);
	}
	return true;
}

Music *Music::load(const Path &filename) {
	msg_write("loading sound " + str(filename));
	auto as = load_stream_start(engine.sound_dir | filename);

	Music *m = new Music();

	if (as.state == AudioStream::State::READY) {

		alGenSources(1, &m->al_source);
		alGenBuffers(2, m->al_buffer);
		m->stream = as;

		// start streaming
		int num_buffers = 0;
		if (as.stream(m->al_buffer[0]))
			num_buffers ++;
		if (as.stream(m->al_buffer[1]))
			num_buffers ++;
		alSourceQueueBuffers(m->al_source, num_buffers, m->al_buffer);


		alSourcef(m->al_source, AL_PITCH,           m->speed);
		alSourcef(m->al_source, AL_GAIN,            m->volume * VolumeMusic);
		alSourcei(m->al_source, AL_LOOPING,         false);
		alSourcei(m->al_source, AL_SOURCE_RELATIVE, AL_TRUE);
	}
	return m;
}

Music::Music() {
	volume = 1;
	speed = 1;
	al_source = 0;
	al_buffer[0] = 0;
	al_buffer[1] = 0;
}

Music::~Music() {
	stop();
	alSourceUnqueueBuffers(al_source, 2, al_buffer);
	load_stream_end(&stream);
	alDeleteBuffers(2, al_buffer);
	alDeleteSources(1, &al_source);
}

void Music::__delete__() {
	this->~Music();
}

void Music::play(bool loop) {
	//alSourcei   (al_source, AL_LOOPING, loop);
	alSourcePlay(al_source);
	alSourcef(al_source, AL_GAIN, volume * VolumeMusic);
}

void Music::set_rate(float rate) {
}

void Music::stop() {
	alSourceStop(al_source);
}

void Music::pause(bool pause) {
	int state;
	alGetSourcei(al_source, AL_SOURCE_STATE, &state);
	if (pause and (state == AL_PLAYING))
		alSourcePause(al_source);
	else if (!pause and (state == AL_PAUSED))
		alSourcePlay(al_source);
}

bool Music::is_playing() {
	int state;
	alGetSourcei(al_source, AL_SOURCE_STATE, &state);
	return (state == AL_PLAYING);
}

bool Music::has_ended() {
	return !is_playing();
}

void Music::iterate() {
	alSourcef(al_source, AL_GAIN, volume * VolumeMusic);
	int processed;
	alGetSourcei(al_source, AL_BUFFERS_PROCESSED, &processed);
	while (processed --) {
		ALuint buf;
		alSourceUnqueueBuffers(al_source, 1, &buf);
		if (stream.stream(buf))
			alSourceQueueBuffers(al_source, 1, &buf);
	}
}

}

#pragma GCC diagnostic pop

#else


namespace audio {

xfer<Sound> Sound::load(const Path &filename){ return nullptr; }
xfer<Sound> Sound::emit(const Path &filename, const vec3 &pos, float min_dist, float max_dist, float speed, float volume, bool loop){ return nullptr; }
Sound::Sound() : BaseClass(BaseClass::Type::SOUND) {}
Sound::~Sound(){}
void Sound::__delete__(){}
void SoundClearSmallCache(){}
void Sound::play(bool repeat){}
void Sound::stop(){}
void Sound::pause(bool pause){}
bool Sound::is_playing(){ return false; }
bool Sound::has_ended(){ return false; }
void Sound::set_data(const vec3 &pos, const vec3 &vel, float min_dist, float max_dist, float speed, float volume){}
Music *Music::load(const Path &filename){ return nullptr; }
Music::~Music(){}
void Music::play(bool repeat){}
void Music::set_rate(float rate){}
void Music::stop(){}
void Music::pause(bool pause){}
bool Music::is_playing(){ return false; }
bool Music::has_ended(){ return false; }
void Music::iterate(){}

void clear_small_cache() {}

}

#endif


