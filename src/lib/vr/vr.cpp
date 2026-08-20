#include "vr.h"
#include <lib/base/base.h>
#include <lib/os/msg.h>
#include <lib/base/algo.h>
#include <lib/ygraphics/Context.h>

#include "lib/math/quaternion.h"
#include "lib/vulkan/vulkan.h"
#include "lib/vulkan/Device.h"
#include "lib/vulkan/Instance.h"
#include "lib/yrenderer/Context.h"
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>

#include "lib/vulkan/common.h"

#if HAS_LIB_OPENXR
#include <vulkan/vulkan.h>
#include <openxr/openxr.h>
//#ifdef USING_VULKAN
#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr_platform.h>
#endif

namespace vulkan {
	extern Array<string> additional_instance_extensions;
	extern Array<string> additional_device_extensions;
	VkSurfaceFormatKHR choose_swap_surface_format(const Array<VkSurfaceFormatKHR>& available_formats, bool gamma_correction);
}

namespace yrenderer {
	void _create_context_stuff(Context* ctx);
}

namespace vr {

Instance* instance = nullptr;

#if HAS_LIB_OPENXR

static XrInstance xrInstance = XR_NULL_HANDLE;
static XrDebugUtilsMessengerEXT m_debugUtilsMessenger;
XrSession m_session = XR_NULL_HANDLE;
XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;
XrSystemId systemID;
bool m_applicationRunning = true;
bool m_sessionRunning = false;
XrSpace m_localSpace = XR_NULL_HANDLE;

std::vector<XrEnvironmentBlendMode> m_applicationEnvironmentBlendModes = {XR_ENVIRONMENT_BLEND_MODE_OPAQUE, XR_ENVIRONMENT_BLEND_MODE_ADDITIVE};
std::vector<XrEnvironmentBlendMode> m_environmentBlendModes = {};
XrEnvironmentBlendMode m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM;

	struct SwapchainInfo {
		XrSwapchain swapchain = XR_NULL_HANDLE;
		int64_t swapchainFormat = 0;
	};


	struct ImageViewCreateInfo {
		void* image;
		enum class Type : uint8_t {
			RTV,
			DSV,
			SRV,
			UAV
		} type;
		enum class View : uint8_t {
			TYPE_1D,
			TYPE_2D,
			TYPE_3D,
			TYPE_CUBE,
			TYPE_1D_ARRAY,
			TYPE_2D_ARRAY,
			TYPE_CUBE_ARRAY,
		} view;
		int64_t format;
		enum class Aspect : uint8_t {
			COLOR_BIT = 0x01,
			DEPTH_BIT = 0x02,
			STENCIL_BIT = 0x04
		} aspect;
		uint32_t baseMipLevel;
		uint32_t levelCount;
		uint32_t baseArrayLayer;
		uint32_t layerCount;
	};

	std::unordered_map<VkImage, VkImageLayout> imageStates;
    std::unordered_map<VkImageView, ImageViewCreateInfo> imageViewResources;
	std::unordered_map<XrSwapchain, std::pair<int, std::vector<XrSwapchainImageVulkanKHR>>> swapchainImagesMap{};

	XrSwapchainImageBaseHeader *AllocateSwapchainImageData(XrSwapchain swapchain, int type, uint32_t count) {
		swapchainImagesMap[swapchain].first = type;
		swapchainImagesMap[swapchain].second.resize(count, {XR_TYPE_SWAPCHAIN_IMAGE_VULKAN_KHR});
		return reinterpret_cast<XrSwapchainImageBaseHeader *>(swapchainImagesMap[swapchain].second.data());
	}
	XrSwapchainImageBaseHeader* GetSwapchainImageData(XrSwapchain swapchain, uint32_t index) {
		return (XrSwapchainImageBaseHeader*)&swapchainImagesMap[swapchain].second[index];
	}
	// XR_DOCS_TAG_BEGIN_GetSwapchainImage_Vulkan
	void* GetSwapchainImage(XrSwapchain swapchain, uint32_t index) {
		VkImage image = swapchainImagesMap[swapchain].second[index].image;
		VkImageLayout layout = swapchainImagesMap[swapchain].first == 0 ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		imageStates[image] = layout;
		return (void *)image;
	}

	void *CreateImageView(const ImageViewCreateInfo &imageViewCI) {
		VkImageView imageView{};
		VkImageViewCreateInfo vkImageViewCI;
		vkImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		vkImageViewCI.pNext = nullptr;
		vkImageViewCI.flags = 0;
		vkImageViewCI.image = (VkImage)imageViewCI.image;
		vkImageViewCI.viewType = VkImageViewType(imageViewCI.view);
		vkImageViewCI.format = (VkFormat)imageViewCI.format;
		vkImageViewCI.components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A};
		vkImageViewCI.subresourceRange.aspectMask = VkImageAspectFlagBits(imageViewCI.aspect);
		vkImageViewCI.subresourceRange.baseMipLevel = imageViewCI.baseMipLevel;
		vkImageViewCI.subresourceRange.levelCount = imageViewCI.levelCount;
		vkImageViewCI.subresourceRange.baseArrayLayer = imageViewCI.baseArrayLayer;
		vkImageViewCI.subresourceRange.layerCount = imageViewCI.layerCount;
		if (vkCreateImageView(vulkan::default_device->device, &vkImageViewCI, nullptr, &imageView) != VK_SUCCESS)
			throw Exception("Failed to create ImageView.");

		imageViewResources[imageView] = imageViewCI;
		return (void *)imageView;
	}

	PFN_xrGetVulkanGraphicsRequirementsKHR xrGetVulkanGraphicsRequirementsKHR = nullptr;
	PFN_xrGetVulkanInstanceExtensionsKHR xrGetVulkanInstanceExtensionsKHR = nullptr;
	PFN_xrGetVulkanDeviceExtensionsKHR xrGetVulkanDeviceExtensionsKHR = nullptr;
	PFN_xrGetVulkanGraphicsDeviceKHR xrGetVulkanGraphicsDeviceKHR = nullptr;

	XrActionSet m_actionSet;
	// An action for grabbing blocks, and an action to change the color of a block.
	XrAction m_grabCubeAction, m_spawnCubeAction, m_changeColorAction;
	// The realtime states of these actions.
	XrActionStateFloat m_grabState[2] = {{XR_TYPE_ACTION_STATE_FLOAT}, {XR_TYPE_ACTION_STATE_FLOAT}};
	XrActionStateBoolean m_changeColorState[2] = {{XR_TYPE_ACTION_STATE_BOOLEAN}, {XR_TYPE_ACTION_STATE_BOOLEAN}};
	XrActionStateBoolean m_spawnCubeState = {XR_TYPE_ACTION_STATE_BOOLEAN};
	// The haptic output action for grabbing cubes.
	XrAction m_buzzAction;
	// The current haptic output value for each controller.
	float m_buzz[2] = {0, 0};
	// The action for getting the hand or controller position and orientation.
	XrAction m_palmPoseAction;
	// The XrPaths for left and right hand hands or controllers.
	XrPath m_handPaths[2] = {0, 0};
	// The spaces that represents the two hand poses.
	XrSpace m_handPoseSpace[2];
	XrActionStatePose m_handPoseState[2] = {{XR_TYPE_ACTION_STATE_POSE}, {XR_TYPE_ACTION_STATE_POSE}};
	// The current poses obtained from the XrSpaces.
	XrPosef m_handPose[2] = {
		{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0}},
		{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0}}};


	// chosen:
	static XrViewConfigurationType view_type = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;
	std::vector<XrViewConfigurationView> m_viewConfigurationViews; // [left, right]

	std::vector<SwapchainInfo> m_colorSwapchainInfos = {};
	std::vector<SwapchainInfo> m_depthSwapchainInfos = {};


void CreateActionSet();
void SuggestBindings();
void RecordCurrentBindings();
void CreateActionPoses();
void AttachActionSet();
void GetEnvironmentBlendModes();
void GetViewConfigurationViews();
void CreateReferenceSpace();
void CreateSwapchains();

inline const char* GetXRErrorString(XrInstance _instance, XrResult result) {
	static char string[XR_MAX_RESULT_STRING_SIZE];
	xrResultToString(_instance, result, string);
	return string;
}

