#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>

const unsigned int WIDTH = 800;
const unsigned int HEIGHT = 600;

/**
* \brief START OF NEW CODE ADDED
*
* Defines how many frames should be processed concurrently (helps prevent it from creeping up in memory usage)
*/
const int MAX_FRAMES_IN_FLIGHT = 2;
/**
* \brief END OF NEW CODE ADDED
*/

const std::vector<const char *> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char *> deviceExtensions =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
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
	auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkDestroyUtilsMessengerEXT" );

	if ( func != nullptr )
	{
		func( instance, debugMessenger, pAllocator );
	}
}

struct QueueFamilyIndices
{
	std::optional<unsigned int> graphicsFamily;
	std::optional<unsigned int> presentFamily;

	bool IsComplete( )
	{
		return graphicsFamily.has_value( ) && presentFamily.has_value( );
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
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

	VkSurfaceKHR _surface;
	VkQueue _presentQueue;

	VkSwapchainKHR _swapChain;
	std::vector<VkImage> _swapChainImages;
	VkFormat _swapChainImageFormat;
	VkExtent2D _swapChainExtent;

	std::vector<VkImageView> _swapChainImageViews;
	std::vector<VkFramebuffer> _swapChainFrameBuffers;

	VkRenderPass _renderPass;
	VkPipelineLayout _pipelineLayout;
	VkPipeline _graphicsPipeline;

	VkCommandPool _commandPool;
	std::vector<VkCommandBuffer> _commandBuffers;

	/**
	* \brief START OF NEW CODE ADDED
	*
	* Each frame should have its own set of semaphores
	* Frames in flight we'll immediately have a synchronization object to wait on before a new frame can use that image
	* All of this will help ensure no more than two frames of work are queued 
	*/
	std::vector<VkSemaphore> _imageAvailableSemaphores;
	std::vector<VkSemaphore> _renderFinishedSemaphores;
	std::vector<VkFence> _inFlightFences;
	std::vector<VkFence> _imagesInFlight;
	unsigned long long currentFrame = 0;
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
		CreateSwapChain( );
		CreateImageViews( );
		CreateRenderPass( );
		CreateGraphicsPipeline( );
		CreateFramebuffers( );
		CreateCommandPool( );
		CreateCommandBuffers( );

		/**
		* \brief START OF NEW CODE ADDED
		*
		* Call the create sync objects method
		*/
		CreateSyncObjects( );
		/**
		* \brief END OF NEW CODE ADDED
		*/
	}

	void MainLoop( )
	{
		while ( !glfwWindowShouldClose( _window ) )
		{
			glfwPollEvents( );

			/**
			* \brief START OF NEW CODE ADDED
			*
			* Draw the frame to be shown to the user
			*/
			DrawFrame( );
			/**
			* \brief END OF NEW CODE ADDED
			*/
		}

		/**
		* \brief START OF NEW CODE ADDED
		*
		* All of the operations in DrawFrame are asynchronous which means that when we exit
		* the loop in MainLoop, drawing and presentation operations may still be going on.
		* Cleaning up resources while that is happening is a bad idea. 
		* We canfix this problem by waiting for the logical device to finish operations before exiting 
		* the MainLoop and destroying the window
		*/
		vkDeviceWaitIdle( _device );
		/**
		* \brief END OF NEW CODE ADDED
		*/
	}

	void Cleanup( )
	{
		/**
		* \brief START OF NEW CODE ADDED
		*
		* Clean up the vectors
		*/
		for ( unsigned long long i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			vkDestroySemaphore( _device, _renderFinishedSemaphores[i], nullptr );
			vkDestroySemaphore( _device, _imageAvailableSemaphores[i], nullptr );
			vkDestroyFence( _device, _inFlightFences[i], nullptr );
		}
		/**
		* \brief END OF NEW CODE ADDED
		*/

		vkDestroyCommandPool( _device, _commandPool, nullptr );

		for ( auto framebuffer : _swapChainFrameBuffers )
		{
			vkDestroyFramebuffer( _device, framebuffer, nullptr );
		}

		vkDestroyPipeline( _device, _graphicsPipeline, nullptr );
		vkDestroyPipelineLayout( _device, _pipelineLayout, nullptr );
		vkDestroyRenderPass( _device, _renderPass, nullptr );

		for ( auto imageView : _swapChainImageViews )
		{
			vkDestroyImageView( _device, imageView, nullptr );
		}

		vkDestroySwapchainKHR( _device, _swapChain, nullptr );
		vkDestroyDevice( _device, nullptr );

		if ( enableValidationLayers )
		{
			DestroyDebugUtilsMessengerEXT( _instance, _debugMessenger, nullptr );
		}

		vkDestroySurfaceKHR( _instance, _surface, nullptr );
		vkDestroyInstance( _instance, nullptr );
		glfwDestroyWindow( _window );
		glfwTerminate( );
	}

