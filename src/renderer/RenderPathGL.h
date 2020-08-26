/*
 * RenderPathGL.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_RENDERPATHGL_H_
#define SRC_RENDERER_RENDERPATHGL_H_

#include "RenderPath.h"
namespace nix {
	class FrameBuffer;
	class Texture;
	class VertexBuffer;
	class UniformBuffer;
}
class Material;
class UBOLight;
class GLFWwindow;


class RenderPathGL : public RenderPath {
public:
	int width, height;
	GLFWwindow* window;
	nix::Texture *tex_black = nullptr;
	nix::Texture *tex_white = nullptr;
	nix::FrameBuffer *fb = nullptr;
	nix::FrameBuffer *fb2 = nullptr;
	nix::FrameBuffer *fb3 = nullptr;
	nix::FrameBuffer *fb4 = nullptr;
	nix::FrameBuffer *fb5 = nullptr;
	nix::FrameBuffer *fb_shadow = nullptr;
	nix::FrameBuffer *fb_shadow2 = nullptr;
	nix::Shader *shader_blur = nullptr;
	nix::Shader *shader_depth = nullptr;
	nix::Shader *shader_out = nullptr;
	nix::Shader *shader_fx = nullptr;
	//nix::Shader *shader_3d = nullptr;
	nix::Shader *shader_shadow = nullptr;

	Array<UBOLight> lights;
	nix::UniformBuffer *ubo_light;
	PerformanceMonitor *perf_mon;
	nix::VertexBuffer *vb_2d;

	//Camera *shadow_cam;
	matrix shadow_proj;

	float shadow_box_size;

	RenderPathGL(GLFWwindow* w, PerformanceMonitor *pm);
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
};




#endif /* SRC_RENDERER_RENDERPATHGL_H_ */
