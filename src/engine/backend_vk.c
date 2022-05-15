#include "engine.h"

#include "backend.h"

#include <alias/ui.h>
#include <alias/data_structure/inline_list.h>
#include <alias/data_structure/vector.h>
#include <alias/data_structure/paged_soa.h>
#include <alias/log.h>

#include "backend_vk_conf.h"

#include <alias/cpp.h>
#define CLEAN(X,...) __VA_ARGS__
#define CLEAN_ID() CLEAN

#define INSTANCE_GET(X) pfn_vkGetInstanceProcAddr(_.instance, #X)
#define DEVICE_GET(X) pfn_vkGetDeviceProcAddr(_.device, #X)

// VALUE, READ, WRITE, ALLOCATOR, INSTANCE, PHYSICAL_DEVICE, DEVICE
#define LOAD_ARGUMENT_VALUE(TYPE, NAME, ...) , TYPE NAME
#define LOAD_ARGUMENT_READ(TYPE, NAME, ...)  , const TYPE * NAME
#define LOAD_ARGUMENT_WRITE(TYPE, NAME, ...) , TYPE * NAME
#define LOAD_ARGUMENT_ALLOCATOR              , const VkAllocationCallbacks * pAllocator
#define LOAD_ARGUMENT_INSTANCE               , VkInstance instance
#define LOAD_ARGUMENT_PHYSICAL_DEVICE        , VkPhysicalDevice physicalDevice
#define LOAD_ARGUMENT_DEVICE                 , VkDevice device
#define LOAD_ARGUMENT(X) ALIAS_CPP_CAT(LOAD_ARGUMENT_,X)

// VALUE, READ, WRITE, ALLOCATOR, INSTANCE, PHYSICAL_DEVICE, DEVICE
#define LOAD_PASS_VALUE(TYPE, NAME, ...) , NAME
#define LOAD_PASS_READ(TYPE, NAME, ...)  , NAME
#define LOAD_PASS_WRITE(TYPE, NAME, ...) , NAME
#define LOAD_PASS_ALLOCATOR              , pAllocator
#define LOAD_PASS_INSTANCE               , instance
#define LOAD_PASS_PHYSICAL_DEVICE        , physicalDevice
#define LOAD_PASS_DEVICE                 , device
#define LOAD_PASS(X) ALIAS_CPP_CAT(LOAD_PASS_,X)

// VALUE, READ, WRITE, ALLOCATOR, INSTANCE, PHYSICAL_DEVICE, DEVICE
#define STATIC_ARGUMENT_VALUE(TYPE, NAME, ...) , TYPE NAME
#define STATIC_ARGUMENT_READ(TYPE, NAME, ...)  , const TYPE * NAME
#define STATIC_ARGUMENT_WRITE(TYPE, NAME, ...) , TYPE * NAME
#define STATIC_ARGUMENT(X) ALIAS_CPP_CAT(STATIC_ARGUMENT_,X)
#define IS_STATIC_ARGUMENT(X) ALIAS_CPP_IS_PROBE(ALIAS_CPP_CAT(IS_STATIC_ARGUMENT_, X)())
#define IS_STATIC_ARGUMENT_VALUE(...) ALIAS_CPP_PROBE
#define IS_STATIC_ARGUMENT_READ(...) ALIAS_CPP_PROBE
#define IS_STATIC_ARGUMENT_WRITE(...) ALIAS_CPP_PROBE

// VALUE, READ, WRITE, ALLOCATOR, INSTANCE, PHYSICAL_DEVICE, DEVICE
#define STATIC_PASS_VALUE(TYPE, NAME, ...) , NAME
#define STATIC_PASS_READ(TYPE, NAME, ...)  , NAME
#define STATIC_PASS_WRITE(TYPE, NAME, ...) , NAME
#define STATIC_PASS_ALLOCATOR              , NULL
#define STATIC_PASS_INSTANCE               , _.instance
#define STATIC_PASS_PHYSICAL_DEVICE        , _.physical_device
#define STATIC_PASS_DEVICE                 , _.device
#define STATIC_PASS(X) ALIAS_CPP_CAT(STATIC_PASS_,X)

#define LAZY_VULKAN_IMPL(NAME, GET, VOID, RETURN, ...) \
extern PFN_##NAME pfn_##NAME; \
static VKAPI_ATTR VOID VKAPI_CALL load_##NAME( ALIAS_CPP_EVAL( ALIAS_CPP_DEFER_2(CLEAN_ID)()( ALIAS_CPP_MAP(LOAD_ARGUMENT, __VA_ARGS__) ) ) ) { \
  PFN_##NAME ptr = (PFN_##NAME)GET(NAME); \
  if(ptr == NULL) { \
    ALIAS_ERROR("failed to load Vulkan function " #NAME); \
  } \
  pfn_##NAME = ptr; \
  RETURN pfn_##NAME( ALIAS_CPP_EVAL( ALIAS_CPP_DEFER_2(CLEAN_ID)()( ALIAS_CPP_MAP(LOAD_PASS, __VA_ARGS__) ) ) ); \
} \
PFN_##NAME pfn_##NAME = load_##NAME; \
static inline VOID NAME( ALIAS_CPP_EVAL( ALIAS_CPP_DEFER_2(CLEAN_ID)()( ALIAS_CPP_FILTER_MAP(IS_STATIC_ARGUMENT, STATIC_ARGUMENT, __VA_ARGS__) ) ) ) { \
  RETURN pfn_##NAME( ALIAS_CPP_EVAL( ALIAS_CPP_DEFER_2(CLEAN_ID)()( ALIAS_CPP_MAP(STATIC_PASS, __VA_ARGS__) ) ) ); \
}

#define INSTANCE_R(NAME, ...) LAZY_VULKAN_IMPL(NAME, INSTANCE_GET,           VkResult, return, __VA_ARGS__)
#define INSTANCE_V(NAME, ...) LAZY_VULKAN_IMPL(NAME, INSTANCE_GET,               void,       , __VA_ARGS__)
#define INSTANCE_F(NAME, ...) LAZY_VULKAN_IMPL(NAME, INSTANCE_GET, PFN_vkVoidFunction, return, __VA_ARGS__)

#define DEVICE_R(NAME, ...) LAZY_VULKAN_IMPL(NAME, DEVICE_GET,           VkResult, return, __VA_ARGS__)
#define DEVICE_V(NAME, ...) LAZY_VULKAN_IMPL(NAME, DEVICE_GET,               void,       , __VA_ARGS__)
#define DEVICE_F(NAME, ...) LAZY_VULKAN_IMPL(NAME, DEVICE_GET, PFN_vkVoidFunction, return, __VA_ARGS__)

#include <uchar.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "stb_image.h"

enum QueueIndex {
    Graphics
  , Transfer
  , Present
  , NUM_QUEUES
};

struct CommandBuffer {
  VkCommandBuffer buffer;
  VkFence fence;
  uint64_t frame;
  bool ready;
  VkDeviceSize stage_start;
  VkDeviceSize stage_end;
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

struct MemoryAllocation {
  VkDeviceMemory memory;
  VkDeviceSize memory_start;
  VkDeviceSize memory_offset;
  VkDeviceSize memory_end;
};

struct Buffer {
  struct MemoryAllocation allocation;
  VkBuffer buffer;
  void * map;
  VkBufferView buffer_view;
  VkFormat format;
};

struct TempPage {
  struct Buffer buffer;
  VkDeviceSize used;
};

// VULKAN_UI_TEXTURE_BATCH_SIZE 64

enum Binding {
    Binding_PerFrame // per frame buffer, samplers
  , Binding_PerView  // per view buffer
  , Binding_PerDraw  // per draw storage buffer (batch of draw data), textures (VULKAN_UI_TEXTURE_BATCH_SIZE), materials (index with texture, other params)
};

#if 0
0:0:
  real time
  real time delta
0:1:
  samplers
1:0:
  camera
  game time
  game time delta
2:0:
  [
    indirect indexed draw parameters ...
    material index
  ] (indexed based on instance index)
2:1:
  [
    texture index
  ] (indexed with draw parameter)
2:2:
  [textures 64] (indexed with material)
#endif

static struct {
  GLFWwindow * window;

  bool validation;

  VkInstance instance;

  bool debug_utils;
  bool debug_report;
  bool debug_marker;
  VkDebugUtilsMessengerEXT debug_utils_callback;
  VkDebugReportCallbackEXT debug_report_callback;

  VkPhysicalDevice                      physical_device;
  VkPhysicalDeviceProperties            physical_device_properties;
  VkPhysicalDeviceFeatures              physical_device_features;
  VkPhysicalDeviceMemoryProperties      physical_device_memory_properties;
  alias_Vector(VkQueueFamilyProperties) physical_device_queue_family_properties;
  alias_Vector(VkExtensionProperties)   physical_device_extensions;

  VkFormat depthstencil_format;

  uint32_t queue_family_index[NUM_QUEUES];

  VkDevice device;

  VkQueue queue[NUM_QUEUES];

  VkSurfaceKHR    surface;
  VkFormat        surface_color_format;
  VkColorSpaceKHR surface_color_space;

  VkCommandPool command_pool[NUM_QUEUES];

  VkSemaphore frame_gpu_transfer_to_gpu_graphics;
  VkSemaphore frame_gpu_present_to_gpu_graphics;
  VkSemaphore frame_gpu_graphics_to_gpu_present;
  uint64_t frame_pending[NUM_QUEUES];
  uint64_t frame_complete[NUM_QUEUES];

  VkRenderPass render_pass;

  VkPipelineCache pipelinecache;

  VkBuffer staging_buffer;
  VkDeviceMemory staging_buffer_memory;
  void * staging_buffer_map;
  VkDeviceSize staging_buffer_size;
  
  uint32_t staging_buffer_unused[4];

  alias_Vector(struct CommandBuffer) command_buffers[NUM_QUEUES];
  uint32_t rendering_cbuf;

  alias_Vector(struct AllocationBlock) allocation_blocks;

  uint32_t swapchain_current_index;

  uint32_t transition_cbuf[NUM_QUEUES];
  uint32_t in_flight[NUM_QUEUES];

  VkSampler samplers[NUM_BACKEND_SAMPLERS];

  VkDescriptorSetLayout descriptor_set_layout[3];
  VkPipelineLayout pipeline_layout;
  VkPipeline ui_pipeline;
  VkDescriptorPool descriptor_pool;

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

  // per frame data
  alias_Vector(struct TempPage) temp_pages;
  uint32_t current_temp_page;
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
  // VkDrawIndexedIndirectCommand
  uint32_t index_count;
  uint32_t instance_count;
  uint32_t first_index;
  int32_t vertex_offset;
  uint32_t first_instance;

