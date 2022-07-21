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
#include "../y/EngineData.h"
#include "../lib/math/vector.h"
#include "../lib/math/quaternion.h"
#include "../lib/os/file.h"
#include "../lib/os/msg.h"
 
#if HAS_LIB_OPENAL


#ifdef OS_WINDOWS
	#include <AL/al.h>
	//#include <alut.h>
	#include <AL/alc.h>
	//#pragma comment(lib,"alut.lib")
	#pragma comment(lib,"OpenAL32.lib")
	/*#pragma comment(lib,"libogg.lib")
	#pragma comment(lib,"libvorbis.lib")
	#pragma comment(lib,"libvorbisfile.lib")*/
#else
	#include <AL/al.h>
	//#include <AL/alut.h>
	#include <AL/alc.h>
#endif

namespace audio {

//extern Array<Sound*> Sounds;
//extern Array<Music*> Musics;


struct SmallAudio {
	unsigned int al_buffer;
	Path filename;
	int ref_count;
};
Array<SmallAudio> small_audio_cache;

AudioFile load_sound_file(const Path &filename);
AudioStream load_sound_start(const Path &filename);
void load_sound_step(AudioStream *as);
void load_sound_end(AudioStream *as);


ALCdevice *al_dev = NULL;
ALCcontext *al_context = NULL;

void init() {
	al_dev = alcOpenDevice(NULL);
	if (al_dev) {
		al_context = alcCreateContext(al_dev, NULL);
		if (al_context)
			alcMakeContextCurrent(al_context);
		else
			throw Exception("could not create openal context");
	} else {
		throw Exception("could not open openal device");
	}
	bool ok = (al_context);

	
	//bool ok = alutInit(NULL, 0);
	if (!ok)
		throw Exception("sound init (openal)");
}

void exit() {
	reset();
	if (al_context)
		alcDestroyContext(al_context);
	al_context = NULL;
	if (al_dev)
		alcCloseDevice(al_dev);
	al_dev = NULL;
//	alutExit();
}

Sound *Sound::load(const Path &filename) {
	int id = -1;

	// cached?
	int cached = -1;
	for (int i=0;i<small_audio_cache.num;i++)
		if (small_audio_cache[i].filename == filename){
			small_audio_cache[i].ref_count ++;
			cached = i;
			break;
		}

	// no -> load from file
	AudioFile af = {0,0,0,0, nullptr};
	if (cached < 0)
		af = load_sound_file(engine.sound_dir << filename);

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


Sound *Sound::emit(const Path &filename, const vector &pos, float min_dist, float max_dist, float speed, float volume, bool loop) {
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

void Sound::set_data(const vector &_pos, const vector &_vel, float min_dist, float max_dist, float _speed, float _volume) {
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

void set_listener(const vector &pos, const quaternion &ang, const vector &vel, float v_sound) {
	ALfloat ListenerOri[6];
	vector dir = ang * vector::EZ;
	ListenerOri[0] = dir.x;
	ListenerOri[1] = dir.y;
	ListenerOri[2] = dir.z;
	vector up = ang * vector::EY;
	ListenerOri[3] = up.x;
	ListenerOri[4] = up.y;
	ListenerOri[5] = up.z;
	alListener3f(AL_POSITION,    pos.x, pos.y, pos.z);
	alListener3f(AL_VELOCITY,    vel.x, vel.y, vel.z);
	alListenerfv(AL_ORIENTATION, ListenerOri);
	alSpeedOfSound(v_sound);
}

bool AudioStream::stream(int buf) {
	if (state != AudioStream::State::READY)
		return false;
	load_sound_step(this);
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
	msg_write("loading sound " + filename.str());
	int id = -1;
	auto as = load_sound_start(engine.sound_dir << filename);

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
	load_sound_end(&stream);
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

#else


namespace audio {

void init(){}
void exit(){}
Sound* Sound::load(const Path &filename){ return nullptr; }
Sound* Sound::emit(const Path &filename, const vector &pos, float min_dist, float max_dist, float speed, float volume, bool loop){ return nullptr; }
Sound::Sound() : BaseClass(BaseClass::Type::SOUND) {}
Sound::~Sound(){}
void Sound::__delete__(){}
void SoundClearSmallCache(){}
void Sound::play(bool repeat){}
void Sound::stop(){}
void Sound::pause(bool pause){}
bool Sound::is_playing(){ return false; }
bool Sound::has_ended(){ return false; }
void Sound::set_data(const vector &pos, const vector &vel, float min_dist, float max_dist, float speed, float volume){}
void set_listener(const vector& pos, const quaternion& ang, const vector& vel, float v_sound) {}
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


