/*
 * RenderPathGLDeferred.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#pragma once

#include "RenderPathGL.h"


class RenderPathGLDeferred : public RenderPathGL {
public:


	nix::FrameBuffer *gbuffer = nullptr;
	nix::Shader *shader_gbuffer_out = nullptr;

	RenderPathGLDeferred(GLFWwindow* w, PerformanceMonitor *pm);
	void draw() override;

	void process_blur(nix::FrameBuffer *source, nix::FrameBuffer *target, float threshold, bool horizontal);
	void process_depth(nix::FrameBuffer *source, nix::FrameBuffer *target, nix::Texture *depth_buffer, bool horizontal);
	void process(const Array<nix::Texture*> &source, nix::FrameBuffer *target, nix::Shader *shader);
	nix::FrameBuffer* do_post_processing(nix::FrameBuffer *source);

	void draw_gui(nix::FrameBuffer *source);
	void render_out(nix::FrameBuffer *source, nix::Texture *bloom);
	void render_into_texture(nix::FrameBuffer *fb);
	void draw_skyboxes();
	void draw_terrains(bool allow_material);
	void draw_objects(bool allow_material);
	void draw_world(bool allow_material);
	void draw_particles();
	void set_material(Material *m);
	void set_textures(const Array<nix::Texture*> &tex);
	void prepare_lights();
	void render_shadow_map(nix::FrameBuffer *sfb, float scale);


	void render_from_gbuffer(nix::FrameBuffer *source, nix::FrameBuffer *target);
};


