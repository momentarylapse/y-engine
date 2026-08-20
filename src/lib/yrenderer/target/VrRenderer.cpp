//
// Created by michi on 8/17/26.
//

#include "VrRenderer.h"
#if __has_include(<lib/vr/vr.h>)
#include <cmath>
#include "../Context.h"
#include <lib/ygraphics/graphics-impl.h>
#include <lib/profiler/Profiler.h>
#include <lib/os/msg.h>
#include <lib/math/rect.h>
#include <openxr/openxr.h>
#include <lib/vr/vr.h>

namespace vr {
	extern bool m_sessionRunning;
	extern int viewWidth, viewHeight;
extern Array<XrView> cur_views;
}

namespace yrenderer {


VrRenderer::VrRenderer(Context* ctx) :
		TargetRenderer(ctx, "vr") {
	if (ctx) {
		device = ctx->device;
		in_flight_fence = new vulkan::Fence(vulkan::default_device);
		command_buffer = new vulkan::CommandBuffer(vulkan::default_device->command_pool);
		render_pass = new vulkan::RenderPass({vr::instance->views[0].textures[0]->image.format, vr::instance->views[0].depth_buffers[0]->image.format}, {});
	}
	gamma_correction = true;
}

RenderParams VrRenderer::create_params(float aspect_ratio) {
	auto p = RenderParams::into_window(vr::instance->views[current_view_index].framebuffer.get(), aspect_ratio);
	p.command_buffer = command_buffer.get();
	p.render_pass = render_pass.get();
	p.area = rect(0, (float)vr::viewWidth, 0, (float)vr::viewHeight);
	return p;
}

bool VrRenderer::start_frame() {
	if (!vr::m_sessionRunning)
		return false;
	if (!vr::render_frame_start()) {
		vr::render_frame_end();
		return false;
	}
	if (!vr::render_layer_start())
		return false;
	return true;
}

void VrRenderer::end_frame() {
	vr::render_layer_end();
	vr::render_frame_end();
}

void VrRenderer::start_view(int index) {
	current_view_index = index;
	vr::instance->start_view(index, render_pass.get());
	in_flight_fence->wait();
	in_flight_fence->reset();

	eye_pos = *(vec3*)&vr::cur_views[index].pose.position;
	eye_pos.z = -eye_pos.z;
	eye_ang = *(quaternion*)&vr::cur_views[index].pose.orientation;
	eye_ang.x = -eye_ang.x;
	eye_ang.y = -eye_ang.y;
	auto f = vr::cur_views[index].fov;
	eye_fov = rect(tanf(f.angleLeft), tanf(f.angleRight), tanf(f.angleDown), tanf(f.angleUp));

	command_buffer->begin();
}

void VrRenderer::end_view() {
	command_buffer->end();
	vulkan::default_device->graphics_queue.submit(command_buffer.get(), {}, {}, in_flight_fence.get());

	vr::instance->end_view(current_view_index);
}

void VrRenderer::prepare(const RenderParams &params) {

}

void VrRenderer::draw(const RenderParams &params) {
	profiler::begin(channel);
	auto cb = params.command_buffer;
	auto rp = params.render_pass;
	auto fb = params.frame_buffer;

	//cb->begin();
	for (auto c: children)
		c->prepare(params);
	//msg_write(fb->area().str());

	cb->begin_render_pass(rp, fb);
	cb->set_viewport(fb->area());

//	cb->clear(fb->area(), {White}, 0);

	for (auto c: children)
		c->draw(params);

	cb->end_render_pass();
	//cb->end();
	profiler::end(channel);

}
}

#endif
