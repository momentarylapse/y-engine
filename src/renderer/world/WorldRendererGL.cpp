/*
 * WorldRendererGL.cpp
 *
 *  Created on: 07.08.2020
 *      Author: michi
 */

#include "WorldRendererGL.h"

#include <renderer/helper/CubeMapSource.h>

#ifdef USING_OPENGL
#include "geometry/GeometryRendererGL.h"
#include "pass/ShadowRenderer.h"
#include "../base.h"
#include <graphics-impl.h>
#include <world/World.h>
#include <world/Light.h>
#include <world/Camera.h>
#include <helper/PerformanceMonitor.h>
#include <y/ComponentManager.h>
#include <lib/os/msg.h>


namespace nix {
	void resolve_multisampling(FrameBuffer *target, FrameBuffer *source);
}
void apply_shader_data(Shader *s, const Any &shader_data);


WorldRendererGL::WorldRendererGL(const string &name, Camera *cam, SceneView& scene_view) :
		WorldRenderer(name, cam, scene_view) {

}




#endif
