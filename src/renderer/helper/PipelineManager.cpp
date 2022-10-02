/*
 * PipelineManager.cpp
 *
 *  Created on: 15 Dec 2021
 *      Author: michi
 */

#include "PipelineManager.h"
#ifdef USING_VULKAN
#include "../../graphics-impl.h"
#include "../../lib/base/map.h"
#include "../../lib/os/msg.h"

namespace PipelineManager {

static base::map<Shader*,GraphicsPipeline*> ob_pipelines;
static base::map<Shader*,GraphicsPipeline*> ob_pipelines_alpha;
static base::map<Shader*,GraphicsPipeline*> ob_ani_pipelines;
static base::map<Shader*,GraphicsPipeline*> ob_pipelines_user;
static base::map<Shader*,GraphicsPipeline*> ob_pipelines_gui;

GraphicsPipeline *get(Shader *s, RenderPass *rp) {
	if (ob_pipelines.contains(s))
		return ob_pipelines[s];
	msg_write("NEW PIPELINE");
	auto p = new GraphicsPipeline(s, rp, 0, "triangles", "3f,3f,2f");
	ob_pipelines.add({s, p});
	return p;
}
GraphicsPipeline *get_alpha(Shader *s, RenderPass *rp, Alpha src, Alpha dst) {
	if (ob_pipelines_alpha.contains(s))
		return ob_pipelines_alpha[s];
	msg_write(format("NEW PIPELINE ALPHA %d %d", (int)src, (int)dst));
	auto p = new GraphicsPipeline(s, rp, 0, "triangles", "3f,3f,2f");
	p->set_z(true, false);
	p->set_blend(src, dst);
	//p->set_culling(0);
	p->rebuild();
	ob_pipelines_alpha.add({s, p});
	return p;
}

GraphicsPipeline *get_ani(Shader *s, RenderPass *rp) {
	if (ob_ani_pipelines.contains(s))
		return ob_ani_pipelines[s];
	msg_write("NEW PIPELINE ANIMATED");
	auto p = new GraphicsPipeline(s, rp, 0, "triangles", "3f,3f,2f,4i,4f");
	ob_ani_pipelines.add({s, p});
	return p;
}

GraphicsPipeline *get_user(Shader *s, RenderPass *rp, const string &format) {
	if (ob_pipelines_user.contains(s))
		return ob_pipelines_user[s];
	msg_write("NEW PIPELINE USER");
	auto p = new GraphicsPipeline(s, rp, 0, "triangles", format);
	ob_pipelines_user.add({s, p});
	return p;
}

GraphicsPipeline *get_gui(Shader *s, RenderPass *rp, const string &format) {
	if (ob_pipelines_gui.contains(s))
		return ob_pipelines_gui[s];
	msg_write("NEW PIPELINE GUI");
	auto p = new GraphicsPipeline(s, rp, 0, "triangles", "3f,3f,2f");
	p->set_blend(Alpha::SOURCE_ALPHA, Alpha::SOURCE_INV_ALPHA);
	p->set_z(false, false);
	p->rebuild();
	ob_pipelines_gui.add({s, p});
	return p;
}

void clear() {
	ob_pipelines.clear();
	ob_pipelines_alpha.clear();
	ob_ani_pipelines.clear();
	ob_pipelines_user.clear();
	ob_pipelines_gui.clear();
}


}

#endif