  uint32_t material;
};

extern PFN_vkGetInstanceProcAddr pfn_vkGetInstanceProcAddr;
extern PFN_vkGetDeviceProcAddr pfn_vkGetDeviceProcAddr;

INSTANCE_R(
    vkCreateInstance
  , READ(VkInstanceCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkInstance, pInstance, 1)
)
INSTANCE_V(
    vkDestroyInstance
  , INSTANCE
  , ALLOCATOR
)
INSTANCE_R(
    vkEnumeratePhysicalDevices
  , INSTANCE
  , WRITE(uint32_t, pPhysicalDeviceCount, 1)
  , WRITE(VkPhysicalDevice, pPhysicalDevices, *pPhysicalDeviceCount)
)
INSTANCE_V(
    vkGetPhysicalDeviceFeatures
  , PHYSICAL_DEVICE
  , WRITE(VkPhysicalDeviceFeatures, pFeatures, 1)
)
INSTANCE_V(
    vkGetPhysicalDeviceFormatProperties
  , PHYSICAL_DEVICE
  , VALUE(VkFormat, format)
  , WRITE(VkFormatProperties, pFormatProperties, 1)
)
// typedef VkResult (VKAPI_PTR *PFN_vkGetPhysicalDeviceImageFormatProperties)(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties);
INSTANCE_V(
    vkGetPhysicalDeviceProperties
  , PHYSICAL_DEVICE
  , WRITE(VkPhysicalDeviceProperties, pProperties)
)
INSTANCE_V(
    vkGetPhysicalDeviceQueueFamilyProperties
  , PHYSICAL_DEVICE
  , WRITE(uint32_t, pQueueFamilyPropertyCount, 1)
  , WRITE(VkQueueFamilyProperties, pQueueFamilyProperties, *pQueueFamilyPropertyCount)
)
INSTANCE_V(
    vkGetPhysicalDeviceMemoryProperties
  , PHYSICAL_DEVICE
  , WRITE(VkPhysicalDeviceMemoryProperties, pMemoryProperties, 1)
)
INSTANCE_F(
    vkGetInstanceProcAddr
  , INSTANCE
  , READ(char, pName, strlen(pName))
)
INSTANCE_F(
    vkGetDeviceProcAddr
  , DEVICE
  , READ(char, pName, strlen(pName))
)
INSTANCE_R(
    vkCreateDevice
  , PHYSICAL_DEVICE
  , READ(VkDeviceCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkDevice, pDevice, 1)
)
// typedef void (VKAPI_PTR *PFN_vkDestroyDevice)(VkDevice device, const VkAllocationCallbacks* pAllocator);
INSTANCE_R(
    vkEnumerateInstanceExtensionProperties
  , READ(char, pLayerName, strlen(pLayerName))
  , WRITE(uint32_t, pPropertyCount, 1)
  , WRITE(VkExtensionProperties, pProperties, pProperties == NULL ? 0 : *pPropertyCount)
)
INSTANCE_R(
    vkEnumerateDeviceExtensionProperties
  , PHYSICAL_DEVICE
  , READ(char, pLayerName, strlen(pLayerName))
  , WRITE(uint32_t, pPropertyCount, 1)
  , WRITE(VkExtensionProperties, pProperties, pProperties == NULL ? 0 : *pPropertyCount)
)
INSTANCE_R(
    vkEnumerateInstanceLayerProperties
  , WRITE(uint32_t, pPropertyCount, 1)
  , WRITE(VkLayerProperties, pProperties, pProperties == NULL ? 0 : *pPropertyCount)
)
// typedef VkResult (VKAPI_PTR *PFN_vkEnumerateDeviceLayerProperties)(VkPhysicalDevice physicalDevice, uint32_t* pPropertyCount, VkLayerProperties* pProperties);
DEVICE_V(
    vkGetDeviceQueue
  , DEVICE
  , VALUE(uint32_t, queueFamilyIndex)
  , VALUE(uint32_t, queueIndex)
  , WRITE(VkQueue, pQueue, 1)
)
DEVICE_R(
    vkQueueSubmit
  , VALUE(VkQueue, queue)
  , VALUE(uint32_t, submitCount)
  , READ(VkSubmitInfo, pSubmits, submitCount)
  , VALUE(VkFence, fence)
)
// typedef VkResult (VKAPI_PTR *PFN_vkQueueWaitIdle)(VkQueue queue);
// typedef VkResult (VKAPI_PTR *PFN_vkDeviceWaitIdle)(VkDevice device);
DEVICE_R(
    vkAllocateMemory
  , DEVICE
  , READ(VkMemoryAllocateInfo, pAllocateInfo, 1)
  , ALLOCATOR
  , WRITE(VkDeviceMemory, pMemory, 1)
)
DEVICE_V(
    vkFreeMemory
  , DEVICE
  , VALUE(VkDeviceMemory, memory)
  , ALLOCATOR
)
DEVICE_R(
    vkMapMemory
  , DEVICE
  , VALUE(VkDeviceMemory, memory)
  , VALUE(VkDeviceSize, offset)
  , VALUE(VkDeviceSize, size)
  , VALUE(VkMemoryMapFlags, flags)
  , WRITE(void*, ppData, 1)
)
DEVICE_V(
    vkUnmapMemory
  , DEVICE
  , VALUE(VkDeviceMemory, memory)
)
DEVICE_R(
    vkFlushMappedMemoryRanges
  , DEVICE
  , VALUE(uint32_t, memoryRangeCount)
  , READ(VkMappedMemoryRange, pMemoryRanges, memoryRangeCount)
)
DEVICE_R(
    vkInvalidateMappedMemoryRanges
  , DEVICE
  , VALUE(uint32_t, memoryRangeCount)
  , READ(VkMappedMemoryRange, pMemoryRanges, memoryRangeCount)
)
DEVICE_V(
    vkGetDeviceMemoryCommitment
  , DEVICE
  , VALUE(VkDeviceMemory, memory)
  , WRITE(VkDeviceSize, pCommittedMemoryInBytes, 1)
)
DEVICE_R(
    vkBindBufferMemory
  , DEVICE
  , VALUE(VkBuffer, buffer)
  , VALUE(VkDeviceMemory, memory)
  , VALUE(VkDeviceSize, memoryOffset)
)
DEVICE_R(
    vkBindImageMemory
  , DEVICE
  , VALUE(VkImage, image)
  , VALUE(VkDeviceMemory, memory)
  , VALUE(VkDeviceSize, memoryOffset)
)
DEVICE_V(
    vkGetBufferMemoryRequirements
  , DEVICE
  , VALUE(VkBuffer, buffer)
  , WRITE(VkMemoryRequirements, pMemoryRequirements, 1)
)
DEVICE_V(
    vkGetImageMemoryRequirements
  , DEVICE
  , VALUE(VkImage, image)
  , WRITE(VkMemoryRequirements, pMemoryRequirements, 1)
)
// typedef void (VKAPI_PTR *PFN_vkGetImageSparseMemoryRequirements)(VkDevice device, VkImage image, uint32_t* pSparseMemoryRequirementCount, VkSparseImageMemoryRequirements* pSparseMemoryRequirements);
// typedef void (VKAPI_PTR *PFN_vkGetPhysicalDeviceSparseImageFormatProperties)(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t* pPropertyCount, VkSparseImageFormatProperties* pProperties);
// typedef VkResult (VKAPI_PTR *PFN_vkQueueBindSparse)(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence);
DEVICE_R(
    vkCreateFence
  , DEVICE
  , READ(VkFenceCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkFence, pFence, 1)
)
// typedef void (VKAPI_PTR *PFN_vkDestroyFence)(VkDevice device, VkFence fence, const VkAllocationCallbacks* pAllocator);
DEVICE_R(
    vkResetFences
  , DEVICE
  , VALUE(uint32_t, fenceCount)
  , READ(VkFence, pFences, fenceCount)
)
DEVICE_R(
    vkGetFenceStatus
  , DEVICE
  , VALUE(VkFence, fence)
)
DEVICE_R(
    vkWaitForFences
  , DEVICE
  , VALUE(uint32_t, fenceCount)
  , READ(VkFence, pFences, fenceCount)
  , VALUE(VkBool32, waitAll)
  , VALUE(uint64_t, timeout)
)
DEVICE_R(
    vkCreateSemaphore
  , DEVICE
  , READ(VkSemaphoreCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkSemaphore, pSemaphore, 1)
)
// typedef void (VKAPI_PTR *PFN_vkDestroySemaphore)(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks* pAllocator);
DEVICE_R(
    vkCreateEvent
  , DEVICE
  , READ(VkEventCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkEvent, pEvent, 1)
)
// typedef void (VKAPI_PTR *PFN_vkDestroyEvent)(VkDevice device, VkEvent event, const VkAllocationCallbacks* pAllocator);
DEVICE_R(
    vkGetEventStatus
  , DEVICE
  , VALUE(VkEvent, event)
)
// typedef VkResult (VKAPI_PTR *PFN_vkSetEvent)(VkDevice device, VkEvent event);
DEVICE_R(
    vkResetEvent
  , DEVICE
  , VALUE(VkEvent, event)
)
// typedef VkResult (VKAPI_PTR *PFN_vkCreateQueryPool)(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkQueryPool* pQueryPool);
// typedef void (VKAPI_PTR *PFN_vkDestroyQueryPool)(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks* pAllocator);
// typedef VkResult (VKAPI_PTR *PFN_vkGetQueryPoolResults)(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags);
INSTANCE_R(
    vkCreateBuffer
  , DEVICE
  , READ(VkBufferCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkBuffer, pBuffer, 1)
)
// typedef void (VKAPI_PTR *PFN_vkDestroyBuffer)(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks* pAllocator);
// typedef VkResult (VKAPI_PTR *PFN_vkCreateBufferView)(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkBufferView* pView);
// typedef void (VKAPI_PTR *PFN_vkDestroyBufferView)(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks* pAllocator);
DEVICE_R(
    vkCreateImage
  , DEVICE
  , READ(VkImageCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkImage, pImage, 1)
)
DEVICE_V(
    vkDestroyImage
  , DEVICE
  , VALUE(VkImage, image)
  , ALLOCATOR
)
// typedef void (VKAPI_PTR *PFN_vkGetImageSubresourceLayout)(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout);
DEVICE_R(
    vkCreateImageView
  , DEVICE
  , READ(VkImageViewCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkImageView, pView, 1)
)
DEVICE_V(
    vkDestroyImageView
  , DEVICE
  , VALUE(VkImageView, imageView)
  , ALLOCATOR
)
DEVICE_R(
    vkCreateShaderModule
  , DEVICE
  , READ(VkShaderModuleCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkShaderModule, pShaderModule, 1)
)
// typedef void (VKAPI_PTR *PFN_vkDestroyShaderModule)(VkDevice device, VkShaderModule shaderModule, const VkAllocationCallbacks* pAllocator);
DEVICE_R(
    vkCreatePipelineCache
  , DEVICE
  , READ(VkPipelineCacheCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkPipelineCache, pPipelineCache, 1)
)
DEVICE_V(
    vkDestroyPipelineCache
  , DEVICE
  , VALUE(VkPipelineCache, pipelineCache)
  , ALLOCATOR
)
// typedef VkResult (VKAPI_PTR *PFN_vkGetPipelineCacheData)(VkDevice device, VkPipelineCache pipelineCache, size_t* pDataSize, void* pData);
// typedef VkResult (VKAPI_PTR *PFN_vkMergePipelineCaches)(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount, const VkPipelineCache* pSrcCaches);
DEVICE_R(
    vkCreateGraphicsPipelines
  , DEVICE
  , VALUE(VkPipelineCache, pipelineCache)
  , VALUE(uint32_t, createInfoCount)
  , READ(VkGraphicsPipelineCreateInfo, pCreateInfos, createInfoCount)
  , ALLOCATOR
  , WRITE(VkPipeline, pPipelines, createInfoCount)
)
// typedef VkResult (VKAPI_PTR *PFN_vkCreateComputePipelines)(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines);
// typedef void (VKAPI_PTR *PFN_vkDestroyPipeline)(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks* pAllocator);
DEVICE_R(
    vkCreatePipelineLayout
  , DEVICE
  , READ(VkPipelineLayoutCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkPipelineLayout, pPipelineLayout, 1)
)
// typedef void (VKAPI_PTR *PFN_vkDestroyPipelineLayout)(VkDevice device, VkPipelineLayout pipelineLayout, const VkAllocationCallbacks* pAllocator);
DEVICE_R(
    vkCreateSampler
  , DEVICE
  , READ(VkSamplerCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkSampler, pSampler, 1)
)
// typedef void (VKAPI_PTR *PFN_vkDestroySampler)(VkDevice device, VkSampler sampler, const VkAllocationCallbacks* pAllocator);
DEVICE_R(
    vkCreateDescriptorSetLayout
  , DEVICE
  , READ(VkDescriptorSetLayoutCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkDescriptorSetLayout, pSetLayout, 1)
)
// typedef void (VKAPI_PTR *PFN_vkDestroyDescriptorSetLayout)(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, const VkAllocationCallbacks* pAllocator);
// typedef VkResult (VKAPI_PTR *PFN_vkCreateDescriptorPool)(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDescriptorPool* pDescriptorPool);
// typedef void (VKAPI_PTR *PFN_vkDestroyDescriptorPool)(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks* pAllocator);
// typedef VkResult (VKAPI_PTR *PFN_vkResetDescriptorPool)(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags);
// typedef VkResult (VKAPI_PTR *PFN_vkAllocateDescriptorSets)(VkDevice device, const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets);
// typedef VkResult (VKAPI_PTR *PFN_vkFreeDescriptorSets)(VkDevice device, VkDescriptorPool descriptorPool, uint32_t descriptorSetCount, const VkDescriptorSet* pDescriptorSets);
// typedef void (VKAPI_PTR *PFN_vkUpdateDescriptorSets)(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies);
DEVICE_R(
    vkCreateFramebuffer
  , DEVICE
  , READ(VkFramebufferCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkFramebuffer, pFramebuffer, 1)
)
DEVICE_V(
    vkDestroyFramebuffer
  , DEVICE
  , VALUE(VkFramebuffer, framebuffer)
  , ALLOCATOR
)
DEVICE_R(
    vkCreateRenderPass
  , DEVICE
  , READ(VkRenderPassCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkRenderPass, pRenderPass, 1)
)
// typedef void (VKAPI_PTR *PFN_vkDestroyRenderPass)(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks* pAllocator);
// typedef void (VKAPI_PTR *PFN_vkGetRenderAreaGranularity)(VkDevice device, VkRenderPass renderPass, VkExtent2D* pGranularity);
DEVICE_R(
    vkCreateCommandPool
  , DEVICE
  , READ(VkCommandPoolCreateInfo, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkCommandPool, pCommandPool, 1)
)
// typedef void (VKAPI_PTR *PFN_vkDestroyCommandPool)(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks* pAllocator);
// typedef VkResult (VKAPI_PTR *PFN_vkResetCommandPool)(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags);
DEVICE_R(
    vkAllocateCommandBuffers
  , DEVICE
  , READ(VkCommandBufferAllocateInfo, pAllocateInfo, 1)
  , WRITE(VkCommandBuffer, pCommandBuffers, 1)
)
// typedef void (VKAPI_PTR *PFN_vkFreeCommandBuffers)(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);
DEVICE_R(
    vkBeginCommandBuffer
  , VALUE(VkCommandBuffer, commandBuffer)
  , READ(VkCommandBufferBeginInfo, pBeginInfo)
)
DEVICE_R(
    vkEndCommandBuffer
  , VALUE(VkCommandBuffer, commandBuffer)
)
// typedef VkResult (VKAPI_PTR *PFN_vkResetCommandBuffer)(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags);
// typedef void (VKAPI_PTR *PFN_vkCmdBindPipeline)(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
DEVICE_V(
    vkCmdSetViewport
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(uint32_t, firstViewport)
  , VALUE(uint32_t, viewportCount)
  , READ(VkViewport, pViewports, viewportCount)
)
DEVICE_V(
    vkCmdSetScissor
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(uint32_t, firstScissor)
  , VALUE(uint32_t, scissorCount)
  , READ(VkRect2D, pScissors)
)
DEVICE_V(
    vkCmdSetLineWidth
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(float, lineWidth)
)
DEVICE_V(
    vkCmdSetDepthBias
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(float, depthBiasConstantFactor)
  , VALUE(float, depthBiasClamp)
  , VALUE(float, depthBiasSlopeFactor)
)
DEVICE_V(
    vkCmdSetBlendConstants
  , VALUE(VkCommandBuffer, commandBuffer)
  , READ(float, blendConstants, 4)
)
DEVICE_V(
    vkCmdSetDepthBounds
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(float, minDepthBounds)
  , VALUE(float, maxDepthBounds)
)
DEVICE_V(
    vkCmdSetStencilCompareMask
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkStencilFaceFlags, faceMask)
  , VALUE(uint32_t, compareMask)
)
DEVICE_V(
    vkCmdSetStencilWriteMask
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkStencilFaceFlags, faceMask)
  , VALUE(uint32_t, writeMask))
