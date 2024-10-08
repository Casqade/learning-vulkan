#include "logger.hpp"
#include "allocator.hpp"

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

#include <fstream>


const std::vector <const char*> ValidationLayers
{
  "VK_LAYER_KHRONOS_validation",
};

const std::vector <const char*> RequiredInstanceExtensions
{
  VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
};

const std::vector <const char*> RequiredDeviceExtensions
{
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};


const VkAllocationCallbacks* GlobalAllocator {};


static std::vector <char>
readFile(
  const std::string& filename )
{
  std::ifstream file {
    filename, std::ios::ate | std::ios::binary };

  if ( file.is_open() == false )
    return {};

  const size_t fileSize = file.tellg();

  std::vector <char> buffer(fileSize);

  file.seekg(0);
  file.read( buffer.data(), fileSize );
  file.close();

  return buffer;
}

static VkResult
CreateDebugUtilsMessengerEXT(
  VkInstance instance,
  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
  const VkAllocationCallbacks* pAllocator,
  VkDebugUtilsMessengerEXT* pDebugMessenger )
{
  static const auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
    instance, "vkCreateDebugUtilsMessengerEXT" );

  if (func != nullptr)
    return func(
      instance, pCreateInfo,
      pAllocator, pDebugMessenger );

  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static void
DestroyDebugUtilsMessengerEXT(
  VkInstance instance,
  VkDebugUtilsMessengerEXT debugMessenger,
  const VkAllocationCallbacks* pAllocator )
{
  static const auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(
    instance, "vkDestroyDebugUtilsMessengerEXT" );

  if ( func != nullptr )
    func(instance, debugMessenger, pAllocator);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData )
{
  spdlog::level::level_enum logLevel {};

  std::string message {};

  switch (messageType)
  {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
      message = "[Vk-General]: {}";
      break;

    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
      message = "[Vk-Validation]: {}";
      break;

    case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
      message = "[Vk-Performance]: {}";
      break;

    case VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT:
      message = "[Vk-AddressBinding]: {}";
      break;
  }

  switch (messageSeverity)
  {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      LOG_TRACE(message, pCallbackData->pMessage);
      break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      LOG_INFO(message, pCallbackData->pMessage);
      break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      LOG_WARN(message, pCallbackData->pMessage);
      break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      LOG_ERROR(message, pCallbackData->pMessage);
      break;

    default:
      break;
  }

  return VK_FALSE;
}


static std::vector <VkPhysicalDevice>
EnumerateSupportedDevices(
  const VkInstance instance )
{
  VkPhysicalDevice device = VK_NULL_HANDLE;

  std::uint32_t deviceCount {};
  vkEnumeratePhysicalDevices(
    instance, &deviceCount, nullptr );

  std::vector <VkPhysicalDevice> devices(deviceCount);

  vkEnumeratePhysicalDevices(
    instance, &deviceCount, devices.data() );

  return devices;
}

struct QueueFamilyCapabilities
{
  bool supportsGraphics {};
  bool supportsPresentation {};

  QueueFamilyCapabilities() = default;
};

struct SwapChainSupportDetails
{
  VkSurfaceCapabilitiesKHR capabilities {};
  std::vector <VkSurfaceFormatKHR> formats {};
  std::vector <VkPresentModeKHR> presentModes {};
};

struct Vertex
{
  glm::vec3 position {};
  glm::vec3 color {};


  static std::vector <VkVertexInputBindingDescription> BindingDescriptions()
  {
    return
    {
      {
        .binding = 0,
        .stride = sizeof(Vertex::position),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
      },
      {
        .binding = 1,
        .stride = sizeof(Vertex::color),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
      },
    };
  }

  static std::vector <VkVertexInputAttributeDescription> AttributeDescriptions()
  {
    return
    {
      VkVertexInputAttributeDescription
      {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0,
      },
      {
        .location = 1,
        .binding = 1,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0,
      },
    };
  }

};

const std::vector <glm::vec3> positions
{
  {0.f, -1.f, 0.f},
  {1.f, 1.f, 0.f},
  {-1.f, 1.f, 0.f},
};

const std::vector <glm::vec3> colors
{
  {1.f, 0.f, 0.f},
  {0.f, 1.f, 0.f},
  {0.f, 0.f, 1.f},
};



class VulkanApp
{
public:
  VulkanApp() = default;


  void run();
  void framebufferResized();


private:
  void init();
  void deinit();

  void initWindow();
  void initVulkan();
  void initVulkanDebugMessenger( const VkDebugUtilsMessengerCreateInfoEXT& );

  void drawFrame();

  void createLogicalDevice();
  void createSwapChain();
  void recreateSwapChain();
  void cleanupSwapChain();
  void createImageViews();
  void createRenderPass();
  void createGraphicsPipeline();
  void createFramebuffers();
  void createCommandPool();
  void createCommandBuffers();
  void createVertexBuffer();
  void createSyncObjects();

  void recordCommandBuffer( VkCommandBuffer, const uint32_t imageIndex );

  bool isPhysicalDeviceSuitable( const VkPhysicalDevice ) const;

  std::vector <QueueFamilyCapabilities> queryQueueFamilyCapabilities( const VkPhysicalDevice ) const;
  SwapChainSupportDetails querySwapChainSupport( const VkPhysicalDevice ) const;
  VkSurfaceFormatKHR pickSwapSurfaceFormat( const std::vector <VkSurfaceFormatKHR>& ) const;
  VkPresentModeKHR pickSwapPresentMode( const std::vector <VkPresentModeKHR>& ) const;
  VkExtent2D pickSwapExtent( const VkSurfaceCapabilitiesKHR& ) const;
  uint32_t findMemoryType( const uint32_t typeFilter, const VkMemoryPropertyFlags );

  VkShaderModule createShaderModule( const std::vector <char>& code ) const;

