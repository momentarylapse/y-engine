#include "TextureRendererVulkan.h"
#ifdef USING_VULKAN
#include "../../graphics-impl.h"

TextureRenderer::TextureRenderer(const shared_array<Texture>& tex) : RenderTask("tex") {
	textures = tex; //new Texture(width, height, "bgra:i8");
//	depth_buffer = new DepthBuffer(texture->width, texture->height, "d:f32", false);
	render_pass = new RenderPass(weak(textures));
	frame_buffer = new FrameBuffer(render_pass.get(), textures);
}

TextureRenderer::~TextureRenderer() = default;

void TextureRenderer::prepare(const RenderParams &params) {
	Renderer::prepare(params);
	render(params);
}


void TextureRenderer::render(const RenderParams& params) {
	auto area = frame_buffer->area();
	if (use_params_area)
		area = params.area;

	auto p = params.with_target(frame_buffer.get()).with_area(area);
	p.render_pass = render_pass.get();

	auto cb = params.command_buffer;

	cb->begin_render_pass(render_pass.get(), frame_buffer.get());
	cb->set_viewport(area);
	cb->set_bind_point(vulkan::PipelineBindPoint::GRAPHICS);
	draw(p);
	cb->end_render_pass();
}


HeadlessRenderer::HeadlessRenderer(vulkan::Device* d, const shared_array<Texture>& tex) : RenderTask("headless")
{
	device = d;
	command_buffer = new CommandBuffer(device->command_pool);
	fence = new vulkan::Fence(device);

	texture_renderer = new TextureRenderer(tex);
}

HeadlessRenderer::~HeadlessRenderer() = default;


void HeadlessRenderer::prepare(const RenderParams &params) {
	Renderer::prepare(params);
	render(params);
}

RenderParams HeadlessRenderer::create_params(const rect& area) const {
	auto p = RenderParams::into_texture(texture_renderer->frame_buffer.get(), area.width() / area.height());
	p.area = area;
	p.command_buffer = command_buffer;
	return p;
}

void HeadlessRenderer::render(const RenderParams& params) {
	const auto p = create_params(texture_renderer->frame_buffer->area());
	command_buffer->begin();
	draw(p);
	device->graphics_queue.submit(command_buffer, {}, {}, fence);
	fence->wait();
	//device->wait_idle();
}

#endif
