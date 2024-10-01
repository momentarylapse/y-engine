//
// Created by Michael Ankele on 2024-10-01.
//

#ifndef AUDIO_H
#define AUDIO_H

class vec3;
class quaternion;

namespace audio {

extern float VolumeMusic, VolumeSound;


void init();
void exit();
void iterate(float dt);
void set_listener(const vec3 &pos, const quaternion &ang, const vec3 &vel, float v_sound);
void reset();
void clear_small_cache();


};



#endif //AUDIO_H
