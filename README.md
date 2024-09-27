# learning-vulkan

Nothing to see here, move along. 
Random cpp bloke learning Vulkan.


Resources:
  - [Vulkan Tutorial](https://vulkan-tutorial.com)
  - [Vulkan Guide (Khronos)](https://github.com/KhronosGroup/Vulkan-Guide)
  - [Vulkan Guide (vblanco)](https://vkguide.dev)


Runtime environment:
  - `VK_ADD_LAYER_PATH`: add a specific path to layers
  - [`VK_LOADER_DEBUG`](https://vulkan.lunarg.com/doc/view/1.3.236.0/windows/LoaderDebugging.html#user-content-loader-logging): emit loader debug messages


Registry paths for [Windows Layer Discovery](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderLayerInterface.md#windows-layer-discovery):
  - Win64: `HKLM\SOFTWARE\Khronos\Vulkan\ExplicitLayers`
  - Win32: `HKLM\SOFTWARE\WOW6432Node\Khronos\Vulkan\ExplicitLayers`

