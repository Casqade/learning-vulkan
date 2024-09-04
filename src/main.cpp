#include <x86intrin.h>
#include "logger.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>


VkResult
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

void
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

  void loop();


  GLFWwindow* mWindow {};
  VkInstance mVkInstance {};

  VkDebugUtilsMessengerEXT mVkDebugMessenger {};
};

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

std::vector <VkPhysicalDevice>
enumerateSupportedDevices(
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

bool
isDeviceSuitable(
  const VkPhysicalDevice device )
{
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;

  vkGetPhysicalDeviceProperties(
    device, &properties );

  vkGetPhysicalDeviceFeatures(
    device, &features );

  return
    properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
    features.geometryShader == true;
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

  std::vector <const char*> validationLayers
  {
    "VK_LAYER_KHRONOS_validation",
  };

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

  const auto result = vkCreateInstance(
    &createInfo, nullptr, &mVkInstance );

  if ( result != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to create instance");
  }

  VkPhysicalDevice suitableDevice = VK_NULL_HANDLE;

  const auto devices = enumerateSupportedDevices(mVkInstance);

  if ( devices.size() == 0 )
  {
    deinit();
    throw std::runtime_error("Vulkan: No devices with Vulkan support available");
  }

  for ( const auto& device : devices )
    if ( isDeviceSuitable(device) == true )
    {
      suitableDevice = device;
      break;
    }

  if ( suitableDevice == VK_NULL_HANDLE )
  {
    deinit();
    throw std::runtime_error("Vulkan: No suitable Vulkan devices available");
  }

  return;

  VkSurfaceKHR surface;
  VkResult err = glfwCreateWindowSurface(
    mVkInstance,
    mWindow,
    NULL,
    &surface );

  if ( err != VK_SUCCESS )
  {
    deinit();
    throw std::runtime_error("Vulkan: Failed to create window surface");
  }

  initVulkanDebugMessenger(debugCreateInfo);
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
  if ( mVkInstance != nullptr )
  {
    DestroyDebugUtilsMessengerEXT(
      mVkInstance,
      mVkDebugMessenger,
      nullptr );

    vkDestroyInstance(mVkInstance, nullptr);
  }

  glfwDestroyWindow(mWindow);
  glfwTerminate();
}

void
VulkanApp::loop()
{
  while ( glfwWindowShouldClose(mWindow) == false )
  {
    glfwPollEvents();
  }
}


class MyClass
{
  int num {};


public:
  MyClass( int newNum )
  {
    num = newNum;
    printf("MyClass()\n");
  }

  ~MyClass()
  {
    printf("~MyClass()\n");
  }

  int value() const
  {
    return num;
  }
};

template <typename T>
struct ListNode
{
  ListNode* next {};

  T data {};


  template <typename ...Args>
  void emplace_back( Args&&... args )
  {
    auto& last = next;

    while ( last != nullptr )
      last = next->next;

    last = new ListNode <T> {
      {}, std::forward <Args> (args)...};
  }

  void push_back( const T& newData )
  {
    auto& last = next;

    while ( last != nullptr )
      last = next->next;

    last = new ListNode <T> (newData);
  }
};


int
main(
  int argc,
  char* argv[] )
{

  std::uint32_t tscAux {};
  const auto tickCount = __rdtscp(&tscAux);

  std::printf("rdtsc: %llu, rdtscp: %llu / %u \n", __rdtsc(), tickCount, tscAux);


  ListNode <MyClass> item { {}, 1 };
  ListNode <MyClass> item1 { {}, 2 };

  item.next = &item1;

  item.emplace_back(3);

  auto item3 = *item.next;
  std::printf("item3 val: %d\n", item3.data.value());

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

