#include "audio.h"
#include "../lib/base/base.h"
#include "../lib/math/vec3.h"
#include "../lib/math/quaternion.h"

#if HAS_LIB_OPENAL

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <al.h>
#include <alc.h>

namespace audio {

ALCdevice *al_dev = nullptr;
ALCcontext *al_context = nullptr;

void init() {
	al_dev = alcOpenDevice(nullptr);
	if (al_dev) {
		al_context = alcCreateContext(al_dev, nullptr);
		if (al_context)
			alcMakeContextCurrent(al_context);
		else
			throw Exception("could not create openal context");
	} else {
		throw Exception("could not open openal device");
	}
}

void exit() {
	reset();
	if (al_context)
		alcDestroyContext(al_context);
	al_context = nullptr;
	if (al_dev)
		alcCloseDevice(al_dev);
	al_dev = nullptr;
}


//Array<Sound*> Sounds;
//Array<Music*> Musics;

float VolumeMusic = 1.0f, VolumeSound = 1.0f;

#if 0
void iterate(float dt)
{
	for (int i=Sounds.num-1;i>=0;i--)
		if (Sounds[i]->Suicidal)
			if (Sounds[i]->Ended())
				delete(Sounds[i]);
	for (int i=0;i<Musics.num;i++)
		Musics[i]->Iterate();
}
#endif

void reset() {
	/*for (int i=Sounds.num-1;i>=0;i--)
		delete(Sounds[i]);
	Sounds.clear();
	for (int i=Musics.num-1;i>=0;i--)
		delete(Musics[i]);
	Musics.clear();*/
}

void set_listener(const vec3& pos, const quaternion& ang, const vec3& vel, float v_sound) {
	ALfloat ListenerOri[6];
	vec3 dir = ang * vec3::EZ;
	ListenerOri[0] = dir.x;
	ListenerOri[1] = dir.y;
	ListenerOri[2] = dir.z;
	vec3 up = ang * vec3::EY;
	ListenerOri[3] = up.x;
	ListenerOri[4] = up.y;
	ListenerOri[5] = up.z;
	alListener3f(AL_POSITION,    pos.x, pos.y, pos.z);
	alListener3f(AL_VELOCITY,    vel.x, vel.y, vel.z);
	alListenerfv(AL_ORIENTATION, ListenerOri);
	alSpeedOfSound(v_sound);
}

}

#else

namespace audio {
void init() {}
void exit() {}
void set_listener(const vec3& pos, const quaternion& ang, const vec3& vel, float v_sound) {}
}

#endif

