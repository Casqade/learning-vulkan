#include "logger.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>


const std::vector <const char*> validationLayers
{
  "VK_LAYER_KHRONOS_validation",
};

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
  bool isPhysicalDeviceSuitable( const VkPhysicalDevice );

  std::vector <QueueFamilyCapabilities> queryQueueFamilyCapabilities( const VkPhysicalDevice );

  void loop();


  GLFWwindow* mWindow {};
  VkInstance mVkInstance {};
  VkDevice mDevice {};
  VkSurfaceKHR mSurface {};
  VkQueue mPresentationQueue {};

  VkDebugUtilsMessengerEXT mVkDebugMessenger {};

  struct
  {
    VkQueue graphics {};
    VkQueue presentation {};

  } mQueues {};
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
  VkApplicationInfo appInfo
  {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "VulkanApp",
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "VulkanEngine",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_0,
  };


  std::vector <const char*> enabledExtensions {};

  {
    uint32_t glfwExtensionCount {};
    const auto glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    for ( size_t i {}; i < glfwExtensionCount; ++i )
      enabledExtensions.emplace_back(glfwExtensions[i]);

    enabledExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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

  const auto enabledMessageSeverities =
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

  const auto enabledMessageTypes =
    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo
  {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = enabledMessageSeverities,
    .messageType = enabledMessageTypes,
    .pfnUserCallback = debugCallback,
    .pUserData = nullptr,
  };

  VkInstanceCreateInfo createInfo
  {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pNext = &debugCreateInfo,
    .pApplicationInfo = &appInfo,
    .enabledLayerCount = static_cast <uint32_t> (validationLayers.size()),
    .ppEnabledLayerNames = validationLayers.data(),
    .enabledExtensionCount = static_cast <uint32_t> (enabledExtensions.size()),
    .ppEnabledExtensionNames = enabledExtensions.data(),
  };

  auto result = vkCreateInstance(
    &createInfo, nullptr, &mVkInstance );

  if ( result != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to create instance");
  }

  initVulkanDebugMessenger(debugCreateInfo);


  result = glfwCreateWindowSurface(
    mVkInstance,
    mWindow,
    NULL,
    &mSurface );

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
}

void
VulkanApp::initVulkanDebugMessenger(
  const VkDebugUtilsMessengerCreateInfoEXT& createInfo )
{
  const auto debugMessengerResult = CreateDebugUtilsMessengerEXT(
    mVkInstance,
    &createInfo,
    nullptr,
    &mVkDebugMessenger );

  if ( debugMessengerResult != VK_SUCCESS )
    throw std::runtime_error("Vulkan: Failed to set up debug messenger");

}

void
VulkanApp::deinit()
{
  mQueues = {};

  if ( mDevice != nullptr )
  {
    vkDestroyDevice(mDevice, nullptr);
    mDevice = {};
  }

  if ( mVkInstance != nullptr )
  {
    vkDestroySurfaceKHR(
      mVkInstance, mSurface, nullptr );

    DestroyDebugUtilsMessengerEXT(
      mVkInstance,
      mVkDebugMessenger,
      nullptr );

    vkDestroyInstance(mVkInstance, nullptr);
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

  VkDeviceQueueCreateInfo queueCreateInfo {};
  queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queueCreateInfo.queueCount = 1;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  std::vector <VkDeviceQueueCreateInfo> queueCreateInfos(
    2, queueCreateInfo );

  queueCreateInfos[0].queueFamilyIndex = graphicsQueueFamilyIndex;
  queueCreateInfos[1].queueFamilyIndex = presentationQueueFamilyIndex;


  VkPhysicalDeviceFeatures deviceFeatures {};

  VkDeviceCreateInfo deviceCreateInfo {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();

  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
  deviceCreateInfo.enabledExtensionCount = 0;

  deviceCreateInfo.enabledLayerCount = validationLayers.size();
  deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();

  const auto result = vkCreateDevice(
    physicalDevice, &deviceCreateInfo,
    nullptr, &mDevice );

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
}

std::vector <QueueFamilyCapabilities>
VulkanApp::queryQueueFamilyCapabilities(
  const VkPhysicalDevice device )
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
VulkanApp::isPhysicalDeviceSuitable(
  const VkPhysicalDevice device )
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

  return
    supportsGraphics == true &&
    supportsPresentation == true;
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

