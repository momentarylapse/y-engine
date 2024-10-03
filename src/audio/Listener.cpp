#include "Listener.h"
#include "../y/Entity.h"

#if HAS_LIB_OPENAL

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <al.h>
#include <alc.h>


namespace audio {

const kaba::Class *Listener::_class = nullptr;

Listener::Listener() {
	component_type = _class;
}

void Listener::apply_data() {
	ALfloat ListenerOri[6];
	vec3 dir = owner->ang * vec3::EZ;
	ListenerOri[0] = -dir.x;
	ListenerOri[1] = dir.y;
	ListenerOri[2] = dir.z;
	vec3 up = owner->ang * vec3::EY;
	ListenerOri[3] = -up.x;
	ListenerOri[4] = up.y;
	ListenerOri[5] = up.z;
	alListener3f(AL_POSITION,    -owner->pos.x, owner->pos.y, owner->pos.z);
	//alListener3f(AL_VELOCITY,    -vel.x, vel.y, vel.z);
	alListenerfv(AL_ORIENTATION, ListenerOri);
	//alSpeedOfSound(v_sound);
	alSpeedOfSound(100000);
}

} // audio

#else

namespace audio {
const kaba::Class *Listener::_class = nullptr;
Listener::Listener() {}
void Listener::apply_data() {}

} // audio

#endif