#define OPENXR_CHECK(x, y)                                                                                                                                  \
{                                                                                                                                                       \
	XrResult result = (x);                                                                                                                              \
	if (!XR_SUCCEEDED(result)) {                                                                                                                        \
		msg_error(format("OPENXR: %d (%s) %s", int(result), (xrInstance ? GetXRErrorString(xrInstance, result) : ""), y)); \
		exit(1);                                                                                                                             \
	}                                                                                                                                                   \
	}

template <typename T>
bool BitwiseCheck(const T &value, const T &checkValue) {
	return ((value & checkValue) == checkValue);
}

XrBool32 OpenXRMessageCallbackFunction(XrDebugUtilsMessageSeverityFlagsEXT messageSeverity, XrDebugUtilsMessageTypeFlagsEXT messageType, const XrDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
	// Lambda to covert an XrDebugUtilsMessageSeverityFlagsEXT to std::string. Bitwise check to concatenate multiple severities to the output string.
	auto GetMessageSeverityString = [](XrDebugUtilsMessageSeverityFlagsEXT messageSeverity) -> string {
		bool separator = false;

		string msgFlags;
		if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) {
			msgFlags += "VERBOSE";
			separator = true;
		}
		if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)) {
			if (separator)
				msgFlags += ",";
			msgFlags += "INFO";
			separator = true;
		}
		if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)) {
			if (separator)
				msgFlags += ",";
			msgFlags += "WARN";
			separator = true;
		}
		if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)) {
			if (separator)
				msgFlags += ",";
			msgFlags += "ERROR";
		}
		return msgFlags;
	};
	// Lambda to covert an XrDebugUtilsMessageTypeFlagsEXT to string. Bitwise check to concatenate multiple types to the output string.
	auto GetMessageTypeString = [] (XrDebugUtilsMessageTypeFlagsEXT messageType) -> string {
		bool separator = false;

		string msgFlags;
		if (BitwiseCheck(messageType, XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)) {
			msgFlags += "GEN";
			separator = true;
		}
		if (BitwiseCheck(messageType, XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)) {
			if (separator)
				msgFlags += ",";
			msgFlags += "SPEC";
			separator = true;
		}
		if (BitwiseCheck(messageType, XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)) {
			if (separator)
				msgFlags += ",";
			msgFlags += "PERF";
		}
		return msgFlags;
	};

	// Collect message data.
	string functionName = (pCallbackData->functionName) ? pCallbackData->functionName : "";
	string messageSeverityStr = GetMessageSeverityString(messageSeverity);
	string messageTypeStr = GetMessageTypeString(messageType);
	string messageId = (pCallbackData->messageId) ? pCallbackData->messageId : "";
	string message = (pCallbackData->message) ? pCallbackData->message : "";

	// String stream final message.
	string errorMessage;
	errorMessage = format("%s(%s / %s): msg num %s - %s", functionName, messageSeverityStr, messageTypeStr, messageId, message);

	// Log and debug break.
	msg_write(errorMessage);
	if (BitwiseCheck(messageSeverity, XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT))
		exit(1);
	return XrBool32();
}

XrDebugUtilsMessengerEXT CreateOpenXRDebugUtilsMessenger(XrInstance m_xrInstance) {
	// Fill out a XrDebugUtilsMessengerCreateInfoEXT structure specifying all severities and types.
	// Set the userCallback to OpenXRMessageCallbackFunction().
	XrDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	debugUtilsMessengerCI.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugUtilsMessengerCI.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | XR_DEBUG_UTILS_MESSAGE_TYPE_CONFORMANCE_BIT_EXT;
	debugUtilsMessengerCI.userCallback = (PFN_xrDebugUtilsMessengerCallbackEXT)OpenXRMessageCallbackFunction;
	debugUtilsMessengerCI.userData = nullptr;

	// Load xrCreateDebugUtilsMessengerEXT() function pointer as it is not default loaded by the OpenXR loader.
	XrDebugUtilsMessengerEXT debugUtilsMessenger{};
	PFN_xrCreateDebugUtilsMessengerEXT xrCreateDebugUtilsMessengerEXT;
	OPENXR_CHECK(xrGetInstanceProcAddr(m_xrInstance, "xrCreateDebugUtilsMessengerEXT", (PFN_xrVoidFunction *)&xrCreateDebugUtilsMessengerEXT), "Failed to get InstanceProcAddr.");

	// Finally create and return the XrDebugUtilsMessengerEXT.
	OPENXR_CHECK(xrCreateDebugUtilsMessengerEXT(m_xrInstance, &debugUtilsMessengerCI, &debugUtilsMessenger), "Failed to create DebugUtilsMessenger.");
	return debugUtilsMessenger;
}

void DestroyOpenXRDebugUtilsMessenger(XrInstance m_xrInstance, XrDebugUtilsMessengerEXT debugUtilsMessenger) {
	// Load xrDestroyDebugUtilsMessengerEXT() function pointer as it is not default loaded by the OpenXR loader.
	PFN_xrDestroyDebugUtilsMessengerEXT xrDestroyDebugUtilsMessengerEXT;
	OPENXR_CHECK(xrGetInstanceProcAddr(m_xrInstance, "xrDestroyDebugUtilsMessengerEXT", (PFN_xrVoidFunction*)&xrDestroyDebugUtilsMessengerEXT), "Failed to get InstanceProcAddr.");

	// Destroy the provided XrDebugUtilsMessengerEXT.
	OPENXR_CHECK(xrDestroyDebugUtilsMessengerEXT(debugUtilsMessenger), "Failed to destroy DebugUtilsMessenger.");
}