  void loop();


  VkDebugUtilsMessengerEXT mVkDebugMessenger {};

  GLFWwindow* mWindow {};
  VkInstance mVkInstance {};
  VkDevice mDevice {};
  VkPhysicalDevice mPhysicalDevice {};
  VkSurfaceKHR mSurface {};

  struct
  {
    std::vector <VkImage> images {};
    std::vector <VkImageView> imageViews {};
    std::vector <VkFramebuffer> framebuffers {};

    VkFormat imageFormat {};
    VkExtent2D extent {};

    VkSwapchainKHR handle {};

    std::vector <VkSemaphore> imageAvailableSignals {};

  } mSwapchain {};

  VkRenderPass mRenderPass {};
  VkPipelineLayout mPipelineLayout {};
  VkPipeline mGraphicsPipeline {};
  VkCommandPool mCommandPool {};
  std::vector <VkCommandBuffer> mCommandBuffers {};

  VkBuffer mVertexBuffer {};
  VkDeviceMemory mVertexBufferMemory{};

  std::vector <VkSemaphore> mRenderFinishedSignals {};
  std::vector <VkFence> mFrameIsRenderingFences {};


  struct
  {
    VkQueue graphics {};
    VkQueue presentation {};

  } mQueues {};

  struct
  {
    uint32_t graphics {};
    uint32_t presentation {};

  } mQueueIndices {};

  size_t mCurrentFrameIndex {};
  size_t mMaxConcurrentFrames {2};

  bool mFramebufferResized {};
};

void
VulkanApp::run()
{
  init();

  loop();
}

void
VulkanApp::init()
{
  initWindow();
  initVulkan();
}

static void
framebufferResizedCallback(
  GLFWwindow* window,
  int width, int height )
{
  const auto app = static_cast <VulkanApp*> (
    glfwGetWindowUserPointer(window) );

  assert(app != nullptr);

  app->framebufferResized();
}

void
VulkanApp::framebufferResized()
{
  mFramebufferResized = true;
}

void
VulkanApp::initWindow()
{
  if ( glfwInit() != GLFW_TRUE )
    throw std::runtime_error("Failed to initialize GLFW");


  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  mWindow = glfwCreateWindow(
    800, 600,
    "VulkanApp",
    nullptr,
    nullptr );

  if ( mWindow == nullptr )
  {
    deinit();
    throw std::runtime_error("GLFW: Failed to create window");
  }


  glfwSetWindowUserPointer(mWindow, this);

  glfwSetFramebufferSizeCallback(
    mWindow, framebufferResizedCallback );
}

void
VulkanApp::initVulkan()
{
  auto instanceExtensions {RequiredInstanceExtensions};

  {
    uint32_t glfwExtensionCount {};
    const auto glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for ( size_t i {}; i < glfwExtensionCount; ++i )
      instanceExtensions.emplace_back(glfwExtensions[i]);
  }

  {
    uint32_t extensionCount {};
    vkEnumerateInstanceExtensionProperties(
      nullptr, &extensionCount, nullptr );

    std::vector <VkExtensionProperties> availableExtensions(extensionCount);

    vkEnumerateInstanceExtensionProperties(
      nullptr, &extensionCount,
      availableExtensions.data() );

    LOG_INFO("{} extensions supported:", extensionCount);

    for ( const auto& extension : availableExtensions )
      LOG_INFO("  {}", extension.extensionName);
  }

  {
    uint32_t layerCount {};
    vkEnumerateInstanceLayerProperties(
      &layerCount, nullptr );

    std::vector <VkLayerProperties> availableLayers(layerCount);

    vkEnumerateInstanceLayerProperties(
      &layerCount, availableLayers.data() );

    LOG_INFO("{} layers available:", layerCount);

    for ( const auto& layer : availableLayers )
      LOG_INFO("  {}", layer.layerName);
  }

  const VkDebugUtilsMessageSeverityFlagsEXT enabledMessageSeverities
  {
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
  };

  const VkDebugUtilsMessageTypeFlagsEXT enabledMessageTypes
  {
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_DEVICE_ADDRESS_BINDING_BIT_EXT
  };

  const VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = enabledMessageSeverities,
    .messageType = enabledMessageTypes,
    .pfnUserCallback = debugCallback,
    .pUserData = nullptr,
  };

  const VkApplicationInfo appInfo
  {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "VulkanApp",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "VulkanEngine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_0,
  };

  const VkInstanceCreateInfo createInfo
  {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = &debugCreateInfo,
    .pApplicationInfo = &appInfo,
    .enabledLayerCount = static_cast <uint32_t> (ValidationLayers.size()),
    .ppEnabledLayerNames = ValidationLayers.data(),
    .enabledExtensionCount = static_cast <uint32_t> (instanceExtensions.size()),
    .ppEnabledExtensionNames = instanceExtensions.data(),
  };

  auto result = vkCreateInstance(
    &createInfo, GlobalAllocator, &mVkInstance );

  if ( result != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to create instance");
  }

  initVulkanDebugMessenger(debugCreateInfo);


  result = glfwCreateWindowSurface(
    mVkInstance, mWindow,
    GlobalAllocator, &mSurface );

  if ( result != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to create window surface");
  }


  const auto devices =
    EnumerateSupportedDevices(mVkInstance);

  if ( devices.size() == 0 )
  {
    deinit();
    throw std::runtime_error("Vulkan: No devices with Vulkan support available");
  }

  for ( const auto& device : devices )
    if ( isPhysicalDeviceSuitable(device) == true )
    {
      mPhysicalDevice = device;
      break;
    }

  if ( mPhysicalDevice == VK_NULL_HANDLE )
  {
    deinit();
    throw std::runtime_error("Vulkan: No suitable Vulkan devices available");
  }

  createLogicalDevice();
  createSwapChain();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandPool();
  createVertexBuffer();
  createCommandBuffers();
  createSyncObjects();
}

