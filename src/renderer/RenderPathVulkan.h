/*
 * RenderPathVulkan.h
 *
 *  Created on: Nov 18, 2021
 *      Author: michi
 */

#pragma once

#include "RenderPath.h"
#ifdef USING_VULKAN
#include "../lib/base/pointer.h"
#include "../lib/base/callable.h"

namespace vulkan {
	class Instance;
	class SwapChain;
	class Fence;
	class Semaphore;
	class RenderPass;
	class DescriptorPool;
	class CommandBuffer;
}

class Material;
class UBOLight;
class GLFWwindow;
class rect;
class Material;

enum class ShaderVariant;


class RenderPathVulkan : public RenderPath {
public:
	int width, height;
	GLFWwindow* window;
	vulkan::Instance *instance;

	vulkan::Fence* in_flight_fence;
	Array<vulkan::Fence*> wait_for_frame_fences;
	vulkan::Semaphore *image_available_semaphore, *render_finished_semaphore;

	Array<vulkan::CommandBuffer*> command_buffers;
	//var cb: vulkan::CommandBuffer*
	vulkan::DescriptorPool* pool;

	vulkan::SwapChain *swap_chain;
	vulkan::RenderPass* _default_render_pass;
	vulkan::DepthBuffer* depth_buffer;
	Array<vulkan::FrameBuffer*> frame_buffers;
	int image_index;
	bool framebuffer_resized;

	void _create_swap_chain_and_stuff();
	void rebuild_default_stuff();


	vulkan::RenderPass *default_render_pass() const;
	vulkan::FrameBuffer *current_frame_buffer() const;
	vulkan::CommandBuffer *current_command_buffer() const;

	shared<Shader> shader;
	rect area() const;

	UniformBuffer* ubo;
	vulkan::DescriptorSet* dset;
	vulkan::Pipeline* pipeline;



	vulkan::Pipeline* pipeline_gui;
	//vulkan::DescriptorSet* dset_gui;
	Array<vulkan::DescriptorSet*> dset_gui;
	Array<UniformBuffer*> ubo_gui;
	Array<VertexBuffer*> vb_gui;
	void prepare_gui(FrameBuffer *source);




	shared<Texture> tex_black;
	shared<Texture> tex_white;
	shared<FrameBuffer> fb_main;
	shared<FrameBuffer> fb_small1;
	shared<FrameBuffer> fb_small2;
	shared<FrameBuffer> fb2;
	shared<FrameBuffer> fb3;
	shared<FrameBuffer> fb_shadow;
	shared<FrameBuffer> fb_shadow2;
	shared<Shader> shader_blur;
	shared<Shader> shader_depth;
	shared<Shader> shader_out;
	shared<Shader> shader_fx;
	//shared<Shader> shader_3d;
	//shared<Shader> shader_shadow;
	//shared<Shader> shader_shadow_animated;
	Material *material_shadow = nullptr;
	shared<Shader> shader_resolve_multisample;

	Array<UBOLight> lights;
	UniformBuffer *ubo_light;
	VertexBuffer *vb_2d;

	shared<DepthBuffer> depth_cube;
	shared<FrameBuffer> fb_cube;
	shared<CubeMap> cube_map;

	//Camera *shadow_cam;
	matrix shadow_proj;
	int shadow_index;

	float shadow_box_size;
	int shadow_resolution;


	bool using_view_space = false;

	RenderPathVulkan(GLFWwindow* win, int w, int h, RenderPathType type);
	virtual ~RenderPathVulkan();

	virtual void render_into_texture(FrameBuffer *fb, Camera *cam, const rect &target_area) = 0;
	void render_into_cubemap(DepthBuffer *fb, CubeMap *cube, const vector &pos);

	bool start_frame() override;
	void end_frame() override;

	void process_blur(FrameBuffer *source, FrameBuffer *target, float threshold, const complex &axis);
	void process_depth(FrameBuffer *source, FrameBuffer *target, const complex &axis);
	void process(const Array<Texture*> &source, FrameBuffer *target, Shader *shader);
	FrameBuffer* do_post_processing(FrameBuffer *source);
	FrameBuffer* resolve_multisampling(FrameBuffer *source);
	void set_material(Material *m, RenderPathType type, ShaderVariant v);
	void set_textures(const Array<Texture*> &tex);
	void draw_gui(vulkan::CommandBuffer *cb);
	void render_out(FrameBuffer *source, Texture *bloom);

	void draw_particles();
	void draw_skyboxes(Camera *c);
	void draw_terrains(bool allow_material);
	void draw_objects_opaque(bool allow_material);
	void draw_objects_transparent(bool allow_material, RenderPathType t);
	void draw_objects_instanced(bool allow_material);
	void prepare_instanced_matrices();
	void prepare_lights(Camera *cam);

	FrameBuffer *next_fb(FrameBuffer *cur);
	rect dynamic_fb_area() const;
};

#endif

