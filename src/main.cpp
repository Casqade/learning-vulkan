#include "logger.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>


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

const VkAllocationCallbacks AllocatorCallbacks
{
  .pUserData {},
  .pfnAllocation {},
  .pfnReallocation {},
  .pfnFree {},
  .pfnInternalAllocation {},
  .pfnInternalFree {},
};

const decltype(AllocatorCallbacks)* GlobalAllocator =
//  &AllocatorCallbacks;
  nullptr;


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

class VulkanApp
{
public:
  VulkanApp() = default;


  void run();


private:
  void init();
  void deinit();

  void initWindow();
  void initVulkan();
  void initVulkanDebugMessenger( const VkDebugUtilsMessengerCreateInfoEXT& );

  void createLogicalDevice( const VkPhysicalDevice );
  void createSwapChain( const VkPhysicalDevice );

  bool isPhysicalDeviceSuitable( const VkPhysicalDevice ) const;

  std::vector <QueueFamilyCapabilities> queryQueueFamilyCapabilities( const VkPhysicalDevice ) const;
  SwapChainSupportDetails querySwapChainSupport( const VkPhysicalDevice ) const;
  VkSurfaceFormatKHR pickSwapSurfaceFormat( const std::vector <VkSurfaceFormatKHR>& ) const;
  VkPresentModeKHR pickSwapPresentMode( const std::vector <VkPresentModeKHR>& ) const;
  VkExtent2D pickSwapExtent( const VkSurfaceCapabilitiesKHR& ) const;

  void loop();


  VkDebugUtilsMessengerEXT mVkDebugMessenger {};

  GLFWwindow* mWindow {};
  VkInstance mVkInstance {};
  VkDevice mDevice {};
  VkSurfaceKHR mSurface {};

  struct
  {
    std::vector <VkImage> images {};

    VkFormat imageFormat {};
    VkExtent2D extent {};

    VkSwapchainKHR handle {};

  } mSwapchain {};


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

void
VulkanApp::initWindow()
{
  if ( glfwInit() != GLFW_TRUE )
    throw std::runtime_error("Failed to initialize GLFW");


  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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


  VkPhysicalDevice suitableDevice = VK_NULL_HANDLE;

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
      suitableDevice = device;
      break;
    }

  if ( suitableDevice == VK_NULL_HANDLE )
  {
    deinit();
    throw std::runtime_error("Vulkan: No suitable Vulkan devices available");
  }

  createLogicalDevice(suitableDevice);
  createSwapChain(suitableDevice);
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
    throw std::runtime_error("Vulkan: Failed to set up debug messenger");

}

void
VulkanApp::deinit()
{
  mQueues = {};
  mQueueIndices = {};

  if ( mSwapchain.handle != VK_NULL_HANDLE )
    vkDestroySwapchainKHR(
      mDevice, mSwapchain.handle, GlobalAllocator);

  mSwapchain.handle = VK_NULL_HANDLE;

  if ( mDevice != VK_NULL_HANDLE )
  {
    vkDestroyDevice(mDevice, GlobalAllocator);
    mDevice = {};
  }

  if ( mVkInstance != VK_NULL_HANDLE )
  {
    vkDestroySurfaceKHR(
      mVkInstance, mSurface, GlobalAllocator );

    DestroyDebugUtilsMessengerEXT(
      mVkInstance,
      mVkDebugMessenger,
      GlobalAllocator );

    vkDestroyInstance(mVkInstance, GlobalAllocator);
    mVkInstance = {};
    mVkDebugMessenger = {};
  }

  glfwDestroyWindow(mWindow);
  glfwTerminate();

  mWindow = {};
}

void
VulkanApp::createLogicalDevice(
  const VkPhysicalDevice physicalDevice )
{
  const auto queueFamilyCapabilities =
    queryQueueFamilyCapabilities(physicalDevice);

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
    physicalDevice, &deviceCreateInfo,
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
VulkanApp::createSwapChain(
  const VkPhysicalDevice device )
{
  const auto swapChainSupport =
    querySwapChainSupport(device);

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
    .pNext = nullptr,
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

void
VulkanApp::loop()
{
  while ( glfwWindowShouldClose(mWindow) == false )
  {
    glfwPollEvents();
  }
}


int
main(
  int argc,
  char* argv[] )
{
  createLogger();

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


  return EXIT_SUCCESS;
}

