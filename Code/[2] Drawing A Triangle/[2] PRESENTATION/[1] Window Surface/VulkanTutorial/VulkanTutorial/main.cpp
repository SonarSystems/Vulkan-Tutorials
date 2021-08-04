#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>

/**
* \brief START OF NEW CODE ADDED
*
* New header
*/
#include <set>
/**
* \brief END OF NEW CODE ADDED
*/

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

const std::vector<const char *> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

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

	/**
	* \brief START OF NEW CODE ADDED
	*
	* Which devices support presentation
	*/
	std::optional<unsigned int> presentFamily;

	bool IsCompleted( )
	{
		return graphicsFamily.has_value( ) && presentFamily.has_value( );
	}
	/**
	* \brief END OF NEW CODE ADDED
	*/
};

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

	/**
	* \brief START OF NEW CODE ADDED
	*
	* Surface to draw to and the presentation queue
	*/
	VkSurfaceKHR _surface;
	VkQueue _presentQueue;
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

		/**
		* \brief START OF NEW CODE ADDED
		*
		* Call the create surface function
		*/
		CreateSurface( );
		/**
		* \brief END OF NEW CODE ADDED
		*/

		PickPhysicalDevice( );
		CreateLogicalDevice( );
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
		vkDestroyDevice( _device, nullptr );

		if ( enableValidationLayers )
		{
			DestroyDebugUtilsMessengerEXT( _instance, _debugMessenger, nullptr );
		}

		/**
		* \brief START OF NEW CODE ADDED
		*
		* Clean up the surface
		*/
		vkDestroySurfaceKHR( _instance, _surface, nullptr);
		/**
		* \brief END OF NEW CODE ADDED
		*/

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

	/**
	* \brief START OF NEW CODE ADDED
	*
	* Create the window surface to draw to
	*/
	void CreateSurface( )
	{
		if ( glfwCreateWindowSurface( _instance, _window, nullptr, &_surface ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create a window surface!" );
		}
	}
	/**
	* \brief END OF NEW CODE ADDED
	*/

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

		/**
		* \brief START OF NEW CODE ADDED
		*
		* graphics and presentation queues
		*/
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<unsigned int> uniqueQueueFamilies = { indices.graphicsFamily.value( ), indices.presentFamily.value( ) };
		/**
		* \brief END OF NEW CODE ADDED
		*/

		float queuePriority = 1.0f;

		/**
		* \brief START OF NEW CODE ADDED
		*
		* Get all the queue families information
		*/
		for ( unsigned int queueFamily : uniqueQueueFamilies )
		{
			VkDeviceQueueCreateInfo queueCreateInfo { };
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back( queueCreateInfo );
		}
		/**
		* \brief END OF NEW CODE ADDED
		*/

		VkPhysicalDeviceFeatures deviceFeatures{ };

		VkDeviceCreateInfo createInfo{ };
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		/**
		* \brief START OF NEW CODE ADDED
		*
		* Update create info count
		*/
		createInfo.queueCreateInfoCount = static_cast<unsigned int>( queueCreateInfos.size( ) );
		createInfo.pQueueCreateInfos = queueCreateInfos.data( );
		/**
		* \brief END OF NEW CODE ADDED
		*/

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = 0;

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

		/**
		* \brief START OF NEW CODE ADDED
		*
		* Get the queue handle for the presentation queue
		*/
		vkGetDeviceQueue( _device, indices.presentFamily.value( ), 0, &_presentQueue );
		/**
		* \brief END OF NEW CODE ADDED
		*/
	}

	bool IsDeviceSuitable( VkPhysicalDevice device )
	{
		QueueFamilyIndices indices = FindQueueFamilies( device );

		return indices.IsCompleted( );
	}

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

			/**
			* \brief START OF NEW CODE ADDED
			*
			* Check if presentation is supported
			*/
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR( device, i, _surface, &presentSupport );

			if ( presentSupport )
			{
				indices.presentFamily = i;
			}
			/**
			* \brief END OF NEW CODE ADDED
			*/

			if ( indices.IsCompleted( ) )
			{
				break;
			}

			/**
			* \brief START OF NEW CODE ADDED
			*
			* Adding missing code from previous tutorials
			*/
			i++;
			/**
			* \brief END OF NEW CODE ADDED
			*/
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