DEVICE_V(
    vkCmdSetStencilReference
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkStencilFaceFlags, faceMask)
  , VALUE(uint32_t, reference)
)
DEVICE_V(
    vkCmdBindDescriptorSets
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkPipelineBindPoint, pipelineBindPoint)
  , VALUE(VkPipelineLayout, layout)
  , VALUE(uint32_t, firstSet)
  , VALUE(uint32_t, descriptorSetCount)
  , READ(VkDescriptorSet, pDescriptorSets, descriptorSetCount)
  , VALUE(uint32_t, dynamicOffsetCount)
  , READ(uint32_t, pDynamicOffsets, dynamicOffsetCount)
)
DEVICE_V(
    vkCmdBindIndexBuffer
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkBuffer, buffer)
  , VALUE(VkDeviceSize, offset)
  , VALUE(VkIndexType, indexType)
)
DEVICE_V(
    vkCmdBindVertexBuffers
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(uint32_t, firstBinding)
  , VALUE(uint32_t, bindingCount)
  , READ(VkBuffer, pBuffers, bindingCount)
  , READ(VkDeviceSize, pOffsets, bindingCount)
)
DEVICE_V(
    vkCmdDraw
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(uint32_t, vertexCount)
  , VALUE(uint32_t, instanceCount)
  , VALUE(uint32_t, firstVertex)
  , VALUE(uint32_t, firstInstance)
)
DEVICE_V(
    vkCmdDrawIndexed
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(uint32_t, indexCount)
  , VALUE(uint32_t, instanceCount)
  , VALUE(uint32_t, firstIndex)
  , VALUE(int32_t, vertexOffset)
  , VALUE(uint32_t, firstInstance)
)
DEVICE_V(
    vkCmdDrawIndirect
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkBuffer, buffer)
  , VALUE(VkDeviceSize, offset)
  , VALUE(uint32_t, drawCount)
  , VALUE(uint32_t, stride)
)
DEVICE_V(
    vkCmdDrawIndexedIndirect
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkBuffer, buffer)
  , VALUE(VkDeviceSize, offset)
  , VALUE(uint32_t, drawCount)
  , VALUE(uint32_t, stride)
)
DEVICE_V(
    vkCmdDispatch
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(uint32_t, groupCountX)
  , VALUE(uint32_t, groupCountY)
  , VALUE(uint32_t, groupCountZ)
)
DEVICE_V(
    vkCmdDispatchIndirect
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkBuffer, buffer)
  , VALUE(VkDeviceSize, offset)
)
DEVICE_V(
    vkCmdCopyBuffer
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkBuffer, srcBuffer)
  , VALUE(VkBuffer, dstBuffer)
  , VALUE(uint32_t, regionCount)
  , READ(VkBufferCopy, pRegions, regionCount)
)
DEVICE_V(
    vkCmdCopyImage
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkImage, srcImage)
  , VALUE(VkImageLayout, srcImageLayout)
  , VALUE(VkImage, dstImage)
  , VALUE(VkImageLayout, dstImageLayout)
  , VALUE(uint32_t, regionCount)
  , READ(VkImageCopy, pRegions, regionCount)
)
DEVICE_V(
    vkCmdBlitImage
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkImage, srcImage)
  , VALUE(VkImageLayout, srcImageLayout)
  , VALUE(VkImage, dstImage)
  , VALUE(VkImageLayout, dstImageLayout)
  , VALUE(uint32_t, regionCount)
  , READ(VkImageBlit, pRegions, regionCount)
  , VALUE(VkFilter, filter)
)
DEVICE_V(
    vkCmdCopyBufferToImage
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkBuffer, srcBuffer)
  , VALUE(VkImage, dstImage)
  , VALUE(VkImageLayout, dstImageLayout)
  , VALUE(uint32_t, regionCount)
  , READ(VkBufferImageCopy, pRegions, regionCount)
)
DEVICE_V(
    vkCmdCopyImageToBuffer
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkImage, srcImage)
  , VALUE(VkImageLayout, srcImageLayout)
  , VALUE(VkBuffer, dstBuffer)
  , VALUE(uint32_t, regionCount)
  , READ(VkBufferImageCopy, pRegions, regionCount)
)
DEVICE_V(
    vkCmdUpdateBuffer
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkBuffer, dstBuffer)
  , VALUE(VkDeviceSize, dstOffset)
  , VALUE(VkDeviceSize, dataSize)
  , READ(void, pData, dataSize)
)
DEVICE_V(
    vkCmdFillBuffer
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkBuffer, dstBuffer)
  , VALUE(VkDeviceSize, dstOffset)
  , VALUE(VkDeviceSize, size)
  , VALUE(uint32_t, data)
)
DEVICE_V(
    vkCmdClearColorImage
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkImage, image)
  , VALUE(VkImageLayout, imageLayout)
  , READ(VkClearColorValue, pColor)
  , VALUE(uint32_t, rangeCount)
  , READ(VkImageSubresourceRange, pRanges)
)
DEVICE_V(
    vkCmdClearDepthStencilImage
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkImage, image)
  , VALUE(VkImageLayout, imageLayout)
  , READ(VkClearDepthStencilValue, pDepthStencil)
  , VALUE(uint32_t, rangeCount)
  , READ(VkImageSubresourceRange, pRanges, rangeCount)
)
DEVICE_V(
    vkCmdClearAttachments
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(uint32_t, attachmentCount)
  , READ(VkClearAttachment, pAttachments, attachmentCount)
  , VALUE(uint32_t, rectCount)
  , READ(VkClearRect, pRects, rectCount)
)
DEVICE_V(
    vkCmdResolveImage
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkImage, srcImage)
  , VALUE(VkImageLayout, srcImageLayout)
  , VALUE(VkImage, dstImage)
  , VALUE(VkImageLayout, dstImageLayout)
  , VALUE(uint32_t, regionCount)
  , READ(VkImageResolve, pRegions, regionCount)
)
DEVICE_V(
    vkCmdSetEvent
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkEvent, event)
  , VALUE(VkPipelineStageFlags, stageMask)
)
DEVICE_V(
    vkCmdResetEvent
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkEvent, event)
  , VALUE(VkPipelineStageFlags, stageMask)
)
DEVICE_V(
    vkCmdWaitEvents
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(uint32_t, eventCount), READ(VkEvent, pEvents, eventCount)
  , VALUE(VkPipelineStageFlags, srcStageMask)
  , VALUE(VkPipelineStageFlags, dstStageMask)
  , VALUE(uint32_t, memoryBarrierCount)
  , READ(VkMemoryBarrier, pMemoryBarriers, memoryBarrierCount)
  , VALUE(uint32_t, bufferMemoryBarrierCount)
  , READ(VkBufferMemoryBarrier, pBufferMemoryBarriers, bufferMemoryBarrierCount)
  , VALUE(uint32_t, imageMemoryBarrierCount)
  , READ(VkImageMemoryBarrier, pImageMemoryBarriers, imageMemoryBarrierCount)
)
DEVICE_V(
    vkCmdPipelineBarrier
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkPipelineStageFlags, srcStageMask)
  , VALUE(VkPipelineStageFlags, dstStageMask)
  , VALUE(VkDependencyFlags, dependencyFlags)
  , VALUE(uint32_t, memoryBarrierCount)
  , READ(VkMemoryBarrier, pMemoryBarriers, memoryBarrierCount)
  , VALUE(uint32_t, bufferMemoryBarrierCount)
  , READ(VkBufferMemoryBarrier, pBufferMemoryBarriers, bufferMemoryBarrierCount)
  , VALUE(uint32_t, imageMemoryBarrierCount)
  , READ(VkImageMemoryBarrier, pImageMemoryBarriers, imageMemoryBarrierCount)
)
DEVICE_V(
    vkCmdBeginQuery
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkQueryPool, queryPool)
  , VALUE(uint32_t, query)
  , VALUE(VkQueryControlFlags, flags)
)
DEVICE_V(
    vkCmdEndQuery
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkQueryPool, queryPool)
  , VALUE(uint32_t, query)
)
DEVICE_V(
    vkCmdResetQueryPool
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkQueryPool, queryPool)
  , VALUE(uint32_t, firstQuery)
  , VALUE(uint32_t, queryCount)
)
DEVICE_V(
    vkCmdWriteTimestamp
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkPipelineStageFlagBits, pipelineStage)
  , VALUE(VkQueryPool, queryPool)
  , VALUE(uint32_t, query)
)
DEVICE_V(
    vkCmdCopyQueryPoolResults
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkQueryPool, queryPool)
  , VALUE(uint32_t, firstQuery)
  , VALUE(uint32_t, queryCount)
  , VALUE(VkBuffer, dstBuffer)
  , VALUE(VkDeviceSize, dstOffset)
  , VALUE(VkDeviceSize, stride)
  , VALUE(VkQueryResultFlags, flags)
)
DEVICE_V(
    vkCmdPushConstants
  , VALUE(VkCommandBuffer, commandBuffer)
  , VALUE(VkPipelineLayout, layout)
  , VALUE(VkShaderStageFlags, stageFlags)
  , VALUE(uint32_t, offset)
  , VALUE(uint32_t, size)
  , READ(void, pValues, size)
)
DEVICE_V(
    vkCmdBeginRenderPass
  , VALUE(VkCommandBuffer, commandBuffer)
  , READ(VkRenderPassBeginInfo, pRenderPassBegin, 1)
  , VALUE(VkSubpassContents, contents)
)
// typedef void (VKAPI_PTR *PFN_vkCmdNextSubpass)(VkCommandBuffer commandBuffer, VkSubpassContents contents);
DEVICE_V(
    vkCmdEndRenderPass
  , VALUE(VkCommandBuffer, commandBuffer)
)
// typedef void (VKAPI_PTR *PFN_vkCmdExecuteCommands)(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer* pCommandBuffers);

// VK_KHR_surface
// typedef void (VKAPI_PTR *PFN_vkDestroySurfaceKHR)(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator);
// typedef VkResult (VKAPI_PTR *PFN_vkGetPhysicalDeviceSurfaceSupportKHR)(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, VkSurfaceKHR surface, VkBool32* pSupported);
INSTANCE_R(
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR
  , PHYSICAL_DEVICE
  , VALUE(VkSurfaceKHR, surface)
  , WRITE(VkSurfaceCapabilitiesKHR, pSurfaceCapabilities, 1)
)
INSTANCE_R(
    vkGetPhysicalDeviceSurfaceFormatsKHR
  , PHYSICAL_DEVICE
  , VALUE(VkSurfaceKHR, surface)
  , WRITE(uint32_t, pSurfaceFormatCount, 1)
  , WRITE(VkSurfaceFormatKHR, pSurfaceFormats, pSurfaceFormats == NULL ? 0 : *pSurfaceFormatCount)
)
INSTANCE_R(
    vkGetPhysicalDeviceSurfacePresentModesKHR
  , PHYSICAL_DEVICE
  , VALUE(VkSurfaceKHR, surface)
  , WRITE(uint32_t, pPresentModeCount, 1)
  , WRITE(VkPresentModeKHR, pPresentModes, pPresentModes == NULL ? 0 : *pPresentModeCount)
)

