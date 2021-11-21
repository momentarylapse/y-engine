/*
 * RendererGL.h
 *
 *  Created on: 21 Nov 2021
 *      Author: michi
 */


#pragma once

#include "Renderer.h"
#include "../graphics-fwd.h"
#ifdef USING_OPENGL
#include "../lib/base/pointer.h"


class RendererGL : public Renderer {
public:
	/*RendererGL();
	virtual ~RendererGL();*/

	virtual FrameBuffer *current_frame_buffer() const = 0;
};

#endif
