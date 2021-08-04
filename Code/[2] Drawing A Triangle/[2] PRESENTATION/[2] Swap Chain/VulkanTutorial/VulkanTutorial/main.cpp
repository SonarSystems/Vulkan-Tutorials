#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>

/**
* \brief START OF NEW CODE ADDED
*
* New header
*/
#include <algorithm>
/**
* \brief END OF NEW CODE ADDED
*/

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

const std::vector<const char *> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

/**
* \brief START OF NEW CODE ADDED
*
* Device extenstion vector
*/
const std::vector<const char *> deviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
/**
* \brief END OF NEW CODE ADDED
*/

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger )
{
	auto func = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );

	if ( func != nullptr )
	{
		return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator )
{
	auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
	
	if ( func != nullptr )
	{
		func( instance, debugMessenger, pAllocator );
	}
}

struct QueueFamilyIndices
{
	std::optional<unsigned int> graphicsFamily;
	std::optional<unsigned int> presentFamily;

	bool IsCompleted( )
	{
		return graphicsFamily.has_value( ) && presentFamily.has_value( );
	}
};

/**
* \brief START OF NEW CODE ADDED
*
* Support details for the swap chain
*/
struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};
/**
* \brief END OF NEW CODE ADDED
*/

class HelloTriangleApplication
{
public:
	void Run( )
	{
		InitWindow( );
		InitVulkan( );
		MainLoop( );
		Cleanup( );
	}

private:
	GLFWwindow *_window;
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debugMessenger;
	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;

	VkDevice _device;
	VkQueue _graphicsQueue;

	VkSurfaceKHR _surface;
	VkQueue _presentQueue;

	/**
	* \brief START OF NEW CODE ADDED
	*
	* Swap chain variables
	*/
	VkSwapchainKHR _swapChain;
	std::vector<VkImage> _swapChainImages;
	VkFormat _swapChainImageFormat;
	VkExtent2D _swapChainExtent;
	/**
	* \brief END OF NEW CODE ADDED
	*/

	void InitWindow( )
	{
		glfwInit( );

		glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
		glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

		_window = glfwCreateWindow( WIDTH, HEIGHT, "Vulkan", nullptr, nullptr );
	}

	void InitVulkan( )
	{
        CreateInstance( );
		SetupDebugMessenger( );
		CreateSurface( );
		PickPhysicalDevice( );
		CreateLogicalDevice( );

		/**
		* \brief START OF NEW CODE ADDED
		*
		* Call the create swap chain function
		*/
		CreateSwapChain( );
		/**
		* \brief END OF NEW CODE ADDED
		*/
	}

	void MainLoop( )
	{
		while ( !glfwWindowShouldClose( _window ) )
		{
			glfwPollEvents( );
		}
	}

	void Cleanup( )
	{
		/**
		* \brief START OF NEW CODE ADDED
		*
		* Cleanup the swap chain
		*/
		vkDestroySwapchainKHR( _device, _swapChain, nullptr );
		/**
		* \brief END OF NEW CODE ADDED
		*/

		vkDestroyDevice( _device, nullptr );

		if ( enableValidationLayers )
		{
			DestroyDebugUtilsMessengerEXT( _instance, _debugMessenger, nullptr );
		}

		vkDestroySurfaceKHR( _instance, _surface, nullptr);

        vkDestroyInstance( _instance, nullptr );

		glfwDestroyWindow( _window );
		glfwTerminate( );
	}