// VK_KHR_swapchain
DEVICE_R(
    vkCreateSwapchainKHR
  , DEVICE
  , READ(VkSwapchainCreateInfoKHR, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkSwapchainKHR, pSwapchain, 1)
)
DEVICE_V(
    vkDestroySwapchainKHR
  , DEVICE
  , VALUE(VkSwapchainKHR, swapchain)
  , ALLOCATOR
)
DEVICE_R(
    vkGetSwapchainImagesKHR
  , DEVICE
  , VALUE(VkSwapchainKHR, swapchain)
  , WRITE(uint32_t, pSwapchainImageCount, 1)
  , WRITE(VkImage, pSwapchainImages, pSwapchainImages == NULL ? 0 : *pSwapchainImageCount)
)
DEVICE_R(
    vkAcquireNextImageKHR
  , DEVICE
  , VALUE(VkSwapchainKHR, swapchain)
  , VALUE(uint64_t, timeout)
  , VALUE(VkSemaphore, semaphore)
  , VALUE(VkFence, fence)
  , WRITE(uint32_t, pImageIndex)
)
DEVICE_R(
    vkQueuePresentKHR
  , VALUE(VkQueue, queue)
  , READ(VkPresentInfoKHR, pPresentInfo, 1)
)
// typedef VkResult (VKAPI_PTR *PFN_vkGetDeviceGroupPresentCapabilitiesKHR)(VkDevice device, VkDeviceGroupPresentCapabilitiesKHR* pDeviceGroupPresentCapabilities);
// typedef VkResult (VKAPI_PTR *PFN_vkGetDeviceGroupSurfacePresentModesKHR)(VkDevice device, VkSurfaceKHR surface, VkDeviceGroupPresentModeFlagsKHR* pModes);
// typedef VkResult (VKAPI_PTR *PFN_vkGetPhysicalDevicePresentRectanglesKHR)(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t* pRectCount, VkRect2D* pRects);
// typedef VkResult (VKAPI_PTR *PFN_vkAcquireNextImage2KHR)(VkDevice device, const VkAcquireNextImageInfoKHR* pAcquireInfo, uint32_t* pImageIndex);

// VK_EXT_debug_report
INSTANCE_R(
    vkCreateDebugReportCallbackEXT
  , INSTANCE
  , READ(VkDebugReportCallbackCreateInfoEXT, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkDebugReportCallbackEXT, pCallback)
)
// typedef void (VKAPI_PTR *PFN_vkDestroyDebugReportCallbackEXT)(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);
// typedef void (VKAPI_PTR *PFN_vkDebugReportMessageEXT)(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage);

// VK_EXT_debug_marker
// typedef VkResult (VKAPI_PTR *PFN_vkDebugMarkerSetObjectTagEXT)(VkDevice device, const VkDebugMarkerObjectTagInfoEXT* pTagInfo);
// typedef VkResult (VKAPI_PTR *PFN_vkDebugMarkerSetObjectNameEXT)(VkDevice device, const VkDebugMarkerObjectNameInfoEXT* pNameInfo);
// typedef void (VKAPI_PTR *PFN_vkCmdDebugMarkerBeginEXT)(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo);
// typedef void (VKAPI_PTR *PFN_vkCmdDebugMarkerEndEXT)(VkCommandBuffer commandBuffer);
// typedef void (VKAPI_PTR *PFN_vkCmdDebugMarkerInsertEXT)(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT* pMarkerInfo);

// VK_EXT_debug_utils
// typedef VkResult (VKAPI_PTR *PFN_vkSetDebugUtilsObjectNameEXT)(VkDevice device, const VkDebugUtilsObjectNameInfoEXT* pNameInfo);
// typedef VkResult (VKAPI_PTR *PFN_vkSetDebugUtilsObjectTagEXT)(VkDevice device, const VkDebugUtilsObjectTagInfoEXT* pTagInfo);
// typedef void (VKAPI_PTR *PFN_vkQueueBeginDebugUtilsLabelEXT)(VkQueue queue, const VkDebugUtilsLabelEXT* pLabelInfo);
// typedef void (VKAPI_PTR *PFN_vkQueueEndDebugUtilsLabelEXT)(VkQueue queue);
// typedef void (VKAPI_PTR *PFN_vkQueueInsertDebugUtilsLabelEXT)(VkQueue queue, const VkDebugUtilsLabelEXT* pLabelInfo);
// typedef void (VKAPI_PTR *PFN_vkCmdBeginDebugUtilsLabelEXT)(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo);
// typedef void (VKAPI_PTR *PFN_vkCmdEndDebugUtilsLabelEXT)(VkCommandBuffer commandBuffer);
// typedef void (VKAPI_PTR *PFN_vkCmdInsertDebugUtilsLabelEXT)(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT* pLabelInfo);
INSTANCE_R(
    vkCreateDebugUtilsMessengerEXT
  , INSTANCE
  , READ(VkDebugUtilsMessengerCreateInfoEXT, pCreateInfo, 1)
  , ALLOCATOR
  , WRITE(VkDebugUtilsMessengerEXT, pMessenger, 1)
)
// typedef void (VKAPI_PTR *PFN_vkDestroyDebugUtilsMessengerEXT)(VkInstance instance, VkDebugUtilsMessengerEXT messenger, const VkAllocationCallbacks* pAllocator);
// typedef void (VKAPI_PTR *PFN_vkSubmitDebugUtilsMessageEXT)(VkInstance instance, VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData);


// ====================================================================================================================
static bool report_vulkan_error(const char * name, VkResult result) {
  if(result >= VK_SUCCESS) {
    return true;
  }

  ALIAS_ERROR("Vulkan error in %s - %s", name,
    VK_ERROR_OUT_OF_HOST_MEMORY                  == result ? "OUT_OF_HOST_MEMORY"                  :
    VK_ERROR_OUT_OF_DEVICE_MEMORY                == result ? "OUT_OF_DEVICE_MEMORY"                :
    VK_ERROR_INITIALIZATION_FAILED               == result ? "INITIALIZATION_FAILED"               :
    VK_ERROR_DEVICE_LOST                         == result ? "DEVICE_LOST"                         :
    VK_ERROR_MEMORY_MAP_FAILED                   == result ? "MEMORY_MAP_FAILED"                   :
    VK_ERROR_LAYER_NOT_PRESENT                   == result ? "LAYER_NOT_PRESENT"                   :
    VK_ERROR_EXTENSION_NOT_PRESENT               == result ? "EXTENSION_NOT_PRESENT"               :
    VK_ERROR_FEATURE_NOT_PRESENT                 == result ? "FEATURE_NOT_PRESENT"                 :
    VK_ERROR_INCOMPATIBLE_DRIVER                 == result ? "INCOMPATIBLE_DRIVER"                 :
    VK_ERROR_TOO_MANY_OBJECTS                    == result ? "TOO_MANY_OBJECTS"                    :
    VK_ERROR_FORMAT_NOT_SUPPORTED                == result ? "FORMAT_NOT_SUPPORTED"                :
    VK_ERROR_FRAGMENTED_POOL                     == result ? "FRAGMENTED_POOL"                     :
    VK_ERROR_OUT_OF_POOL_MEMORY                  == result ? "OUT_OF_POOL_MEMORY"                  :
    VK_ERROR_INVALID_EXTERNAL_HANDLE             == result ? "INVALID_EXTERNAL_HANDLE"             :
    VK_ERROR_FRAGMENTATION                       == result ? "FRAGMENTATION"                       :
    VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS      == result ? "INVALID_OPAQUE_CAPTURE_ADDRESS"      :
    VK_ERROR_SURFACE_LOST_KHR                    == result ? "SURFACE_LOST_KHR"                    :
    VK_ERROR_NATIVE_WINDOW_IN_USE_KHR            == result ? "NATIVE_WINDOW_IN_USE_KHR"            :
    VK_ERROR_OUT_OF_DATE_KHR                     == result ? "OUT_OF_DATE_KHR"                     :
    VK_ERROR_INCOMPATIBLE_DISPLAY_KHR            == result ? "INCOMPATIBLE_DISPLAY_KHR"            :
    VK_ERROR_VALIDATION_FAILED_EXT               == result ? "VALIDATION_FAILED_EXT"               :
    VK_ERROR_INVALID_SHADER_NV                   == result ? "INVALID_SHADER_NV"                   :
    VK_ERROR_NOT_PERMITTED_EXT                   == result ? "NOT_PERMITTED_EXT"                   :
    VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT == result ? "FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" :
                                                             "UNKNOWN"
  );

  raise(SIGABRT);

  return false;
}

// ====================================================================================================================
#define VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