void
VulkanApp::initVulkanDebugMessenger(
  const VkDebugUtilsMessengerCreateInfoEXT& createInfo )
{
  const auto debugMessengerResult =
    CreateDebugUtilsMessengerEXT(
      mVkInstance, &createInfo,
      GlobalAllocator, &mVkDebugMessenger );

  if ( debugMessengerResult != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to set up debug messenger");
  }
}

void
VulkanApp::deinit()
{
  mQueues = {};
  mQueueIndices = {};
  mCommandBuffers = {};

  cleanupSwapChain();

  if ( mVertexBuffer != VK_NULL_HANDLE )
  {
    vkDestroyBuffer(
      mDevice, mVertexBuffer, GlobalAllocator );

    mVertexBuffer = {};
  }

  if ( mVertexBufferMemory != VK_NULL_HANDLE )
  {
    vkFreeMemory(
      mDevice, mVertexBufferMemory, GlobalAllocator );

    mVertexBufferMemory = {};
  }

  if ( mGraphicsPipeline != VK_NULL_HANDLE )
  {
    vkDestroyPipeline(
      mDevice, mGraphicsPipeline, GlobalAllocator );

    mGraphicsPipeline = {};
  }

  if ( mPipelineLayout != VK_NULL_HANDLE )
  {
    vkDestroyPipelineLayout(
      mDevice, mPipelineLayout, GlobalAllocator );

    mPipelineLayout = {};
  }

  if ( mRenderPass != VK_NULL_HANDLE )
  {
    vkDestroyRenderPass(
      mDevice, mRenderPass, GlobalAllocator );

    mRenderPass = {};
  }

  if ( mDevice != VK_NULL_HANDLE )
  {
    for ( size_t i {}; i < mMaxConcurrentFrames; ++i )
    {
      if ( mSwapchain.imageAvailableSignals[i] != VK_NULL_HANDLE )
        vkDestroySemaphore(
          mDevice, mSwapchain.imageAvailableSignals[i],
          GlobalAllocator );

      if ( mRenderFinishedSignals[i] != VK_NULL_HANDLE )
        vkDestroySemaphore(
          mDevice, mRenderFinishedSignals[i],
          GlobalAllocator );

      if ( mFrameIsRenderingFences[i] != VK_NULL_HANDLE )
        vkDestroyFence(
          mDevice, mFrameIsRenderingFences[i],
          GlobalAllocator );
    }
  }

  mSwapchain = {};

  if ( mCommandPool != VK_NULL_HANDLE )
  {
    vkDestroyCommandPool(
      mDevice, mCommandPool, GlobalAllocator );

    mCommandPool = {};
  }

  if ( mDevice != VK_NULL_HANDLE )
  {
    vkDestroyDevice(mDevice, GlobalAllocator);
    mDevice = {};
  }

  if ( mVkInstance != VK_NULL_HANDLE )
  {
    DestroyDebugUtilsMessengerEXT(
      mVkInstance,
      mVkDebugMessenger,
      GlobalAllocator );

    vkDestroySurfaceKHR(
      mVkInstance, mSurface, GlobalAllocator );

    vkDestroyInstance(mVkInstance, GlobalAllocator);

    mVkInstance = {};
    mVkDebugMessenger = {};
  }

  glfwDestroyWindow(mWindow);
  glfwTerminate();

  mWindow = {};
}

void
VulkanApp::recreateSwapChain()
{
  assert(mWindow != nullptr);

  int width {};
  int height {};

  glfwGetFramebufferSize(
    mWindow, &width, &height );

  while ( width == 0 || height == 0 )
  {
    glfwGetFramebufferSize(
      mWindow, &width, &height );

    glfwWaitEvents();
  }


  assert(mDevice != VK_NULL_HANDLE);

  vkDeviceWaitIdle(mDevice);

  cleanupSwapChain();

  createSwapChain();
  createImageViews();
  createFramebuffers();
}

void
VulkanApp::cleanupSwapChain()
{
  for ( auto& framebuffer : mSwapchain.framebuffers )
  {
    vkDestroyFramebuffer(
      mDevice, framebuffer, GlobalAllocator );

    framebuffer = {};
  }

  for ( auto& imageView : mSwapchain.imageViews )
  {
    vkDestroyImageView(
      mDevice, imageView, GlobalAllocator );

    imageView = {};
  }

  if ( mSwapchain.handle != VK_NULL_HANDLE )
    vkDestroySwapchainKHR(
      mDevice, mSwapchain.handle, GlobalAllocator );

  mSwapchain.handle = VK_NULL_HANDLE;
}

