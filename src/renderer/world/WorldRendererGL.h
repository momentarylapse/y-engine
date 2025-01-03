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
	WorldRendererGL(const string &name, Camera *cam, SceneView& scene_view);
};

#endif