static bool create_instance(void) {
  uint32_t count;

  uint32_t enabled_extensions_count = 0;
  const char * enabled_extensions[16];

  uint32_t enabled_layers_count = 0;
  const char * enabled_layers[1];

  pfn_vkGetInstanceProcAddr = glfwGetInstanceProcAddress;

  {
    const char ** extensions = glfwGetRequiredInstanceExtensions(&count);
    for(uint32_t i = 0; i < count; i++) {
      ALIAS_INFO("enabeling Vulkan instance extension %s", extensions[i]);
      enabled_extensions[enabled_extensions_count++] = extensions[i];
    }
  }

  if(_.validation) {
    if(!report_vulkan_error("vkEnumerateInstanceLayerProperties", vkEnumerateInstanceLayerProperties(&count, NULL))) {
      return false;
    }
    VkLayerProperties * layer_properties = malloc(sizeof(*layer_properties) * count);
    if(!report_vulkan_error("vkEnumerateInstanceLayerProperties", vkEnumerateInstanceLayerProperties(&count, layer_properties))) {
      return false;
    }

    _.validation = 0;

    for(uint32_t i = 0; i < count; i++) {
      if(strcmp(VALIDATION_LAYER_NAME, layer_properties[i].layerName) == 0) {
        enabled_layers[enabled_layers_count++] = VALIDATION_LAYER_NAME;
        _.validation = 1;
        break;
      }
    }

    free(layer_properties);

    if(_.validation) {
      ALIAS_INFO("Vulkan validation requested. layer enabled");

      if(!report_vulkan_error("vkEnumerateInstanceExtensionProperties", vkEnumerateInstanceExtensionProperties(VALIDATION_LAYER_NAME, &count, NULL))) {
        return false;
      }
      VkExtensionProperties * extension_properties = malloc(sizeof(*extension_properties) * count);
      if(!report_vulkan_error("vkEnumerateInstanceExtensionProperties", vkEnumerateInstanceExtensionProperties(VALIDATION_LAYER_NAME, &count, extension_properties))) {
        return false;
      }

      _.debug_utils = false;
      _.debug_report = false;
      _.debug_marker = false;

      for(uint32_t i = 0; i < count; i++) {
        if(strcmp(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, extension_properties[i].extensionName) == 0) {
          ALIAS_INFO("enabeling Vulkan instance extension " VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
          enabled_extensions[enabled_extensions_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
          _.debug_utils = true;
          break;
        }
      }

      if(!_.debug_utils) {
        for(uint32_t i = 0; i < count; i++) {
          if(strcmp(VK_EXT_DEBUG_REPORT_EXTENSION_NAME, extension_properties[i].extensionName) == 0) {
            ALIAS_INFO("enabeling Vulkan instance extension " VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            enabled_extensions[enabled_extensions_count++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
            _.debug_report = true;
          }

          if(strcmp(VK_EXT_DEBUG_MARKER_EXTENSION_NAME, extension_properties[i].extensionName) == 0) {
            ALIAS_INFO("enabeling Vulkan instance extension " VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
            enabled_extensions[enabled_extensions_count++] = VK_EXT_DEBUG_MARKER_EXTENSION_NAME;
            _.debug_marker = true;
          }
        }
      }
    } else {
      ALIAS_INFO("Vulkan validation requested. not found and disabled");
    }
  }

  if(!report_vulkan_error("vkCreateInstance", vkCreateInstance(&(VkInstanceCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO
    , .pApplicationInfo = &(VkApplicationInfo) {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO
      , .pApplicationName = "Alias Engine"
      , .applicationVersion = VK_MAKE_VERSION(1, 0, 0)
      , .pEngineName = "Alias Engine"
      , .engineVersion = VK_MAKE_VERSION(1, 0, 0)
      , .apiVersion = VK_API_VERSION_1_0
      }
    , .enabledExtensionCount = enabled_extensions_count
    , .ppEnabledExtensionNames = enabled_extensions
    , .enabledLayerCount = enabled_layers_count
    , .ppEnabledLayerNames = enabled_layers
  }, &_.instance))) {
    ALIAS_ERROR("failed to create Vulkan instance");
    return false;
  }
  ALIAS_TRACE("created Vulkan instance");

  return true;
}

// ====================================================================================================================
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT     message_severity
  , VkDebugUtilsMessageTypeFlagsEXT            message_type
  , const VkDebugUtilsMessengerCallbackDataEXT * callback_data
  , void                                       * user_data
) {
  ALIAS_ERROR("Vulkan debug called with message %s", callback_data->pMessage);
  raise(SIGABRT);
  return VK_TRUE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report_callback(
    VkDebugReportFlagsEXT                       flags,
    VkDebugReportObjectTypeEXT                  objectType,
    uint64_t                                    object,
    size_t                                      location,
    int32_t                                     messageCode,
    const char*                                 pLayerPrefix,
    const char*                                 pMessage,
    void*                                       pUserData
) {
  ALIAS_ERROR("Vulkan debug called with message %s", pMessage);
  raise(SIGABRT);
  return VK_TRUE;
}

static bool create_debug_report_callback(void) {
  if(!_.validation) {
    return true;
  }
  if(_.debug_utils) {
    if(!report_vulkan_error("vkCreateDebugUtilsMessengerEXT", vkCreateDebugUtilsMessengerEXT(
        &(VkDebugUtilsMessengerCreateInfoEXT) {
          .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT
        , .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
        , .messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
        , .pfnUserCallback = debug_utils_callback
        }
      , &_.debug_utils_callback
      ))) {
      ALIAS_ERROR("failed to create Vulkan debug utils messenger");
      return false;
    }
    ALIAS_TRACE("created Vulkan debug utils messenger");
    return true;
  } else if(_.debug_report) {
    if(!report_vulkan_error("vkCreateDebugReportCallbackEXT", vkCreateDebugReportCallbackEXT(
        &(VkDebugReportCallbackCreateInfoEXT) {
          .sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT
        , .flags       = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT
        , .pfnCallback = debug_report_callback
        }
      , &_.debug_report_callback
      ))) {
      ALIAS_ERROR("failed to create Vulkan debug utils messenger");
      return false;
    }
    ALIAS_TRACE("created Vulkan debug utils messenger");
    return true;
  }
}

// ====================================================================================================================
static bool select_physical_device(void) {
  VkResult result;

  uint32_t count;
  vkEnumeratePhysicalDevices(&count, NULL);

  VkPhysicalDevice * physical_devices = alias_malloc(alias_default_MemoryCB(), sizeof(*physical_devices) * count, alignof(*physical_devices));
  vkEnumeratePhysicalDevices(&count, physical_devices);

  _.physical_device = physical_devices[0];

  alias_free(alias_default_MemoryCB(), physical_devices, sizeof(*physical_devices) * count, alignof(*physical_devices));

  vkGetPhysicalDeviceProperties(&_.physical_device_properties);

  vkGetPhysicalDeviceFeatures(&_.physical_device_features);

  vkGetPhysicalDeviceMemoryProperties(&_.physical_device_memory_properties);

  vkGetPhysicalDeviceQueueFamilyProperties(&count, NULL);
  alias_Vector_space_for(&_.physical_device_queue_family_properties, alias_default_MemoryCB(), count);
  _.physical_device_queue_family_properties.length = count;
  vkGetPhysicalDeviceQueueFamilyProperties(&count, _.physical_device_queue_family_properties.data);

  vkEnumerateDeviceExtensionProperties(NULL, &count, NULL);
  alias_Vector_space_for(&_.physical_device_extensions, alias_default_MemoryCB(), count);
  _.physical_device_extensions.length = count;
  vkEnumerateDeviceExtensionProperties(NULL, &count, _.physical_device_extensions.data);

  static const VkFormat formats[] = {
      VK_FORMAT_D32_SFLOAT_S8_UINT
    , VK_FORMAT_D32_SFLOAT
    , VK_FORMAT_D24_UNORM_S8_UINT
    , VK_FORMAT_D16_UNORM_S8_UINT
    , VK_FORMAT_D16_UNORM
    };
  for(uint32_t i = 0; i < sizeof(formats)/sizeof(formats[0]); i++) {
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(formats[i], &format_properties);
    if(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      ALIAS_INFO("chose depth-stencil format %i", formats[i]);
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

  _.queue_family_index[Graphics] = select_queue_family_index(VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_COMPUTE_BIT);
  queue_create_infos[queue_create_infos_count++] = (VkDeviceQueueCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
    , .queueFamilyIndex = _.queue_family_index[Graphics]
    , .queueCount = 1
    , .pQueuePriorities = &zero
    };
  
  _.queue_family_index[Transfer] = select_queue_family_index(VK_QUEUE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT);
  if(_.queue_family_index[Transfer] != _.queue_family_index[Graphics]) {
    queue_create_infos[queue_create_infos_count++] = (VkDeviceQueueCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
      , .queueFamilyIndex = _.queue_family_index[Transfer]
      , .queueCount = 1
      , .pQueuePriorities = &zero
      };
  }

  _.queue_family_index[Present] = select_queue_family_index(VK_QUEUE_PRESENT_BIT, 0);
  if(_.queue_family_index[Present] != _.queue_family_index[Graphics] && _.queue_family_index[Present] != _.queue_family_index[Transfer]) {
    queue_create_infos[queue_create_infos_count++] = (VkDeviceQueueCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO
      , .queueFamilyIndex = _.queue_family_index[Present]
      , .queueCount = 1
      , .pQueuePriorities = &zero
      };
  }

  uint32_t enabled_extensions_count = 0;
  const char * enabled_extensions[1];

  enabled_extensions[enabled_extensions_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;

  uint32_t enabled_layers_count = 0;
  const char * enabled_layers[1];

  if(!report_vulkan_error("vkCreateDevice", vkCreateDevice(
      &(VkDeviceCreateInfo) {
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
    , &_.device
  ))) {
    ALIAS_ERROR("failed to created Vulkan device");
    return false;
  }

  ALIAS_TRACE("created Vulkan device");

  vkGetDeviceQueue(_.queue_family_index[Graphics], 0, &_.queue[Graphics]);
  vkGetDeviceQueue(_.queue_family_index[Transfer], 0, &_.queue[Transfer]);
  vkGetDeviceQueue(_.queue_family_index[Present], 0, &_.queue[Present]);

  return true;
}

// ====================================================================================================================
static bool create_surface(void) {
  if(VK_SUCCESS != glfwCreateWindowSurface(_.instance, _.window, NULL, &_.surface)) {
    ALIAS_ERROR("failed to create Vulkan surface");
    return false;
  }

  ALIAS_TRACE("created Vulkan surface");

  uint32_t count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(_.surface, &count, NULL);
  VkSurfaceFormatKHR * formats = alias_malloc(alias_default_MemoryCB(), sizeof(*formats) * count, alignof(*formats));
  vkGetPhysicalDeviceSurfaceFormatsKHR(_.surface, &count, formats);

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
  return report_vulkan_error("vkCreateCommandPool", vkCreateCommandPool(
      &(VkCommandPoolCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO
      , .queueFamilyIndex = queue_family_index
      , .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
      }
    , command_pool
    ));
}

// ====================================================================================================================
static bool create_frame_sync_primitives(void) {
  return report_vulkan_error("vkCreateSemaphore", vkCreateSemaphore(
      &(VkSemaphoreCreateInfo) { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }
    , &_.frame_gpu_transfer_to_gpu_graphics
    )) && report_vulkan_error("vkCreateSemaphore", vkCreateSemaphore(
      &(VkSemaphoreCreateInfo) { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }
    , &_.frame_gpu_graphics_to_gpu_present
    )) && report_vulkan_error("vkCreateSemaphore", vkCreateSemaphore(
      &(VkSemaphoreCreateInfo) { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }
    , &_.frame_gpu_present_to_gpu_graphics
    ));
}

// ====================================================================================================================
static bool create_render_pass(void) {
  return VK_SUCCESS == vkCreateRenderPass(
      &(VkRenderPassCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO
      , .attachmentCount = 2
      , .pAttachments = (VkAttachmentDescription[]) {
          {   .flags = 0
            , .format = _.surface_color_format
            , .samples = VK_SAMPLE_COUNT_1_BIT
            , .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR
            , .storeOp = VK_ATTACHMENT_STORE_OP_STORE
            , .stencilLoadOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
            , .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
            , .initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            , .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            }
        , {   .flags = 0
            , .format = _.depthstencil_format
            , .samples = VK_SAMPLE_COUNT_1_BIT
            , .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR
            , .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
            , .stencilLoadOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
            , .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE
            , .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            , .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
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
            .attachment = 1
          , .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
          }
        , .preserveAttachmentCount = 0
        , .pPreserveAttachments = NULL
        }
      , .dependencyCount = 0
      , .pDependencies = NULL
      }
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
        &(VkPipelineCacheCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
        }
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
      &(VkPipelineCacheCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
      , .initialDataSize = buf.len
      , .pInitialData = buf.base
      }
    , &_.pipelinecache
    );

  alias_free(alias_default_MemoryCB(), buf.base, buf.len, 4);

  return VK_SUCCESS == result;
}

// ====================================================================================================================
static bool destroy_pipeline_cache(void) {
  // TODO save
  vkDestroyPipelineCache(_.pipelinecache);
}

// ====================================================================================================================
static bool create_device_memory(
    VkMemoryRequirements requirements
  , VkMemoryPropertyFlagBits properties
  , VkDeviceMemory *result
) {
  uint32_t index;

  for(index = 0; index < _.physical_device_memory_properties.memoryTypeCount; index++) {
    if((requirements.memoryTypeBits & (1 << index)) && (_.physical_device_memory_properties.memoryTypes[index].propertyFlags & properties) == properties) {
      break;
    }
  }

  if(index == _.physical_device_memory_properties.memoryTypeCount) {
    ALIAS_ERROR("memory type not found for properties %i", properties);
    return false;
  }

  return report_vulkan_error("vkAllocateMemory", vkAllocateMemory(
      &(VkMemoryAllocateInfo) {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO
      , .allocationSize = requirements.size
      , .memoryTypeIndex = index
      }
    , result
    ));
}

// ====================================================================================================================
static bool create_staging_buffer(void) {
  _.staging_buffer_size = 64 << 20; // 64 MB
  _.staging_buffer_unused[1] = _.staging_buffer_size;
  
  if(!report_vulkan_error("vkCreateBuffer", vkCreateBuffer(
      &(VkBufferCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO
      , .size = _.staging_buffer_size
      , .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
      }
    , &_.staging_buffer
  ))) {
    return false;
  }

  VkMemoryRequirements memory_requirements;
  vkGetBufferMemoryRequirements(_.staging_buffer, &memory_requirements);

  if(!create_device_memory(memory_requirements, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &_.staging_buffer_memory)) {
    return false;
  }
  
  if(!report_vulkan_error("vkMapMemory", vkMapMemory(_.staging_buffer_memory, 0, _.staging_buffer_size, 0, &_.staging_buffer_map))) {
    return false;
  }

  return report_vulkan_error("vkBindBufferMemory", vkBindBufferMemory(_.staging_buffer, _.staging_buffer_memory, 0));
}

// ====================================================================================================================
static void flush_transfer_command_buffer(void) {
  #define buffers _.command_buffers[Transfer]

  for(uint32_t index = 0; index < buffers.length; index++) {
    if(buffers.data[index].ready) {
      continue;
    }

    VkDeviceSize start = buffers.data[index].stage_start;
    VkDeviceSize end = buffers.data[index].stage_end;
    if(start <= end) {
      continue;
    }
    
    uint32_t unused[4];
    alias_memory_copy(unused, sizeof(unused), _.staging_buffer_unused, sizeof(_.staging_buffer_unused));
    
    if(end == unused[0]) {
      unused[0] = start;
    } else if(start == unused[1]) {
      unused[1] = end;
    } else if(end == unused[2]) {
      unused[2] = start;
    } else if(start == unused[3]) {
      unused[3] = end;
    } else {
      continue;
    }

    if(unused[3] == unused[0]) {
      unused[0] = unused[2];
      unused[2] = 0;
      unused[3] = 0;
    }

    VkResult result = vkGetFenceStatus(buffers.data[index].fence);
    if(result == VK_SUCCESS) {
      alias_memory_copy(_.staging_buffer_unused, sizeof(_.staging_buffer_unused), unused, sizeof(unused));

      buffers.data[index].ready = true;
      buffers.data[index].stage_start = 0;
      buffers.data[index].stage_end = 0;

      if(_.frame_pending[Transfer] > _.frame_complete[Transfer]) {
        _.frame_complete[Transfer] = _.frame_complete[Transfer];
      }
      _.in_flight[Transfer]--;
      
      break;
    }
  }

  #undef buffers
}

// ====================================================================================================================
static uint32_t acquire_command_buffer(enum QueueIndex queue) {
  uint32_t index;

  if(queue == Transfer && _.in_flight[queue] == _.command_buffers[queue].length) {
    // make sure the transfer queue has at least one free command buffer
    flush_transfer_command_buffer();
  }

  for(index = 0; index < _.command_buffers[queue].length; index++) {
    VkDeviceSize start = _.command_buffers[queue].data[index].stage_start;
    VkDeviceSize end = _.command_buffers[queue].data[index].stage_end;

    // only wait for fences on transfer with flush_transfer_command_buffer
    VkResult result = _.command_buffers[queue].data[index].ready ? VK_SUCCESS : (start == end ? vkGetFenceStatus(_.command_buffers[queue].data[index].fence) : VK_NOT_READY);
    if(result == VK_SUCCESS) {
      if(_.frame_pending[queue] > _.frame_complete[queue]) {
        _.frame_complete[queue] = _.frame_complete[queue];
      }
      _.in_flight[queue] -= !_.command_buffers[queue].data[index].ready;

      goto ready_command_buffer;
    }
  }

  index = _.command_buffers[queue].length;
  alias_Vector_space_for(&_.command_buffers[queue], alias_default_MemoryCB(), 1);
  struct CommandBuffer * cbuf = alias_Vector_push(&_.command_buffers[queue]);

  if(!report_vulkan_error("vkAllocateCommandBuffers", vkAllocateCommandBuffers(
      &(VkCommandBufferAllocateInfo) {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO
      , .commandPool        = _.command_pool[queue]
      , .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY
      , .commandBufferCount = 1
      }
    , &cbuf->buffer
  ))) {
    return -1;
  }

  if(!report_vulkan_error("vkCreateFence", vkCreateFence(
      &(VkFenceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
      , .flags = VK_FENCE_CREATE_SIGNALED_BIT
      }
    , &cbuf->fence
  ))) {
    return -1;
  }

ready_command_buffer:
  vkResetFences(1, &_.command_buffers[queue].data[index].fence);

  _.command_buffers[queue].data[index].frame = 0;
  _.command_buffers[queue].data[index].ready = 0;
  
  if(!report_vulkan_error("vkBeginCommandBuffer", vkBeginCommandBuffer(
      _.command_buffers[queue].data[index].buffer
    , &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
      , .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    }
  ))) {
    return -1;
  }

  return index;
}

// ====================================================================================================================
static bool transition_image_from_undefined_to_present(VkImage image) {
  enum QueueIndex queue = Graphics;
  if(_.transition_cbuf[queue] == -1) {
    _.transition_cbuf[queue] = acquire_command_buffer(queue);
  }
  vkCmdPipelineBarrier(
      _.command_buffers[queue].data[_.transition_cbuf[queue]].buffer
    , VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
    , VK_PIPELINE_STAGE_TRANSFER_BIT
    , 0
    , 0, NULL
    , 0, NULL
    , 1, &(VkImageMemoryBarrier){
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
      , .srcAccessMask       = 0
      , .dstAccessMask       = VK_ACCESS_TRANSFER_READ_BIT
      , .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED
      , .newLayout           = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
      , .srcQueueFamilyIndex = _.queue_family_index[queue]
      , .dstQueueFamilyIndex = _.queue_family_index[queue]
      , .image               = image
      , .subresourceRange    = {
          .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT
        , .baseMipLevel   = 0
        , .levelCount     = 1
        , .baseArrayLayer = 0
        , .layerCount     = 1
        }
      }
    );
  return true;
}

// ====================================================================================================================
static bool create_swapchain(uint32_t * width, uint32_t * height, bool vsync) {
  VkSwapchainKHR old = _.swapchain;

  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_.surface, &surface_capabilities);

  uint32_t present_modes_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(_.surface, &present_modes_count, NULL);
  VkPresentModeKHR * present_modes = alias_malloc(alias_default_MemoryCB(), sizeof(*present_modes) * present_modes_count, alignof(*present_modes));
  vkGetPhysicalDeviceSurfacePresentModesKHR(_.surface, &present_modes_count, present_modes);

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

  uint32_t num_images = surface_capabilities.maxImageCount == 0 ? surface_capabilities.minImageCount + 1 : surface_capabilities.maxImageCount;

  ALIAS_TRACE("requesting %i images from bounds of %i:%i", num_images, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);

  VkSurfaceTransformFlagsKHR pretransform = surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
                                          ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR : surface_capabilities.currentTransform;

  VkCompositeAlphaFlagBitsKHR composite_alpha = surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR          ? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR          :
                                                surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR  ? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR  :
                                                surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR ? VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR :
                                                                                                                                            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR         ;

  VkResult result = vkCreateSwapchainKHR(
      &(VkSwapchainCreateInfoKHR) {
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
    , &_.swapchain
    );
  if(result < VK_SUCCESS) {
    ALIAS_ERROR("failed to create Vulkan swapchain");
  } else {
    ALIAS_TRACE("created Vulkan swapchain");
  }

  if(old != VK_NULL_HANDLE) {
    for(uint32_t i = 0; i < _.swapchain_image_views.length; i++) {
      vkDestroyImageView(_.swapchain_image_views.data[i]);
    }
    vkDestroySwapchainKHR(old);
  }

  if(!report_vulkan_error("vkCreateSwapchainKHR", result)) {
    return false;
  }

  uint32_t count;
  vkGetSwapchainImagesKHR(_.swapchain, &count, NULL);
  _.swapchain_images.length = count;
  _.swapchain_image_views.length = count;
  alias_Vector_space_for(&_.swapchain_images, alias_default_MemoryCB(), 0);
  alias_Vector_space_for(&_.swapchain_image_views, alias_default_MemoryCB(), 0);
  vkGetSwapchainImagesKHR(_.swapchain, &count, _.swapchain_images.data);

  ALIAS_TRACE("preparing %i swapchain images ...", count);

  for(uint32_t i = 0; i < count; i++) {
    if(!transition_image_from_undefined_to_present(_.swapchain_images.data[i])) {
      while(i--) {
        vkDestroyImageView(_.swapchain_image_views.data[i]);
      }
      vkDestroySwapchainKHR(_.swapchain);
      return false;
    }
    
    result = vkCreateImageView(
        &(VkImageViewCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
        , .image = _.swapchain_images.data[i]
        , .viewType = VK_IMAGE_VIEW_TYPE_2D
        , .format = _.surface_color_format
        , .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }
        , .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
          , .baseMipLevel = 0
          , .levelCount = 1
          , .baseArrayLayer = 0
          , .layerCount = 1
          }
        }
      , &_.swapchain_image_views.data[i]
      );

    if(!report_vulkan_error("vkCreateImageView", result)) {
      while(i--) {
        vkDestroyImageView(_.swapchain_image_views.data[i]);
      }
      vkDestroySwapchainKHR(_.swapchain);
      return false;
    }
  }

  ALIAS_TRACE("... success!");

  return true;
}

// ====================================================================================================================
static bool create_depthbuffer(void) {
  VkResult result;

  result = vkCreateImage(
      &(VkImageCreateInfo) {
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
    , &_.depthbuffer
    );

  if(!report_vulkan_error("vkCreateImage", result)) {
    return false;
  }

  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements(_.depthbuffer, &memory_requirements);

  if(!create_device_memory(memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_.depthbuffer_memory)) {
    ALIAS_ERROR("could not allocate Vulkan memory for depth buffer");
    vkDestroyImage(_.depthbuffer);
    return false;
  }

  if(!report_vulkan_error("vkBindImageMemory", vkBindImageMemory(_.depthbuffer, _.depthbuffer_memory, 0))) {
    vkDestroyImage(_.depthbuffer);
    // TODO free_device_memory(_.depthbuffer_memory);
    vkFreeMemory(_.depthbuffer_memory);
    return false;
  }

  if(!report_vulkan_error("vkCreateImageView", vkCreateImageView(
      &(VkImageViewCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
      , .image = _.depthbuffer
      , .viewType = VK_IMAGE_VIEW_TYPE_2D
      , .format = _.depthstencil_format
      , .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }
      , .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT
        , .baseMipLevel = 0
        , .levelCount = 1
        , .baseArrayLayer = 0
        , .layerCount = 1
        }
      }
    , &_.depthbuffer_view
  ))) {
    vkDestroyImage(_.depthbuffer);
    vkFreeMemory(_.depthbuffer_memory);
    return false;
  }

  return true;
}

// ====================================================================================================================
static void destroy_depthbuffer(void) {
  vkDestroyImageView(_.depthbuffer_view);
  vkDestroyImage(_.depthbuffer);
  vkFreeMemory(_.depthbuffer_memory);
}

// ====================================================================================================================
static void destroy_framebuffers(void) {
  while(_.framebuffers.length) {
    vkDestroyFramebuffer(*alias_Vector_pop(&_.framebuffers));
  }
}

// ====================================================================================================================
static bool create_framebuffers(void) {
  destroy_framebuffers();
  
  alias_Vector_set_capacity(&_.framebuffers, alias_default_MemoryCB(), _.swapchain_image_views.length);

  ALIAS_TRACE("creating %i framebuffer(s) ...", _.swapchain_image_views.length);

  for(uint32_t i = 0; i < _.swapchain_image_views.length; i++) {
    if(!report_vulkan_error("vkCreateFramebuffer", vkCreateFramebuffer(
        &(VkFramebufferCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO
        , .renderPass = _.render_pass
        , .attachmentCount = 2
        , .pAttachments = (VkImageView[]) { _.swapchain_image_views.data[i], _.depthbuffer_view }
        , .width = _.swapchain_extents.width
        , .height = _.swapchain_extents.height
        , .layers = 1
        }
      , alias_Vector_push(&_.framebuffers)
    ))) {
      alias_Vector_pop(&_.framebuffers);
      destroy_framebuffers();
      return false;
    }
  }

  ALIAS_TRACE("... success!");

  return true;
}


// ====================================================================================================================
static bool create_samplers(void) {
  struct {
    VkFilter min;
    VkFilter max;
    VkSamplerMipmapMode mipmap;
  } data[NUM_BACKEND_SAMPLERS] = {
      [BackendSampler_NEAREST] = { VK_FILTER_NEAREST, VK_FILTER_NEAREST, VK_SAMPLER_MIPMAP_MODE_NEAREST }
    , [BackendSampler_LINEAR] = { VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR }
  };

  for(uint32_t i = 0; i < NUM_BACKEND_SAMPLERS; i++) {
     if(!report_vulkan_error("vkCreateSampler", vkCreateSampler(&(VkSamplerCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO
        , .flags = 0
        , .magFilter = data[i].min
        , .minFilter = data[i].max
        , .mipmapMode = data[i].mipmap
        , .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT
        , .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT
        , .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT
        , .mipLodBias = 0
        , .anisotropyEnable = VK_FALSE
        , .maxAnisotropy = 0
        , .compareEnable = VK_FALSE
        , .compareOp = VK_COMPARE_OP_NEVER
        , .minLod = 0
        , .maxLod = 0
        , .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK
        , .unnormalizedCoordinates = VK_FALSE
        }, &_.samplers[i]))) {
      return false;
    }
  }

  return true;
}

// ====================================================================================================================
static bool create_descriptor_set_layouts(void) {
  if(!report_vulkan_error("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(&(VkDescriptorSetLayoutCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
    , .flags = 0
    , .bindingCount = 2
    , .pBindings = (VkDescriptorSetLayoutBinding[]) {
        { // frame uniform buffer
          .binding = 0
        , .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        , .descriptorCount = 1
        , .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
        }
      , { // samplers
          .binding = 1
        , .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER
        , .descriptorCount = 2
        , .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
        , .pImmutableSamplers = _.samplers
        }
      }
    }, &_.descriptor_set_layout[0]))) {
    return false;
  }

  if(!report_vulkan_error("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(&(VkDescriptorSetLayoutCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
    , .flags = 0
    , .bindingCount = 1
    , .pBindings = (VkDescriptorSetLayoutBinding[]) {
        { // view uniform buffer
          .binding = 0
        , .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        , .descriptorCount = 1
        , .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
        }
      }
    }, &_.descriptor_set_layout[1]))) {
    return false;
  }

  if(!report_vulkan_error("vkCreateDescriptorSetLayout", vkCreateDescriptorSetLayout(&(VkDescriptorSetLayoutCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO
    , .flags = 0
    , .bindingCount = 3
    , .pBindings = (VkDescriptorSetLayoutBinding[]) {
        { // draw parameters
          .binding = 0
        , .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
        , .descriptorCount = 1
        , .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
        }
      , { // batch of material information
          .binding = 1
        , .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
        , .descriptorCount = 1
        , .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
        }
      , { // batch of textures
          .binding = 2
        , .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        , .descriptorCount = VULKAN_UI_TEXTURE_BATCH_SIZE
        , .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
        }
      }
    }, &_.descriptor_set_layout[2]))) {
    return false;
  }

  return true;
}

// ====================================================================================================================
static bool create_pipeline_layout(void) {
  if(!report_vulkan_error("vkCreatePipelineLayout", vkCreatePipelineLayout(&(VkPipelineLayoutCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
      , .setLayoutCount = 3
      , .pSetLayouts = _.descriptor_set_layout
    }, &_.pipeline_layout))) {
    return false;
  }

  return true;
}

// ====================================================================================================================
extern const uint32_t * _binary_engine_ui_vert_spv_start;
extern const uint32_t * _binary_engine_ui_vert_spv_end;

extern const uint32_t * _binary_engine_ui_frag_spv_start;
extern const uint32_t * _binary_engine_ui_frag_spv_end;

static bool create_ui_pipeline(void) {
  VkShaderModule vert;
  VkShaderModule frag;

  if(!report_vulkan_error("vkCreateShaderModule", vkCreateShaderModule(&(VkShaderModuleCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO
    , .codeSize = _binary_engine_ui_vert_spv_end - _binary_engine_ui_vert_spv_start
    , .pCode = _binary_engine_ui_vert_spv_start
    }, &vert))) {
    return false;
  }

  if(!report_vulkan_error("vkCreateShaderModule", vkCreateShaderModule(&(VkShaderModuleCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO
    , .codeSize = _binary_engine_ui_frag_spv_end - _binary_engine_ui_frag_spv_start
    , .pCode = _binary_engine_ui_frag_spv_start
    }, &frag))) {
    return false;
  }

  if(!report_vulkan_error("vkCreateGraphicsPipeline", vkCreateGraphicsPipelines(
      _.pipelinecache
    , 1
    , &(VkGraphicsPipelineCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
      , .pNext = NULL
      , .flags = 0
      , .stageCount = 2
      , .pStages = (VkPipelineShaderStageCreateInfo[]) {
          {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO
          , .stage = VK_SHADER_STAGE_VERTEX_BIT
          , .module = vert
          , .pName = "main"
          }
        , {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO
          , .stage = VK_SHADER_STAGE_FRAGMENT_BIT
          , .module = frag
          , .pName = "main"
          }
        }
      , .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
        , .vertexBindingDescriptionCount = 1
        , .pVertexBindingDescriptions = &(VkVertexInputBindingDescription) {
            .binding = 0
          , .stride = sizeof(struct BackendUIVertex)
          , .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
          }
        , .vertexAttributeDescriptionCount = 3
        , .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[]) {
            {
              .location = 0
            , .binding = 0
            , .format = VK_FORMAT_R32G32_SFLOAT
            , .offset = offsetof(struct BackendUIVertex, xy)
            }
          , {
              .location = 1
            , .binding = 0
            , .format = VK_FORMAT_R32G32B32A32_SFLOAT
            , .offset = offsetof(struct BackendUIVertex, rgba)
            }
          , {
              .location = 2
            , .binding = 0
            , .format = VK_FORMAT_R32G32_SFLOAT
            , .offset = offsetof(struct BackendUIVertex, st)
            }
          }
        }
      , .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO
        , .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        , .primitiveRestartEnable = VK_FALSE
        }
      , .pViewportState = &(VkPipelineViewportStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO
        , .viewportCount = 1
        , .scissorCount = 1
        }
      , .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO
        , .depthClampEnable = VK_FALSE
        , .rasterizerDiscardEnable = VK_FALSE
        , .polygonMode = VK_POLYGON_MODE_FILL
        , .cullMode = VK_CULL_MODE_BACK_BIT
        , .frontFace = VK_FRONT_FACE_CLOCKWISE
        , .depthBiasEnable = VK_FALSE
        , .lineWidth = 1.0f
        }
      , .pDepthStencilState = &(VkPipelineDepthStencilStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
        , .depthTestEnable = VK_FALSE
        , .depthWriteEnable = VK_FALSE
        , .depthCompareOp = VK_COMPARE_OP_NEVER
        , .depthBoundsTestEnable = VK_FALSE
        , .stencilTestEnable = VK_FALSE
        , .front = (VkStencilOpState) {
            .failOp = VK_STENCIL_OP_KEEP
          , .passOp = VK_STENCIL_OP_KEEP
          , .depthFailOp = VK_STENCIL_OP_KEEP
          , .compareOp = VK_COMPARE_OP_NEVER
          , .compareMask = 0
          , .writeMask = 0
          , .reference = 0
          }
        , .back = (VkStencilOpState) {
            .failOp = VK_STENCIL_OP_KEEP
          , .passOp = VK_STENCIL_OP_KEEP
          , .depthFailOp = VK_STENCIL_OP_KEEP
          , .compareOp = VK_COMPARE_OP_NEVER
          , .compareMask = 0
          , .writeMask = 0
          , .reference = 0
          }
        , .minDepthBounds = 0.0f
        , .maxDepthBounds = 1.0f
        }
      , .pColorBlendState = &(VkPipelineColorBlendStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO
        , .logicOpEnable = VK_FALSE
        , .logicOp = VK_LOGIC_OP_CLEAR
        , .attachmentCount = 1
        , .pAttachments = &(VkPipelineColorBlendAttachmentState) {
            .blendEnable = VK_TRUE
          , .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA
          , .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
          , .colorBlendOp = VK_BLEND_OP_ADD
          , .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA
          , .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
          , .alphaBlendOp = VK_BLEND_OP_ADD
          , .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
          }
        , .blendConstants = { 0, 0, 0, 0 }
        }
      , .pDynamicState = &(VkPipelineDynamicStateCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO
        , .dynamicStateCount = 2
        , .pDynamicStates = (VkDynamicState[]) {
            VK_DYNAMIC_STATE_VIEWPORT
          , VK_DYNAMIC_STATE_SCISSOR
          }
        }
      , .layout = _.pipeline_layout
      , .renderPass = _.render_pass
      , .subpass = 0
      , .basePipelineHandle = VK_NULL_HANDLE
      , .basePipelineIndex = 0
      }
    , &_.ui_pipeline
    ))) {
    return false;
  }

  return true;
}