void Instance::create_session(yrenderer::Context* ctx) {
//	CreateActionSet();
//	SuggestBindings();


	msg_write("--------create session");
	msg_write(p2s(ctx->context->instance->instance));
	msg_write(p2s(ctx->context->device->physical_device));
	msg_write(p2s(ctx->context->device->device));
	msg_write(*ctx->context->device->indices.graphics_family);
	msg_write(p2s((void*)systemID));


	XrGraphicsBindingVulkanKHR graphicsBinding;
	graphicsBinding = {XR_TYPE_GRAPHICS_BINDING_VULKAN_KHR};
	graphicsBinding.instance = ctx->context->instance->instance;
	graphicsBinding.physicalDevice = ctx->context->device->physical_device;
	graphicsBinding.device = ctx->context->device->device;
	graphicsBinding.queueFamilyIndex = *ctx->context->device->indices.graphics_family;
	graphicsBinding.queueIndex = 0;

	XrSessionCreateInfo sessionCI{XR_TYPE_SESSION_CREATE_INFO};
	sessionCI.next = &graphicsBinding;
	sessionCI.createFlags = 0;
	sessionCI.systemId = systemID;

	OPENXR_CHECK(xrCreateSession(xrInstance, &sessionCI, &m_session), "Failed to create Session.");


	/*CreateActionPoses();
	AttachActionSet();*/
	CreateReferenceSpace();
	//CreateSwapchains();
}


    // XR_DOCS_TAG_BEGIN_CreateXrPath
    XrPath CreateXrPath(const char *path_string) {
        XrPath xrPath;
        OPENXR_CHECK(xrStringToPath(xrInstance, path_string, &xrPath), "Failed to create XrPath from string.");
        return xrPath;
    }
    string FromXrPath(XrPath path) {
        uint32_t strl;
        char text[XR_MAX_PATH_LENGTH];
        XrResult res;
        res = xrPathToString(xrInstance, path, XR_MAX_PATH_LENGTH, &strl, text);
        string str;
        if (res == XR_SUCCESS) {
            str = text;
        } else {
            OPENXR_CHECK(res, "Failed to retrieve path.");
        }
        return str;
    }
    // XR_DOCS_TAG_END_CreateXrPath
    // XR_DOCS_TAG_BEGIN_CreateActionSet
    void CreateActionSet() {
        XrActionSetCreateInfo actionSetCI{XR_TYPE_ACTION_SET_CREATE_INFO};
        // The internal name the runtime uses for this Action Set.
        strncpy(actionSetCI.actionSetName, "openxr-tutorial-actionset", XR_MAX_ACTION_SET_NAME_SIZE);
        // Localized names are required so there is a human-readable action name to show the user if they are rebinding Actions in an options screen.
        strncpy(actionSetCI.localizedActionSetName, "OpenXR Tutorial ActionSet", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);
        // Set a priority: this comes into play when we have multiple Action Sets, and determines which Action takes priority in binding to a specific input.
        actionSetCI.priority = 0;

        OPENXR_CHECK(xrCreateActionSet(xrInstance, &actionSetCI, &m_actionSet), "Failed to create ActionSet.");
        // XR_DOCS_TAG_END_CreateActionSet

        // XR_DOCS_TAG_BEGIN_CreateActionLambda
        auto CreateAction = [](XrAction &xrAction, const char *name, XrActionType xrActionType, Array<const char *> subaction_paths = {}) -> void {
            XrActionCreateInfo actionCI{XR_TYPE_ACTION_CREATE_INFO};
            // The type of action: float input, pose, haptic output etc.
            actionCI.actionType = xrActionType;
            // Subaction paths, e.g. left and right hand. To distinguish the same action performed on different devices.
            Array<XrPath> subaction_xrpaths;
            for (auto p : subaction_paths) {
                subaction_xrpaths.add(CreateXrPath(p));
            }
            actionCI.countSubactionPaths = (uint32_t)subaction_xrpaths.num;
            actionCI.subactionPaths = &subaction_xrpaths[0];
            // The internal name the runtime uses for this Action.
            strncpy(actionCI.actionName, name, XR_MAX_ACTION_NAME_SIZE);
            // Localized names are required so there is a human-readable action name to show the user if they are rebinding the Action in an options screen.
            strncpy(actionCI.localizedActionName, name, XR_MAX_LOCALIZED_ACTION_NAME_SIZE);
            OPENXR_CHECK(xrCreateAction(m_actionSet, &actionCI, &xrAction), "Failed to create Action.");
        };
        // XR_DOCS_TAG_END_CreateActionLambda
        // XR_DOCS_TAG_BEGIN_CreateActions
        // An Action for grabbing cubes.
        CreateAction(m_grabCubeAction, "grab-cube", XR_ACTION_TYPE_FLOAT_INPUT, {"/user/hand/left", "/user/hand/right"});
        CreateAction(m_spawnCubeAction, "spawn-cube", XR_ACTION_TYPE_BOOLEAN_INPUT);
        CreateAction(m_changeColorAction, "change-color", XR_ACTION_TYPE_BOOLEAN_INPUT, {"/user/hand/left", "/user/hand/right"});
        // An Action for the position of the palm of the user's hand - appropriate for the location of a grabbing Actions.
        CreateAction(m_palmPoseAction, "palm-pose", XR_ACTION_TYPE_POSE_INPUT, {"/user/hand/left", "/user/hand/right"});
        // An Action for a vibration output on one or other hand.
        CreateAction(m_buzzAction, "buzz", XR_ACTION_TYPE_VIBRATION_OUTPUT, {"/user/hand/left", "/user/hand/right"});
        // For later convenience we create the XrPaths for the subaction path names.
        m_handPaths[0] = CreateXrPath("/user/hand/left");
        m_handPaths[1] = CreateXrPath("/user/hand/right");
    }
    // XR_DOCS_TAG_END_CreateActions

    // XR_DOCS_TAG_BEGIN_SuggestBindings1
    void SuggestBindings() {
        auto SuggestBindings = [](const char *profile_path, Array<XrActionSuggestedBinding> bindings) -> bool {
            // The application can call xrSuggestInteractionProfileBindings once per interaction profile that it supports.
            XrInteractionProfileSuggestedBinding interactionProfileSuggestedBinding{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            interactionProfileSuggestedBinding.interactionProfile = CreateXrPath(profile_path);
            interactionProfileSuggestedBinding.suggestedBindings = &bindings[0];
            interactionProfileSuggestedBinding.countSuggestedBindings = (uint32_t)bindings.num;
            if (xrSuggestInteractionProfileBindings(xrInstance, &interactionProfileSuggestedBinding) == XrResult::XR_SUCCESS)
                return true;
            msg_write(format("Failed to suggest bindings with %s", profile_path));
            return false;
        };
        // XR_DOCS_TAG_END_SuggestBindings1
        // XR_DOCS_TAG_BEGIN_SuggestBindings2
        bool any_ok = false;
        // Each Action here has two paths, one for each SubAction path.
        any_ok |= SuggestBindings("/interaction_profiles/khr/simple_controller", {{m_changeColorAction, CreateXrPath("/user/hand/left/input/select/click")},
                                                                                  {m_grabCubeAction, CreateXrPath("/user/hand/right/input/select/click")},
                                                                                  {m_spawnCubeAction, CreateXrPath("/user/hand/right/input/menu/click")},
                                                                                  {m_palmPoseAction, CreateXrPath("/user/hand/left/input/grip/pose")},
                                                                                  {m_palmPoseAction, CreateXrPath("/user/hand/right/input/grip/pose")},
                                                                                  {m_buzzAction, CreateXrPath("/user/hand/left/output/haptic")},
                                                                                  {m_buzzAction, CreateXrPath("/user/hand/right/output/haptic")}});
        // XR_DOCS_TAG_END_SuggestBindings2
        // XR_DOCS_TAG_BEGIN_SuggestTouchNativeBindings
        // Each Action here has two paths, one for each SubAction path.
        any_ok |= SuggestBindings("/interaction_profiles/oculus/touch_controller", {{m_grabCubeAction, CreateXrPath("/user/hand/left/input/squeeze/value")},
                                                                                    {m_grabCubeAction, CreateXrPath("/user/hand/right/input/squeeze/value")},
                                                                                    {m_spawnCubeAction, CreateXrPath("/user/hand/right/input/a/click")},
                                                                                    {m_changeColorAction, CreateXrPath("/user/hand/left/input/trigger/value")},
                                                                                    {m_changeColorAction, CreateXrPath("/user/hand/right/input/trigger/value")},
                                                                                    {m_palmPoseAction, CreateXrPath("/user/hand/left/input/grip/pose")},
                                                                                    {m_palmPoseAction, CreateXrPath("/user/hand/right/input/grip/pose")},
                                                                                    {m_buzzAction, CreateXrPath("/user/hand/left/output/haptic")},
                                                                                    {m_buzzAction, CreateXrPath("/user/hand/right/output/haptic")}});
        // XR_DOCS_TAG_END_SuggestTouchNativeBindings
        // XR_DOCS_TAG_BEGIN_SuggestBindings3
        if (!any_ok) {
            exit(1);
        }
    }
    void RecordCurrentBindings() {
        if (m_session) {
            // now we are ready to:
            XrInteractionProfileState interactionProfile = {XR_TYPE_INTERACTION_PROFILE_STATE, 0, 0};
            // for each action, what is the binding?
            OPENXR_CHECK(xrGetCurrentInteractionProfile(m_session, m_handPaths[0], &interactionProfile), "Failed to get profile.");
            if (interactionProfile.interactionProfile) {
                msg_write("user/hand/left ActiveProfile " + FromXrPath(interactionProfile.interactionProfile));
            }
            OPENXR_CHECK(xrGetCurrentInteractionProfile(m_session, m_handPaths[1], &interactionProfile), "Failed to get profile.");
            if (interactionProfile.interactionProfile) {
                msg_write("user/hand/right ActiveProfile " + FromXrPath(interactionProfile.interactionProfile));
            }
        }
    }
    // XR_DOCS_TAG_END_SuggestBindings3
    // XR_DOCS_TAG_BEGIN_CreateActionPoses
    void CreateActionPoses() {
        // Create an xrSpace for a pose action.
        auto CreateActionPoseSpace = [](XrSession session, XrAction xrAction, const char *subaction_path = nullptr) -> XrSpace {
            XrSpace xrSpace;
            const XrPosef xrPoseIdentity = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};
            // Create frame of reference for a pose action
            XrActionSpaceCreateInfo actionSpaceCI{XR_TYPE_ACTION_SPACE_CREATE_INFO};
            actionSpaceCI.action = xrAction;
            actionSpaceCI.poseInActionSpace = xrPoseIdentity;
            if (subaction_path)
                actionSpaceCI.subactionPath = CreateXrPath(subaction_path);
            OPENXR_CHECK(xrCreateActionSpace(session, &actionSpaceCI, &xrSpace), "Failed to create ActionSpace.");
            return xrSpace;
        };
        m_handPoseSpace[0] = CreateActionPoseSpace(m_session, m_palmPoseAction, "/user/hand/left");
        m_handPoseSpace[1] = CreateActionPoseSpace(m_session, m_palmPoseAction, "/user/hand/right");
    }
    // XR_DOCS_TAG_END_CreateActionPoses
    // XR_DOCS_TAG_BEGIN_AttachActionSet
    void AttachActionSet() {
        // Attach the action set we just made to the session. We could attach multiple action sets!
        XrSessionActionSetsAttachInfo actionSetAttachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
        actionSetAttachInfo.countActionSets = 1;
        actionSetAttachInfo.actionSets = &m_actionSet;
        OPENXR_CHECK(xrAttachSessionActionSets(m_session, &actionSetAttachInfo), "Failed to attach ActionSet to Session.");
    }

    void PollActions(XrTime predictedTime) {
        // Update our action set with up-to-date input data.
        // First, we specify the actionSet we are polling.
        XrActiveActionSet activeActionSet{};
        activeActionSet.actionSet = m_actionSet;
        activeActionSet.subactionPath = XR_NULL_PATH;
        // Now we sync the Actions to make sure they have current data.
        XrActionsSyncInfo actionsSyncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
        actionsSyncInfo.countActiveActionSets = 1;
        actionsSyncInfo.activeActionSets = &activeActionSet;
        OPENXR_CHECK(xrSyncActions(m_session, &actionsSyncInfo), "Failed to sync Actions.");
        // XR_DOCS_TAG_END_PollActions
        // XR_DOCS_TAG_BEGIN_PollActions2
        XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
        // We pose a single Action, twice - once for each subAction Path.
        actionStateGetInfo.action = m_palmPoseAction;
        // For each hand, get the pose state if possible.
        for (int i = 0; i < 2; i++) {
            // Specify the subAction Path.
            actionStateGetInfo.subactionPath = m_handPaths[i];
            OPENXR_CHECK(xrGetActionStatePose(m_session, &actionStateGetInfo, &m_handPoseState[i]), "Failed to get Pose State.");
            if (m_handPoseState[i].isActive) {
                XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
                XrResult res = xrLocateSpace(m_handPoseSpace[i], m_localSpace, predictedTime, &spaceLocation);
                if (XR_UNQUALIFIED_SUCCESS(res) &&
                    (spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                    (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
                    m_handPose[i] = spaceLocation.pose;
                } else {
                    m_handPoseState[i].isActive = false;
                }
            }
        }

		msg_write(str(*(vec3*)&m_handPose[0].position));
		msg_write(str(*(quaternion*)&m_handPose[0].orientation));

        // XR_DOCS_TAG_END_PollActions2
        // XR_DOCS_TAG_BEGIN_PollActions3
        for (int i = 0; i < 2; i++) {
            actionStateGetInfo.action = m_grabCubeAction;
            actionStateGetInfo.subactionPath = m_handPaths[i];
            OPENXR_CHECK(xrGetActionStateFloat(m_session, &actionStateGetInfo, &m_grabState[i]), "Failed to get Float State of Grab Cube action.");
        }
        for (int i = 0; i < 2; i++) {
            actionStateGetInfo.action = m_changeColorAction;
            actionStateGetInfo.subactionPath = m_handPaths[i];
            OPENXR_CHECK(xrGetActionStateBoolean(m_session, &actionStateGetInfo, &m_changeColorState[i]), "Failed to get Boolean State of Change Color action.");
        }
        // The Spawn Cube action has no subActionPath:
        {
            actionStateGetInfo.action = m_spawnCubeAction;
            actionStateGetInfo.subactionPath = 0;
            OPENXR_CHECK(xrGetActionStateBoolean(m_session, &actionStateGetInfo, &m_spawnCubeState), "Failed to get Boolean State of Spawn Cube action.");
        }
        // XR_DOCS_TAG_END_PollActions3
        // XR_DOCS_TAG_BEGIN_PollActions4
        for (int i = 0; i < 2; i++) {
            m_buzz[i] *= 0.5f;
            if (m_buzz[i] < 0.01f)
                m_buzz[i] = 0.0f;
            XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
            vibration.amplitude = m_buzz[i];
            vibration.duration = XR_MIN_HAPTIC_DURATION;
            vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

            XrHapticActionInfo hapticActionInfo{XR_TYPE_HAPTIC_ACTION_INFO};
            hapticActionInfo.action = m_buzzAction;
            hapticActionInfo.subactionPath = m_handPaths[i];
            OPENXR_CHECK(xrApplyHapticFeedback(m_session, &hapticActionInfo, (XrHapticBaseHeader *)&vibration), "Failed to apply haptic feedback.");
        }
    }

void PollEvents() {

	// Poll OpenXR for a new event.
	XrEventDataBuffer eventData{XR_TYPE_EVENT_DATA_BUFFER};
	auto XrPollEvents = [&]() -> bool {
		eventData = {XR_TYPE_EVENT_DATA_BUFFER};
		return xrPollEvent(xrInstance, &eventData) == XR_SUCCESS;
	};

	while (XrPollEvents()) {
		switch (eventData.type) {
			// Log the number of lost events from the runtime.
			case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
				XrEventDataEventsLost *eventsLost = reinterpret_cast<XrEventDataEventsLost *>(&eventData);
				msg_write(format("OPENXR: Events Lost: %d", (int)eventsLost->lostEventCount));
				break;
			}
			// Log that an instance loss is pending and shutdown the application.
			case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
				XrEventDataInstanceLossPending *instanceLossPending = reinterpret_cast<XrEventDataInstanceLossPending *>(&eventData);
				msg_write(format("OPENXR: Instance Loss Pending at: %d", (int64)instanceLossPending->lossTime));
				m_sessionRunning = false;
				m_applicationRunning = false;
				exit(1);
				break;
			}
			// Log that the interaction profile has changed.
			case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
				XrEventDataInteractionProfileChanged *interactionProfileChanged = reinterpret_cast<XrEventDataInteractionProfileChanged *>(&eventData);
				msg_write(format("OPENXR: Interaction Profile changed for Session: %s", p2s(interactionProfileChanged->session)));
				if (interactionProfileChanged->session != m_session) {
					msg_write("XrEventDataInteractionProfileChanged for unknown Session");
					break;
				}
				RecordCurrentBindings();
				break;
			}
			// Log that there's a reference space change pending.
			case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
				XrEventDataReferenceSpaceChangePending *referenceSpaceChangePending = reinterpret_cast<XrEventDataReferenceSpaceChangePending *>(&eventData);
				msg_write("OPENXR: Reference Space Change pending for Session: " + p2s(referenceSpaceChangePending->session));
				if (referenceSpaceChangePending->session != m_session) {
					msg_write("XrEventDataReferenceSpaceChangePending for unknown Session");
					break;
				}
				break;
			}
			// Session State changes:
			case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
				XrEventDataSessionStateChanged *sessionStateChanged = reinterpret_cast<XrEventDataSessionStateChanged *>(&eventData);
				if (sessionStateChanged->session != m_session) {
					msg_write("XrEventDataSessionStateChanged for unknown Session");
					break;
				}

				if (sessionStateChanged->state == XR_SESSION_STATE_READY) {
					// SessionState is ready. Begin the XrSession using the XrViewConfigurationType.
					XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
					sessionBeginInfo.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
					OPENXR_CHECK(xrBeginSession(m_session, &sessionBeginInfo), "Failed to begin Session.");
					m_sessionRunning = true;
				}
				if (sessionStateChanged->state == XR_SESSION_STATE_STOPPING) {
					// SessionState is stopping. End the XrSession.
					OPENXR_CHECK(xrEndSession(m_session), "Failed to end Session.");
					m_sessionRunning = false;
				}
				if (sessionStateChanged->state == XR_SESSION_STATE_EXITING) {
					// SessionState is exiting. Exit the application.
					m_sessionRunning = false;
                    m_applicationRunning = false;
				}
				if (sessionStateChanged->state == XR_SESSION_STATE_LOSS_PENDING) {
					// SessionState is loss pending. Exit the application.
					// It's possible to try a reestablish an XrInstance and XrSession, but we will simply exit here.
					m_sessionRunning = false;
                    m_applicationRunning = false;
				}
				// Store state for reference across the application.
				m_sessionState = sessionStateChanged->state;
				break;
			}
			default: {
				break;
			}
		}
	}
}

