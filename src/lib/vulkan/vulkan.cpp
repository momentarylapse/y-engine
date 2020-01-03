#if HAS_LIB_VULKAN

#include "vulkan.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <optional>
#include <set>
#include <array>

#include "helper.h"
#include "../base/base.h"
#include "../math/vector.h"
#include "../math/matrix.h"

const int MAX_FRAMES_IN_FLIGHT = 1;

//#define NDEBUG

Array<const char*> sa2pa(const Array<string> &sa) {
	Array<const char*> pa;
	for (string &s: sa)
		pa.add((const char*)s.data);
	return pa;
}



VkResult create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}



static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}




namespace vulkan {



	const std::vector<const char*> validation_layers = {
		"VK_LAYER_LUNARG_standard_validation"
	};

	const std::vector<const char*> device_extensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	#ifdef NDEBUG
	bool enable_validation_layers = false;
	#else
	bool enable_validation_layers = true;
	#endif



	void setup_debug_messenger() {
		if (!enable_validation_layers) return;
		std::cout << " VALIDATION LAYER!\n";

		VkDebugUtilsMessengerCreateInfoEXT create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		create_info.pfnUserCallback = debug_callback;

		if (create_debug_utils_messenger_ext(instance, &create_info, nullptr, &debug_messenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	VkSemaphore create_semaphore();
	VkFence create_fence();



bool framebuffer_resized = false;

GLFWwindow* vulkan_window;
int device_width, device_height; // default (window)
int target_width, target_height; // current





VkInstance instance;
VkDebugUtilsMessengerEXT debug_messenger;
VkSurfaceKHR surface;

VkPhysicalDevice physical_device = VK_NULL_HANDLE;
VkDevice device;

VkQueue graphics_queue;
VkQueue present_queue;

SwapChain swap_chain;

RenderPass *default_render_pass;

DepthBuffer depth_buffer;

class Renderer {
public:

	uint32_t image_index = 0;
	uint32_t current_frame = 0;
	bool is_default = false;
	std::vector<VkSemaphore> image_available_semaphores;
	std::vector<VkSemaphore> render_finished_semaphores;
	std::vector<VkFence> in_flight_fences;

	void create_sync_objects();
	void destroy();

	bool start_frame();
	void present();
	void submit_command_buffer(CommandBuffer *cb);
};
Renderer default_renderer;
Renderer alt_renderer;
FrameBuffer *current_framebuffer;



void init(GLFWwindow* window) {
	std::cout << "vulkan init" << "\n";
	vulkan_window = window;

	device_width = 0;
	device_height = 0;
	while (device_width == 0 or device_height == 0) {
		glfwGetFramebufferSize(vulkan_window, &device_width, &device_height);
		glfwWaitEvents();
	}

	create_instance();
	setup_debug_messenger();
	create_surface();
	pick_physical_device();
	create_logical_device();
	create_swap_chain();
	swap_chain.create_image_views();
	create_command_pool();
	depth_buffer.create(swap_chain.extent, find_depth_format());

	default_render_pass = new RenderPass({swap_chain.image_format, depth_buffer.format});
	create_framebuffers(default_render_pass);

	default_renderer.is_default = true;
	default_renderer.create_sync_objects();
	alt_renderer.create_sync_objects();
}

void destroy() {
	std::cout << "vulkan destroy" << "\n";
	cleanup_swap_chain();

	destroy_descriptor_pool(descriptor_pool);

	default_renderer.destroy();
	alt_renderer.destroy();

	destroy_command_pool();

	vkDestroyDevice(device, nullptr);

	if (enable_validation_layers) {
		destroy_debug_utils_messenger_ext(instance, debug_messenger, nullptr);
	}

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void on_resize(int width, int height) {
	device_width = width;
	device_height = height;
	framebuffer_resized = true;
}


void recreate_swap_chain() {
	std::cout << "recreate swap chain" << "\n";

	vkDeviceWaitIdle(device);

	cleanup_swap_chain();

	create_swap_chain();
	swap_chain.create_image_views();
	depth_buffer.create(swap_chain.extent, find_depth_format());

	default_render_pass->create();
	create_framebuffers(default_render_pass);
	//shader = new Shader("shaders/vert4.spv", "shaders/frag4.spv");
	for (auto *p: pipelines)
		p->create();
}


void cleanup_swap_chain() {
	depth_buffer.destroy();

	for (auto framebuffer: swap_chain.framebuffers) {
		framebuffer.destroy();
	}

	for (auto *p: pipelines)
		p->create();
	default_render_pass->destroy();

	for (auto image_view: swap_chain.image_views) {
		vkDestroyImageView(device, image_view, nullptr);
	}

	vkDestroySwapchainKHR(device, swap_chain.swap_chain, nullptr);
}

bool check_validation_layer_support() {
	uint32_t layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

	std::vector<VkLayerProperties> available_layers(layer_count);
	vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

	for (const char* layer_name : validation_layers) {
		bool layer_found = false;

		for (const auto& layer_properties : available_layers) {
			if (strcmp(layer_name, layer_properties.layerName) == 0) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found) {
			return false;
		}
	}

	return true;
}
std::vector<const char*> get_required_extensions() {
	uint32_t glfw_extension_count = 0;
	const char** glfw_extensions;
	glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

	std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

	if (enable_validation_layers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}



void create_instance() {
	if (enable_validation_layers and !check_validation_layer_support()) {
		//throw std::runtime_error("validation layers requested, but not available!");
		std::cout << "validation layers requested, but not available!" << '\n';
		enable_validation_layers = false;
	}

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Y-Engine";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "Y-Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;

	auto extensions = get_required_extensions();
	create_info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	create_info.ppEnabledExtensionNames = extensions.data();

	if (enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	} else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}
}

void create_surface() {
	if (glfwCreateWindowSurface(instance, vulkan_window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}

void pick_physical_device() {
	uint32_t device_count = 0;
	vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

	if (device_count == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(device_count);
	vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

	for (const auto& device: devices) {
		if (is_device_suitable(device)) {
			physical_device = device;
			break;
		}
	}

	if (physical_device == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}


bool is_device_suitable(VkPhysicalDevice device) {
	QueueFamilyIndices indices = find_queue_families(device);

	bool extensions_supported = check_device_extension_support(device);

	bool swap_chain_adequate = false;
	if (extensions_supported) {
		SwapChainSupportDetails swapChainSupport = query_swap_chain_support(device);
		swap_chain_adequate = !swapChainSupport.formats.empty() and !swapChainSupport.presentModes.empty();
	}
	VkPhysicalDeviceFeatures supported_features;
	vkGetPhysicalDeviceFeatures(device, &supported_features);

	return indices.is_complete() and extensions_supported and swap_chain_adequate and supported_features.samplerAnisotropy;
}

SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

	if (format_count != 0) {
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
	}

	uint32_t present_mode_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, nullptr);

	if (present_mode_count != 0) {
		details.presentModes.resize(present_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_mode_count, details.presentModes.data());
	}

	return details;
}

bool check_device_extension_support(VkPhysicalDevice device) {
	uint32_t extension_count;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

	std::vector<VkExtensionProperties> available_extensions(extension_count);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

	for (const auto& extension : available_extensions) {
		required_extensions.erase(extension.extensionName);
	}

	return required_extensions.empty();
}


void create_logical_device() {
	QueueFamilyIndices indices = find_queue_families(physical_device);

	std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
	std::set<uint32_t> unique_queue_families = {indices.graphics_family.value(), indices.present_family.value()};

	float queue_priority = 1.0f;
	for (uint32_t queue_family : unique_queue_families) {
		VkDeviceQueueCreateInfo queue_create_info = {};
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.queueFamilyIndex = queue_family;
		queue_create_info.queueCount = 1;
		queue_create_info.pQueuePriorities = &queue_priority;
		queue_create_infos.push_back(queue_create_info);
	}

	VkPhysicalDeviceFeatures device_features = {};
	device_features.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
	create_info.pQueueCreateInfos = queue_create_infos.data();

	create_info.pEnabledFeatures = &device_features;

	create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	create_info.ppEnabledExtensionNames = device_extensions.data();

	if (enable_validation_layers) {
		create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		create_info.ppEnabledLayerNames = validation_layers.data();
	} else {
		create_info.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physical_device, &create_info, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(device, indices.graphics_family.value(), 0, &graphics_queue);
	vkGetDeviceQueue(device, indices.present_family.value(), 0, &present_queue);
}





void create_swap_chain() {
	SwapChainSupportDetails swap_chain_support = query_swap_chain_support(physical_device);

	VkSurfaceFormatKHR surface_format = choose_swap_surface_format(swap_chain_support.formats);
	VkPresentModeKHR present_mode = choose_swap_present_mode(swap_chain_support.presentModes);
	VkExtent2D extent = choose_swap_extent(swap_chain_support.capabilities);

	uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;
	if (swap_chain_support.capabilities.maxImageCount > 0 and image_count > swap_chain_support.capabilities.maxImageCount) {
		image_count = swap_chain_support.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = surface;

	info.minImageCount = image_count;
	info.imageFormat = surface_format.format;
	info.imageColorSpace = surface_format.colorSpace;
	info.imageExtent = extent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = find_queue_families(physical_device);
	uint32_t queue_family_indices[] = {indices.graphics_family.value(), indices.present_family.value()};

	if (indices.graphics_family != indices.present_family) {
		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = queue_family_indices;
	} else {
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	info.preTransform = swap_chain_support.capabilities.currentTransform;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = present_mode;
	info.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(device, &info, nullptr, &swap_chain.swap_chain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swap_chain.swap_chain, &image_count, nullptr);
	swap_chain.images.resize(image_count);
	vkGetSwapchainImagesKHR(device, swap_chain.swap_chain, &image_count, &swap_chain.images[0]);

	swap_chain.image_format = surface_format.format;
	swap_chain.extent = extent;
}




VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& available_formats) {
	if (available_formats.size() == 1 and available_formats[0].format == VK_FORMAT_UNDEFINED) {
		return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
	}

	for (const auto& format: available_formats) {
		if (format.format == VK_FORMAT_B8G8R8A8_UNORM and format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return format;
		}
	}

	return available_formats[0];
}

VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR> available_present_modes) {
	VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& mode : available_present_modes) {
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return mode;
		} else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			best_mode = mode;
		}
	}

	return best_mode;
}

VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} else {

		VkExtent2D actual_extent = {
			static_cast<uint32_t>(device_width),
			static_cast<uint32_t>(device_height)
		};

		actual_extent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
		actual_extent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));

		return actual_extent;
	}
}


void create_framebuffers(RenderPass *rp) {
	swap_chain.framebuffers.resize(swap_chain.image_views.num);

	for (size_t i=0; i<swap_chain.image_views.num; i++) {
		swap_chain.framebuffers[i].create(rp, {swap_chain.image_views[i], depth_buffer.view}, swap_chain.extent);
	}
}



void SwapChain::create_image_views() {
	image_views.resize(images.num);

	for (uint32_t i=0; i<images.num; i++) {
		image_views[i] = create_image_view(images[i], image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}
}






void Renderer::destroy() {
	for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
		vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
		vkDestroyFence(device, in_flight_fences[i], nullptr);
	}
}


VkSemaphore create_semaphore() {
	VkSemaphoreCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkSemaphore semaphore;
	if (vkCreateSemaphore(device, &info, nullptr, &semaphore) != VK_SUCCESS) {
		throw std::runtime_error("failed to create semaphore");
	}

	return semaphore;
}

VkFence create_fence() {
	VkFenceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkFence fence;
	if (vkCreateFence(device, &info, nullptr, &fence) != VK_SUCCESS) {
		throw std::runtime_error("failed to create fence");
	}
	return fence;
}


void Renderer::create_sync_objects() {
	image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
	in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

	for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
		image_available_semaphores[i] = create_semaphore();
		render_finished_semaphores[i] = create_semaphore();
		in_flight_fences[i] = create_fence();

		std::cout << "-create sema-   image " << image_available_semaphores[i] << ", render "<< render_finished_semaphores[i] << "\n";
		std::cout << "-create fence-  " << in_flight_fences[i] << "\n";
	}
}

