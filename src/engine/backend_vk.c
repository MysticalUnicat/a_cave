#include "engine.h"

#include "backend.h"

#include <alias/ui.h>
#include <alias/data_structure/inline_list.h>
#include <alias/data_structure/vector.h>
#include <alias/data_structure/paged_soa.h>
#include <alias/log.h>

#include <uchar.h>

#include "glad_vk.h"

#include <GLFW/glfw3.h>

#include "stb_image.h"

struct CommandBuffer {
  VkCommandBuffer buffer;
  VkFence fence;
};

struct AllocationBlockSegment {
  VkDeviceSize memory_start;
  VkDeviceSize memory_end;
};

struct AllocationBlock {
  uint32_t memory_type;
  VkDeviceMemory memory;
  VkDeviceSize size;
  void * map;
  alias_Vector(struct AllocationBlockSegment) segments;
};

struct Buffer {
  VkDeviceMemory memory;
  VkDeviceSize memory_start;
  VkDeviceSize memory_offset;
  VkDeviceSize memory_end;
  VkBuffer buffer;
  void * map;
  VkBufferView buffer_view;
  VkFormat format;
};

static struct {
  GLFWwindow * window;

  bool validation;

  VkInstance instance;
  
  VkDebugUtilsMessengerEXT debug_callback;

  VkPhysicalDevice                      physical_device;
  VkPhysicalDeviceProperties            physical_device_properties;
  VkPhysicalDeviceFeatures              physical_device_features;
  VkPhysicalDeviceMemoryProperties      physical_device_memory_properties;
  alias_Vector(VkQueueFamilyProperties) physical_device_queue_family_properties;
  alias_Vector(VkExtensionProperties)   physical_device_extensions;

  VkFormat depthstencil_format;

  uint32_t graphics_queue_family_index;
  uint32_t transfer_queue_family_index;
  uint32_t present_queue_family_index;

  VkDevice device;

  VkSurfaceKHR    surface;
  VkFormat        surface_color_format;
  VkColorSpaceKHR surface_color_space;

  VkCommandPool graphics_command_pool;
  VkCommandPool transfer_command_pool;

  VkSemaphore frame_gpu_present_to_gpu_graphics;
  VkSemaphore frame_gpu_graphics_to_gpu_present;

  VkRenderPass render_pass;

  VkPipelineCache pipelinecache;

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  void * staging_buffer_map;
  VkDeviceSize staging_buffer_size;
  VkDeviceSize staging_buffer_used;

  alias_Vector(struct CommandBuffer) command_buffers[2];
  uint32_t rendering_cbuf;

  alias_Vector(struct AllocationBlock) allocation_blocks;

  uint32_t swapchain_current_index;

  // begin recreation on resize
  VkSwapchainKHR            swapchain;
  VkExtent2D                swapchain_extents;
  alias_Vector(VkImage)     swapchain_images;
  alias_Vector(VkImageView) swapchain_image_views;

  VkImage depthbuffer;
  VkDeviceMemory depthbuffer_memory;
  VkImageView depthbuffer_view;

  alias_Vector(VkFramebuffer) framebuffers;
  // end recreation on resize
} _;

struct PerFrameBuffer {
  uint32_t index;
  float real_time;
  float real_time_delta;
};

struct PerViewBuffer2D {
  float camera[16];
  float game_time;
  float game_time_delta;
};

struct PerDrawBuffer {
  int placeholder;
};

// ====================================================================================================================
static bool create_instance(void) {
  uint32_t count;

  uint32_t enabled_extensions_count = 0;
  const char * enabled_extensions[16];

  uint32_t enabled_layers_count = 0;
  const char * enabled_layers[1];

  {
    const char ** extensions = glfwGetRequiredInstanceExtensions(&count);
    for(uint32_t i = 0; i < count; i++) {
      enabled_extensions[enabled_extensions_count++] = extensions[i];
    }
  }

  if(_.validation) {
    enabled_layers[enabled_layers_count++] = "VK_LAYER_KHRONOS_validation";
  }

  return VK_SUCCESS == vkCreateInstance(&(VkInstanceCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
    , .pApplicationInfo = &(VkApplicationInfo) {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO
      , .pApplicationName = "Alias Engine"
      , .applicationVersion = VK_MAKE_VERSION(1, 0, 0)
      , .pEngineName = "Alias Engine"
      , .engineVersion = VK_MAKE_VERSION(1, 0, 0)
      , .apiVersion = VK_VERSION_1_0
      }
    , .enabledExtensionCount = enabled_extensions_count
    , .ppEnabledExtensionNames = enabled_extensions
    , .enabledLayerCount = enabled_layers_count
    , .ppEnabledLayerNames = enabled_layers
  }, NULL, &_.instance);
}