void Instance::iterate() {

	if (!m_sessionRunning)
		return;


	XrFrameState frameState{XR_TYPE_FRAME_STATE};
	XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
	OPENXR_CHECK(xrWaitFrame(m_session, &frameWaitInfo, &frameState), "Failed to wait for XR Frame.");

	// Tell the OpenXR compositor that the application is beginning the frame.
	XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
	OPENXR_CHECK(xrBeginFrame(m_session, &frameBeginInfo), "Failed to begin the XR Frame.");

	// Variables for rendering and layer composition.
	bool rendered = false;
	//RenderLayerInfo renderLayerInfo;
	//renderLayerInfo.predictedDisplayTime = frameState.predictedDisplayTime;

	// Check that the session is active and that we should render.
	bool sessionActive = (m_sessionState == XR_SESSION_STATE_SYNCHRONIZED || m_sessionState == XR_SESSION_STATE_VISIBLE || m_sessionState == XR_SESSION_STATE_FOCUSED);
	//if (sessionActive && frameState.shouldRender) {
		PollActions(frameState.predictedDisplayTime);
	//}
}

Array<const char*> activeInstanceExtensions;

void* _create_instance(const string& engine, const string& app_name) {
	instance = new Instance;

	XrApplicationInfo AI;
	strncpy(AI.applicationName, app_name.c_str(), XR_MAX_APPLICATION_NAME_SIZE);
	AI.applicationVersion = 1;
	strncpy(AI.engineName, engine.c_str(), XR_MAX_ENGINE_NAME_SIZE);
	AI.engineVersion = 1;
	AI.apiVersion = XR_API_VERSION_1_0;


	Array<const char*> extensions = {
		XR_EXT_DEBUG_UTILS_EXTENSION_NAME,
		XR_KHR_VULKAN_ENABLE_EXTENSION_NAME
	};

	// Get all the API Layers from the OpenXR runtime.
	uint32_t apiLayerCount = 0;
	Array<XrApiLayerProperties> apiLayerProperties;
	OPENXR_CHECK(xrEnumerateApiLayerProperties(0, &apiLayerCount, nullptr), "Failed to enumerate ApiLayerProperties.");
	apiLayerProperties.resize((int)apiLayerCount);
	for (auto& p: apiLayerProperties)
		p = {.type = XR_TYPE_API_LAYER_PROPERTIES};
	OPENXR_CHECK(xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount, &apiLayerProperties[0]), "Failed to enumerate ApiLayerProperties.");

        // Check the requested API layers against the ones from the OpenXR. If found add it to the Active API Layers.
