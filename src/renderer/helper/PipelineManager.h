/*
 * PipelineManager.h
 *
 *  Created on: 15 Dec 2021
 *      Author: michi
 */

#pragma once
#include "../../graphics-fwd.h"
#ifdef USING_VULKAN

class string;

namespace PipelineManager {

GraphicsPipeline *get(Shader *s, RenderPass *rp);
GraphicsPipeline *get_alpha(Shader *s, RenderPass *rp, Alpha src, Alpha dst, bool write_z);
GraphicsPipeline *get_ani(Shader *s, RenderPass *rp);
GraphicsPipeline *get_user(Shader *s, RenderPass *rp, const string &format);
GraphicsPipeline *get_gui(Shader *s, RenderPass *rp, const string &format);

void clear();

}

#endif
