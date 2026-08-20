#pragma once

#include <lib/base/base.h>
#include <lib/base/pointer.h>
#include <lib/ygraphics/graphics-fwd.h>
#include <lib/math/rect.h>
#include <lib/math/vec3.h>
#include <lib/math/quaternion.h>

namespace yrenderer {
	struct Context;
}

namespace vr {

struct View {
	shared_array<ygfx::Texture> textures;
	shared_array<ygfx::Texture> depth_buffers;
	shared<ygfx::FrameBuffer> framebuffer;
};

struct Controller {
	bool active = false;
	vec3 pos;
	quaternion ang;
};

class Instance {
public:
	void* instance = nullptr;

	Array<View> views;

	yrenderer::Context* create_yrenderer();
	void create_session(yrenderer::Context* ctx);
	void iterate();

	bool start_frame();
	void end_frame();
	void start_view(int index, vulkan::RenderPass* render_pass);
	void end_view(int index);

	int image_index = 0;

	float scale = 1.0f;
	vec3 eye_pos(int index) const;
	quaternion eye_ang(int index) const;
	rect eye_fov(int index) const;
	Controller controllers[2];
};



void init(const string& engine, const string& app_name);
void end();

void* _create_instance(const string& engine, const string& app_name);
void _create_debug_messenger();
void _destroy_debug_messenger();

void CreateSwapchains();
void DestroySwapchains();

void GetViewConfigurationViews();
void GetEnvironmentBlendModes();

void CreateReferenceSpace();
void DestroyReferenceSpace();

void PollEvents();

bool render_frame_start();
void render_frame_end();
bool render_layer_start();
void render_layer_end();

extern Instance* instance;

}