/*        for (auto &requestLayer : m_apiLayers) {
            for (auto &layerProperty : apiLayerProperties) {
                // strcmp returns 0 if the strings match.
                if (strcmp(requestLayer.c_str(), layerProperty.layerName) != 0) {
                    continue;
                } else {
                    m_activeAPILayers.push_back(requestLayer.c_str());
                    break;
                }
            }
        }*/

	// Get all the Instance Extensions from the OpenXR instance.
	uint32_t extensionCount = 0;
	Array<XrExtensionProperties> extensionProperties;
	OPENXR_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr), "Failed to enumerate InstanceExtensionProperties.");
	extensionProperties.resize((int)extensionCount);
	for (auto& p: extensionProperties)
		p = {.type = XR_TYPE_EXTENSION_PROPERTIES};
	OPENXR_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, &extensionProperties[0]), "Failed to enumerate InstanceExtensionProperties.");

	// Check the requested Instance Extensions against the ones from the OpenXR runtime.
	// If an extension is found add it to Active Instance Extensions.
	// Log error if the Instance Extension is not found.
	for (auto &requestedInstanceExtension : extensions) {
		bool found = false;
		for (auto &extensionProperty : extensionProperties) {
			// strcmp returns 0 if the strings match.
			if (strcmp(requestedInstanceExtension, extensionProperty.extensionName) != 0) {
				continue;
			} else {
				activeInstanceExtensions.add(requestedInstanceExtension);
				found = true;
				break;
			}
		}
		if (!found) {
			msg_error(format("Failed to find OpenXR instance extension: %s", requestedInstanceExtension));
		}
	}

	// Fill out an XrInstanceCreateInfo structure and create an XrInstance.
	XrInstanceCreateInfo instanceCI{XR_TYPE_INSTANCE_CREATE_INFO};
	instanceCI.createFlags = 0;
	instanceCI.applicationInfo = AI;
	instanceCI.enabledApiLayerCount = 0;//static_cast<uint32_t>(m_activeAPILayers.size());
	instanceCI.enabledApiLayerNames = nullptr;//m_activeAPILayers.data();
	instanceCI.enabledExtensionCount = static_cast<uint32_t>(activeInstanceExtensions.num);
	instanceCI.enabledExtensionNames = &activeInstanceExtensions[0];
	OPENXR_CHECK(xrCreateInstance(&instanceCI, &xrInstance), "Failed to create Instance.");
	instance->instance = xrInstance;

	return xrInstance;
}

void _create_debug_messenger() {
	if (base::find(activeInstanceExtensions, (const char*)XR_EXT_DEBUG_UTILS_EXTENSION_NAME))
		m_debugUtilsMessenger = CreateOpenXRDebugUtilsMessenger(xrInstance);
}


void _destroy_debug_messenger() {
	if (m_debugUtilsMessenger != XR_NULL_HANDLE)
		DestroyOpenXRDebugUtilsMessenger(xrInstance, m_debugUtilsMessenger);  // From OpenXRDebugUtils.h.
}

void _get_system_id() {
	// Get the instance's properties and log the runtime name and version.
	XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
	OPENXR_CHECK(xrGetInstanceProperties(xrInstance, &instanceProperties), "Failed to get InstanceProperties.");

	msg_write(format("OpenXR Runtime: %s - %d.%d.%d", instanceProperties.runtimeName, (int)XR_VERSION_MAJOR(instanceProperties.runtimeVersion), (int)XR_VERSION_MINOR(instanceProperties.runtimeVersion), (int)XR_VERSION_PATCH(instanceProperties.runtimeVersion)));

	XrFormFactor formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	systemID = {};
	XrSystemProperties systemProperties = {XR_TYPE_SYSTEM_PROPERTIES};

	// Get the XrSystemId from the instance and the supplied XrFormFactor.
	XrSystemGetInfo systemGI{XR_TYPE_SYSTEM_GET_INFO};
	systemGI.formFactor = formFactor;
	OPENXR_CHECK(xrGetSystem(xrInstance, &systemGI, &systemID), "Failed to get SystemID.");

	// Get the System's properties for some general information about the hardware and the vendor.
	OPENXR_CHECK(xrGetSystemProperties(xrInstance, systemID, &systemProperties), "Failed to get SystemProperties.");
	msg_write(format("got system: %s", systemProperties.systemName));
	msg_write(format("max %d x %d", (int)systemProperties.graphicsProperties.maxSwapchainImageHeight, (int)systemProperties.graphicsProperties.maxSwapchainImageWidth));
}

void init(const string& engine, const string& app_name) {
	msg_error("init openxr...");
	instance = new Instance;

	_create_instance(engine, app_name);
	_create_debug_messenger();
	_get_system_id();

	GetViewConfigurationViews();
	GetEnvironmentBlendModes();

	OPENXR_CHECK(xrGetInstanceProcAddr(xrInstance, "xrGetVulkanGraphicsRequirementsKHR", (PFN_xrVoidFunction *)&xrGetVulkanGraphicsRequirementsKHR), "Failed to get InstanceProcAddr for xrGetVulkanGraphicsRequirementsKHR.");
	OPENXR_CHECK(xrGetInstanceProcAddr(xrInstance, "xrGetVulkanInstanceExtensionsKHR", (PFN_xrVoidFunction *)&xrGetVulkanInstanceExtensionsKHR), "Failed to get InstanceProcAddr for xrGetVulkanInstanceExtensionsKHR.");
	OPENXR_CHECK(xrGetInstanceProcAddr(xrInstance, "xrGetVulkanDeviceExtensionsKHR", (PFN_xrVoidFunction *)&xrGetVulkanDeviceExtensionsKHR), "Failed to get InstanceProcAddr for xrGetVulkanDeviceExtensionsKHR.");
	OPENXR_CHECK(xrGetInstanceProcAddr(xrInstance, "xrGetVulkanGraphicsDeviceKHR", (PFN_xrVoidFunction *)&xrGetVulkanGraphicsDeviceKHR), "Failed to get InstanceProcAddr for xrGetVulkanGraphicsDeviceKHR.");
}

void end() {
	OPENXR_CHECK(xrDestroySession(m_session), "Failed to destroy Session.");
	_destroy_debug_messenger();
	OPENXR_CHECK(xrDestroyInstance(xrInstance), "Failed to destroy Instance.");
}

