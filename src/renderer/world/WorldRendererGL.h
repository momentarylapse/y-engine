/*
 * WorldRendererGL.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#pragma once

#include "WorldRenderer.h"
#ifdef USING_OPENGL
#include "geometry/RenderViewData.h"


class rect;
class Material;
class Any;

enum class ShaderVariant;

class ShadowRenderer;
class GeometryRenderer;
class RenderViewData;


class WorldRendererGL : public WorldRenderer {
public:
	WorldRendererGL(const string &name, Camera *cam, RenderPathType type);
	void create_more();

	virtual void render_into_texture(FrameBuffer *fb, Camera *cam, RenderViewData &rvd) {};
	void render_into_cubemap(CubeMapSource& source);

	owned<GeometryRenderer> geo_renderer;
	owned<ShadowRenderer> shadow_renderer;

	void prepare_lights(Camera *cam, RenderViewData &rvd);
};

#endif