void
VulkanApp::createLogicalDevice()
{
  const auto queueFamilyCapabilities =
    queryQueueFamilyCapabilities(mPhysicalDevice);

  size_t graphicsQueueFamilyIndex {};
  size_t presentationQueueFamilyIndex {};

  for ( size_t i {}; i < queueFamilyCapabilities.size(); ++i )
    if ( queueFamilyCapabilities[i].supportsGraphics == true )
    {
      graphicsQueueFamilyIndex = i;
      break;
    }

  for ( size_t i {}; i < queueFamilyCapabilities.size(); ++i )
    if ( queueFamilyCapabilities[i].supportsPresentation == true )
    {
      presentationQueueFamilyIndex = i;
      break;
    }


  float queuePriority = 1.f;

  const VkDeviceQueueCreateInfo queueCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueCount = 1,
    .pQueuePriorities = &queuePriority,
  };

  std::vector <VkDeviceQueueCreateInfo> queueCreateInfos(
    1 + graphicsQueueFamilyIndex != presentationQueueFamilyIndex,
    queueCreateInfo );

  queueCreateInfos[0].queueFamilyIndex = graphicsQueueFamilyIndex;

  if ( graphicsQueueFamilyIndex != presentationQueueFamilyIndex )
    queueCreateInfos[1].queueFamilyIndex = presentationQueueFamilyIndex;


  VkPhysicalDeviceFeatures deviceFeatures {};

  const VkDeviceCreateInfo deviceCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .queueCreateInfoCount = static_cast <uint32_t> (queueCreateInfos.size()),
    .pQueueCreateInfos = queueCreateInfos.data(),
    .enabledExtensionCount = static_cast <uint32_t> (RequiredDeviceExtensions.size()),
    .ppEnabledExtensionNames = RequiredDeviceExtensions.data(),
    .pEnabledFeatures = &deviceFeatures,
  };

  const auto result = vkCreateDevice(
    mPhysicalDevice, &deviceCreateInfo,
    GlobalAllocator, &mDevice );

  if ( result != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to create logical device");
  }

  vkGetDeviceQueue(
    mDevice,
    graphicsQueueFamilyIndex, 0,
    &mQueues.graphics );

  vkGetDeviceQueue(
    mDevice,
    presentationQueueFamilyIndex, 0,
    &mQueues.presentation );

  mQueueIndices.graphics = graphicsQueueFamilyIndex;
  mQueueIndices.presentation = presentationQueueFamilyIndex;
}

void
VulkanApp::createSwapChain()
{
  const auto swapChainSupport =
    querySwapChainSupport(mPhysicalDevice);

  const auto surfaceFormat =
    pickSwapSurfaceFormat(swapChainSupport.formats);

  const auto presentMode =
    pickSwapPresentMode(swapChainSupport.presentModes);

  const auto extent =
    pickSwapExtent(swapChainSupport.capabilities);

  const auto& caps = swapChainSupport.capabilities;

  const auto requestedImageCount = std::clamp(
    caps.minImageCount + 1,
    caps.minImageCount,
    caps.maxImageCount > 0
      ? caps.maxImageCount
      : caps.minImageCount + 1 );


  VkSwapchainCreateInfoKHR createInfo
  {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .surface = mSurface,
    .minImageCount = requestedImageCount,
    .imageFormat = surfaceFormat.format,
    .imageColorSpace = surfaceFormat.colorSpace,
    .imageExtent = extent,
    .imageArrayLayers = 1,
    .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .preTransform = caps.currentTransform,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = presentMode,
    .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE,
  };

  const uint32_t queueFamilyIndices[]
  {
    mQueueIndices.graphics,
    mQueueIndices.presentation,
  };

  if ( mQueueIndices.graphics != mQueueIndices.presentation )
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
  {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
  }

  const auto result = vkCreateSwapchainKHR(
    mDevice, &createInfo,
    GlobalAllocator, &mSwapchain.handle );

  if ( result != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to create swap chain");
  }

  uint32_t actualImageCount {};

  vkGetSwapchainImagesKHR(
    mDevice, mSwapchain.handle,
    &actualImageCount, nullptr );

  mSwapchain.images.resize(actualImageCount);

  vkGetSwapchainImagesKHR(
    mDevice, mSwapchain.handle,
    &actualImageCount,
    mSwapchain.images.data() );

  mSwapchain.extent = extent;
  mSwapchain.imageFormat = surfaceFormat.format;
}

