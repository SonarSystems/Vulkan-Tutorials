#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

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

	/**
	* \brief START OF NEW CODE ADDED
	* 
	* Vulkan instance
	*/
	VkInstance _instance;
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
        /**
        * \brief START OF NEW CODE ADDED
		* 
		* Call the Vulkan create method
        */
        CreateInstance( );
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
		* Clean up the Vulkan instance
		*/
        vkDestroyInstance( _instance, nullptr );
		/**
		* \brief END OF NEW CODE ADDED
		*/

		glfwDestroyWindow( _window );
		glfwTerminate( );
	}

	/**
	* \brief START OF NEW CODE ADDED
	* 
	* Create our Vulkan object
	*/
	void CreateInstance( )
	{
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

		uint32_t glfwExtensionCount = 0;
		const char **glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;

		createInfo.enabledLayerCount = 0;

		if ( vkCreateInstance( &createInfo, nullptr, &_instance ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to create instance!" );
		}

		/**
		* \brief OPTIONAL
		* 
		* ONLY FOR CHECKING EXTENSIONS
		*/
		std::vector<VkExtensionProperties> extensions( glfwExtensionCount );

		vkEnumerateInstanceExtensionProperties( nullptr, &glfwExtensionCount, extensions.data( ) );

		std::cout << "available extensions:\n";

		for ( const auto &extension : extensions )
		{
			std::cout << '\t' << extension.extensionName << '\n';
		}
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