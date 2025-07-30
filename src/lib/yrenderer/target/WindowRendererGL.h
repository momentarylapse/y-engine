/*
 * WindowRendererGL.h
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */


#pragma once

#include "TargetRenderer.h"
#ifdef USING_OPENGL

struct GLFWwindow;

namespace yrenderer {

class WindowRendererGL : public TargetRenderer {
public:
	explicit WindowRendererGL(GLFWwindow* win);


	bool start_frame();
	void end_frame(const RenderParams& params);

	void prepare(const RenderParams& params) override;
	void draw(const RenderParams& params) override;

	RenderParams create_params(float aspect_ratio);

	GLFWwindow* window;

	ygfx::DepthBuffer* _depth_buffer;
	ygfx::FrameBuffer* _frame_buffer;
};

}

#endif