// ====================================================================================================================
bool Vulkan_init(uint32_t * width, uint32_t * height, GLFWwindow * window) {
  alias_memory_clear(&_, sizeof(_));
  _.window = window;
  _.validation = 1;
  for(uint32_t i = 0; i < NUM_QUEUES; i++) {
    _.transition_cbuf[i] = -1;
  }

  if(!glfwVulkanSupported()) {
    ALIAS_ERROR("Vulkan is not supported");
    return false;
  }
  
  if(create_instance()
    && create_debug_report_callback()
    && select_physical_device()
    && create_device()
    && create_surface()
    && create_command_pool(_.queue_family_index[Graphics], &_.command_pool[Graphics])
    && create_command_pool(_.queue_family_index[Transfer], &_.command_pool[Transfer])
    && create_frame_sync_primitives()
    && create_staging_buffer()
    && create_render_pass()
    && create_pipeline_cache()

    && create_samplers()
    && create_descriptor_set_layouts()
    && create_pipeline_layout()
    && create_ui_pipeline()

    && create_swapchain(width, height, false)
    && create_depthbuffer()
    && create_framebuffers()
  ) {
    ALIAS_INFO("Vulkan initialized successfully");
    return true;
  }
  return false;
}

// ====================================================================================================================
void Vulkan_cleanup(void) {
}