void
VulkanApp::createImageViews()
{
  mSwapchain.imageViews.resize(mSwapchain.images.size());

  for ( size_t i {}; i < mSwapchain.images.size(); ++i )
  {
    VkImageViewCreateInfo createInfo
    {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = mSwapchain.images[i],
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = mSwapchain.imageFormat,
      .components
      {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
      },
      .subresourceRange
      {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
    };

    const auto result = vkCreateImageView(
      mDevice, &createInfo,
      GlobalAllocator,
      &mSwapchain.imageViews[i] );

    if ( result != VK_SUCCESS )
    {
      deinit();
      throw std::runtime_error("Vulkan: Failed to create image views");
    }
  }
}

void
VulkanApp::createRenderPass()
{
  const VkAttachmentDescription colorAttachment
  {
    .format = mSwapchain.imageFormat,
    .samples = VK_SAMPLE_COUNT_1_BIT,
    .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
    .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  const VkAttachmentReference colorAttachmentReference
  {
    .attachment = 0,
    .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  const VkSubpassDescription subpass
  {
    .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachmentReference,
  };

  const VkSubpassDependency subpassDependency
  {
    .srcSubpass = VK_SUBPASS_EXTERNAL,
    .dstSubpass = 0,
    .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    .srcAccessMask = 0,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };

  const VkRenderPassCreateInfo renderPassCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
    .attachmentCount = 1,
    .pAttachments = &colorAttachment,
    .subpassCount = 1,
    .pSubpasses = &subpass,
    .dependencyCount = 1,
    .pDependencies = &subpassDependency,
  };

  const auto result = vkCreateRenderPass(
    mDevice, &renderPassCreateInfo,
    GlobalAllocator, &mRenderPass );

  if ( result != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to create render pass");
  }
}

void
VulkanApp::createGraphicsPipeline()
{
  const std::string vertShaderPath = "shaders/triangle_vs.spv";
  const std::string fragShaderPath = "shaders/triangle_fs.spv";

  const auto vertShaderCode = readFile(vertShaderPath);

  if ( vertShaderCode.empty() == true )
  {
    deinit();
    throw std::runtime_error("IO: Failed to open shader file '" + vertShaderPath + "'");
  }

  const auto fragShaderCode = readFile(fragShaderPath);

  if ( fragShaderCode.empty() == true )
  {
    deinit();
    throw std::runtime_error("IO: Failed to open shader file '" + fragShaderPath + "'");
  }


  const auto shaderModuleVertex = createShaderModule(vertShaderCode);

  if ( shaderModuleVertex == VK_NULL_HANDLE )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to create vertex shader module");
  }

  const auto shaderModuleFragment = createShaderModule(fragShaderCode);

  if ( shaderModuleFragment == VK_NULL_HANDLE )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to create fragment shader module");
  }

  const VkPipelineShaderStageCreateInfo shaderStages[]
  {
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_VERTEX_BIT,
      .module = shaderModuleVertex,
      .pName = "main",
    },
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
      .module = shaderModuleFragment,
      .pName = "main",
    },
  };

  const auto bindingDescriptions = Vertex::BindingDescriptions();
  const auto attributeDescriptions = Vertex::AttributeDescriptions();

  const VkPipelineVertexInputStateCreateInfo vertexInputInfo
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = static_cast <uint32_t> (bindingDescriptions.size()),
    .pVertexBindingDescriptions = bindingDescriptions.data(),
    .vertexAttributeDescriptionCount = static_cast <uint32_t> (attributeDescriptions.size()),
    .pVertexAttributeDescriptions = attributeDescriptions.data(),
  };

  const VkPipelineInputAssemblyStateCreateInfo inputAssembly
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE,
  };

  const VkViewport viewport
  {
    .x = 0.f,
    .y = 0.f,
    .width = static_cast <float> (mSwapchain.extent.width),
    .height = static_cast <float> (mSwapchain.extent.height),
    .minDepth = 0.f,
    .maxDepth = 1.f,
  };

  const VkRect2D scissor
  {
    .offset = {0, 0},
    .extent = mSwapchain.extent,
  };

  const std::vector <VkDynamicState> dynamicPipelineStates
  {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR,
  };

  const VkPipelineDynamicStateCreateInfo dynamicPipelineState
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = static_cast <uint32_t> (dynamicPipelineStates.size()),
    .pDynamicStates = dynamicPipelineStates.data(),
  };

  const VkPipelineViewportStateCreateInfo viewportState
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor,
  };

  const VkPipelineRasterizationStateCreateInfo rasterizer
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .depthClampEnable = VK_FALSE,
    .rasterizerDiscardEnable = VK_FALSE,
    .polygonMode = VK_POLYGON_MODE_FILL,
    .cullMode = VK_CULL_MODE_BACK_BIT,
    .frontFace = VK_FRONT_FACE_CLOCKWISE,
    .depthBiasEnable = VK_FALSE,
    .depthBiasConstantFactor = 0.f,
    .depthBiasClamp = 0.f,
    .depthBiasSlopeFactor = 0.f,
    .lineWidth = 1.f,
  };

  const VkPipelineMultisampleStateCreateInfo multisampling
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 1.f,
    .pSampleMask = nullptr,
    .alphaToCoverageEnable = VK_FALSE,
    .alphaToOneEnable = VK_FALSE,
  };

  const VkPipelineColorBlendAttachmentState colorBlendAttachment
  {
    .blendEnable = VK_FALSE,
    .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
    .colorBlendOp = VK_BLEND_OP_ADD,
    .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
    .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
    .alphaBlendOp = VK_BLEND_OP_ADD,
    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };

  const VkPipelineColorBlendStateCreateInfo colorBlending
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .logicOp = VK_LOGIC_OP_COPY,
    .attachmentCount = 1,
    .pAttachments = &colorBlendAttachment,
    .blendConstants = {0.f, 0.f, 0.f, 0.f},
  };

  const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = 0,
    .pSetLayouts = nullptr,
    .pushConstantRangeCount = 0,
    .pPushConstantRanges = nullptr,
  };

  const auto pipelineLayoutResult = vkCreatePipelineLayout(
    mDevice, &pipelineLayoutCreateInfo,
    GlobalAllocator, &mPipelineLayout );

  if ( pipelineLayoutResult != VK_SUCCESS )
    throw std::runtime_error("Vulkan: Failed to create pipeline layout");


  const VkGraphicsPipelineCreateInfo pipelineCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = 2,
    .pStages = shaderStages,
    .pVertexInputState = &vertexInputInfo,
    .pInputAssemblyState = &inputAssembly,
    .pViewportState = &viewportState,
    .pRasterizationState = &rasterizer,
    .pMultisampleState = &multisampling,
    .pDepthStencilState = nullptr,
    .pColorBlendState = &colorBlending,
    .pDynamicState = &dynamicPipelineState,
    .layout = mPipelineLayout,
    .renderPass = mRenderPass,
    .subpass = 0,
    .basePipelineHandle = VK_NULL_HANDLE,
    .basePipelineIndex = -1,
  };

  const auto graphicsPipelineResult = vkCreateGraphicsPipelines(
    mDevice, VK_NULL_HANDLE, 1,
    &pipelineCreateInfo, GlobalAllocator,
    &mGraphicsPipeline );

  vkDestroyShaderModule(
    mDevice, shaderModuleVertex,
    GlobalAllocator );

  vkDestroyShaderModule(
    mDevice, shaderModuleFragment,
    GlobalAllocator );

  if ( graphicsPipelineResult != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to create graphics pipeline");
  }
}

void
VulkanApp::createFramebuffers()
{
  mSwapchain.framebuffers.resize(
    mSwapchain.imageViews.size() );

  for ( size_t i {}; i < mSwapchain.imageViews.size(); ++i )
  {
    const VkImageView attachments[]
    {
      mSwapchain.imageViews[i],
    };

    const VkFramebufferCreateInfo framebufferCreateInfo
    {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = mRenderPass,
      .attachmentCount = 1,
      .pAttachments = attachments,
      .width = mSwapchain.extent.width,
      .height = mSwapchain.extent.height,
      .layers = 1,
    };

    const auto result = vkCreateFramebuffer(
      mDevice, &framebufferCreateInfo,
      GlobalAllocator, &mSwapchain.framebuffers[i] );

    if ( result != VK_SUCCESS )
    {
      deinit();
      throw std::runtime_error("Vulkan: Failed to create framebuffer");
    }
  }
}