// ====================================================================================================================
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT     message_severity
  , VkDebugUtilsMessageTypeFlagsEXT            message_type
  , const VkDebugUtilsMessengerCallbackDataEXT * callback_data
  , void                                       * user_data
) {
  return VK_TRUE;
}

static bool create_debug_report_callback(void) {
  if(!_.validation) {
    return true;
  }
  PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_.instance, "vkCreateDebugUtilsMessengerEXT");
  return VK_SUCCESS == createDebugUtilsMessengerEXT(
      _.instance
    , &(VkDebugUtilsMessengerCreateInfoEXT) {
        .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT
      , .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
      , .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
      , .pfnUserCallback = debug_callback
      }
    , NULL
    , &_.debug_callback
    );
}

// ====================================================================================================================
static bool select_physical_device(void) {
  VkResult result;

  uint32_t count;
  vkEnumeratePhysicalDevices(_.instance, &count, NULL);

  VkPhysicalDevice * physical_devices = alias_malloc(alias_default_MemoryCB(), sizeof(*physical_devices) * count, alignof(*physical_devices));
  vkEnumeratePhysicalDevices(_.instance, &count, physical_devices);

  _.physical_device = physical_devices[0];

  alias_free(alias_default_MemoryCB(), physical_devices, sizeof(*physical_devices) * count, alignof(*physical_devices));

  vkGetPhysicalDeviceProperties(_.physical_device, &_.physical_device_properties);

  vkGetPhysicalDeviceFeatures(_.physical_device, &_.physical_device_features);

  vkGetPhysicalDeviceMemoryProperties(_.physical_device, &_.physical_device_memory_properties);

  vkGetPhysicalDeviceQueueFamilyProperties(_.physical_device, &count, NULL);
  alias_Vector_space_for(&_.physical_device_queue_family_properties, alias_default_MemoryCB(), count);
  _.physical_device_queue_family_properties.length = count;
  vkGetPhysicalDeviceQueueFamilyProperties(_.physical_device, &count, _.physical_device_queue_family_properties.data);

  vkEnumerateDeviceExtensionProperties(_.physical_device, NULL, &count, NULL);
  alias_Vector_space_for(&_.physical_device_extensions, alias_default_MemoryCB(), count);
  _.physical_device_extensions.length = count;
  vkEnumerateDeviceExtensionProperties(_.physical_device, NULL, &count, _.physical_device_extensions.data);

  static const VkFormat formats[] = {
      VK_FORMAT_D32_SFLOAT_S8_UINT
    , VK_FORMAT_D32_SFLOAT
    , VK_FORMAT_D24_UNORM_S8_UINT
    , VK_FORMAT_D16_UNORM_S8_UINT
    , VK_FORMAT_D16_UNORM
    };
  for(uint32_t i = 0; i < sizeof(formats)/sizeof(formats[0]); i++) {
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(_.physical_device, formats[i], &format_properties);
    if(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      _.depthstencil_format = formats[i];
      break;
    }
  }

  return true;
}

// ====================================================================================================================
#define VK_QUEUE_PRESENT_BIT 0x10000000

static uint32_t select_queue_family_index(VkQueueFlagBits require, VkQueueFlagBits prefer_absent) {
  for(uint32_t i = 0; i < _.physical_device_queue_family_properties.length; i++) {
    VkQueueFlagBits flags = _.physical_device_queue_family_properties.data[i].queueFlags;
    flags |= glfwGetPhysicalDevicePresentationSupport(_.instance, _.physical_device, i) ? VK_QUEUE_PRESENT_BIT : 0;
    if((flags & require) && !(flags & prefer_absent)) {
      return i;
    }
  }
  for(uint32_t i = 0; i < _.physical_device_queue_family_properties.length; i++) {
    VkQueueFlagBits flags = _.physical_device_queue_family_properties.data[i].queueFlags;
    flags |= glfwGetPhysicalDevicePresentationSupport(_.instance, _.physical_device, i) ? VK_QUEUE_PRESENT_BIT : 0;
    if(flags & require) {
      return i;
    }
  }
  return -1;
}