Array<string> GetInstanceExtensionsForOpenXR(XrInstance m_xrInstance, XrSystemId systemId) {
	uint32_t extensionNamesSize = 0;
	OPENXR_CHECK(xrGetVulkanInstanceExtensionsKHR(m_xrInstance, systemId, 0, &extensionNamesSize, nullptr), "Failed to get Vulkan Instance Extensions.");

	string extensionNames;
	extensionNames.resize((int)extensionNamesSize);
	OPENXR_CHECK(xrGetVulkanInstanceExtensionsKHR(m_xrInstance, systemId, extensionNamesSize, &extensionNamesSize, (char*)&extensionNames[0]), "Failed to get Vulkan Instance Extensions.");
	msg_write(">>>>>  INSTANCE EXT:  " + extensionNames);
	return extensionNames.explode(" ");
}

Array<string> GetDeviceExtensionsForOpenXR(XrInstance m_xrInstance, XrSystemId systemId) {
	uint32_t extensionNamesSize = 0;
	OPENXR_CHECK(xrGetVulkanDeviceExtensionsKHR(m_xrInstance, systemId, 0, &extensionNamesSize, nullptr), "Failed to get Vulkan Device Extensions.");

	string extensionNames;
	extensionNames.resize((int)extensionNamesSize);
	OPENXR_CHECK(xrGetVulkanDeviceExtensionsKHR(m_xrInstance, systemId, extensionNamesSize, &extensionNamesSize, (char*)&extensionNames[0]), "Failed to get Vulkan Device Extensions.");
	msg_write(">>>>>  DEVICE EXT:  " + extensionNames);
	return extensionNames.explode(" ");
}


void GetEnvironmentBlendModes() {
	// Retrieves the available blend modes. The first call gets the count of the array that will be returned. The next call fills out the array.
	uint32_t environmentBlendModeCount = 0;
	OPENXR_CHECK(xrEnumerateEnvironmentBlendModes(xrInstance, systemID, view_type, 0, &environmentBlendModeCount, nullptr), "Failed to enumerate EnvironmentBlend Modes.");
	m_environmentBlendModes.resize(environmentBlendModeCount);
	OPENXR_CHECK(xrEnumerateEnvironmentBlendModes(xrInstance, systemID, view_type, environmentBlendModeCount, &environmentBlendModeCount, m_environmentBlendModes.data()), "Failed to enumerate EnvironmentBlend Modes.");

	// Pick the first application supported blend mode supported by the hardware.
	for (const XrEnvironmentBlendMode &environmentBlendMode : m_applicationEnvironmentBlendModes) {
		if (std::find(m_environmentBlendModes.begin(), m_environmentBlendModes.end(), environmentBlendMode) != m_environmentBlendModes.end()) {
			m_environmentBlendMode = environmentBlendMode;
			break;
		}
	}
	if (m_environmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM) {
		msg_error("Failed to find a compatible blend mode. Defaulting to XR_ENVIRONMENT_BLEND_MODE_OPAQUE.");
		m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	}
}

void GetViewConfigurationViews() {
	uint32_t viewConfigurationCount = 0;
	OPENXR_CHECK(xrEnumerateViewConfigurations(xrInstance, systemID, 0, &viewConfigurationCount, nullptr), "Failed to enumerate View Configurations.");
	// supported by vr runtime:
	Array<XrViewConfigurationType> m_viewConfigurations;
	m_viewConfigurations.resize((int)viewConfigurationCount);
	OPENXR_CHECK(xrEnumerateViewConfigurations(xrInstance, systemID, viewConfigurationCount, &viewConfigurationCount, &m_viewConfigurations[0]), "Failed to enumerate View Configurations.");

	// Pick the first application supported View Configuration Type con supported by the hardware.
	if (base::find(m_viewConfigurations, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO)) {
		view_type = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
		msg_write("view config: stereo!");
	} else if (base::find(m_viewConfigurations, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO)) {
		view_type = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_MONO;
		msg_write("view config: mono!");
	} else {
		msg_write("Failed to find a view configuration type. Defaulting to XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO.");
		view_type = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
	}

	// Gets the View Configuration Views. The first call gets the count of the array that will be returned. The next call fills out the array.
	uint32_t viewConfigurationViewCount = 0;
	OPENXR_CHECK(xrEnumerateViewConfigurationViews(xrInstance, systemID, view_type, 0, &viewConfigurationViewCount, nullptr), "Failed to enumerate ViewConfiguration Views.");
	m_viewConfigurationViews.resize(viewConfigurationViewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
	OPENXR_CHECK(xrEnumerateViewConfigurationViews(xrInstance, systemID, view_type, viewConfigurationViewCount, &viewConfigurationViewCount, m_viewConfigurationViews.data()), "Failed to enumerate ViewConfiguration Views.");
	msg_write(format("view count: %d", (int)viewConfigurationViewCount));
}

void CreateReferenceSpace() {
	// Fill out an XrReferenceSpaceCreateInfo structure and create a reference XrSpace, specifying a Local space with an identity pose as the origin.
	XrReferenceSpaceCreateInfo referenceSpaceCI{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
	referenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
	referenceSpaceCI.poseInReferenceSpace = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};
	OPENXR_CHECK(xrCreateReferenceSpace(m_session, &referenceSpaceCI, &m_localSpace), "Failed to create ReferenceSpace.");
}
void DestroyReferenceSpace() {
	// Destroy the reference XrSpace.
	OPENXR_CHECK(xrDestroySpace(m_localSpace), "Failed to destroy Space.")
}

void CreateSwapchains() {
	msg_write("create swap chain...");
	// Get the supported swapchain formats as an array of int64_t and ordered by runtime preference.
	uint32_t formatCount = 0;
	OPENXR_CHECK(xrEnumerateSwapchainFormats(m_session, 0, &formatCount, nullptr), "Failed to enumerate Swapchain Formats");
	std::vector<int64_t> formats(formatCount);
	OPENXR_CHECK(xrEnumerateSwapchainFormats(m_session, formatCount, &formatCount, formats.data()), "Failed to enumerate Swapchain Formats");
	msg_write("----formats");
	for (auto f: formats)
	msg_write(i642s(f));
	msg_write("----");
	//vulkan::default_device->find_depth_format();
	/*if (m_graphicsAPI->SelectDepthSwapchainFormat(formats) == 0) {
		msg_error("Failed to find depth format for Swapchain.");
		exit(1);
	}*/


	//Resize the SwapchainInfo to match the number of view in the View Configuration.
	m_colorSwapchainInfos.resize(m_viewConfigurationViews.size());
	m_depthSwapchainInfos.resize(m_viewConfigurationViews.size());

	// Per view, create a color and depth swapchain, and their associated image views.
	for (size_t i = 0; i < m_viewConfigurationViews.size(); i++) {
		// XR_DOCS_TAG_BEGIN_CreateSwapchains
		SwapchainInfo &colorSwapchainInfo = m_colorSwapchainInfos[i];
		SwapchainInfo &depthSwapchainInfo = m_depthSwapchainInfos[i];

		// Fill out an XrSwapchainCreateInfo structure and create an XrSwapchain.
		// Color.
		XrSwapchainCreateInfo swapchainCI{XR_TYPE_SWAPCHAIN_CREATE_INFO};
		swapchainCI.createFlags = 0;
		swapchainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
		//vulkan::choose_swap_surface_format(formats, true);
		swapchainCI.format = VK_FORMAT_R8G8B8A8_SRGB;
		swapchainCI.sampleCount = m_viewConfigurationViews[i].recommendedSwapchainSampleCount;  // Use the recommended values from the XrViewConfigurationView.
		swapchainCI.width = m_viewConfigurationViews[i].recommendedImageRectWidth;
		swapchainCI.height = m_viewConfigurationViews[i].recommendedImageRectHeight;
		swapchainCI.faceCount = 1;
		swapchainCI.arraySize = 1;
		swapchainCI.mipCount = 1;
		OPENXR_CHECK(xrCreateSwapchain(m_session, &swapchainCI, &colorSwapchainInfo.swapchain), "Failed to create Color Swapchain");
		colorSwapchainInfo.swapchainFormat = swapchainCI.format;  // Save the swapchain format for later use.

		// Depth.
		swapchainCI.createFlags = 0;
		swapchainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		swapchainCI.format = VK_FORMAT_D32_SFLOAT;//vulkan::default_device->find_depth_format();
		swapchainCI.sampleCount = m_viewConfigurationViews[i].recommendedSwapchainSampleCount;  // Use the recommended values from the XrViewConfigurationView.
		swapchainCI.width = m_viewConfigurationViews[i].recommendedImageRectWidth;
		swapchainCI.height = m_viewConfigurationViews[i].recommendedImageRectHeight;
		swapchainCI.faceCount = 1;
		swapchainCI.arraySize = 1;
		swapchainCI.mipCount = 1;
		OPENXR_CHECK(xrCreateSwapchain(m_session, &swapchainCI, &depthSwapchainInfo.swapchain), "Failed to create Depth Swapchain");
		depthSwapchainInfo.swapchainFormat = swapchainCI.format;  // Save the swapchain format for later use.
		// XR_DOCS_TAG_END_CreateSwapchains

		// XR_DOCS_TAG_BEGIN_EnumerateSwapchainImages
		// Get the number of images in the color/depth swapchain and allocate Swapchain image data via GraphicsAPI to store the returned array.
		uint32_t colorSwapchainImageCount = 0;
		OPENXR_CHECK(xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, 0, &colorSwapchainImageCount, nullptr), "Failed to enumerate Color Swapchain Images.");
		XrSwapchainImageBaseHeader *colorSwapchainImages = AllocateSwapchainImageData(colorSwapchainInfo.swapchain, 0, colorSwapchainImageCount);
		OPENXR_CHECK(xrEnumerateSwapchainImages(colorSwapchainInfo.swapchain, colorSwapchainImageCount, &colorSwapchainImageCount, colorSwapchainImages), "Failed to enumerate Color Swapchain Images.");

		uint32_t depthSwapchainImageCount = 0;
		OPENXR_CHECK(xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, 0, &depthSwapchainImageCount, nullptr), "Failed to enumerate Depth Swapchain Images.");
		XrSwapchainImageBaseHeader *depthSwapchainImages = AllocateSwapchainImageData(depthSwapchainInfo.swapchain, 1, depthSwapchainImageCount);
		OPENXR_CHECK(xrEnumerateSwapchainImages(depthSwapchainInfo.swapchain, depthSwapchainImageCount, &depthSwapchainImageCount, depthSwapchainImages), "Failed to enumerate Depth Swapchain Images.");
		// XR_DOCS_TAG_END_EnumerateSwapchainImages

		msg_write(format("swapchain images: %d color, %d depth", (int)colorSwapchainImageCount, (int)depthSwapchainImageCount));

		View v;

		// Per image in the swapchains, fill out a GraphicsAPI::ImageViewCreateInfo structure and create a color/depth image view.
		for (uint32_t j = 0; j < colorSwapchainImageCount; j++) {
			ImageViewCreateInfo imageViewCI;
			imageViewCI.image = GetSwapchainImage(colorSwapchainInfo.swapchain, j);
			imageViewCI.type = ImageViewCreateInfo::Type::RTV;
			imageViewCI.view = ImageViewCreateInfo::View::TYPE_2D;
			imageViewCI.format = colorSwapchainInfo.swapchainFormat;
			imageViewCI.aspect = ImageViewCreateInfo::Aspect::COLOR_BIT;
			imageViewCI.baseMipLevel = 0;
			imageViewCI.levelCount = 1;
			imageViewCI.baseArrayLayer = 0;
			imageViewCI.layerCount = 1;


			auto t = new vulkan::Texture;
			t->width = (int)m_viewConfigurationViews[i].recommendedImageRectWidth;
			t->height = (int)m_viewConfigurationViews[i].recommendedImageRectHeight;
			t->image.image = (VkImage)imageViewCI.image;
			t->image.format = (VkFormat)imageViewCI.format;
			t->view = (VkImageView)CreateImageView(imageViewCI);
			v.textures.add(t);
		}
		for (uint32_t j = 0; j < depthSwapchainImageCount; j++) {
			ImageViewCreateInfo imageViewCI;
			imageViewCI.image = GetSwapchainImage(depthSwapchainInfo.swapchain, j);
			imageViewCI.type = ImageViewCreateInfo::Type::DSV;
			imageViewCI.view = ImageViewCreateInfo::View::TYPE_2D;
			imageViewCI.format = depthSwapchainInfo.swapchainFormat;
			imageViewCI.aspect = ImageViewCreateInfo::Aspect::DEPTH_BIT;
			imageViewCI.baseMipLevel = 0;
			imageViewCI.levelCount = 1;
			imageViewCI.baseArrayLayer = 0;
			imageViewCI.layerCount = 1;

			auto t = new vulkan::Texture;
			t->type = vulkan::Texture::Type::DEPTH;
			t->width = (int)m_viewConfigurationViews[i].recommendedImageRectWidth;
			t->height = (int)m_viewConfigurationViews[i].recommendedImageRectHeight;
			t->image.image = (VkImage)imageViewCI.image;
			t->image.format = (VkFormat)imageViewCI.format;
			t->view = (VkImageView)CreateImageView(imageViewCI);
			v.depth_buffers.add(t);
		}
		instance->views.add(v);
	}
	msg_write("ok");
}

