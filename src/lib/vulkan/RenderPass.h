//
//  RenderPass.hpp
//   * load/store color/depth buffers
//
//  Created by <author> on 06/02/2019.
//
//

#ifndef RenderPass_hpp
#define RenderPass_hpp

#if HAS_LIB_VULKAN



#include "../base/base.h"
#include "../image/color.h"
#include <vulkan/vulkan.h>
#include <vector>

namespace vulkan{

	class RenderPass {
	public:
		RenderPass(const Array<VkFormat> &format, bool clear = true);
		~RenderPass();

		void __init__(const Array<VkFormat> &format, bool clear = true);
		void __delete__();

		void create();
		void destroy();

		VkRenderPass render_pass;
		color clear_color;
		float clear_z;
		unsigned int clear_stencil;

	private:
		VkAttachmentDescription color_attachment;
		VkAttachmentDescription depth_attachment;
		VkAttachmentReference color_attachment_ref;
		VkAttachmentReference depth_attachment_ref;
		VkSubpassDescription subpass;
		VkSubpassDependency dependency;
	};
};

#endif

#endif /* RenderPass_hpp */
