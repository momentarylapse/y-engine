#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <cstdlib>
#include <chrono>

#include "lib/hui/hui.h"
#include "lib/vulkan/vulkan.h"
#include "lib/math/math.h"
#include "lib/image/color.h"
#include "lib/image/image.h"
#include "lib/kaba/kaba.h"

#include "world/material.h"
#include "world/model.h"
#include "world/camera.h"


// pipeline: shader + z buffer / blending parameters

// descriptor set: textures + shader uniform buffers


void ExternalModelCleanup(Model *m) {}

bool AllowXContainer = true;

using namespace std::chrono;


struct UniformBufferObject {
	alignas(16) matrix model;
	alignas(16) matrix view;
	alignas(16) matrix proj;
};


namespace vulkan {
	extern RenderPass *render_pass;
	extern VkDescriptorPool descriptor_pool;
};

class YEngine {
public:
	
	void run() {
		init();
		main_loop();
		cleanup();
	}

private:
	GLFWwindow* window;

	vulkan::CommandBuffer *cb;
	vulkan::Pipeline *pipeline;

	
	Model *model;
	vulkan::DescriptorSet *dset;
	vulkan::UBOWrapper *ubo;
	//Camera *cam;


	void init() {
		window = create_window();
		vulkan::init(window);
		Kaba::init();

		std::cout << "on init..." << "\n";

		auto shader = vulkan::Shader::load("shaders/3d.shader");
		pipeline = vulkan::Pipeline::build(shader, vulkan::render_pass, 1, false);
		//pipeline->wireframe = true;
		//pipeline->set_blend(VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR);
//		pipeline->set_blend(0.6f);
//		pipeline->set_z(false, false);
		pipeline->create();


		ubo = new vulkan::UBOWrapper(sizeof(UniformBufferObject));
		vulkan::descriptor_pool = vulkan::create_descriptor_pool();

		cb = new vulkan::CommandBuffer();
		
		MaterialInit();
		model = new Model();
		model->load("xwing.model");
		dset = new vulkan::DescriptorSet(shader->descr_layouts[0], {ubo}, {model->material[0]->textures[0]});

		CameraReset();
	}
	
	GLFWwindow* create_window() {
		GLFWwindow* window;
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		window = glfwCreateWindow(1024, 768, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
		return window;
	}

	static void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
		vulkan::on_resize(width, height);
	}

	void main_loop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			draw_frame();
		}

		vkDeviceWaitIdle(vulkan::device);
	}

	void cleanup() {
		vulkan::destroy();

		glfwDestroyWindow(window);

		glfwTerminate();
	}



	matrix mtr(const vector &t, const vector &a) {
		matrix mt, mr;
		mt = matrix::translation(t);
		mr = matrix::rotation(a);
		return mt * mr;
	}

	struct Speedometer {
		int frames = 0;
		high_resolution_clock::time_point prev = high_resolution_clock::now();
		void tick() {
			frames ++;
		 	auto now = high_resolution_clock::now();
			float dt = std::chrono::duration<float, std::chrono::seconds::period>(now - prev).count();
			if (dt > 1.0f) {
				std::cout << (float)frames/dt << " fps\n";
				frames = 0;
				prev = now;
			}
		}
	};
	Speedometer speedometer;

	void draw_frame() {
		speedometer.tick();

		cam->pos = 1000*vector::EZ;
		cam->set_view();

		static auto start_time = high_resolution_clock::now();

		auto current_time = high_resolution_clock::now();
		float time = duration<float, seconds::period>(current_time - start_time).count();

		UniformBufferObject u;
		u.proj = cam->m_projection.transpose();
		u.view = cam->m_view.transpose();
		u.model = mtr(model->pos, vector(-0.3f,0.5f,time)).transpose();
		ubo->update(&u);

		if (!vulkan::start_frame())
			return;

		float pc = 0;


		cb->begin();
		cb->begin_render_pass(vulkan::render_pass, Black);
		cb->set_pipeline(pipeline);

		cb->bind_descriptor_set(0, dset);
		cb->draw(model->mesh[0]->sub[0].vertex_buffer);


		cb->end_render_pass();
		cb->end();

		vulkan::submit_command_buffer(cb);

		vulkan::end_frame();
	}



};

int hui_main(const Array<string> &arg) {
	YEngine app;
	msg_init();

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