// ====================================================================================================================
static bool create_device(void) {
  float zero = 0;

  uint32_t queue_create_infos_count = 0;
  VkDeviceQueueCreateInfo queue_create_infos[3];

  _.graphics_queue_family_index = select_queue_family_index(VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT);
  queue_create_infos[queue_create_infos_count++] = (VkDeviceQueueCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
    , .queueFamilyIndex = _.graphics_queue_family_index
    , .queueCount = 1
    , .pQueuePriorities = &zero
    };
  
  _.transfer_queue_family_index = select_queue_family_index(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
  if(_.transfer_queue_family_index != _.graphics_queue_family_index) {
    queue_create_infos[queue_create_infos_count++] = (VkDeviceQueueCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
      , .queueFamilyIndex = _.transfer_queue_family_index
      , .queueCount = 1
      , .pQueuePriorities = &zero
      };
  }

  _.present_queue_family_index = select_queue_family_index(VK_QUEUE_PRESENT_BIT, 0);
  if(_.present_queue_family_index != _.graphics_queue_family_index && _.present_queue_family_index != _.transfer_queue_family_index) {
    queue_create_infos[queue_create_infos_count++] = (VkDeviceQueueCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
      , .queueFamilyIndex = _.present_queue_family_index
      , .queueCount = 1
      , .pQueuePriorities = &zero
      };
  }

  uint32_t enabled_extensions_count = 0;
  const char * enabled_extensions[1];

  enabled_extensions[enabled_extensions_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

  uint32_t enabled_layers_count = 0;
  const char * enabled_layers[1];
  
  return VK_SUCCESS == vkCreateDevice(
      _.physical_device
    , &(VkDeviceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO
      , .pEnabledFeatures = &(VkPhysicalDeviceFeatures) {
          .multiDrawIndirect = VK_TRUE
        , .largePoints = VK_TRUE
        }
      , .pQueueCreateInfos = queue_create_infos
      , .queueCreateInfoCount = queue_create_infos_count
      , .enabledExtensionCount = enabled_extensions_count
      , .ppEnabledExtensionNames = enabled_extensions
      , .enabledLayerCount = 0
      , .ppEnabledLayerNames = NULL
      }
    , NULL
    , &_.device
    );
}

// ====================================================================================================================
static bool create_surface(void) {
  if(VK_SUCCESS != glfwCreateWindowSurface(_.instance, _.window, NULL, &_.surface)) {
    return false;
  }

  uint32_t count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(_.physical_device, _.surface, &count, NULL);
  VkSurfaceFormatKHR * formats = alias_malloc(alias_default_MemoryCB(), sizeof(*formats) * count, alignof(*formats));
  vkGetPhysicalDeviceSurfaceFormatsKHR(_.physical_device, _.surface, &count, formats);

  if(count == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
    formats[0].format = VK_FORMAT_B8G8R8A8_UNORM;
  }

  _.surface_color_format = formats[0].format;
  _.surface_color_space = formats[0].colorSpace;

  for(uint32_t i = 0; i < count; i++) {
    if(formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
      _.surface_color_format = formats[i].format;
      _.surface_color_space = formats[i].colorSpace;
      break;
    }
  }

  alias_free(alias_default_MemoryCB(), formats, sizeof(*formats) * count, alignof(*formats));
  
  return true;
}

// ====================================================================================================================
static bool create_command_pool(uint32_t queue_family_index, VkCommandPool * command_pool) {
  return VK_SUCCESS == vkCreateCommandPool(
      _.device
    , &(VkCommandPoolCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
      , .queueFamilyIndex = queue_family_index
      , .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
      }
    , NULL
    , command_pool
    );
}

// ====================================================================================================================
static bool create_frame_sync_primitives(void) {
  vkCreateSemaphore(
      _.device
    , &(VkSemaphoreCreateInfo) { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }
    , NULL
    , &_.frame_gpu_present_to_gpu_graphics
    );

  vkCreateSemaphore(
      _.device
    , &(VkSemaphoreCreateInfo) { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }
    , NULL
    , &_.frame_gpu_graphics_to_gpu_present
    );

  return true;
}

// ====================================================================================================================
static bool create_render_pass(void) {
  return VK_SUCCESS == vkCreateRenderPass(
      _.device
    , &(VkRenderPassCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
      , .attachmentCount = 2
      , .pAttachments = (VkAttachmentDescription[]) {
          {
              .flags = 0
            , .format = _.surface_color_format
            , .samples = VK_SAMPLE_COUNT_1_BIT
            , .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR
            , .storeOp = VK_ATTACHMENT_STORE_OP_STORE
            , .stencilLoadOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
            , .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
            , .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
            , .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            }
        , {
              .flags = 0
            , .format = _.surface_color_format
            , .samples = VK_SAMPLE_COUNT_1_BIT
            , .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR
            , .storeOp = VK_ATTACHMENT_STORE_OP_STORE
            , .stencilLoadOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
            , .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
            , .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
            , .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            }
        }
      , .subpassCount = 1
      , .pSubpasses = &(VkSubpassDescription) {
          .flags = 0
        , .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS
        , .inputAttachmentCount = 0
        , .pInputAttachments = NULL
        , .colorAttachmentCount = 1
        , .pColorAttachments = &(VkAttachmentReference) {
            .attachment = 0
          , .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
          }
        , .pResolveAttachments = NULL
        , .pDepthStencilAttachment = &(VkAttachmentReference) {
            .attachment = 0
          , .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
          }
        , .preserveAttachmentCount = 0
        , .pPreserveAttachments = NULL
        }
      , .dependencyCount = 0
      , .pDependencies = NULL
      }
    , NULL
    , &_.render_pass
    );
}

// ====================================================================================================================
static bool create_pipeline_cache(void) {
  uv_fs_t req;
  uv_buf_t buf;
  int file;

  file = uv_fs_open(Engine_uv_loop(), &req, "vulkan_pipeline_cache", O_RDONLY, 0, NULL);
  uv_fs_req_cleanup(&req);

  if(file < 0) {
    return VK_SUCCESS == vkCreatePipelineCache(
        _.device
      , &(VkPipelineCacheCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
        }
      , NULL
      , &_.pipelinecache
      );
  }

  uv_fs_fstat(Engine_uv_loop(), &req, file, NULL);

  buf.len = req.statbuf.st_size;
  uv_fs_req_cleanup(&req);

  buf.base = alias_malloc(alias_default_MemoryCB(), buf.len, 4);
  uv_fs_read(Engine_uv_loop(), &req, file, &buf, /* nbufs */ 1, /* offset */ 0, NULL);
  uv_fs_req_cleanup(&req);

  uv_fs_close(Engine_uv_loop(), &req, file, NULL);
  uv_fs_req_cleanup(&req);

  VkResult result = vkCreatePipelineCache(
      _.device
    , &(VkPipelineCacheCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
      , .initialDataSize = buf.len
      , .pInitialData = buf.base
      }
    , NULL
    , &_.pipelinecache
    );

  alias_free(alias_default_MemoryCB(), buf.base, buf.len, 4);

  return VK_SUCCESS == result;
}

// ====================================================================================================================
static bool destroy_pipeline_cache(void) {
  vkDestroyPipelineCache(_.device, _.pipelinecache, NULL);
}

// ====================================================================================================================
static bool create_device_memory(
    VkMemoryRequirements requirements
  , VkMemoryPropertyFlagBits properties
  , VkDeviceMemory *result
) {
  uint32_t index;

  for (index = 0; index < _.physical_device_memory_properties.memoryTypeCount; index++) {
    if (0 != requirements.memoryTypeBits & (1 << index) &&
        properties == _.physical_device_memory_properties.memoryTypes[index].propertyFlags & properties) {
      break;
    }
  }

  if(index == _.physical_device_memory_properties.memoryTypeCount) {
    return false;
  }

  return VK_SUCCESS == vkAllocateMemory(
      _.device
    , &(VkMemoryAllocateInfo) {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
      , .allocationSize = requirements.size
      , .memoryTypeIndex = index
      }
    , NULL
    , result
    );
}

// ====================================================================================================================
static bool create_staging_buffer(void) {
  _.staging_buffer_size = 64 << 20; // 64 MB
  
  vkCreateBuffer(
      _.device
    , &(VkBufferCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
      , .size = _.staging_buffer_size
      , .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
      }
    , NULL
    , &_.staging_buffer
    );

  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(_.device, _.staging_buffer, &memory_requirements);

  create_device_memory(memory_requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &_.staging_buffer_memory);
  vkMapMemory(_.device, _.staging_buffer_memory, 0, _.staging_buffer_size, 0, &_.staging_buffer_map);

  vkBindBufferMemory(_.device, _.staging_buffer, _.staging_buffer_memory, 0);
}

/// flush a single staging buffer, hopefully 
static void flush_staging_buffer(void) {
  
}

static VkSemaphore flush_transfers(void) {
}

// ====================================================================================================================
static bool create_swapchain(uint32_t * width, uint32_t * height, bool vsync) {
  VkSwapchainKHR old = _.swapchain;

  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_.physical_device, _.surface, &surface_capabilities);

  uint32_t present_modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(_.physical_device, _.surface, &present_modes_count, NULL);
  VkPresentModeKHR * present_modes = alias_malloc(alias_default_MemoryCB(), sizeof(*present_modes) * present_modes_count, alignof(*present_modes));
  vkGetPhysicalDeviceSurfacePresentModesKHR(_.physical_device, _.surface, &present_modes_count, present_modes);

  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  if(!vsync) {
    for(uint32_t i = 0; i < present_modes_count; i++) {
      if(present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
        present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
      }
      if(present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
        present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
      }
    }
  }

  alias_free(alias_default_MemoryCB(), present_modes, sizeof(*present_modes) * present_modes_count, alignof(*present_modes));

  if(surface_capabilities.currentExtent.width == -1) {
    _.swapchain_extents.width = *width;
    _.swapchain_extents.height = *height;
  } else {
    _.swapchain_extents = surface_capabilities.currentExtent;
    *width = _.swapchain_extents.width;
    *height = _.swapchain_extents.height;
  }

  uint32_t num_images = surface_capabilities.maxImageCount > 0 ? surface_capabilities.minImageCount + 1 : surface_capabilities.maxImageCount;

  VkSurfaceTransformFlagsKHR pretransform = surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
                                          ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : surface_capabilities.currentTransform;

  VkCompositeAlphaFlagBitsKHR composite_alpha = surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR          ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR          :
                                                surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR  ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR  :
                                                surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR ? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR :
                                                                                                                                            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR         ;

  VkResult result = vkCreateSwapchainKHR(
      _.device
    , &(VkSwapchainCreateInfoKHR) {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR
      , .surface = _.surface
      , .minImageCount = num_images
      , .imageFormat = _.surface_color_format
      , .imageColorSpace = _.surface_color_space
      , .imageExtent = _.swapchain_extents
      , .imageUsage =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
          | (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0)
          | (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0)
      , .preTransform = pretransform
      , .imageArrayLayers = 1
      , .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE
      , .queueFamilyIndexCount = 0
      , .presentMode = present_mode
      , .oldSwapchain = old
      , .clipped = VK_TRUE
      , .compositeAlpha = composite_alpha
      }
    , NULL
    , &_.swapchain
    );

  if(old != VK_NULL_HANDLE) {
    for(uint32_t i = 0; i < _.swapchain_image_views.length; i++) {
      vkDestroyImageView(_.device, _.swapchain_image_views.data[i], NULL);
    }
    vkDestroySwapchainKHR(_.device, old, NULL);
  }

  if(result != VK_SUCCESS) {
    return false;
  }

  uint32_t count;
  vkGetSwapchainImagesKHR(_.device, _.swapchain, &count, NULL);
  _.swapchain_images.length = count;
  alias_Vector_space_for(&_.swapchain_images, alias_default_MemoryCB(), 0);
  vkGetSwapchainImagesKHR(_.device, _.swapchain, &count, _.swapchain_images.data);

  for(uint32_t i = 0; i < count; i++) {
    result = vkCreateImageView(
        _.device
      , &(VkImageViewCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
        , .image = _.swapchain_images.data[i]
        , .viewType = VK_IMAGE_VIEW_TYPE_2D
        , .format = _.surface_color_format
        , .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }
        , .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT
          , .baseMipLevel = 0
          , .levelCount = 1
          , .baseArrayLayer = 0
          , .layerCount = 1
          }
        }
      , NULL
      , &_.swapchain_image_views.data[i]
      );

    if(result != VK_SUCCESS) {
      while(i--) {
        vkDestroyImageView(_.device, _.swapchain_image_views.data[i], NULL);
      }
      vkDestroySwapchainKHR(_.device, _.swapchain, NULL);
      return false;
    }
  }

  return true;
}

