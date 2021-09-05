/*
 * RenderPath.h
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#pragma once

#include "../lib/math/matrix.h"
#include "../lib/image/color.h"

#if HAS_LIB_VULKAN
namespace vulkan {
	class Shader;
	class Pipeline;
	class CommandBuffer;
	class VertexBuffer;
	class DescriptorSet;
	class UniformBuffer;
	class Texture;
}
#endif
class Renderer;
class RendererVulkan;
class ShadowMapRenderer;
class PerformanceMonitor;
class World;
class Camera;
class matrix;
class vector;
class quaternion;

namespace nix {
	class Shader;
}

matrix mtr(const vector &t, const quaternion &a);



struct UBOMatrices {
	alignas(16) matrix model;
	alignas(16) matrix view;
	alignas(16) matrix proj;
};

struct UBOFog {
	alignas(16) color col;
	alignas(16) float distance;
};


enum class RenderPathType {
	NONE,
	FORWARD,
	DEFERRED
};

class RenderPath {
public:
	RenderPath();
	virtual ~RenderPath() {}
	virtual void draw() = 0;
	virtual void start_frame() = 0;
	virtual void end_frame() = 0;

	nix::Shader *shader_2d = nullptr;

	// dynamic resolution scaling
	float resolution_scale_x = 1.0f;
	float resolution_scale_y = 1.0f;

	int ch_render= -1;
	int ch_gui = -1, ch_out = -1, ch_end = -1;
	int ch_post = -1, ch_post_blur = -1, ch_post_focus = -1;
	int ch_pre = -1, ch_bg = -1, ch_fx = -1, ch_world = -1, ch_prepare_lights = -1, ch_shadow = -1;

	RenderPathType type = RenderPathType::NONE;
};


