/*
 * RenderPathForward.h
 *
 *  Created on: Jan 19, 2020
 *      Author: michi
 */

#ifndef SRC_RENDERER_RENDERPATHFORWARD_H_
#define SRC_RENDERER_RENDERPATHFORWARD_H_

#include "RenderPath.h"

class RenderPathForward: public RenderPath {
public:
	RenderPathForward(Renderer *renderer, PerformanceMonitor *perf_mon);
	virtual ~RenderPathForward();

	void draw() override;

	vulkan::DescriptorSet *rp_create_dset(const Array<vulkan::Texture*> &tex, vulkan::UBOWrapper *ubo) override;


	vulkan::Shader *fw_shader_a;
	vulkan::Pipeline *fw_pipeline_a;
};

#endif /* SRC_RENDERER_RENDERPATHFORWARD_H_ */