void
VulkanApp::createCommandPool()
{
  const auto queueFamilyCapabilities =
    queryQueueFamilyCapabilities(mPhysicalDevice);

  uint32_t graphicsQueueFamilyIndex {};

  for ( size_t i {}; i < queueFamilyCapabilities.size(); ++i )
    if ( queueFamilyCapabilities[i].supportsGraphics == true )
    {
      graphicsQueueFamilyIndex = i;
      break;
    }

  const VkCommandPoolCreateInfo poolCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = graphicsQueueFamilyIndex,
  };

  const auto result = vkCreateCommandPool(
    mDevice, &poolCreateInfo,
    GlobalAllocator, &mCommandPool );

  if ( result != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to create command pool");
  }
}

void
VulkanApp::createCommandBuffers()
{
  mCommandBuffers.resize(mMaxConcurrentFrames);

  const VkCommandBufferAllocateInfo cmdBufferAllocateInfo
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = mCommandPool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = static_cast <uint32_t> (mCommandBuffers.size()),
  };

  const auto result = vkAllocateCommandBuffers(
    mDevice, &cmdBufferAllocateInfo,
    mCommandBuffers.data() );

  if ( result != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to allocate command buffer");
  }
}

void
VulkanApp::recordCommandBuffer(
  VkCommandBuffer cmdBuffer,
  const uint32_t imageIndex )
{
  const VkCommandBufferBeginInfo cmdBufferBeginInfo
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = 0,
    .pInheritanceInfo = nullptr,
  };

  const auto cmdBeginResult = vkBeginCommandBuffer(
    cmdBuffer, &cmdBufferBeginInfo );

  if ( cmdBeginResult != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to begin recording command buffer");
  }


  const VkClearValue clearColor = {{{0.f, 0.f, 0.f, 1.f}}};

  const VkRenderPassBeginInfo renderPassBeginInfo
  {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    .renderPass = mRenderPass,
    .framebuffer = mSwapchain.framebuffers[imageIndex],
    .renderArea =
    {
      .offset = {0, 0},
      .extent = mSwapchain.extent,
    },
    .clearValueCount = 1,
    .pClearValues = &clearColor,
  };

  vkCmdBeginRenderPass(
    cmdBuffer, &renderPassBeginInfo,
    VK_SUBPASS_CONTENTS_INLINE );


  vkCmdBindPipeline(
    cmdBuffer,
    VK_PIPELINE_BIND_POINT_GRAPHICS,
    mGraphicsPipeline );


  const VkBuffer vertexBuffers[]
  {
    mVertexBuffer,
    mVertexBuffer,
  };

  const VkDeviceSize offsets[]
  {
    0,
    sizeof(Vertex::position) * positions.size(),
  };

  vkCmdBindVertexBuffers(
    cmdBuffer, 0, 2,
    vertexBuffers, offsets );


  const VkViewport viewport
  {
    .x = 0.f,
    .y = 0.f,
    .width = static_cast <float> (mSwapchain.extent.width),
    .height = static_cast <float> (mSwapchain.extent.height),
    .minDepth = 0.f,
    .maxDepth = 1.f,
  };

  vkCmdSetViewport(
    cmdBuffer,
    0, 1, &viewport );


  const VkRect2D scissor
  {
    .offset = {0, 0},
    .extent = mSwapchain.extent,
  };

  vkCmdSetScissor(
    cmdBuffer,
    0, 1, &scissor );


  vkCmdDraw(
    cmdBuffer,
    3, 1,
    0, 0 );


  vkCmdEndRenderPass(cmdBuffer);


  const auto cmdEndResult = vkEndCommandBuffer(cmdBuffer);

  if ( cmdEndResult != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to record command buffer");
  }
}

uint32_t
VulkanApp::findMemoryType(
  uint32_t typeFilter,
  const VkMemoryPropertyFlags properties )
{
  VkPhysicalDeviceMemoryProperties memoryProperties;
  vkGetPhysicalDeviceMemoryProperties(
    mPhysicalDevice, &memoryProperties );

  for ( uint32_t i {}; i < memoryProperties.memoryTypeCount; ++i )
    if (  typeFilter & (1 << i) &&
          (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties )
      return i;

  deinit();
  throw std::runtime_error("Vulkan: Failed to find suitable memory type");
}

void
VulkanApp::createVertexBuffer()
{
  const auto positionsSize =
    sizeof(decltype(positions)::value_type) * positions.size();

  const auto colorsSize =
    sizeof(decltype(colors)::value_type) * colors.size();

  const auto bufferSize =
    positionsSize +
    colorsSize;

  const VkBufferCreateInfo bufferCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = bufferSize,
    .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  const auto result = vkCreateBuffer(
    mDevice, &bufferCreateInfo,
    GlobalAllocator, &mVertexBuffer );

  if ( result != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to create vertex buffer");
  }


  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements(
    mDevice, mVertexBuffer,
    &memoryRequirements );

  const auto memoryPropertyFlags =
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  const VkMemoryAllocateInfo allocateInfo
  {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .allocationSize = memoryRequirements.size,
    .memoryTypeIndex = findMemoryType(
      memoryRequirements.memoryTypeBits,
      memoryPropertyFlags ),
  };

  const auto allocationResult = vkAllocateMemory(
    mDevice, &allocateInfo,
    GlobalAllocator,
    &mVertexBufferMemory );

  if ( allocationResult != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to allocate vertex buffer memory");
  }


  const auto bufferBindResult = vkBindBufferMemory(
    mDevice,
    mVertexBuffer,
    mVertexBufferMemory,
    0 );

  if ( bufferBindResult != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to bind vertex buffer memory");
  }


  void* data;

  const auto mapMemoryResult = vkMapMemory(
    mDevice, mVertexBufferMemory,
    0, bufferCreateInfo.size,
    0, &data );

  if ( mapMemoryResult != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to map vertex buffer memory");
  }

  std::memcpy(
    data, positions.data(),
    positionsSize );

  std::memcpy(
    static_cast <char*> (data) + positionsSize,
    colors.data(), colorsSize );

  vkUnmapMemory(
    mDevice, mVertexBufferMemory );
}

void
VulkanApp::createSyncObjects()
{
  mSwapchain.imageAvailableSignals.resize(mMaxConcurrentFrames);
  mRenderFinishedSignals.resize(mMaxConcurrentFrames);
  mFrameIsRenderingFences.resize(mMaxConcurrentFrames);

  const VkSemaphoreCreateInfo semaphoreCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  const VkFenceCreateInfo fenceCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };

  for ( size_t i {}; i < mMaxConcurrentFrames; ++i )
  {
    const auto semaphore1CreateStatus = vkCreateSemaphore(
      mDevice, &semaphoreCreateInfo,
      GlobalAllocator, &mSwapchain.imageAvailableSignals[i] );

    const auto semaphore2CreateStatus = vkCreateSemaphore(
      mDevice, &semaphoreCreateInfo,
      GlobalAllocator, &mRenderFinishedSignals[i] );

    const auto fenceCreateStatus = vkCreateFence(
      mDevice, &fenceCreateInfo,
      GlobalAllocator, &mFrameIsRenderingFences[i] );

    if ( semaphore1CreateStatus != VK_SUCCESS || semaphore2CreateStatus != VK_SUCCESS )
    {
      deinit();
      throw std::runtime_error("Vulkan: Failed to create semaphores");
    }

    if ( fenceCreateStatus != VK_SUCCESS )
    {
      deinit();
      throw std::runtime_error("Vulkan: Failed to create fence");
    }
  }
}

