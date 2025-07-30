//
// Created by michi on 12/29/24.
//

#ifndef LIGHTMETER_H
#define LIGHTMETER_H

#include <lib/yrenderer/Renderer.h>
#include <lib/yrenderer/helper/ComputeTask.h>

class Camera;

class LightMeter : public yrenderer::ComputeTask {
public:
	LightMeter(ResourceManager* resource_manager, ygfx::Texture* tex);
	ygfx::UniformBuffer* params;
	ygfx::ShaderStorageBuffer* buf;
	Array<int> histogram;
	float brightness;
	ygfx::Texture* texture;
	void read();
	void setup();
	void adjust_camera(Camera* cam);

	int ch_prepare;
};



#endif //LIGHTMETER_H