// ====================================================================================================================
static bool create_depthbuffer(void) {
  VkResult result;

  result = vkCreateImage(
      _.device
    , &(VkImageCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
      , .imageType = VK_IMAGE_TYPE_2D
      , .format = _.depthstencil_format
      , .extent = { _.swapchain_extents.width, _.swapchain_extents.height, 1 }
      , .mipLevels = 1
      , .arrayLayers = 1
      , .samples = VK_SAMPLE_COUNT_1_BIT
      , .tiling = VK_IMAGE_TILING_OPTIMAL
      , .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
      , .sharingMode = VK_SHARING_MODE_EXCLUSIVE
      , .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
      }
    , NULL
    , &_.depthbuffer
    );

  if(VK_SUCCESS != result) {
    return false;
  }

  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements(_.device, _.depthbuffer, &memory_requirements);

  if(!create_device_memory(memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_.depthbuffer_memory)) {
    vkDestroyImage(_.device, _.depthbuffer, NULL);
    return false;
  }

  vkBindImageMemory(_.device, _.depthbuffer, _.depthbuffer_memory, 0);

  result = vkCreateImageView(
      _.device
    , &(VkImageViewCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
      , .image = _.depthbuffer
      , .viewType = VK_IMAGE_VIEW_TYPE_2D
      , .format = _.depthstencil_format
      , .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }
      , .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
        , .baseMipLevel = 0
        , .levelCount = 1
        , .baseArrayLayer = 0
        , .layerCount = 1
        }
      }
    , NULL
    , &_.depthbuffer_view
    );

  if(VK_SUCCESS != result) {
    vkDestroyImage(_.device, _.depthbuffer, NULL);
    vkFreeMemory(_.device, _.depthbuffer_memory, NULL);
    return false;
  }

  return true;
}

