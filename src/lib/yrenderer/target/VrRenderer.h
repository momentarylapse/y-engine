//
// Created by michi on 8/17/26.
//

#pragma once
#if __has_include(<lib/vr/vr.h>)

#include "WindowRenderer.h"
#include <lib/math/quaternion.h>

namespace yrenderer {

#ifdef USING_VULKAN

class VrRenderer : public TargetRenderer {
public:
	VrRenderer(Context* ctx);

	bool start_frame();
	void end_frame();
	void start_view(int index);
	void end_view();

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	RenderParams create_params(float aspect_ratio);

	owned<Fence> in_flight_fence;
	owned<ygfx::CommandBuffer> command_buffer;
	owned<vulkan::RenderPass> render_pass;
	Device *device;
	bool gamma_correction;
	int current_view_index = 0;
	vec3 eye_pos;
	quaternion eye_ang;
	rect eye_fov;
};

#endif

}

#endif