// ====================================================================================================================
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

// ====================================================================================================================
static void simple_enqueue(enum QueueIndex queue, uint32_t index) {
  report_vulkan_error("vkEndCommandBuffer", vkEndCommandBuffer(_.command_buffers[queue].data[index].buffer));
  report_vulkan_error("vkQueueSubmit", vkQueueSubmit(
      _.queue[queue]
    , 1
    , &(VkSubmitInfo) {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO
      , .waitSemaphoreCount   = 0
      , .pWaitSemaphores      = NULL
      , .pWaitDstStageMask    = NULL
      , .commandBufferCount   = 1
      , .pCommandBuffers      = &_.command_buffers[queue].data[index].buffer
      , .signalSemaphoreCount = 0
      , .pSignalSemaphores    = NULL
      }
    , _.command_buffers[queue].data[index].fence
    ));
  _.transition_cbuf[queue] = -1;
  _.in_flight[queue]++;
}

// ====================================================================================================================
static void _image_read(uv_fs_t * req) {
  struct ImageLoadContext * ctx = (struct ImageLoadContext *)req->data;
  uv_fs_req_cleanup(req);

  if(req->result < 0) {
    alias_free(alias_default_MemoryCB(), ctx->buf.base, ctx->buf.len, 4);
    alias_free(alias_default_MemoryCB(), ctx, sizeof(*ctx), alignof(*ctx));
    return;
  }

  struct BackendImage * image = ctx->image;

  // TODO use QOI loading instead of STBI, also require pre-process images with it
  //      using QOI loading will also allow pipe reading
  int width, height, channels;
  stbi_uc * pixels = stbi_load_from_memory(ctx->buf.base, ctx->buf.len, &width, &height, &channels, STBI_rgb_alpha);

  // free file contents and close
  alias_free(alias_default_MemoryCB(), ctx->buf.base, ctx->buf.len, 4);
  uv_fs_close(Engine_uv_loop(), &ctx->req, ctx->fd, _image_close);

  // Vulkan loading here
  VkDeviceSize size = width * height * 4;

  VkDeviceSize offset;

  while(true) {
    if(_.staging_buffer_unused[0] + size < _.staging_buffer_unused[1]) {
      offset = _.staging_buffer_unused[0];
      _.staging_buffer_unused[0] += size;
      break;
    }

    if(_.staging_buffer_unused[2] + size < _.staging_buffer_unused[3]) {
      offset = _.staging_buffer_unused[2];
      _.staging_buffer_unused[2] += size;
      break;
    }

    flush_transfer_command_buffer();
  }

  void * dst = _.staging_buffer_map + offset;

  alias_memory_copy(dst, size, pixels, size);

  stbi_image_free(pixels);

  uint32_t mip_levels = 1 + log2(alias_max(width, height));

  vkCreateImage(
      &(VkImageCreateInfo) {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO
      , .flags         = 0
      , .imageType     = VK_IMAGE_TYPE_2D
      , .format        = VK_FORMAT_R8G8B8A8_UNORM
      , .extent        = { width, height, 1 }
      , .mipLevels     = mip_levels
      , .arrayLayers   = 1
      , .samples       = VK_SAMPLE_COUNT_1_BIT
      , .tiling        = VK_IMAGE_TILING_OPTIMAL
      , .usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
      , .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
      }
    , (VkImage *)&image->image
    );

  VkMemoryRequirements memory_requirements;
  vkGetImageMemoryRequirements((VkImage)image->image, &memory_requirements);

  create_device_memory(memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, (VkDeviceMemory *)&image->memory);

  vkBindImageMemory((VkImage)image->image, (VkDeviceMemory)image->memory, 0);

  VkImageView imageview;
  vkCreateImageView(
      &(VkImageViewCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
      , .image = (VkImage)image->image
      , .viewType = VK_IMAGE_VIEW_TYPE_2D
      , .format = VK_FORMAT_R8G8B8A8_UNORM
      , .components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A }
      , .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
        , .baseMipLevel = 0
        , .levelCount = mip_levels
        , .baseArrayLayer = 0
        , .layerCount = 1
        }
      }
    , &imageview
    );

  image->imageview = (uint64_t)imageview;

  // temp Transfer command buffer:
  //   ACQUIRE
  //   transition image[..] to transfer dst from undefined
  //   copy memory to image[0]
  //   SUBMIT
  // 
  // per frame transition:
  //   transition image[..] to shader src from transfer dst with pass-off to Graphics
  //
  // per frame graphics:
  //   transition image[..] to shader src from transfer dst with pass-off from Transfer
  //   for image[1..]:
  //     transition image[i-1] to transfer src from transfer dst
  //     blit image[i-1] to image[i]
  //   transition image[..-1] to transfer dst from transfer dst

  uint32_t temp_index = acquire_command_buffer(Transfer);

  vkCmdPipelineBarrier(
      _.command_buffers[Transfer].data[temp_index].buffer
    , VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT
    , VK_PIPELINE_STAGE_TRANSFER_BIT
    , 0
    , 0, NULL
    , 0, NULL
    , 1, &(VkImageMemoryBarrier){
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
      , .srcAccessMask       = 0
      , .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT
      , .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED
      , .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
      , .srcQueueFamilyIndex = _.queue_family_index[Transfer]
      , .dstQueueFamilyIndex = _.queue_family_index[Transfer]
      , .image               = (VkImage)image->image
      , .subresourceRange    = {
          .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT
        , .baseMipLevel   = 0
        , .levelCount     = mip_levels
        , .baseArrayLayer = 0
        , .layerCount     = 1
        }
      }
    );

  vkCmdCopyBufferToImage(
      _.command_buffers[Transfer].data[temp_index].buffer
    , _.staging_buffer
    , (VkImage)image->image
    , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    , 1
    , &(VkBufferImageCopy) {
        offset
      , 0
      , 0
      , (VkImageSubresourceLayers) {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
        , .mipLevel = 0
        , .baseArrayLayer = 0
        , .layerCount = 1
        }
      , (VkOffset3D) {     0,      0, 0 }
      , (VkExtent3D) { width, height, 1 }
      }
    );

  vkFlushMappedMemoryRanges(1, &(VkMappedMemoryRange) {
      .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE
    , .memory = _.staging_buffer_memory
    , .offset = offset
    , .size = size
    });

  _.command_buffers[Transfer].data[temp_index].stage_start = offset;
  _.command_buffers[Transfer].data[temp_index].stage_end = offset + size;
  simple_enqueue(Transfer, temp_index);

  // pass off to Graphics
  if(_.transition_cbuf[Transfer] == -1) {
    _.transition_cbuf[Transfer] = acquire_command_buffer(Transfer);
  }

  vkCmdPipelineBarrier(
      _.command_buffers[Transfer].data[_.transition_cbuf[Transfer]].buffer
    , VK_PIPELINE_STAGE_TRANSFER_BIT
    , VK_PIPELINE_STAGE_TRANSFER_BIT
    , 0
    , 0, NULL
    , 0, NULL
    , 1, &(VkImageMemoryBarrier){
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
      , .srcAccessMask       = 0
      , .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT
      , .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
      , .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
      , .srcQueueFamilyIndex = _.queue_family_index[Transfer]
      , .dstQueueFamilyIndex = _.queue_family_index[Graphics]
      , .image               = (VkImage)image->image
      , .subresourceRange    = {
          .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT
        , .baseMipLevel   = 0
        , .levelCount     = mip_levels
        , .baseArrayLayer = 0
        , .layerCount     = 1
        }
      }
    );

  // pass off from Transfer
  if(_.transition_cbuf[Graphics] == -1) {
    _.transition_cbuf[Graphics] = acquire_command_buffer(Graphics);
  }

  vkCmdPipelineBarrier(
      _.command_buffers[Graphics].data[_.transition_cbuf[Graphics]].buffer
    , VK_PIPELINE_STAGE_TRANSFER_BIT
    , VK_PIPELINE_STAGE_TRANSFER_BIT
    , 0
    , 0, NULL
    , 0, NULL
    , 1, &(VkImageMemoryBarrier){
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
      , .srcAccessMask       = 0
      , .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT
      , .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
      , .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
      , .srcQueueFamilyIndex = _.queue_family_index[Transfer]
      , .dstQueueFamilyIndex = _.queue_family_index[Graphics]
      , .image               = (VkImage)image->image
      , .subresourceRange    = {
          .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT
        , .baseMipLevel   = 0
        , .levelCount     = mip_levels
        , .baseArrayLayer = 0
        , .layerCount     = 1
        }
      }
    );

  for(uint32_t i = 1; i < mip_levels; i++) {
    vkCmdPipelineBarrier(
        _.command_buffers[Graphics].data[_.transition_cbuf[Graphics]].buffer
      , VK_PIPELINE_STAGE_TRANSFER_BIT
      , VK_PIPELINE_STAGE_TRANSFER_BIT
      , 0
      , 0, NULL
      , 0, NULL
      , 1, &(VkImageMemoryBarrier){
          .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER
        , .srcAccessMask       = 0
        , .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT
        , .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        , .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
        , .srcQueueFamilyIndex = _.queue_family_index[Graphics]
        , .dstQueueFamilyIndex = _.queue_family_index[Graphics]
        , .image               = (VkImage)image->image
        , .subresourceRange    = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT
          , .baseMipLevel   = i - 1
          , .levelCount     = 1
          , .baseArrayLayer = 0
          , .layerCount     = 1
          }
        }
      );

    uint32_t mipped_width = alias_max(1, width >> 1);
    uint32_t mipped_height = alias_max(1, height >> 1);

    vkCmdBlitImage(
        _.command_buffers[Graphics].data[_.transition_cbuf[Graphics]].buffer
      , (VkImage)image->image
      , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
      , (VkImage)image->image
      , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
      , 1
      , &(VkImageBlit) {
          .srcSubresource = {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
            , .mipLevel = i - 1
            , .baseArrayLayer = 0
            , .layerCount = 1
            }
        , .srcOffsets[0] = { 0, 0, 0 }
        , .srcOffsets[1] = { width, height, 1 }
        , .dstSubresource = {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
            , .mipLevel = i
            , .baseArrayLayer = 0
            , .layerCount = 1
            }
        , .dstOffsets[0] = { 0, 0, 0 }
        , .dstOffsets[1] = { mipped_width, mipped_height, 1 }
        }
      , VK_FILTER_LINEAR
      );

    width = mipped_width;
    height = mipped_height;
  }
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
  vkDestroyImageView((VkImageView)image->imageview);
  vkDestroyImage((VkImage)image->image);
  vkFreeMemory((VkDeviceMemory)image->memory);
}