// ====================================================================================================================
static void destroy_depthbuffer(void) {
  vkDestroyImageView(_.device, _.depthbuffer_view, NULL);
  vkDestroyImage(_.device, _.depthbuffer, NULL);
  vkFreeMemory(_.device, _.depthbuffer_memory, NULL);
}

// ====================================================================================================================
static void destroy_framebuffers(void) {
  while(_.framebuffers.length) {
    vkDestroyFramebuffer(_.device, *alias_Vector_pop(&_.framebuffers), NULL);
  }
}

// ====================================================================================================================
static bool create_framebuffers(void) {
  destroy_framebuffers();
  
  alias_Vector_set_capacity(&_.framebuffers, alias_default_MemoryCB(), _.swapchain_image_views.length);

  for(uint32_t i = 0; i < _.swapchain_image_views.length; i++) {
    VkResult result = vkCreateFramebuffer(
        _.device
      , &(VkFramebufferCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO
        , .renderPass = _.render_pass
        , .attachmentCount = 2
        , .pAttachments = (VkImageView[]) { _.swapchain_image_views.data[i], _.depthbuffer_view }
        , .width = _.swapchain_extents.width
        , .height = _.swapchain_extents.height
        , .layers = 1
        }
      , NULL
      , alias_Vector_push(&_.framebuffers)
      );

    if(VK_SUCCESS != result) {
      alias_Vector_pop(&_.framebuffers);
      destroy_framebuffers();
      return false;
    }
  }

  return true;
}

