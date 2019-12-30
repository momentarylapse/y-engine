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
#include "world/world.h"
#include "world/object.h"


// pipeline: shader + z buffer / blending parameters

// descriptor set: textures + shader uniform buffers

void DrawSplashScreen(const string &str, float per) {
	std::cerr << " - splash - " << str.c_str() << "\n";
}

void ExternalModelCleanup(Model *m) {}
extern vulkan::Shader *_default_shader_;

string ObjectDir;

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

class YEngineApp {
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



	void init() {
		window = create_window();
		vulkan::init(window);
		Kaba::init();

		std::cout << "on init..." << "\n";

		engine.set_dirs("Textures/", "Maps/", "Objects/", "Sound", "Scripts/", "Materials/", "Fonts/");

		auto shader = vulkan::Shader::load("3d.shader");
		_default_shader_ = shader;
		pipeline = vulkan::Pipeline::build(shader, vulkan::render_pass, 1, false);
		//pipeline->wireframe = true;
		//pipeline->set_blend(VK_BLEND_FACTOR_SRC_COLOR, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR);
//		pipeline->set_blend(0.6f);
//		pipeline->set_z(false, false);
		pipeline->create();

		vulkan::descriptor_pool = vulkan::create_descriptor_pool();

		cb = new vulkan::CommandBuffer();
		
		MaterialInit();
		world.reset();
		world.background = color(0.2f, 0.2f, 0.4f, 1);
		auto *model = world.create_object("xwing", "xwing", v_0, quaternion::ID);

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



	matrix mtr(const vector &t, const quaternion &a) {
		auto mt = matrix::translation(t);
		auto mr = matrix::rotation_q(a);
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

	float time;

	void render_world(vulkan::CommandBuffer *cb) {
		cb->begin_render_pass(vulkan::render_pass, world.background);
		cb->set_pipeline(pipeline);

		for (auto &s: world.sorted_opaque) {
			Model *m = s.model;

			m->ang = quaternion::rotation_v(vector(-0.3f,0.5f,time));

			UniformBufferObject u;
			u.proj = cam->m_projection.transpose();
			u.view = cam->m_view.transpose();
			u.model = mtr(m->pos, m->ang).transpose();
			s.ubo->update(&u);

			cb->bind_descriptor_set(0, s.dset);
			cb->draw(m->mesh[0]->sub[0].vertex_buffer);
		}

		cb->end_render_pass();

	}

	void draw_frame() {
		speedometer.tick();

		cam->pos = 1000*vector::EZ;
		cam->set_view();

		static auto start_time = high_resolution_clock::now();

		auto current_time = high_resolution_clock::now();
		time = duration<float, seconds::period>(current_time - start_time).count();

		if (!vulkan::start_frame())
			return;

		float pc = 0;


		cb->begin();
		render_world(cb);
		cb->end();

		vulkan::submit_command_buffer(cb);

		vulkan::end_frame();
	}



};

int hui_main(const Array<string> &arg) {
	YEngineApp app;
	msg_init();

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
