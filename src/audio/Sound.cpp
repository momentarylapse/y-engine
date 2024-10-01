/*----------------------------------------------------------------------------*\
| Nix sound                                                                    |
| -> sound emitting and music playback                                         |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.11.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "Sound.h"
#include "audio.h"
#include "../y/EngineData.h"
#include "../lib/math/math.h"
#include "../lib/os/file.h"
#include "../lib/os/msg.h"
 

#ifdef HAS_LIB_OGG
	#include <vorbis/codec.h>
	#include <vorbis/vorbisfile.h>
	#include <vorbis/vorbisenc.h>
#endif

namespace audio {

//Array<Sound*> Sounds;
//Array<Music*> Musics;

float VolumeMusic = 1.0f, VolumeSound = 1.0f;

#if 0
void SoundCalcMove()
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
	clear_small_cache();
}


}
