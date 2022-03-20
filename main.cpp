#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> // includes Vulkan API because of line above

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <set>

const uint32_t WIDTH = 500;
const uint32_t HEIGHT = WIDTH;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	GLFWwindow* window;
	VkInstance instance;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSurfaceKHR surface;
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME // KHR swapchain
	};

	void initWindow() {
		glfwInit();
		// disable OpenGL
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// disable window resize for now (complex to implement?)
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		window = glfwCreateWindow(WIDTH, HEIGHT, "Triangle", nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
	}

	void mainLoop() {
		while(!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void createInstance() {
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available");
		}

		VkApplicationInfo appInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Triangle",
			.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
			.pEngineName = "No Engine",
			.apiVersion = VK_API_VERSION_1_0,
		};

		// Get GLFW's required extensions
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		VkInstanceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &appInfo,
			.enabledExtensionCount = glfwExtensionCount,
			.ppEnabledExtensionNames = glfwExtensions,
		};

		// include validation layers if enabled
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		// create vulkan instace
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create Vulkan instance");
		}

		// get instance extensions
		uint32_t instanceExtensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
		std::vector<VkExtensionProperties> instanceExtensions(instanceExtensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, instanceExtensions.data());

		// check that required extenions are present
		for (int i = 0; i < glfwExtensionCount; i++) {
			const char* glfwExtension = glfwExtensions[i];
			bool found = false;
			for (const VkExtensionProperties& instanceExtension : instanceExtensions) {
				if (std::strcmp(glfwExtension, instanceExtension.extensionName) == 0) {
					found = true;
					break;
				}
			}
			if (!found) {
				char* message;
				std::sprintf(message, "Failed to load required GLFW extension \"%s\"", glfwExtension);
				throw std::runtime_error(message);
			}
		}
	}

	bool checkValidationLayerSupport() {
		// get available layers
		uint32_t n;
		vkEnumerateInstanceLayerProperties(&n, nullptr);
		std::vector<VkLayerProperties> availableLayers(n);
		vkEnumerateInstanceLayerProperties(&n, availableLayers.data());

		// ensure all validation layers are present
		for (const char* name : validationLayers) {
			bool found = false;
			for (const VkLayerProperties layerProperties : availableLayers) {
				if (std::strcmp(name, layerProperties.layerName) == 0) {
					found = true;
					break;
				}
			}
			if (!found) {
				return false;
			}
		}

		return true;
	}

	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface");
		}
	}

	void pickPhysicalDevice() {
		// TODO: make this better? Read https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Physical_devices_and_queue_families

		// get number of physical devices
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support");
		}

		// get vector of physical devices
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		// check if physical devices meet requirements
		for (const VkPhysicalDevice& device : devices) {
			if (isDeviceSuitable(device)) {
				physicalDevice = device;
				break;
			}
		}

		// check that a device was selected
		if (physicalDevice == VK_NULL_HANDLE) {
			throw std::runtime_error("failed to find suitable GPU");
		}
	}

	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		// check that all families have been found
		bool isComplete() {
			return
				graphicsFamily.has_value()
				&& presentFamily.has_value()
			;
		}
	};

	bool isDeviceSuitable(VkPhysicalDevice device) {
		return
			findQueueFamilies(device).isComplete()
			&& checkDeviceExtensionSupport(device)
		;
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		// get queue families
		uint32_t n = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &n, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilies(n);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &n, queueFamilies.data());

		// find index of queue family that supports required features
		int i = 0;
		for (const VkQueueFamilyProperties& family : queueFamilies) {
			// graphics support
			if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}

			// present support
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}

			// break once found all
			if (indices.isComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		// get available extensions
		uint32_t n;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &n, nullptr);
		std::vector<VkExtensionProperties> availableExtensions(n);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &n, availableExtensions.data());

		// make copy of deviceExtensions
		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		// erase required extensions that are fulfilled
		for (const VkExtensionProperties& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice); // duplicate code

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {
			indices.graphicsFamily.value(),
			indices.presentFamily.value(),
		};

		// spec queues to create
		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = queueFamily,
				.queueCount = 1,
				.pQueuePriorities = &queuePriority,
			};
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// spec features to use
		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
			.pQueueCreateInfos = queueCreateInfos.data(),
			.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.pEnabledFeatures = &deviceFeatures,
		};

		// validation layers
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			// for compatability with old Vulkan implementations
			createInfo.ppEnabledLayerNames = validationLayers.data();
		} else {
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device");
		}

		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	void cleanup() {
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}
};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