void
VulkanApp::drawFrame()
{
  vkWaitForFences( mDevice,
    1, &mFrameIsRenderingFences[mCurrentFrameIndex], VK_TRUE,
    std::numeric_limits <uint64_t>::max() );


  uint32_t acquiredImageIndex {};

  const auto imageAquireResult = vkAcquireNextImageKHR(
    mDevice, mSwapchain.handle,
    std::numeric_limits <uint64_t>::max(),
    mSwapchain.imageAvailableSignals[mCurrentFrameIndex],
    VK_NULL_HANDLE,
    &acquiredImageIndex );

  if ( imageAquireResult == VK_ERROR_OUT_OF_DATE_KHR )
  {
    recreateSwapChain();
    return;
  }

  if ( imageAquireResult != VK_SUBOPTIMAL_KHR && imageAquireResult != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to acquire swap chain image");
  }


  vkResetFences( mDevice,
    1, &mFrameIsRenderingFences[mCurrentFrameIndex] );


  vkResetCommandBuffer(
    mCommandBuffers[mCurrentFrameIndex], 0 );

  recordCommandBuffer(
    mCommandBuffers[mCurrentFrameIndex],
    acquiredImageIndex );


  const VkPipelineStageFlags waitStages[]
  {
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
  };

  const VkSemaphore waitSemaphores[]
  {
    mSwapchain.imageAvailableSignals[mCurrentFrameIndex],
  };

  const VkSemaphore signalSemaphores[]
  {
    mRenderFinishedSignals[mCurrentFrameIndex],
  };

  const VkSubmitInfo submitInfo
  {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = waitSemaphores,
    .pWaitDstStageMask = waitStages,
    .commandBufferCount = 1,
    .pCommandBuffers = &mCommandBuffers[mCurrentFrameIndex],
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = signalSemaphores,
  };

  const auto queueSubmitResult = vkQueueSubmit(
    mQueues.graphics,
    1, &submitInfo,
    mFrameIsRenderingFences[mCurrentFrameIndex] );

  if ( queueSubmitResult != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to submit draw command buffer");
  }


  const VkSwapchainKHR swapChains[]
  {
    mSwapchain.handle,
  };

  const VkPresentInfoKHR presentInfo
  {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = signalSemaphores,
    .swapchainCount = 1,
    .pSwapchains = swapChains,
    .pImageIndices = &acquiredImageIndex,
    .pResults = nullptr,
  };

  const auto queuePresentResult = vkQueuePresentKHR(
    mQueues.presentation,
    &presentInfo );

  if (  queuePresentResult == VK_ERROR_OUT_OF_DATE_KHR ||
        queuePresentResult == VK_SUBOPTIMAL_KHR ||
        mFramebufferResized == true )
  {
    recreateSwapChain();
    mFramebufferResized = false;
  }

  else if (queuePresentResult != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to present swap chain image");
  }

  ++mCurrentFrameIndex %= mMaxConcurrentFrames;
}

std::vector <QueueFamilyCapabilities>
VulkanApp::queryQueueFamilyCapabilities(
  const VkPhysicalDevice device ) const
{
  std::uint32_t queueFamilyCount {};

  vkGetPhysicalDeviceQueueFamilyProperties(
    device, &queueFamilyCount, nullptr );

  std::vector <VkQueueFamilyProperties> queueFamilies(
    queueFamilyCount );

  vkGetPhysicalDeviceQueueFamilyProperties(
    device, &queueFamilyCount,
    queueFamilies.data() );

  std::vector <QueueFamilyCapabilities> queueFamilyCapabilities(
    queueFamilyCount );

  for ( std::size_t i {}; i < queueFamilies.size(); ++i )
  {
    queueFamilyCapabilities[i].supportsGraphics =
      queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

    VkBool32 presentationSupport {};
    vkGetPhysicalDeviceSurfaceSupportKHR(
      device, i, mSurface,
      &presentationSupport );

    queueFamilyCapabilities[i].supportsPresentation =
      presentationSupport;
  }

  return queueFamilyCapabilities;
}