bool Renderer::start_frame() {

	std::cout << "-start frame-   wait fence " << in_flight_fences[current_frame] << "\n";
	vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, std::numeric_limits<uint64_t>::max());

	if (!is_default)
		return true;

	current_framebuffer = &swap_chain.framebuffers[image_index];

	std::cout << "-aquire image-   wait sem image " << image_available_semaphores[current_frame] << "\n";
	VkResult result = vkAcquireNextImageKHR(device, swap_chain.swap_chain, std::numeric_limits<uint64_t>::max(), image_available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreate_swap_chain();
		return false;
	} else if (result != VK_SUCCESS and result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	return true;
}


void Renderer::submit_command_buffer(CommandBuffer *cb) {
	std::cout << "-submit-\n";

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = {image_available_semaphores[current_frame]};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	if (is_default) {
		std::cout << " wait sema image " << image_available_semaphores[current_frame] << "\n";
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = wait_semaphores;
	} else {
		std::cout <<" NON_DEFAULT.... no wait\n";
		submit_info.waitSemaphoreCount = 0;
		submit_info.pWaitSemaphores = nullptr;
	}
	submit_info.pWaitDstStageMask = wait_stages;

	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cb->buffer;

	std::cout << " signal sema render " << render_finished_semaphores[current_frame] << "\n";
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &render_finished_semaphores[current_frame];


	std::cout << " reset/submit fence " << in_flight_fences[current_frame] << "\n";
	vkResetFences(device, 1, &in_flight_fences[current_frame]);

	VkResult result = vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]);
	if (result != VK_SUCCESS) {
		std::cerr << " SUBMIT ERROR " << result << "\n";
		throw std::runtime_error("failed to submit draw command buffer!");
	}
}


