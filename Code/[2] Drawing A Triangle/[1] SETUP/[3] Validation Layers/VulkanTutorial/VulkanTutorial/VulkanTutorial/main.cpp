#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

/**
* \brief START OF NEW CODE ADDED
* 
* Validation layers vector and messenger methods
*/
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

	/**
	* \brief START OF NEW CODE ADDED
	*
	* Debug messenger object
	*/
	VkDebugUtilsMessengerEXT _debugMessenger;
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

		/**
		* \brief START OF NEW CODE ADDED
		*
		* Setup debug messenger
		*/
		SetupDebugMessenger();
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
		* Setup Check if validation layers are enabled, if they are then clean up the object
		*/
		if ( enableValidationLayers )
		{
			DestroyDebugUtilsMessengerEXT( _instance, _debugMessenger, nullptr );
		}
		/**
		* \brief END OF NEW CODE ADDED
		*/

        vkDestroyInstance( _instance, nullptr );

		glfwDestroyWindow( _window );
		glfwTerminate( );
	}

	void CreateInstance( )
	{
		/**
		* \brief START OF NEW CODE ADDED
		*
		* Setup Check if validation layers are enabled but not supported and throw an error accordingly
		*/
		if ( enableValidationLayers && !CheckValidationLayerSupport( ) )
		{
			throw std::runtime_error( "validation layers requested, but not available!" );
		}
		/**
		* \brief END OF NEW CODE ADDED
		*/

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

		/**
		* \brief START OF NEW CODE ADDED
		*
		* Setup Check extensions and enable validation layers
		*/
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
		/**
		* \brief END OF NEW CODE ADDED
		*/

		if ( vkCreateInstance( &createInfo, nullptr, &_instance ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to create instance!" );
		}
	}

	/**
	* \brief START OF NEW CODE ADDED
	*
	* Setup Implementing methods for debug messenger
	*/
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
	/**
	* \brief END OF NEW CODE ADDED
	*/
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