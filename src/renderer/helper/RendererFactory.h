/*
 * RendererFactory.h
 *
 *  Created on: 11 Oct 2023
 *      Author: michi
 */

#ifndef SRC_RENDERER_HELPER_RENDERERFACTORY_H_
#define SRC_RENDERER_HELPER_RENDERERFACTORY_H_

struct GLFWwindow;
class Camera;

class RenderPath;

RenderPath* create_render_path(Camera *cam);
void create_full_renderer(GLFWwindow* window, Camera *cam);

#endif /* SRC_RENDERER_HELPER_RENDERERFACTORY_H_ */
