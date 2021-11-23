/*
 * GuiRendererGL.h
 *
 *  Created on: 23 Nov 2021
 *      Author: michi
 */

#pragma once

#include "../Renderer.h"
#ifdef USING_OPENGL

class GuiRendererGL : public Renderer {
public:
	GuiRendererGL(Renderer *parent);
	virtual ~GuiRendererGL();

	void draw() override;

	void draw_gui(FrameBuffer *source);

	Shader *shader;
	int ch_gui = -1;
};

#endif
