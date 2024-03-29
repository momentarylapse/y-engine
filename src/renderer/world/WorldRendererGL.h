/*
 * WorldRendererGL.h
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#pragma once

#include "WorldRenderer.h"
#ifdef USING_OPENGL


class rect;
class Material;
class Any;

enum class ShaderVariant;

class ShadowRendererGL;
class GeometryRendererGL;


class WorldRendererGL : public WorldRenderer {
public:
	WorldRendererGL(const string &name, Camera *cam, RenderPathType type);
	void create_more();

	virtual void render_into_texture(FrameBuffer *fb, Camera *cam) {};
	void render_into_cubemap(DepthBuffer *fb, CubeMap *cube, const CubeMapParams &params);

	owned<GeometryRendererGL> geo_renderer;
	owned<ShadowRendererGL> shadow_renderer;

	void prepare_lights();
};

#endif

