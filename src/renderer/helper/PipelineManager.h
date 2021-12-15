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

Pipeline *get(Shader *s, RenderPass *rp);
Pipeline *get_alpha(Shader *s, RenderPass *rp, Alpha src, Alpha dst);
Pipeline *get_ani(Shader *s, RenderPass *rp);
Pipeline *get_user(Shader *s, RenderPass *rp, const string &format);

}

#endif
