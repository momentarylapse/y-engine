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
#include "../y/Entity.h"
#include "../world/World.h" // FIXME
#include "../lib/os/file.h"
#include "../lib/os/msg.h"

#if HAS_LIB_OPENAL

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <al.h>
#include <alc.h>


namespace audio {


const kaba::Class *Sound::_class = nullptr;


Sound& emit_sound_file(const Path &filename, const vec3 &pos) {
	auto e = world.create_entity(pos, quaternion::ID);
	auto s = e->add_component<Sound>();
	s->set_buffer(load_buffer(filename));
	s->suicidal = true;
	s->play(false);
	return *s;
}

Sound& emit_sound_buffer(const Array<float>& samples, float sample_rate, const vec3 &pos) {
	auto e = world.create_entity(pos, quaternion::ID);
	auto s = e->add_component<Sound>();
	s->set_buffer(create_buffer(samples, sample_rate));
	s->suicidal = true;
	s->play(false);
	return *s;
}

Sound::Sound() {
	component_type = _class;
	buffer = nullptr;
	suicidal = false;
	volume = 1;
	speed = 1;
	al_source = 0;
	loop = false;
	min_distance = 100;
	max_distance = 10000;

	alGenSources(1, &al_source);
}

Sound::~Sound() {
	stop();
	set_buffer(nullptr);
	//alDeleteBuffers(1, &al_buffer);
	alDeleteSources(1, &al_source);
}

void Sound::set_buffer(AudioBuffer* _buffer) {
	if (buffer) {
		buffer->ref_count --;
	}
	buffer = _buffer;
	if (buffer) {
		buffer->ref_count ++;
		alSourcei (al_source, AL_BUFFER, buffer->al_buffer);
	}
}

void Sound::_apply_data() {
	alSourcef (al_source, AL_PITCH,    speed);
	alSourcef (al_source, AL_GAIN,     volume * VolumeSound);
	alSource3f(al_source, AL_POSITION, -owner->pos.x, owner->pos.y, owner->pos.z);
//	alSource3f(al_source, AL_VELOCITY, -vel.x, vel.y, vel.z);
	alSourcei (al_source, AL_LOOPING,  loop);
	alSourcef (al_source, AL_REFERENCE_DISTANCE, min_distance);
	alSourcef (al_source, AL_MAX_DISTANCE, max_distance);
	alSourcef (al_source, AL_ROLLOFF_FACTOR, 1.0f);
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

bool Sound::is_playing() const {
	int state;
	alGetSourcei(al_source, AL_SOURCE_STATE, &state);
	return (state == AL_PLAYING);
}

bool Sound::has_ended() const {
	return !is_playing(); // TODO... (paused...)
}


}

#pragma GCC diagnostic pop

#else


namespace audio {

const kaba::Class *Sound::_class = nullptr;

xfer<Sound> load_sound(const Path &filename){ return nullptr; }
xfer<Sound> emit_sound(const Path &filename, const vec3 &pos, float min_dist, float max_dist, float speed, float volume, bool loop){ return nullptr; }
Sound::Sound() {}
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