bool
AreRequiredExtensionsAvailable(
  const VkPhysicalDevice device )
{
  std::uint32_t extensionCount {};
  vkEnumerateDeviceExtensionProperties(
    device, nullptr,
    &extensionCount, nullptr );

  std::vector <VkExtensionProperties> deviceExtensions(
    extensionCount );

  vkEnumerateDeviceExtensionProperties(
    device, nullptr,
    &extensionCount, deviceExtensions.data() );

  for ( const auto& requiredExtension : RequiredDeviceExtensions )
  {
    const auto extensionIter = std::find_if(
      deviceExtensions.cbegin(), deviceExtensions.cend(),
      [requiredExtension] ( const VkExtensionProperties& extension )
      {
        return std::strcmp(requiredExtension, extension.extensionName) == 0;
      });

    if ( extensionIter == deviceExtensions.end() )
      return false;
  }

  return true;
}

bool
VulkanApp::isPhysicalDeviceSuitable(
  const VkPhysicalDevice device ) const
{
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(
    device, &properties );

  if ( properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
    return false;


  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(
    device, &features );

  if ( features.geometryShader == false )
    return false;


  const auto queueFamilyCapabilities =
    queryQueueFamilyCapabilities(device);

  bool supportsGraphics {};
  bool supportsPresentation {};

  for ( size_t i {}; i < queueFamilyCapabilities.size(); ++i )
  {
    supportsGraphics |= queueFamilyCapabilities[i].supportsGraphics;
    supportsPresentation |= queueFamilyCapabilities[i].supportsPresentation;
  }

  const auto supportsExtensions =
    AreRequiredExtensionsAvailable(device);

  bool supportsSwapChain {};

  if ( supportsExtensions == true )
  {
    const auto swapChainSupport =
      querySwapChainSupport(device);

    supportsSwapChain =
      swapChainSupport.formats.empty() == false &&
      swapChainSupport.presentModes.empty() == false;
  }

  return
    supportsGraphics &&
    supportsPresentation &&
    supportsExtensions &&
    supportsSwapChain;
}

SwapChainSupportDetails
VulkanApp::querySwapChainSupport(
  const VkPhysicalDevice device ) const
{
  uint32_t formatCount;
  uint32_t presentModeCount;

  vkGetPhysicalDeviceSurfaceFormatsKHR(
    device, mSurface,
    &formatCount, nullptr );

  vkGetPhysicalDeviceSurfacePresentModesKHR(
    device, mSurface,
    &presentModeCount, nullptr );


  SwapChainSupportDetails details {};

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    device, mSurface, &details.capabilities );

  details.formats.resize(formatCount);
  details.presentModes.resize(presentModeCount);


  vkGetPhysicalDeviceSurfaceFormatsKHR(
    device, mSurface,
    &formatCount, details.formats.data() );

  vkGetPhysicalDeviceSurfacePresentModesKHR(
    device, mSurface,
    &presentModeCount, details.presentModes.data() );

  return details;
}

VkSurfaceFormatKHR
VulkanApp::pickSwapSurfaceFormat(
  const std::vector <VkSurfaceFormatKHR>& formats ) const
{
  for ( const auto& format : formats )
  {
    if (  format.format == VK_FORMAT_B8G8R8A8_SRGB &&
          format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
      return format;
  }

  return formats.front();
}

VkPresentModeKHR
VulkanApp::pickSwapPresentMode(
  const std::vector <VkPresentModeKHR>& presentModes ) const
{
  for ( const auto& mode : presentModes )
    if ( mode == VK_PRESENT_MODE_MAILBOX_KHR )
      return mode;

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
VulkanApp::pickSwapExtent(
  const VkSurfaceCapabilitiesKHR& capabilities ) const
{
  if ( capabilities.currentExtent.width != std::numeric_limits <uint32_t>::max() )
    return capabilities.currentExtent;

  int width;
  int height;
  glfwGetFramebufferSize(mWindow, &width, &height);

  return
  {
    std::clamp(
      static_cast <uint32_t> (width),
      capabilities.minImageExtent.width,
      capabilities.maxImageExtent.width ),

    std::clamp(
      static_cast <uint32_t> (height),
      capabilities.minImageExtent.height,
      capabilities.maxImageExtent.height ),
  };
}

VkShaderModule
VulkanApp::createShaderModule(
  const std::vector <char>& code ) const
{
  VkShaderModuleCreateInfo createInfo
  {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = code.size(),
    .pCode = reinterpret_cast <const uint32_t*> (code.data()),
  };

  VkShaderModule module {};

  vkCreateShaderModule(
    mDevice, &createInfo,
    GlobalAllocator,
    &module );

  return module;
}

void
VulkanApp::loop()
{
  while ( glfwWindowShouldClose(mWindow) == false )
  {
    glfwPollEvents();
    drawFrame();
  }

  if ( mDevice != VK_NULL_HANDLE )
    vkDeviceWaitIdle(mDevice);
}


int
main(
  int argc,
  char* argv[] )
{
  createLogger();

  cqdeVk::Allocator allocator {};

  const VkAllocationCallbacks AllocatorCallbacks
  {
    .pUserData = &allocator,
    .pfnAllocation = cqdeVk::allocate,
    .pfnReallocation = cqdeVk::reallocate,
    .pfnFree = cqdeVk::free,
    .pfnInternalAllocation = cqdeVk::internalAllocate,
    .pfnInternalFree = cqdeVk::internalFree,
  };

  GlobalAllocator = &AllocatorCallbacks;

  VulkanApp app {};

  try
  {
    app.run();
  }
  catch ( const std::exception& e )
  {
    LOG_ERROR("{}", e.what());
    return EXIT_FAILURE;
  }

  allocator.printMemoryUsage();

  return EXIT_SUCCESS;
}