	void CreateInstance( )
	{
		if ( enableValidationLayers && !CheckValidationLayerSupport( ) )
		{
			throw std::runtime_error( "validation layers requested, but not available!" );
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = GetRequiredExtensions( );
		createInfo.enabledExtensionCount = static_cast< uint32_t >( extensions.size( ) );
		createInfo.ppEnabledExtensionNames = extensions.data( );

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{ };
		if ( enableValidationLayers )
		{
			createInfo.enabledLayerCount = static_cast< uint32_t >( validationLayers.size( ) );
			createInfo.ppEnabledLayerNames = validationLayers.data( );

			PopulateDebugMessengerCreateInfo( debugCreateInfo );
			createInfo.pNext = ( VkDebugUtilsMessengerCreateInfoEXT * )&debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		if ( vkCreateInstance( &createInfo, nullptr, &_instance ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to create instance!" );
		}
	}

	void PopulateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT &createInfo )
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;
	}

	void SetupDebugMessenger( )
	{
		if ( !enableValidationLayers ) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo( createInfo );

		if ( CreateDebugUtilsMessengerEXT( _instance, &createInfo, nullptr, &_debugMessenger ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to set up debug messenger!" );
		}
	}

	void CreateSurface( )
	{
		if ( glfwCreateWindowSurface( _instance, _window, nullptr, &_surface ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create a window surface!" );
		}
	}

	void PickPhysicalDevice( )
	{
		unsigned int deviceCount = 0;
		vkEnumeratePhysicalDevices( _instance, &deviceCount, nullptr );

		if ( deviceCount == 0 )
		{
			throw std::runtime_error( "Failed to find any GPUs with Vulkan" );
		}

		std::vector<VkPhysicalDevice> devices( deviceCount );
		vkEnumeratePhysicalDevices( _instance, &deviceCount, devices.data( ) );

		for ( const auto &device : devices )
		{
			if ( IsDeviceSuitable( device ) )
			{
				_physicalDevice = device;

				break;
			}
		}

		if ( _physicalDevice == VK_NULL_HANDLE )
		{
			throw std::runtime_error( "Failed to find a suitable GPU" );
		}
	}

	void CreateLogicalDevice( )
	{
		QueueFamilyIndices indices = FindQueueFamilies( _physicalDevice );

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<unsigned int> uniqueQueueFamilies = { indices.graphicsFamily.value( ), indices.presentFamily.value( ) };

		float queuePriority = 1.0f;

		for ( unsigned int queueFamily : uniqueQueueFamilies )
		{
			VkDeviceQueueCreateInfo queueCreateInfo { };
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back( queueCreateInfo );
		}

		VkPhysicalDeviceFeatures deviceFeatures{ };

		VkDeviceCreateInfo createInfo{ };
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<unsigned int>( queueCreateInfos.size( ) );
		createInfo.pQueueCreateInfos = queueCreateInfos.data( );

		createInfo.pEnabledFeatures = &deviceFeatures;

		/**
		* \brief START OF NEW CODE ADDED
		*
		* Get device extensions
		*/
		createInfo.enabledExtensionCount = static_cast<unsigned int>( deviceExtensions.size( ) );
		createInfo.ppEnabledExtensionNames = deviceExtensions.data( );
		/**
		* \brief END OF NEW CODE ADDED
		*/

		if ( enableValidationLayers )
		{
			createInfo.enabledLayerCount = static_cast<unsigned int>( validationLayers.size( ) );
			createInfo.ppEnabledLayerNames = validationLayers.data( );
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if ( vkCreateDevice( _physicalDevice, &createInfo, nullptr, &_device ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create logical device" );
		}

		vkGetDeviceQueue( _device, indices.graphicsFamily.value( ), 0, &_graphicsQueue );
		vkGetDeviceQueue( _device, indices.presentFamily.value( ), 0, &_presentQueue );
	}

	/**
	* \brief START OF NEW CODE ADDED
	*
	* Creating the swap chain methods
	*/
	void CreateSwapChain( )
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( _physicalDevice );

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat( swapChainSupport.formats );
		VkPresentModeKHR presentMode = ChooseSwapPresentMode( swapChainSupport.presentModes );
		VkExtent2D extent = ChooseSwapExtent( swapChainSupport.capabilities );

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if ( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount )
		{
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = _surface;

		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		QueueFamilyIndices indices = FindQueueFamilies( _physicalDevice );
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value( ), indices.presentFamily.value( ) };

		if ( indices.graphicsFamily != indices.presentFamily )
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if ( vkCreateSwapchainKHR( _device, &createInfo, nullptr, &_swapChain ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to create swap chain!" );
		}

		vkGetSwapchainImagesKHR( _device, _swapChain, &imageCount, nullptr );
		_swapChainImages.resize( imageCount );
		vkGetSwapchainImagesKHR( _device, _swapChain, &imageCount, _swapChainImages.data( ) );

		_swapChainImageFormat = surfaceFormat.format;
		_swapChainExtent = extent;
	}

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR> &availableFormats )
	{
		for ( const auto &availableFormat : availableFormats )
		{
			if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR ChooseSwapPresentMode( const std::vector<VkPresentModeKHR> &availablePresentModes )
	{
		for ( const auto &availablePresentMode : availablePresentModes )
		{
			if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
			{
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseSwapExtent( const VkSurfaceCapabilitiesKHR &capabilities )
	{
		if ( capabilities.currentExtent.width != UINT32_MAX )
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize( _window, &width, &height );

			VkExtent2D actualExtent =
			{
				static_cast< uint32_t >( width ),
				static_cast< uint32_t >( height )
			};

			actualExtent.width = std::clamp( actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width );
			actualExtent.height = std::clamp( actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height );

			return actualExtent;
		}
	}

	SwapChainSupportDetails QuerySwapChainSupport( VkPhysicalDevice device )
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, _surface, &details.capabilities );

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR( device, _surface, &formatCount, nullptr );

		if ( formatCount != 0 )
		{
			details.formats.resize( formatCount );
			vkGetPhysicalDeviceSurfaceFormatsKHR( device, _surface, &formatCount, details.formats.data( ) );
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR( device, _surface, &presentModeCount, nullptr );

		if ( presentModeCount != 0 )
		{
			details.presentModes.resize( presentModeCount );
			vkGetPhysicalDeviceSurfacePresentModesKHR( device, _surface, &presentModeCount, details.presentModes.data( ) );
		}

		return details;
	}

	bool IsDeviceSuitable( VkPhysicalDevice device )
	{
		QueueFamilyIndices indices = FindQueueFamilies( device );

		bool extensionsSupported = CheckDeviceExtensionSupport( device );

		bool swapChainAdequate = false;
		if ( extensionsSupported )
		{
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport( device );
			swapChainAdequate = !swapChainSupport.formats.empty( ) && !swapChainSupport.presentModes.empty( );
		}

		return indices.IsCompleted( ) && extensionsSupported && swapChainAdequate;
	}

	bool CheckDeviceExtensionSupport( VkPhysicalDevice device )
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, nullptr );

		std::vector<VkExtensionProperties> availableExtensions( extensionCount );
		vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, availableExtensions.data( ) );

		std::set<std::string> requiredExtensions( deviceExtensions.begin( ), deviceExtensions.end( ) );

		for ( const auto &extension : availableExtensions )
		{
			requiredExtensions.erase( extension.extensionName );
		}

		return requiredExtensions.empty( );
	}
	/**
	* \brief END OF NEW CODE ADDED
	*/

	QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice device )
	{
		QueueFamilyIndices indices;

		unsigned int queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

		std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
		vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data( ) );

		int i = 0;

		for ( const auto &queueFamily : queueFamilies )
		{
			if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
			{
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR( device, i, _surface, &presentSupport );

			if ( presentSupport )
			{
				indices.presentFamily = i;
			}

			if ( indices.IsCompleted( ) )
			{
				break;
			}

			i++;
		}

		return indices;
	}

	std::vector<const char *> GetRequiredExtensions( )
	{
		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

		std::vector<const char *> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );

		if ( enableValidationLayers )
		{
			extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
		}

		return extensions;
	}

	bool CheckValidationLayerSupport( )
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

		std::vector<VkLayerProperties> availableLayers( layerCount );
		vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data( ) );

		for ( const char *layerName : validationLayers )
		{
			bool layerFound = false;

			for ( const auto &layerProperties : availableLayers )
			{
				if ( strcmp( layerName, layerProperties.layerName ) == 0 )
				{
					layerFound = true;
					break;
				}
			}

			if ( !layerFound )
			{
				return false;
			}
		}

		return true;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData )
	{
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
};

int main( )
{
	HelloTriangleApplication app;

	try
	{
		app.Run( );
	}
	catch ( const std::exception &e )
	{
		std::cerr << e.what( ) << std::endl;

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}