bool Vulkan_init(uint32_t * width, uint32_t * height, GLFWwindow * window) {
  _.window = window;
  
  return
       create_instance()
    && select_physical_device()
    && create_device()
    && create_surface()
    && create_command_pool(_.graphics_queue_family_index, &_.graphics_command_pool)
    && create_command_pool(_.transfer_queue_family_index, &_.transfer_command_pool)
    && create_frame_sync_primitives()
    && create_render_pass()
    && create_pipeline_cache()
    && create_swapchain(width, height, false)
    && create_depthbuffer()
    && create_framebuffers()
    ;
}

void Vulkan_cleanup(void) {
}

static uint32_t acquire_command_buffer(uint32_t queue) {
  for(uint32_t i = 0; i < _.command_buffers[queue].length; i++) {
    VkResult result = vkGetFenceStatus(_.device, _.command_buffers[queue].data[i].fence);
    if(result == VK_SUCCESS) {
      vkResetFences(_.device, 1, &_.command_buffers[queue].data[i].fence);
      return i;
    }
  }

  uint32_t index = _.command_buffers[queue].length;
  alias_Vector_space_for(&_.command_buffers[queue], alias_default_MemoryCB(), 1);
  struct CommandBuffer * cbuf = alias_Vector_push(&_.command_buffers[queue]);

  vkAllocateCommandBuffers(
      _.device
    , &(VkCommandBufferAllocateInfo) {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
      , .commandPool        = queue ? _.transfer_command_pool : _.graphics_command_pool
      , .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY
      , .commandBufferCount = 1
      }
    , &cbuf->buffer
    );

  vkCreateFence(
      _.device
    , &(VkFenceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
      , .flags = VK_FENCE_CREATE_SIGNALED_BIT
      }
    , NULL
    , &cbuf->fence
    );

  return index;
}