void BackendUIVertex_render(const struct BackendImage * image, struct BackendUIVertex * vertexes, uint32_t num_indexes, const uint32_t * indexes) {
  VkImageView imageview = (VkImageView)image->image;

  if(imageview == VK_NULL_HANDLE) {
    return;
  }

  
}

static void set_default_viewport_scissor(void) {
  VkCommandBuffer cbuf = _.command_buffers[Graphics].data[_.rendering_cbuf].buffer;

  vkCmdSetViewport(cbuf, 1, 1, &(VkViewport) {
      .x = 0
    , .y = 0
    , .width = _.swapchain_extents.width
    , .height = _.swapchain_extents.height
    , .minDepth = 0
    , .maxDepth = 1
    });

  vkCmdSetScissor(cbuf, 1, 1, &(VkRect2D) {
      .offset = { 0, 0 }
    , .extent = { _.swapchain_extents.width, _.swapchain_extents.height }
    });
}

void Backend_begin_rendering(uint32_t screen_width, uint32_t screen_height) {
  vkAcquireNextImageKHR(_.swapchain, UINT64_MAX, _.frame_gpu_present_to_gpu_graphics, VK_NULL_HANDLE, &_.swapchain_current_index);
  _.rendering_cbuf = acquire_command_buffer(Graphics);

  VkCommandBuffer cbuf = _.command_buffers[Graphics].data[_.rendering_cbuf].buffer;
  
  vkCmdBeginRenderPass(
      cbuf
    , &(VkRenderPassBeginInfo) {
        .sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO
      , .framebuffer = _.framebuffers.data[_.swapchain_current_index]
      , .renderArea  = { .offset = { 0, 0 }, .extent = { _.swapchain_extents.width, _.swapchain_extents.height } }
      , .clearValueCount = 2
      , .pClearValues = (VkClearValue[]) {
          { .color.float32 = { 0, 0, 0, 1 } }
        , { .depthStencil.depth = 1, .depthStencil.stencil = 0 }
        }
      }
    , VK_SUBPASS_CONTENTS_INLINE
    );

  set_default_viewport_scissor();
}

void Backend_end_rendering(void) {
  vkCmdEndRenderPass(_.command_buffers[Graphics].data[_.rendering_cbuf].buffer);

  VkSemaphore transfer_semaphore = VK_NULL_HANDLE;

  // maybe produce tx
  if(_.transition_cbuf[Transfer] != -1) {
    report_vulkan_error("vkEndCommandBuffer", vkEndCommandBuffer(_.command_buffers[Transfer].data[_.transition_cbuf[Transfer]].buffer));
    transfer_semaphore = _.frame_gpu_transfer_to_gpu_graphics;
    report_vulkan_error("vkQueueSubmit", vkQueueSubmit(
        _.queue[Transfer]
      , 1
      , &(VkSubmitInfo) {
          .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO
        , .waitSemaphoreCount   = 0
        , .pWaitSemaphores      = NULL
        , .pWaitDstStageMask    = NULL
        , .commandBufferCount   = 1
        , .pCommandBuffers      = &_.command_buffers[Transfer].data[_.transition_cbuf[Transfer]].buffer
        , .signalSemaphoreCount = 1
        , .pSignalSemaphores    = &transfer_semaphore
        }
      , _.command_buffers[Transfer].data[_.transition_cbuf[Transfer]].fence
      ));
    _.command_buffers[Transfer].data[_.transition_cbuf[Transfer]].frame = _.frame_pending[Transfer]++;
    _.transition_cbuf[Transfer] = -1;
    _.in_flight[Transfer]++;
  }

  // maybe consume tx
  if(_.transition_cbuf[Graphics] != -1) {
    report_vulkan_error("vkEndCommandBuffer", vkEndCommandBuffer(_.command_buffers[Graphics].data[_.transition_cbuf[Graphics]].buffer));
    report_vulkan_error("vkQueueSubmit", vkQueueSubmit(
        _.queue[Graphics]
      , 1
      , &(VkSubmitInfo) {
          .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO
        , .waitSemaphoreCount   = transfer_semaphore == VK_NULL_HANDLE ? 0 : 1
        , .pWaitSemaphores      = transfer_semaphore == VK_NULL_HANDLE ? 0 : &transfer_semaphore
        , .pWaitDstStageMask    = (VkPipelineStageFlags[]){ VK_PIPELINE_STAGE_TRANSFER_BIT }
        , .commandBufferCount   = 1
        , .pCommandBuffers      = &_.command_buffers[Graphics].data[_.transition_cbuf[Graphics]].buffer
        , .signalSemaphoreCount = 0
        , .pSignalSemaphores    = NULL
        }
      , _.command_buffers[Graphics].data[_.transition_cbuf[Graphics]].fence
      ));
    transfer_semaphore = VK_NULL_HANDLE;
    _.transition_cbuf[Graphics] = -1;
    _.in_flight[Graphics]++;
  }

  // consume tx if exist
  report_vulkan_error("vkEndCommandBuffer", vkEndCommandBuffer(_.command_buffers[Graphics].data[_.rendering_cbuf].buffer));
  report_vulkan_error("vkQueueSubmit", vkQueueSubmit(
      _.queue[Graphics]
    , 1
    , &(VkSubmitInfo) {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO
      , .waitSemaphoreCount   = transfer_semaphore == VK_NULL_HANDLE ? 1 : 2
      , .pWaitSemaphores      = (VkSemaphore[]){ _.frame_gpu_present_to_gpu_graphics, transfer_semaphore }
      , .pWaitDstStageMask    = (VkPipelineStageFlags[]){ VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT }
      , .commandBufferCount   = 1
      , .pCommandBuffers      = &_.command_buffers[Graphics].data[_.rendering_cbuf].buffer
      , .signalSemaphoreCount = 1
      , .pSignalSemaphores    = &_.frame_gpu_graphics_to_gpu_present
      }
    , _.command_buffers[Graphics].data[_.rendering_cbuf].fence
    ));
  _.command_buffers[Graphics].data[_.rendering_cbuf].frame = _.frame_pending[Graphics]++;
  _.in_flight[Graphics]++;

  vkQueuePresentKHR(_.queue[Present], &(VkPresentInfoKHR) {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR
    , .waitSemaphoreCount = 1
    , .pWaitSemaphores = &_.frame_gpu_graphics_to_gpu_present
    , .swapchainCount = 1
    , .pSwapchains = &_.swapchain
    , .pImageIndices = &_.swapchain_current_index
    , .pResults = NULL
    });
  
  glfwPollEvents();
}

void Backend_begin_2d(struct BackendMode2D mode) {
  VkCommandBuffer cbuf = _.command_buffers[Graphics].data[_.rendering_cbuf].buffer;

  VkViewport viewport = {
      .x = alias_pga2d_point_x(mode.viewport_min) * _.swapchain_extents.width
    , .y = alias_pga2d_point_y(mode.viewport_min) * _.swapchain_extents.height
    , .width = (alias_pga2d_point_x(mode.viewport_max) - alias_pga2d_point_x(mode.viewport_min)) * _.swapchain_extents.width
    , .height = (alias_pga2d_point_y(mode.viewport_max) - alias_pga2d_point_y(mode.viewport_min)) * _.swapchain_extents.height
    , .minDepth = 0
    , .maxDepth = 1
    };

  VkRect2D scissor = { .offset = { viewport.x, viewport.y }, .extent = { viewport.width, viewport.height } };

  vkCmdSetViewport(cbuf, 1, 1, &viewport);
  vkCmdSetScissor(cbuf, 1, 1, &scissor);
}

void Backend_end_2d(void) {
  set_default_viewport_scissor();
}
