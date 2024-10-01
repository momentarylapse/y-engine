//
// Created by Michael Ankele on 2024-10-01.
//

#include "audio.h"
#include "../lib/base/base.h"

namespace audio {

#if HAS_LIB_OPENAL

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <al.h>
//#include <AL/alut.h>
#include <alc.h>


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
	bool ok = (al_context);


	//bool ok = alutInit(nullptr, 0);
	if (!ok)
		throw Exception("sound init (openal)");
}

void exit() {
	reset();
	if (al_context)
		alcDestroyContext(al_context);
	al_context = nullptr;
	if (al_dev)
		alcCloseDevice(al_dev);
	al_dev = nullptr;
	//	alutExit();
}


#else

void init() {}
void exit() {}

#endif

}