void FreeSwapchainImageData(XrSwapchain swapchain) {
	swapchainImagesMap[swapchain].second.clear();
	swapchainImagesMap.erase(swapchain);
}

void DestroySwapchains() {
	// Per view in the view configuration:
	for (size_t i = 0; i < vr::m_viewConfigurationViews.size(); i++) {
		SwapchainInfo &colorSwapchainInfo = vr::m_colorSwapchainInfos[i];
		SwapchainInfo &depthSwapchainInfo = vr::m_depthSwapchainInfos[i];

		// Destroy the color and depth image views from GraphicsAPI.
		instance->views[i].textures.clear();
		instance->views[i].depth_buffers.clear();

		// Free the Swapchain Image Data.
		FreeSwapchainImageData(colorSwapchainInfo.swapchain);
		FreeSwapchainImageData(depthSwapchainInfo.swapchain);

		// Destroy the swapchains.
		OPENXR_CHECK(xrDestroySwapchain(colorSwapchainInfo.swapchain), "Failed to destroy Color Swapchain");
		OPENXR_CHECK(xrDestroySwapchain(depthSwapchainInfo.swapchain), "Failed to destroy Depth Swapchain");
	}
}

yrenderer::Context* Instance::create_yrenderer() {
	msg_write("vr create_yrenderer");

	XrGraphicsRequirementsVulkanKHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_VULKAN_KHR};
	OPENXR_CHECK(xrGetVulkanGraphicsRequirementsKHR(xrInstance, systemID, &graphicsRequirements), "Failed to get Graphics Requirements for Vulkan.");
	msg_write(format("min vk version: %x", (int64)graphicsRequirements.minApiVersionSupported));
	msg_write(format("max vk version: %x", (int64)graphicsRequirements.maxApiVersionSupported));

	vulkan::additional_instance_extensions = GetInstanceExtensionsForOpenXR(xrInstance, systemID);

	auto vk_instance = vulkan::init({"glfw", "validation", "api=1.2", "rtx?", "verbosity=3"});



	auto device = new vulkan::Device;

	VkPhysicalDevice physicalDeviceFromXR;
	OPENXR_CHECK(xrGetVulkanGraphicsDeviceKHR(xrInstance, systemID, vk_instance->instance, &physicalDeviceFromXR), "Failed to get Graphics Device for Vulkan.");
	device->physical_device = physicalDeviceFromXR;

	vulkan::additional_device_extensions = GetDeviceExtensionsForOpenXR(xrInstance, systemID);
	msg_write("logical dev");
	device->create_logical_device(nullptr);


	device->command_pool = new vulkan::CommandPool(device);

	if (device->features.contains(vulkan::Feature::RTX))
		device->get_rtx_properties();

	vulkan::default_device = device;

	auto ctx = new ygfx::Context(vk_instance, device);
	auto ctx2 = new yrenderer::Context(ctx);
	yrenderer::_create_context_stuff(ctx2);
	return ctx2;
}

struct RenderLayerInfo {
	XrTime predictedDisplayTime = 0;
	std::vector<XrCompositionLayerBaseHeader *> layers;
	XrCompositionLayerProjection layerProjection = {XR_TYPE_COMPOSITION_LAYER_PROJECTION};
	std::vector<XrCompositionLayerProjectionView> layerProjectionViews;
};

XrFrameState frameState;
RenderLayerInfo renderLayerInfo;
Array<XrView> cur_views;
uint32_t viewCount = 0;
int viewWidth, viewHeight;
SwapchainInfo *colorSwapchainInfo;
SwapchainInfo *depthSwapchainInfo;
uint32_t colorImageIndex = 0;
uint32_t depthImageIndex = 0;

