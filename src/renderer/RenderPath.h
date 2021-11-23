/*
 * RenderPath.h
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#pragma once

#include "Renderer.h"
#include "../graphics-fwd.h"
#include "../lib/math/matrix.h"
#include "../lib/image/color.h"
#include "../lib/base/callable.h"
//#include "../lib/base/pointer.h"

class ShadowMapRenderer;
class PerformanceMonitor;
class World;
class Camera;
class matrix;
class vector;
class quaternion;

matrix mtr(const vector &t, const quaternion &a);



struct UBOMatrices {
	alignas(16) matrix model;
	alignas(16) matrix view;
	alignas(16) matrix proj;
};

struct UBOFog {
	alignas(16) color col;
	alignas(16) float distance;
};


enum class RenderPathType {
	NONE,
	FORWARD,
	DEFERRED
};

struct RenderInjector {
	using Callback = Callable<void()>;
	const Callback *func;
};

struct PostProcessor {
	using Callback = Callable<FrameBuffer*(FrameBuffer*)>;
	const Callback *func;
	int channel;
};

class RenderPath : public Renderer {
public:
	RenderPath(const string &name, Renderer *parent);
	virtual ~RenderPath() {}

	// dynamic resolution scaling
	float resolution_scale_x = 1.0f;
	float resolution_scale_y = 1.0f;

	int ch_post = -1, ch_post_focus = -1;
	int ch_pre = -1, ch_bg = -1, ch_fx = -1, ch_world = -1, ch_prepare_lights = -1, ch_shadow = -1;

	RenderPathType type = RenderPathType::NONE;


	Array<PostProcessor> post_processors;
	void add_post_processor(const PostProcessor::Callback *f);

	Array<RenderInjector> fx_injectors;
	void add_fx_injector(const RenderInjector::Callback *f);

	void reset();
};