struct ImageLoadContext {
  struct BackendImage * image;
  uv_fs_t req;
  int fd;
  uv_stat_t stat;
  uv_buf_t buf;
};

static void _image_close(uv_fs_t * req) {
  struct ImageLoadContext * ctx = (struct ImageLoadContext *)req->data;
  uv_fs_req_cleanup(req);
  alias_free(alias_default_MemoryCB(), ctx, sizeof(*ctx), alignof(*ctx));
}

static void _image_read(uv_fs_t * req) {
  struct ImageLoadContext * ctx = (struct ImageLoadContext *)req->data;
  uv_fs_req_cleanup(req);

  if(req->result < 0) {
    alias_free(alias_default_MemoryCB(), ctx->buf.base, ctx->buf.len, 4);
    alias_free(alias_default_MemoryCB(), ctx, sizeof(*ctx), alignof(*ctx));
    return;
  }

  struct BackendImage * image = ctx->image;

  int width, height, channels;
  stbi_uc * pixels = stbi_load_from_memory(ctx->buf.base, ctx->buf.len, &width, &height, &channels, STBI_rgb_alpha);

  alias_free(alias_default_MemoryCB(), ctx->buf.base, ctx->buf.len, 4);
  uv_fs_close(Engine_uv_loop(), &ctx->req, ctx->fd, _image_close);

  // Vulkan loading here
  VkDeviceSize size = width * height * 4;
  while(_.staging_buffer_used + size > _.staging_buffer_size) {
    flush_staging_buffer();
  }

  void * dst = _.staging_buffer_map + _.staging_buffer_used;
  _.staging_buffer_used += size;

  alias_memory_copy(dst, size, pixels, size);

  stbi_image_free(pixels);

  vkCreateImage(
      _.device
    , &(VkImageCreateInfo) {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
      , .flags         = 0
      , .imageType     = VK_IMAGE_TYPE_2D
      , .format        = VK_FORMAT_R8G8B8A8_UNORM
      , .extent        = { width, height, 1 }
      , .mipLevels     = 1 + log2(alias_max(width, height))
      , .arrayLayers   = 1
      , .samples       = VK_SAMPLE_COUNT_1_BIT
      , .tiling        = VK_IMAGE_TILING_OPTIMAL
      , .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
      , .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
      }
    , NULL
    , (VkImage *)&image->image
    );

  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements(_.device, (VkImage)image->image, &memory_requirements);

  create_device_memory(memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, (VkDeviceMemory *)&image->memory);

  vkBindImageMemory(_.device, (VkImage)image->image, (VkDeviceMemory)image->memory, 0);

  VkImageView imageview;
  vkCreateImageView(
      _.device
    , &(VkImageViewCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
      , .image = (VkImage)image->image
      , .viewType = VK_IMAGE_VIEW_TYPE_2D
      , .format = VK_FORMAT_R8G8B8A8_UNORM
      , .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }
      , .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
        , .baseMipLevel = 0
        , .levelCount = 1
        , .baseArrayLayer = 0
        , .layerCount = 1
        }
      }
    , NULL
    , &imageview
    );

  image->imageview = (uint64_t)imageview;
}

static void _image_fstat(uv_fs_t * req) {
  struct ImageLoadContext * ctx = (struct ImageLoadContext *)req->data;

  if(req->result < 0) {
    uv_fs_req_cleanup(req);
    alias_free(alias_default_MemoryCB(), ctx, sizeof(*ctx), alignof(*ctx));
    return;
  }

  ctx->stat = req->statbuf;
  uv_fs_req_cleanup(req);

  ctx->buf.len = ctx->stat.st_size;
  ctx->buf.base = alias_malloc(alias_default_MemoryCB(), ctx->buf.len, 4);

  uv_fs_read(Engine_uv_loop(), &ctx->req, ctx->fd, &ctx->buf, 1, 0, _image_read);
}

static void _image_open(uv_fs_t * req) {
  struct ImageLoadContext * ctx = (struct ImageLoadContext *)req->data;

  if(req->result < 0) {
    uv_fs_req_cleanup(req);
    alias_free(alias_default_MemoryCB(), ctx, sizeof(*ctx), alignof(*ctx));
    return;
  }

  ctx->fd = req->result;
  uv_fs_req_cleanup(req);

  uv_fs_fstat(Engine_uv_loop(), &ctx->req, ctx->fd, _image_fstat);
}

