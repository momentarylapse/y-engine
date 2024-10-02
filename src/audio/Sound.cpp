/*----------------------------------------------------------------------------*\
| Nix sound                                                                    |
| -> sound emitting and music playback                                         |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.11.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "Sound.h"
#include "Loading.h"
#include "../lib/os/file.h"
#include "../lib/os/msg.h"

#if HAS_LIB_OPENAL

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <al.h>
#include <alc.h>


namespace audio {





xfer<Sound> load_sound(const Path &filename) {
	auto buffer = load_buffer(filename);
	Sound *s = new Sound(buffer);
	return s;
}

xfer<Sound> create_sound(const Array<float>& samples) {
	auto buffer = create_buffer(samples);
	Sound *s = new Sound(buffer);
	return s;
}

xfer<Sound> emit_sound(const Path &filename, const vec3 &pos, float min_dist, float max_dist, float speed, float volume, bool loop) {
	Sound *s = load_sound(filename);
	s->suicidal = true;
	s->set_data(pos, v_0, min_dist, max_dist, speed, volume);
	s->play(loop);
	return s;
}

xfer<Sound> emit_sound_buffer(const Array<float>& buffer, const vec3 &pos, float min_dist, float max_dist, float speed, float volume, bool loop) {
	Sound *s = create_sound(buffer);
	s->suicidal = true;
	s->set_data(pos, v_0, min_dist, max_dist, speed, volume);
	s->play(loop);
	return s;
}

Sound::Sound(AudioBuffer* _buffer) : BaseClass(BaseClass::Type::SOUND) {
	buffer = _buffer;
	buffer->ref_count ++;
	suicidal = false;
	pos = v_0;
	vel = v_0;
	volume = 1;
	speed = 1;
	al_source = 0;
	loop = false;



	alGenSources(1, &al_source);

	// set up al-source
	alSourcei (al_source, AL_BUFFER,   buffer->al_buffer);
	alSourcef (al_source, AL_PITCH,    speed);
	alSourcef (al_source, AL_GAIN,     volume * VolumeSound);
	alSource3f(al_source, AL_POSITION, -pos.x, pos.y, pos.z);
	alSource3f(al_source, AL_VELOCITY, -vel.x, vel.y, vel.z);
	alSourcei (al_source, AL_LOOPING,  loop);
}

Sound::~Sound() {
	stop();
	buffer->ref_count --;
	//alDeleteBuffers(1, &al_buffer);
	alDeleteSources(1, &al_source);
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
	alSource3f(al_source, AL_POSITION, -pos.x, pos.y, pos.z);
	alSource3f(al_source, AL_VELOCITY, -vel.x, vel.y, vel.z);
	//alSourcei (al_source, AL_LOOPING,  false);
	alSourcef (al_source, AL_REFERENCE_DISTANCE, min_dist);
	alSourcef (al_source, AL_MAX_DISTANCE, max_dist);
	alSourcef (al_source, AL_ROLLOFF_FACTOR, 1.0f);
}


}

#pragma GCC diagnostic pop

#else


namespace audio {

xfer<Sound> load_sound(const Path &filename){ return nullptr; }
xfer<Sound> emit_sound(const Path &filename, const vec3 &pos, float min_dist, float max_dist, float speed, float volume, bool loop){ return nullptr; }
Sound::Sound(AudioBuffer*) : BaseClass(BaseClass::Type::SOUND) {}
Sound::~Sound() = default;
void SoundClearSmallCache(){}
void Sound::play(bool repeat){}
void Sound::stop(){}
void Sound::pause(bool pause){}
bool Sound::is_playing(){ return false; }
bool Sound::has_ended(){ return false; }
void Sound::set_data(const vec3 &pos, const vec3 &vel, float min_dist, float max_dist, float speed, float volume){}

void clear_small_cache() {}

}

#endif
