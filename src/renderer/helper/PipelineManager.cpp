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

Pipeline *get(Shader *s, RenderPass *rp) {
	static Map<Shader*,Pipeline*> ob_pipelines;
	if (ob_pipelines.contains(s))
		return ob_pipelines[s];
	msg_write("NEW PIPELINE");
	auto p = new Pipeline(s, rp, 0, "triangles", "3f,3f,2f");
	ob_pipelines.add({s, p});
	return p;
}
Pipeline *get_alpha(Shader *s, RenderPass *rp, Alpha src, Alpha dst) {
	static Map<Shader*,Pipeline*> ob_pipelines_alpha;
	if (ob_pipelines_alpha.contains(s))
		return ob_pipelines_alpha[s];
	msg_write(format("NEW PIPELINE ALPHA %d %d", (int)src, (int)dst));
	auto p = new Pipeline(s, rp, 0, "triangles", "3f,3f,2f");
	p->set_z(true, false);
	p->set_blend(src, dst);
	//p->set_culling(0);
	p->rebuild();
	ob_pipelines_alpha.add({s, p});
	return p;
}

Pipeline *get_ani(Shader *s, RenderPass *rp) {
	static Map<Shader*,Pipeline*> ob_ani_pipelines;
	if (ob_ani_pipelines.contains(s))
		return ob_ani_pipelines[s];
	msg_write("NEW PIPELINE ANIMATED");
	auto p = new Pipeline(s, rp, 0, "triangles", "3f,3f,2f,4i,4f");
	ob_ani_pipelines.add({s, p});
	return p;
}

Pipeline *get_user(Shader *s, RenderPass *rp, const string &format) {
	static Map<Shader*,Pipeline*> ob_pipelines;
	if (ob_pipelines.contains(s))
		return ob_pipelines[s];
	msg_write("NEW PIPELINE USER");
	auto p = new Pipeline(s, rp, 0, "triangles", format);
	ob_pipelines.add({s, p});
	return p;
}

Pipeline *get_gui(Shader *s, RenderPass *rp, const string &format) {
	static Map<Shader*,Pipeline*> ob_pipelines;
	if (ob_pipelines.contains(s))
		return ob_pipelines[s];
	msg_write("NEW PIPELINE GUI");
	auto p = new vulkan::Pipeline(s, rp, 0, "triangles", "3f,3f,2f");
	p->set_blend(Alpha::SOURCE_ALPHA, Alpha::SOURCE_INV_ALPHA);
	p->set_z(false, false);
	p->rebuild();
	ob_pipelines.add({s, p});
	return p;
}


}

#endif