void BackendImage_load(struct BackendImage * image, const char * filename) {
  struct ImageLoadContext * ctx = alias_malloc(alias_default_MemoryCB(), sizeof(*ctx), alignof(*ctx));
  ctx->image = image;
  ctx->req.data = ctx;
  uv_fs_open(Engine_uv_loop(), &ctx->req, filename, O_RDONLY, 0, _image_open);
}

void BackendImage_unload(struct BackendImage * image) {
  vkDestroyImageView(_.device, (VkImageView)image->imageview, NULL);
  vkDestroyImage(_.device, (VkImage)image->image, NULL);
  vkFreeMemory(_.device, (VkDeviceMemory)image->memory, NULL);
}

void BackendUIVertex_render(const struct BackendImage * image, struct BackendUIVertex * vertexes, uint32_t num_indexes, const uint32_t * indexes) {
  VkImageView imageview = (VkImageView)image->image;

  if(imageview == VK_NULL_HANDLE) {
    // TODO
    // image = &missing_image
    // imageview = atomic_load(&image->image)
  }

  
}

void Backend_begin_rendering(uint32_t screen_width, uint32_t screen_height) {
  vkAcquireNextImageKHR(_.device, _.swapchain, UINT64_MAX, _.frame_gpu_present_to_gpu_graphics, VK_NULL_HANDLE, &_.swapchain_current_index);
  
  _.rendering_cbuf = acquire_command_buffer(0);
}

void Backend_end_rendering(void) {
  VkSemaphore transfer_done = flush_transfers();

  vkEndCommandBuffer(_.command_buffers[0].data[_.rendering_cbuf].buffer);

  VkQueue queue;
  vkGetDeviceQueue(_.device, _.graphics_queue_family_index, 0, &queue);
  vkQueueSubmit(
      queue
    , 1
    , &(VkSubmitInfo) {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO
      , .waitSemaphoreCount   = transfer_done == VK_NULL_HANDLE ? 1 : 2
      , .pWaitSemaphores      = (VkSemaphore[]){ _.frame_gpu_present_to_gpu_graphics, transfer_done }
      , .pWaitDstStageMask    = (VkPipelineStageFlags[]){ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
      , .commandBufferCount   = 1
      , .pCommandBuffers      = &_.command_buffers[0].data[_.rendering_cbuf].buffer
      , .signalSemaphoreCount = 1
      , .pSignalSemaphores    = &_.frame_gpu_graphics_to_gpu_present
      }
    , _.command_buffers[0].data[_.rendering_cbuf].fence
    );
  
  glfwPollEvents();
}

void Backend_begin_2d(struct BackendMode2D mode) {
  VkCommandBuffer cbuf = _.command_buffers[0].data[_.rendering_cbuf].buffer;

  VkViewport viewport = {
      .x = alias_pga2d_point_x(mode.viewport_min) * _.swapchain_extents.width
    , .y = alias_pga2d_point_y(mode.viewport_min) * _.swapchain_extents.height
    , .width = (alias_pga2d_point_x(mode.viewport_max) - alias_pga2d_point_x(mode.viewport_min)) * _.swapchain_extents.width
    , .height = (alias_pga2d_point_y(mode.viewport_max) - alias_pga2d_point_y(mode.viewport_min)) * _.swapchain_extents.height
    , .minDepth = 0
    , .maxDepth = 1
    };

  VkRect2D scissor = { .offset = { viewport.x, viewport.y }, .extent = { viewport.width, viewport.height } };
  
  vkCmdBeginRenderPass(
      cbuf
    , &(VkRenderPassBeginInfo) {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO
      , .renderPass  = _.render_pass
      , .framebuffer = _.framebuffers.data[_.swapchain_current_index]
      , .renderArea  = scissor
      , .clearValueCount = 2
      , .pClearValues = (VkClearValue[]) {
          { .color.float32 = { mode.background.r, mode.background.g, mode.background.b, mode.background.a } }
        , { .depthStencil.depth = 1, .depthStencil.stencil = 0 }
        }
      }
    , VK_SUBPASS_CONTENTS_INLINE
    );

  vkCmdSetViewport(cbuf, 1, 1, &viewport);
  vkCmdSetScissor(cbuf, 1, 1, &scissor);
}

void Backend_end_2d(void) {
  vkCmdEndRenderPass(_.command_buffers[0].data[_.rendering_cbuf].buffer);
}