	void CreateInstance( )
	{
		if ( enableValidationLayers && !CheckValidationLayerSupport( ) )
		{
			throw std::runtime_error( "Validation layers requested, but not available" );
		}

		VkApplicationInfo appInfo{ };
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
		appInfo.pEngineName = "SoSy Game Engine";
		appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{ };
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = GetRequiredExtensions( );
		createInfo.enabledExtensionCount = static_cast<unsigned int>( extensions.size( ) );
		createInfo.ppEnabledExtensionNames = extensions.data( );

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{ };

		if ( enableValidationLayers )
		{
			createInfo.enabledLayerCount = static_cast<unsigned int>( validationLayers.size( ) );
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
			throw std::runtime_error( "Failed to create Vulkan Instance!" );
		}
		/*
		std::vector<VkExtensionProperties> extensions( glfwExtentionCount );

		vkEnumerateInstanceExtensionProperties( nullptr, &glfwExtentionCount, extensions.data( ) );

		std::cout << "Available Extensions: " << std::endl;

		for ( const auto &extension : extensions )
		{
			std::cout << '\t' << extension.extensionName << std::endl;
		}*/
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
		if ( !enableValidationLayers )
		{ return; }

		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		PopulateDebugMessengerCreateInfo( createInfo );

		if ( CreateDebugUtilsMessengerEXT( _instance, &createInfo, nullptr, &_debugMessenger ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to setup up debug messenger" );
		}
	}

	void CreateSurface( )
	{
		if ( glfwCreateWindowSurface( _instance, _window, nullptr, &_surface ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create a window surface" );
		}
	}

	void PickPhysicalDevice( )
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices( _instance, &deviceCount, nullptr );

		if ( deviceCount == 0 )
		{
			throw std::runtime_error( "Failed to find any GPUs with Vulkan support!" );
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
			throw std::runtime_error( "Failed to find a suitable GPU!" );
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

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		createInfo.queueCreateInfoCount = static_cast<unsigned int>( queueCreateInfos.size( ) );
		createInfo.pQueueCreateInfos = queueCreateInfos.data( );

		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<unsigned int>( deviceExtensions.size( ) );
		createInfo.ppEnabledExtensionNames = deviceExtensions.data( );

		if ( enableValidationLayers )
		{
			createInfo.enabledLayerCount = static_cast< uint32_t >( validationLayers.size( ) );
			createInfo.ppEnabledLayerNames = validationLayers.data( );
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if ( vkCreateDevice( _physicalDevice, &createInfo, nullptr, &_device ) != VK_SUCCESS )
		{
			throw std::runtime_error( "failed to create logical device!" );
		}

		vkGetDeviceQueue( _device, indices.graphicsFamily.value( ), 0, &_graphicsQueue );
		vkGetDeviceQueue( _device, indices.presentFamily.value( ), 0, &_presentQueue );
	}

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

	void CreateImageViews( )
	{
		_swapChainImageViews.resize( _swapChainImages.size( ) );

		for ( unsigned long long i = 0; i < _swapChainImages.size( ); i++ )
		{
			VkImageViewCreateInfo createInfo{ };
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = _swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = _swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount= 1;

			if ( vkCreateImageView( _device, &createInfo, nullptr, &_swapChainImageViews[i] ) != VK_SUCCESS )
			{
				throw std::runtime_error( "Failed to create image views" );
			}
		}
	}

	void CreateRenderPass( )
	{
		VkAttachmentDescription colorAttachment{ };
		colorAttachment.format = _swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{ };
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{ };
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		/**
		* \brief START OF NEW CODE ADDED
		*
		* Remember that the subpasses in a render pass automatically take care of image layout transitions.
		* These transitions are controlled by subpass dependencies, which specify memory and execution dependencies between subpasses.
		* We have only a single subpass right now, but the operations right before and right after this subpass also count as implicit "subpasses".
		*/
		VkSubpassDependency dependency{ };
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		/**
		* \brief END OF NEW CODE ADDED
		*/

		VkRenderPassCreateInfo renderPassInfo{ };
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		/**
		* \brief START OF NEW CODE ADDED
		*
		* Reference the created dependencies in the render pass info
		*/
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;
		/**
		* \brief END OF NEW CODE ADDED
		*/

		if ( vkCreateRenderPass( _device, &renderPassInfo, nullptr, &_renderPass ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create pass" );
		}
	}

	void CreateGraphicsPipeline( )
	{
		auto vertShaderCode = ReadFile( "res/shaders/vert.spv" );
		auto fragShaderCode = ReadFile( "res/shaders/frag.spv" );

		VkShaderModule vertShaderModule = CreateShaderModule( vertShaderCode );
		VkShaderModule fragShaderModule = CreateShaderModule( fragShaderCode );

		VkPipelineShaderStageCreateInfo vertShaderStageInfo { };
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo { };
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{ };
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{ };
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{ };
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = ( float )_swapChainExtent.width;
		viewport.height = ( float )_swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{ };
		scissor.offset = { 0, 0 };
		scissor.extent = _swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState{ };
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{ };
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{ };
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{ };
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{ };
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{ };
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		if ( vkCreatePipelineLayout( _device, &pipelineLayoutInfo, nullptr, &_pipelineLayout ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to Create Pipeline Layout" );
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{ };
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = _pipelineLayout;
		pipelineInfo.renderPass = _renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if ( vkCreateGraphicsPipelines( _device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create a graphics pipeline" );
		}

		vkDestroyShaderModule( _device, fragShaderModule, nullptr );
		vkDestroyShaderModule( _device, vertShaderModule, nullptr );
	}

	void CreateFramebuffers( )
	{
		_swapChainFrameBuffers.resize( _swapChainImageViews.size( ) );

		for ( unsigned long long i = 0; i < _swapChainImageViews.size( ); i++ )
		{
			VkImageView attachments[] =
			{
				_swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{ };
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = _renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = _swapChainExtent.width;
			framebufferInfo.height = _swapChainExtent.height;
			framebufferInfo.layers = 1;

			if ( vkCreateFramebuffer( _device, &framebufferInfo, nullptr, &_swapChainFrameBuffers[i] ) != VK_SUCCESS )
			{
				throw std::runtime_error( "Failed to create framebuffers" );
			}
		}
	}

	void CreateCommandPool( )
	{
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies( _physicalDevice );

		VkCommandPoolCreateInfo poolInfo{ };
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value( );

		if ( vkCreateCommandPool( _device, &poolInfo, nullptr, &_commandPool ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create command pool" );
		}
	}

	void CreateCommandBuffers( )
	{
		_commandBuffers.resize( _swapChainFrameBuffers.size( ) );

		VkCommandBufferAllocateInfo allocInfo{ };
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = _commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = ( unsigned int )_commandBuffers.size( );

		if ( vkAllocateCommandBuffers( _device, &allocInfo, _commandBuffers.data( ) ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to allocate command buffers" );
		}

		for ( unsigned long long i = 0; i < _commandBuffers.size( ); i++ )
		{
			VkCommandBufferBeginInfo beginInfo{ };
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			if ( vkBeginCommandBuffer( _commandBuffers[i], &beginInfo ) != VK_SUCCESS )
			{
				throw std::runtime_error( "Failed to beging recording command buffer" );
			}

			VkRenderPassBeginInfo renderPassInfo{ };
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = _renderPass;
			renderPassInfo.framebuffer = _swapChainFrameBuffers[i];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = _swapChainExtent;

			VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass( _commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

			vkCmdBindPipeline( _commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline );

			vkCmdDraw( _commandBuffers[i], 3, 1, 0, 0 );

			vkCmdEndRenderPass( _commandBuffers[i] );

			if ( vkEndCommandBuffer( _commandBuffers[i] ) != VK_SUCCESS )
			{
				throw std::runtime_error( "Failed to record command buffer" );
			}
		}
	}

	/**
	* \brief START OF NEW CODE ADDED
	*
	* Create the sync objects and draw the frame
	*/
	void CreateSyncObjects( )
	{
		// Match the semaphore sizes with the maximum number of frames in flight
		_imageAvailableSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
		_renderFinishedSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
		_inFlightFences.resize( MAX_FRAMES_IN_FLIGHT );
		// Match the images in flight size to the swap chain images size
		_imagesInFlight.resize( _swapChainImages.size( ), VK_NULL_HANDLE );

		// Create the semaphore create info object
		VkSemaphoreCreateInfo semaphoreInfo{ };
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		// Create the fence create info object
		VkFenceCreateInfo fenceInfo{ };
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		// Create all the semaphores and fences
		for ( unsigned long long i = 0; i < MAX_FRAMES_IN_FLIGHT; i++ )
		{
			if ( vkCreateSemaphore( _device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i] ) != VK_SUCCESS ||
				vkCreateSemaphore( _device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i] ) != VK_SUCCESS ||
				vkCreateFence( _device, &fenceInfo, nullptr, &_inFlightFences[i] ) != VK_SUCCESS )
			{
				throw std::runtime_error( "failed to create synchronization objects for a frame!" );
			}
		}
	}

	void DrawFrame( )
	{
		// Wait for the frame to be finished
		vkWaitForFences( _device, 1, &_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX );

		// Get the next image to be drawn
		unsigned int imageIndex;
		vkAcquireNextImageKHR( _device, _swapChain, UINT64_MAX, _imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex );

		// Make sure the image is not NULL
		if ( _imagesInFlight[imageIndex] != VK_NULL_HANDLE )
		{
			vkWaitForFences( _device, 1, &_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX );
		}

		// Match the in flight images to the current frame
		_imagesInFlight[imageIndex] = _inFlightFences[currentFrame];

		// Queue submission and synchronization is configured through parameters in the VkSubmitInfo structure
		VkSubmitInfo submitInfo{ };
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		// These parameters specify which semaphores to wait on before execution begins and in which stage(s) of the pipeline to wait. 
		VkSemaphore waitSemaphores[] = { _imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		// Each entry in the waitStages array corresponds to the semaphore with the same index in pWaitSemaphores.
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];

		// The next two parameters specify which command buffers to actually submit for execution.
		VkSemaphore signalSemaphores[] = { _renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		// Reset the fences
		vkResetFences( _device, 1, &_inFlightFences[currentFrame] );

		// Submit the queue
		if ( vkQueueSubmit( _graphicsQueue, 1, &submitInfo, _inFlightFences[currentFrame] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to submit draw command buffer!" );
		}

		// The last step of drawing a frame is submitting the result back to the swap chain to have it eventually show up on the screen.
		VkPresentInfoKHR presentInfo{ };
		// Specify which semaphores to wait on before presentation can happen, just like VkSubmitInfo
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		// Specify the swap chains to present images to and the index of the image for each swap chain.
		// This will almost always be a single one.
		VkSwapchainKHR swapChains[] = { _swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;

		presentInfo.pImageIndices = &imageIndex;

		// Cubmits the request to present an image to the swap chain
		vkQueuePresentKHR( _presentQueue, &presentInfo );

		// Move to the next frame every time
		// By using the modulo (%) operator,
		// we ensure that the frame index loops around after every MAX_FRAMES_IN_FLIGHT enqueued frames.
		currentFrame = ( currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
	}
	/**
	* \brief END OF NEW CODE ADDED
	*/

	VkShaderModule CreateShaderModule( const std::vector<char> &code )
	{
		VkShaderModuleCreateInfo createInfo { };
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size( );
		createInfo.pCode = reinterpret_cast<const unsigned int *>( code.data( ) );

		VkShaderModule shaderModule;

		if ( vkCreateShaderModule( _device, &createInfo, nullptr, &shaderModule ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed To Create shader module" );
		}

		return shaderModule;
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

		return indices.IsComplete( ) && extensionsSupported && swapChainAdequate;
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

	QueueFamilyIndices FindQueueFamilies( VkPhysicalDevice device )
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
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

			if ( indices.IsComplete( ) )
			{
				break;
			}

			i++;
		}

		return indices;
	}

	std::vector<const char *>GetRequiredExtensions( )
	{
		unsigned int glfwExtensionCount = 0;
		const char **glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

		std::vector<const char*> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );

		if ( enableValidationLayers )
		{
			extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
		}

		return extensions;
	}

	bool CheckValidationLayerSupport( )
	{
		unsigned int layerCount;
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
			{ return false; }
		}

		return true;
	}

	static std::vector<char> ReadFile( const std::string &filename )
	{
		std::ifstream file( filename, std::ios::ate | std::ios::binary );

		if ( !file.is_open( ) )
		{
			throw std::runtime_error( "Failed to open file" );
		}

		unsigned long long fileSize = ( unsigned long long )file.tellg( );
		std::vector<char> buffer( fileSize );

		file.seekg( 0 );
		file.read( buffer.data( ), fileSize );

		file.close( );

		return buffer;
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData )
	{
		std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

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