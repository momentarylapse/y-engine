#include "MultisampleResolver.h"
#include "ThroughShaderRenderer.h"
#ifdef USING_OPENGL
#include "../target/TextureRendererGL.h"
#else
#include "../target/TextureRendererVulkan.h"
#endif
#include "../../helper/ResourceManager.h"
#include "../../graphics-impl.h"


namespace nix {
	void resolve_multisampling(FrameBuffer *target, FrameBuffer *source);
}

MultisampleResolver::MultisampleResolver(Texture* tex_ms, Texture* depth_ms, Texture* tex_out, Texture* depth_out) : RenderTask("ms") {
	shader_resolve_multisample = resource_manager->load_shader("forward/resolve-multisample.shader");
	tsr = new ThroughShaderRenderer("ms", {tex_ms, depth_ms}, shader_resolve_multisample);

	into_texture = new TextureRenderer("tex", {tex_out, depth_out});
	into_texture->add_child(tsr.get());
	into_texture->use_params_area = true;
}

void MultisampleResolver::render(const RenderParams& params) {
	// resolve
	if (true) {
		tsr->data.dict_set("width:0", into_texture->frame_buffer->width);
		tsr->data.dict_set("height:4", into_texture->frame_buffer->height);
		tsr->set_source(dynamicly_scaled_source());
		into_texture->render(params.with_area(dynamicly_scaled_area(into_texture->frame_buffer.get())));
	} else {
		// not sure, why this does not work... :(
		//			nix::resolve_multisampling(fb_main.get(), fb_main_ms.get());
	}
}