void Renderer::present() {
	if (!is_default)
		return;
	std::cout << "-present-   wait sem " << render_finished_semaphores[current_frame] << "\n";
	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_finished_semaphores[current_frame];
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swap_chain.swap_chain;
	present_info.pImageIndices = &image_index;

	VkResult result = vkQueuePresentKHR(present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR or framebuffer_resized) {
		framebuffer_resized = false;
		recreate_swap_chain();
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool start_frame() {
	target_width = device_width;
	target_height = device_height;
	return default_renderer.start_frame();
}

bool alt_start_frame() {
	return alt_renderer.start_frame();
}

void submit_command_buffer(CommandBuffer *cb) {
	default_renderer.submit_command_buffer(cb);
}

void alt_submit_command_buffer(CommandBuffer *cb) {
	alt_renderer.submit_command_buffer(cb);
}

void end_frame() {
	default_renderer.present();
}

void alt_end_frame() {
	/*std::cout << "-end -alt-  wait sema render " << alt_renderer.render_finished_semaphores[alt_renderer.current_frame] << "\n";

	uint64_t values[] = {1};

	VkSemaphoreWaitInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO_KHR;
	info.flags = VK_SEMAPHORE_WAIT_ANY_BIT_KHR;
	info.semaphoreCount = 1;
	info.pSemaphores = &alt_renderer.render_finished_semaphores[alt_renderer.current_frame];
	info. pValues = values;
	vkWaitSemaphoresKHR(device, &info, 1000000000);*/
}

void wait_device_idle() {
	vkDeviceWaitIdle(device);
}






static void framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
	on_resize(width, height);
}

GLFWwindow* create_window(const string &title, int width, int height) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	//glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebuffer_resize_callback);
	return window;
}

bool window_handle(GLFWwindow *window) {
	if (glfwWindowShouldClose(window))
		return true;
	glfwPollEvents();
	return false;
}

void window_close(GLFWwindow *window) {
	glfwDestroyWindow(window);

	glfwTerminate();
}

}

#endif
