//
// Created by michi on 12/29/24.
//

#ifndef LIGHTMETER_H
#define LIGHTMETER_H

#include "../Renderer.h"

class ComputeTask;
class Camera;

class LightMeter {
public:
	LightMeter(ResourceManager* resource_manager, Texture* tex, int channel);
	ComputeTask* compute;
	UniformBuffer* params;
	ShaderStorageBuffer* buf;
	Array<int> histogram;
	float brightness;
	int ch_post_brightness = -1;
	void measure(const RenderParams& params, Texture* tex);
	void adjust_camera(Camera* cam);
};



#endif //LIGHTMETER_H