bool render_frame_start() {
	// Get the XrFrameState for timing and rendering info.
	frameState = {XR_TYPE_FRAME_STATE};
	XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
	OPENXR_CHECK(xrWaitFrame(m_session, &frameWaitInfo, &frameState), "Failed to wait for XR Frame.");

	// Tell the OpenXR compositor that the application is beginning the frame.
	XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
	OPENXR_CHECK(xrBeginFrame(m_session, &frameBeginInfo), "Failed to begin the XR Frame.");

	// Variables for rendering and layer composition.
	renderLayerInfo = {};
	renderLayerInfo.predictedDisplayTime = frameState.predictedDisplayTime;

	// Check that the session is active and that we should render.
	bool sessionActive = (m_sessionState == XR_SESSION_STATE_SYNCHRONIZED || m_sessionState == XR_SESSION_STATE_VISIBLE || m_sessionState == XR_SESSION_STATE_FOCUSED);
	return sessionActive and frameState.shouldRender;
}

void render_frame_end() {
	XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
	frameEndInfo.displayTime = frameState.predictedDisplayTime;
	frameEndInfo.environmentBlendMode = m_environmentBlendMode;
	frameEndInfo.layerCount = static_cast<uint32_t>(renderLayerInfo.layers.size());
	frameEndInfo.layers = renderLayerInfo.layers.data();
	OPENXR_CHECK(xrEndFrame(m_session, &frameEndInfo), "Failed to end the XR Frame.");
}

bool render_layer_start() {
	// Locate the views from the view configuration within the (reference) space at the display time.
	cur_views.resize((int)m_viewConfigurationViews.size());
	for (auto& v: cur_views)
		v = {XR_TYPE_VIEW};

	XrViewState viewState{XR_TYPE_VIEW_STATE};  // Will contain information on whether the position and/or orientation is valid and/or tracked.
	XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
	viewLocateInfo.viewConfigurationType = view_type;
	viewLocateInfo.displayTime = renderLayerInfo.predictedDisplayTime;
	viewLocateInfo.space = m_localSpace;
	viewCount = 0;
	XrResult result = xrLocateViews(m_session, &viewLocateInfo, &viewState, static_cast<uint32_t>(cur_views.num), &viewCount, &cur_views[0]);
	if (result != XR_SUCCESS) {
		msg_error("Failed to locate Views.");
		return false;
	}

	// Resize the layer projection views to match the view count. The layer projection views are used in the layer projection.
	renderLayerInfo.layerProjectionViews.resize(viewCount, {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW});
	return true;
}

void render_layer_end() {
	renderLayerInfo.layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader *>(&renderLayerInfo.layerProjection));

	// Fill out the XrCompositionLayerProjection structure for usage with xrEndFrame().
	renderLayerInfo.layerProjection.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
	renderLayerInfo.layerProjection.space = m_localSpace;
	renderLayerInfo.layerProjection.viewCount = static_cast<uint32_t>(renderLayerInfo.layerProjectionViews.size());
	renderLayerInfo.layerProjection.views = renderLayerInfo.layerProjectionViews.data();
}

bool Instance::start_frame() {
	if (!m_sessionRunning)
		return false;
//	msg_write("<<----");

	// Get the XrFrameState for timing and rendering info.
	frameState = {XR_TYPE_FRAME_STATE};
	XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
	OPENXR_CHECK(xrWaitFrame(m_session, &frameWaitInfo, &frameState), "Failed to wait for XR Frame.");

	// Tell the OpenXR compositor that the application is beginning the frame.
	XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
	OPENXR_CHECK(xrBeginFrame(m_session, &frameBeginInfo), "Failed to begin the XR Frame.");

	renderLayerInfo.predictedDisplayTime = frameState.predictedDisplayTime;
	return true;
}

void Instance::end_frame() {
	render_frame_end();
//	msg_write("---->>");
}

void Instance::start_view(int index, vulkan::RenderPass* render_pass) {
//	msg_write("<<");

	colorSwapchainInfo = &m_colorSwapchainInfos[index];
	depthSwapchainInfo = &m_depthSwapchainInfos[index];

	// Acquire and wait for an image from the swapchains.
	// Get the image index of an image in the swapchains.
	// The timeout is infinite.
	colorImageIndex = 0;
	depthImageIndex = 0;
	XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
	OPENXR_CHECK(xrAcquireSwapchainImage(colorSwapchainInfo->swapchain, &acquireInfo, &colorImageIndex), "Failed to acquire Image from the Color Swapchian");
	OPENXR_CHECK(xrAcquireSwapchainImage(depthSwapchainInfo->swapchain, &acquireInfo, &depthImageIndex), "Failed to acquire Image from the Depth Swapchian");
	image_index = (int)colorImageIndex;
//	msg_write(format("acq  %d  %d", colorImageIndex, depthImageIndex));

	XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
	waitInfo.timeout = XR_INFINITE_DURATION;
	OPENXR_CHECK(xrWaitSwapchainImage(colorSwapchainInfo->swapchain, &waitInfo), "Failed to wait for Image from the Color Swapchain");
	OPENXR_CHECK(xrWaitSwapchainImage(depthSwapchainInfo->swapchain, &waitInfo), "Failed to wait for Image from the Depth Swapchain");

//	msg_write("v: " + str(*(vec3*)&cur_views[index].pose.position) + str(*(quaternion*)&cur_views[index].pose.orientation));


	// Get the width and height and construct the viewport and scissors.
	const uint32_t &width = m_viewConfigurationViews[index].recommendedImageRectWidth;
	const uint32_t &height = m_viewConfigurationViews[index].recommendedImageRectHeight;
	viewWidth = width;
	viewHeight = height;

	// Fill out the XrCompositionLayerProjectionView structure specifying the pose and fov from the view.
	// This also associates the swapchain image with this layer projection view.
	renderLayerInfo.layerProjectionViews[index] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
	renderLayerInfo.layerProjectionViews[index].pose = cur_views[index].pose;
	renderLayerInfo.layerProjectionViews[index].fov = cur_views[index].fov;
	renderLayerInfo.layerProjectionViews[index].subImage.swapchain = colorSwapchainInfo->swapchain;
	renderLayerInfo.layerProjectionViews[index].subImage.imageRect.offset.x = 0;
	renderLayerInfo.layerProjectionViews[index].subImage.imageRect.offset.y = 0;
	renderLayerInfo.layerProjectionViews[index].subImage.imageRect.extent.width = static_cast<int32_t>(width);
	renderLayerInfo.layerProjectionViews[index].subImage.imageRect.extent.height = static_cast<int32_t>(height);
	renderLayerInfo.layerProjectionViews[index].subImage.imageArrayIndex = 0;  // Useful for multiview rendering.


	if (views[index].framebuffer)
		views[index].framebuffer->update(render_pass, {views[index].textures[(int)colorImageIndex], views[index].depth_buffers[(int)depthImageIndex]});
	else
		views[index].framebuffer = new vulkan::FrameBuffer(render_pass, {views[index].textures[(int)colorImageIndex], views[index].depth_buffers[(int)depthImageIndex]});
}

void Instance::end_view(int) {
	XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
	OPENXR_CHECK(xrReleaseSwapchainImage(colorSwapchainInfo->swapchain, &releaseInfo), "Failed to release Image back to the Color Swapchain");
	OPENXR_CHECK(xrReleaseSwapchainImage(depthSwapchainInfo->swapchain, &releaseInfo), "Failed to release Image back to the Depth Swapchain");
//	msg_write(">>");
}

#else
void init(const string& engine, const string& app_name) {}
void end() {}

yrenderer::Context* Instance::create_yrenderer() { return nullptr; }
void Instance::create_session(yrenderer::Context* ctx) {}
void Instance::iterate() {}

bool Instance::start_frame() { return false; }
void Instance::end_frame() {}
void Instance::start_view(int index, vulkan::RenderPass* render_pass) {}
void Instance::end_view(int index) {}


void* _create_instance(const string& engine, const string& app_name) { return nullptr; }
void _create_debug_messenger() {}
void _destroy_debug_messenger() {}

void CreateSwapchains() {}
void DestroySwapchains() {}

void GetViewConfigurationViews() {}
void GetEnvironmentBlendModes() {}

void CreateReferenceSpace() {}
void DestroyReferenceSpace() {}

void PollEvents() {}

bool render_frame_start() { return false; }
void render_frame_end() {}
bool render_layer_start() { return false; }
void render_layer_end() {}

#endif
}




