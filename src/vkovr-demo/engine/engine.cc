#include <vkovr-demo/engine/engine.h>

#include <iostream>
#include <thread>
#include <chrono>

#include <GLFW/glfw3.h>

#include <glm/gtc/type_ptr.hpp>

#include <vkovr-demo/engine/ubo/camera_ubo.h>

namespace demo
{
namespace engine
{
namespace
{
auto align(vk::DeviceSize offset, vk::DeviceSize alignment)
{
  return (offset + alignment - 1) & ~(alignment - 1);
}

const std::vector<std::string> vrExpectedInstanceExtensions = {
  VK_KHR_SURFACE_EXTENSION_NAME,
  VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
  "VK_KHR_win32_surface",
  VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME,
  VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
  VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
};

const std::vector<std::string> vrExpectedDeviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
  "VK_KHR_external_memory_win32",
  VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME,
  "VK_KHR_external_fence_win32",
  VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
  "VK_KHR_external_semaphore_win32",
};

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
  VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
  VkDebugUtilsMessageTypeFlagsEXT message_type,
  const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
  void* pUserData)
{
  if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    std::cerr << callback_data->pMessage << std::endl << std::endl;

  return VK_FALSE;
}
}

Engine::Engine(GLFWwindow* window, uint32_t width, uint32_t height)
  : vrWorker_{ this }
  , width_{ width }
  , height_{ height }
{
  createInstance(window);
  createDevice();
  createResourcePools();
  createSwapchain();
  createRenderPass();
  createFramebuffer();
  prepareResources();
  createRenderer();
  createCommandBuffers();
  createSynchronizationObjects();
}

Engine::~Engine()
{
  device_.waitIdle();

  vrWorker_.join();

  destroySynchronizationObjects();
  destroyCommandBuffers();
  destroyRenderer();
  destroyResources();
  destroyFramebuffer();
  destroyRenderPass();
  destroySwapchain();
  destroyResourcePools();
  destroyDevice();
  destroyInstance();
}

void Engine::createInstance(GLFWwindow* window)
{
  const auto instanceExtensions = vk::enumerateInstanceExtensionProperties();
  std::cout << "Instance extensions:" << std::endl;
  for (int i = 0; i < instanceExtensions.size(); i++)
    std::cout << "  " << instanceExtensions[i].extensionName << std::endl;

  const auto instanceLayers = vk::enumerateInstanceLayerProperties();
  std::cout << "Instance layers:" << std::endl;
  for (int i = 0; i < instanceLayers.size(); i++)
    std::cout << "  " << instanceLayers[i].layerName << std::endl;

  vk::ApplicationInfo appInfo;
  appInfo
    .setPApplicationName("Superlucent")
    .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
    .setPEngineName("Superlucent Engine")
    .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
    .setApiVersion(VK_API_VERSION_1_2);

  std::vector<const char*> layers = {
    "VK_LAYER_KHRONOS_validation",
  };

  std::vector<std::string> extensions = {
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
  };

  uint32_t numGlfwExtensions = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&numGlfwExtensions);
  for (uint32_t i = 0; i < numGlfwExtensions; i++)
    extensions.push_back(glfwExtensions[i]);

  for (const auto& ovrExtension : vrExpectedInstanceExtensions)
    extensions.push_back(ovrExtension);

  // Create instance
  std::vector<const char*> extensionCstr;
  for (const auto& extension : extensions)
    extensionCstr.push_back(extension.c_str());

  vk::InstanceCreateInfo instanceCreateInfo;
  instanceCreateInfo
    .setPApplicationInfo(&appInfo)
    .setPEnabledLayerNames(layers)
    .setPEnabledExtensionNames(extensionCstr);

  vk::DebugUtilsMessengerCreateInfoEXT messengerCreateInfo;
  messengerCreateInfo
    .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose)
    .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
    .setPfnUserCallback(debugCallback);

  vk::StructureChain<vk::InstanceCreateInfo, vk::DebugUtilsMessengerCreateInfoEXT> chain{
    instanceCreateInfo, messengerCreateInfo
  };
  instance_ = vk::createInstance(chain.get<vk::InstanceCreateInfo>());

  // Create messneger
  vk::DynamicLoader dl;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
  vk::DispatchLoaderDynamic dld{ instance_, vkGetInstanceProcAddr };
  messenger_ = instance_.createDebugUtilsMessengerEXT(chain.get<vk::DebugUtilsMessengerCreateInfoEXT>(), nullptr, dld);

  // Create surface
  VkSurfaceKHR surfaceHandle;
  glfwCreateWindowSurface(instance_, window, nullptr, &surfaceHandle);
  surface_ = surfaceHandle;
}

void Engine::destroyInstance()
{
  instance_.destroySurfaceKHR(surface_);

  vk::DynamicLoader dl;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
  vk::DispatchLoaderDynamic dld{ instance_, vkGetInstanceProcAddr };
  instance_.destroyDebugUtilsMessengerEXT(messenger_, nullptr, dld);

  instance_.destroy();
}

void Engine::createDevice()
{
  // Choose the first GPU
  physicalDevice_ = instance_.enumeratePhysicalDevices()[0];

  // Available extensions
  const auto deviceExtensions = physicalDevice_.enumerateDeviceExtensionProperties();
  std::cout << "Device extensions:" << std::endl;
  for (int i = 0; i < deviceExtensions.size(); i++)
    std::cout << "  " << deviceExtensions[i].extensionName << std::endl;

  // Device properties
  uboAlignment_ = physicalDevice_.getProperties().limits.minUniformBufferOffsetAlignment;
  ssboAlignment_ = physicalDevice_.getProperties().limits.minStorageBufferOffsetAlignment;

  // Find general queue capable of graphics, compute and present
  constexpr auto queueFlag = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute;
  const auto queueFamilyProperties = physicalDevice_.getQueueFamilyProperties();
  for (int i = 0; i < queueFamilyProperties.size(); i++)
  {
    if ((queueFamilyProperties[i].queueFlags & queueFlag) == queueFlag &&
      physicalDevice_.getSurfaceSupportKHR(i, surface_) &&
      queueFamilyProperties[i].queueCount >= 2)
    {
      queueIndex_ = i;
      break;
    }
  }

  std::vector<float> queuePriorities = {
    1.f, 1.f, 1.f,
  };
  vk::DeviceQueueCreateInfo queueCreateInfo{ {},
    queueIndex_, queuePriorities
  };

  // Device extensions
  std::vector<std::string> extensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
  };

  for (const auto& ovrExtension : vrExpectedDeviceExtensions)
    extensions.push_back(ovrExtension);

  // Device features
  auto features = physicalDevice_.getFeatures();
  features
    .setSamplerAnisotropy(true);

  // Create device
  std::vector<const char*> extensionCstr;
  for (const auto& extension : extensions)
    extensionCstr.push_back(extension.c_str());

  vk::DeviceCreateInfo deviceCreateInfo;
  deviceCreateInfo
    .setQueueCreateInfos(queueCreateInfo)
    .setPEnabledExtensionNames(extensionCstr)
    .setPEnabledFeatures(&features);
  device_ = physicalDevice_.createDevice(deviceCreateInfo);

  queue_ = device_.getQueue(queueIndex_, 0);
  vrQueue_ = device_.getQueue(queueIndex_, 1);
  presentQueue_ = device_.getQueue(queueIndex_, 2);
}

void Engine::destroyDevice()
{
  device_.destroy();
}

void Engine::createResourcePools()
{
  // Command pool
  vk::CommandPoolCreateInfo commandPoolCreateInfo;
  commandPoolCreateInfo
    .setQueueFamilyIndex(queueIndex_)
    .setFlags(vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
  commandPool_ = device_.createCommandPool(commandPoolCreateInfo);

  // Descriptor pool
  constexpr auto descriptorCount = 1000;
  constexpr auto setCount = 1000;
  std::vector<vk::DescriptorPoolSize> poolSizes = {
    {vk::DescriptorType::eUniformBuffer, descriptorCount},
    {vk::DescriptorType::eStorageBuffer, descriptorCount},
    {vk::DescriptorType::eCombinedImageSampler, descriptorCount},
  };

  vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
  descriptorPoolCreateInfo
    .setMaxSets(setCount)
    .setPoolSizes(poolSizes);
  descriptorPool_ = device_.createDescriptorPool(descriptorPoolCreateInfo);

  // Memory pool
  constexpr vk::DeviceSize memorySize = 1024 * 1024 * 1024; // 1GB
  MemoryPoolCreateInfo memoryPoolCreateInfo;
  memoryPoolCreateInfo.device = device_;
  memoryPoolCreateInfo.physicalDevice = physicalDevice_;
  memoryPoolCreateInfo.deviceMemorySize = memorySize;
  memoryPoolCreateInfo.hostMemorySize = memorySize;
  memoryPool_ = createMemoryPool(memoryPoolCreateInfo);
}

void Engine::destroyResourcePools()
{
  device_.destroyCommandPool(commandPool_);
  device_.destroyDescriptorPool(descriptorPool_);
  memoryPool_.destroy();
}

void Engine::createSwapchain()
{
  SwapchainCreateInfo swapchainCreateInfo;
  swapchainCreateInfo.device = device_;
  swapchainCreateInfo.physicalDevice = physicalDevice_;
  swapchainCreateInfo.surface = surface_;
  swapchainCreateInfo.width = width_;
  swapchainCreateInfo.height = height_;
  swapchain_ = engine::createSwapchain(swapchainCreateInfo);
}

void Engine::destroySwapchain()
{
  swapchain_.destroy();
}

void Engine::createRenderPass()
{
  RenderPassCreateInfo renderPassCreateInfo;
  renderPassCreateInfo.device = device_;
  renderPassCreateInfo.format = swapchain_.getImageFormat();
  renderPassCreateInfo.samples = vk::SampleCountFlagBits::e4;
  renderPassCreateInfo.finalLayout = vk::ImageLayout::ePresentSrcKHR;
  renderPass_ = engine::createRenderPass(renderPassCreateInfo);
}

void Engine::destroyRenderPass()
{
  renderPass_.destroy();
}

void Engine::createFramebuffer()
{
  FramebufferCreateInfo framebufferCreateInfo;
  framebufferCreateInfo.device = device_;
  framebufferCreateInfo.colorImageViews = swapchain_.getImageViews();
  framebufferCreateInfo.pMemoryPool = &memoryPool_;
  framebufferCreateInfo.width = width_;
  framebufferCreateInfo.height = height_;
  framebufferCreateInfo.maxWidth = 1920;
  framebufferCreateInfo.maxHeight = 1080;
  framebufferCreateInfo.pRenderPass = &renderPass_;
  framebuffer_ = engine::createFramebuffer(framebufferCreateInfo);
}

void Engine::destroyFramebuffer()
{
  framebuffer_.destroy();
}

void Engine::createRenderer()
{
  RendererCreateInfo rendererCreateInfo;
  rendererCreateInfo.device = device_;
  rendererCreateInfo.physicalDevice = physicalDevice_;
  rendererCreateInfo.pRenderPass = &renderPass_;
  rendererCreateInfo.descriptorPool = descriptorPool_;
  rendererCreateInfo.imageCount = swapchain_.getImageCount();
  rendererCreateInfo.textureImageView = texture_.getImageView();
  rendererCreateInfo.sampler = sampler_;
  rendererCreateInfo.pMemoryPool = &memoryPool_;
  renderer_ = engine::createRenderer(rendererCreateInfo);
}

void Engine::destroyRenderer()
{
  renderer_.destroy();
}

void Engine::prepareResources()
{
  mesh_ = std::make_unique<scene::Mesh>();
  mesh_->setHasNormal();
  mesh_->setHasTexture();

  scene::Mesh::Vertex vertex;

  constexpr float pi = 3.141592f;
  constexpr int segment = 16;
  for (int i = 0; i <= segment * 2; i++)
  {
    const auto theta = static_cast<float>(i) / (segment * 2.f) * pi * 2.f;
    const auto cosTheta = std::cos(theta);
    const auto sinTheta = std::sin(theta);

    vertex.position = { 0.f, 0.f, 1.f };
    vertex.normal = vertex.position;
    vertex.tex_coord = glm::vec2{ (static_cast<float>(i) + 0.5f) / (segment * 2.f), 1.f } * 8.f;
    mesh_->addVertex(vertex);

    for (int j = 1; j < segment; j++)
    {
      const auto phi = static_cast<float>(j) / segment * pi;
      const auto cosPhi = std::cos(phi);
      const auto sinPhi = std::sin(phi);

      vertex.position = { cosTheta * sinPhi, sinTheta * sinPhi, cosPhi };
      vertex.normal = vertex.position;
      vertex.tex_coord = glm::vec2{ static_cast<float>(i) / (segment * 2.f), 1.f - static_cast<float>(j) / segment } * 8.f;

      mesh_->addVertex(vertex);
    }

    vertex.position = { 0.f, 0.f, -1.f };
    vertex.normal = vertex.position;
    vertex.tex_coord = glm::vec2{ (static_cast<float>(i) + 0.5f) / (segment * 2.f), 0.f } * 8.f;
    mesh_->addVertex(vertex);
  }

  for (int i = 0; i < segment * 2; i++)
  {
    mesh_->addFace({ i * (segment + 1), i * (segment + 1) + 1, (i + 1) * (segment + 1) + 1 });

    for (int j = 1; j < segment - 1; j++)
    {
      const auto f0 = i * (segment + 1) + j;
      const auto f1 = i * (segment + 1) + j + 1;
      const auto f2 = (i + 1) * (segment + 1) + j;
      const auto f3 = (i + 1) * (segment + 1) + j + 1;

      mesh_->addFace({ f0, f1, f2 });
      mesh_->addFace({ f1, f3, f2 });
    }

    mesh_->addFace({ i * (segment + 1) + segment, (i + 1) * (segment + 1) + segment - 1, i * (segment + 1) + segment - 1 });
  }

  // Checkerboard texture
  constexpr int checkerboardTextureLength = 128;
  std::vector<uint8_t> checkerboardTexture(checkerboardTextureLength * checkerboardTextureLength * 4);
  for (int u = 0; u < checkerboardTextureLength; u++)
  {
    for (int v = 0; v < checkerboardTextureLength; v++)
    {
      uint8_t color = (255 - 64) + 64 * !((u < checkerboardTextureLength / 2) ^ (v < checkerboardTextureLength / 2));
      checkerboardTexture[(v * checkerboardTextureLength + u) * 4 + 0] = color;
      checkerboardTexture[(v * checkerboardTextureLength + u) * 4 + 1] = color;
      checkerboardTexture[(v * checkerboardTextureLength + u) * 4 + 2] = color;
      checkerboardTexture[(v * checkerboardTextureLength + u) * 4 + 3] = 255;
    }
  }
  constexpr int checkerboardTextureSize = checkerboardTextureLength * checkerboardTextureLength * sizeof(uint8_t) * 4;

  // Buffer
  const auto& vertices = mesh_->vertices();
  const auto& faces = mesh_->faces();

  const auto vertexBufferSize = sizeof(vertices[0]) * vertices.size();
  const auto indexBufferSize = sizeof(faces[0]) * faces.size();

  const auto meshBufferSize = vertexBufferSize + indexBufferSize;
  const auto bufferSize = meshBufferSize + checkerboardTextureSize;
  const auto checkerboardTextureOffset = meshBufferSize;

  vk::BufferCreateInfo bufferCreateInfo;
  bufferCreateInfo
    .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer)
    .setSize(bufferSize);
  meshBuffer_ = device_.createBuffer(bufferCreateInfo);
  meshIndexOffset_ = vertexBufferSize;
  meshIndexCount_ = faces.size() * 3;

  const auto meshMemory = memoryPool_.allocateDeviceMemory(meshBuffer_);
  device_.bindBufferMemory(meshBuffer_, meshMemory.memory, meshMemory.offset);

  // Staging buffer for mesh
  bufferCreateInfo
    .setUsage(vk::BufferUsageFlagBits::eTransferSrc)
    .setSize(bufferSize);
  stagingBuffer_ = device_.createBuffer(bufferCreateInfo);

  const auto stagingMemory = memoryPool_.allocatePersistentlyMappedMemory(stagingBuffer_);
  device_.bindBufferMemory(stagingBuffer_, stagingMemory.memory, stagingMemory.offset);

  memcpy(stagingMemory.map, vertices.data(), vertexBufferSize);
  memcpy(stagingMemory.map + vertexBufferSize, faces.data(), indexBufferSize);
  memcpy(stagingMemory.map + meshBufferSize, checkerboardTexture.data(), checkerboardTextureSize);

  // Copy to device memory
  vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
  commandBufferAllocateInfo
    .setLevel(vk::CommandBufferLevel::ePrimary)
    .setCommandPool(commandPool_)
    .setCommandBufferCount(1);
  const auto copyCommandBuffer = device_.allocateCommandBuffers(commandBufferAllocateInfo)[0];
  copyCommandBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

  vk::BufferCopy copyRegion;
  copyRegion
    .setSrcOffset(0)
    .setDstOffset(0)
    .setSize(meshBufferSize);
  copyCommandBuffer.copyBuffer(stagingBuffer_, meshBuffer_, copyRegion);

  vk::BufferMemoryBarrier bufferMemoryBarrier;
  bufferMemoryBarrier
    .setBuffer(meshBuffer_)
    .setOffset(0)
    .setSize(bufferSize)
    .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
    .setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eIndexRead)
    .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
    .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
  copyCommandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eVertexInput, {},
    {}, bufferMemoryBarrier, {});

  // Generate texture
  TextureCreateInfo textureCreateInfo;
  textureCreateInfo.device = device_;
  textureCreateInfo.width = checkerboardTextureLength;
  textureCreateInfo.height = checkerboardTextureLength;
  textureCreateInfo.mipLevel = mipLevel_;
  textureCreateInfo.pMemoryPool = &memoryPool_;
  textureCreateInfo.srcBuffer = stagingBuffer_;
  textureCreateInfo.srcOffset = checkerboardTextureOffset;
  textureCreateInfo.commandBuffer = copyCommandBuffer;
  texture_ = engine::createTexture(textureCreateInfo);

  copyCommandBuffer.end();

  vk::SubmitInfo submitInfo;
  submitInfo.setCommandBuffers(copyCommandBuffer);
  queue_.submit(submitInfo);

  // Create sampler
  SamplerCreateInfo samplerCreateInfo;
  samplerCreateInfo.device = device_;
  samplerCreateInfo.mipLevel = mipLevel_;
  sampler_ = engine::createSampler(samplerCreateInfo);
}

void Engine::destroyResources()
{
  mesh_ = nullptr;

  device_.destroyBuffer(meshBuffer_);
  device_.destroyBuffer(stagingBuffer_);
  texture_.destroy();
  sampler_.destroy();
}

void Engine::createCommandBuffers()
{
  const auto imageCount = swapchain_.getImageCount();

  vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
  commandBufferAllocateInfo
    .setLevel(vk::CommandBufferLevel::ePrimary)
    .setCommandPool(commandPool_)
    .setCommandBufferCount(imageCount);
  drawCommandBuffers_ = device_.allocateCommandBuffers(commandBufferAllocateInfo);

  // Transfer command buffer
  commandBufferAllocateInfo.setCommandBufferCount(1);
  transferCommandBuffer_ = device_.allocateCommandBuffers(commandBufferAllocateInfo)[0];
}

void Engine::destroyCommandBuffers()
{
  drawCommandBuffers_.clear();
}

void Engine::createSynchronizationObjects()
{
  const auto imageCount = swapchain_.getImageCount();

  for (int i = 0; i < imageCount; i++)
    imageAvailableSemaphores_.emplace_back(device_.createSemaphore({}));

  for (int i = 0; i < imageCount; i++)
    renderFinishedSemaphores_.emplace_back(device_.createSemaphore({}));

  for (int i = 0; i < imageCount; i++)
    renderFinishedFences_.emplace_back(device_.createFence({ vk::FenceCreateFlagBits::eSignaled }));
}

void Engine::destroySynchronizationObjects()
{
  for (auto semaphore : imageAvailableSemaphores_)
    device_.destroySemaphore(semaphore);
  imageAvailableSemaphores_.clear();

  for (auto semaphore : renderFinishedSemaphores_)
    device_.destroySemaphore(semaphore);
  renderFinishedSemaphores_.clear();

  for (auto fence : renderFinishedFences_)
    device_.destroyFence(fence);
  renderFinishedFences_.clear();
}

void Engine::recreateSwapchain()
{
  // TODO
}

void Engine::resize(uint32_t width, uint32_t height)
{
  width_ = width;
  height_ = height;

  // TODO: recreate relevant objects
}

void Engine::updateCamera(const CameraUbo& camera)
{
  renderer_.updateCamera(camera);
}

void Engine::updateLight(const LightUbo& light)
{
  vrWorker_.updateLight(light);
  renderer_.updateLight(light);
}

void Engine::startVr()
{
  VrWorkerRunInfo runInfo;
  runInfo.instance = instance_;
  runInfo.physicalDevice = physicalDevice_;
  runInfo.device = device_;
  runInfo.queue = vrQueue_;
  runInfo.queueIndex = queueIndex_;
  runInfo.pMemoryPool = &memoryPool_;
  runInfo.meshBuffer = meshBuffer_;
  runInfo.meshIndexCount = meshIndexCount_;
  runInfo.meshIndexOffset = meshIndexOffset_;
  runInfo.pTexture = &texture_;
  runInfo.pSampler = &sampler_;
  vrWorker_.run(runInfo);
}

void Engine::terminateVr()
{
  vrWorker_.terminate();
}

void Engine::drawFrame()
{
  constexpr auto scale = 0.5f;
  glm::mat4 objectModel{ 1.f };
  objectModel[0][0] = scale;
  objectModel[1][1] = scale;
  objectModel[2][2] = scale;

  const auto orientation = getObjectOrientation();
  objectModel = objectModel * glm::mat4{ orientation };

  const auto eyePoses = vrWorker_.getEyePoses();
  const glm::vec3 cameraPosition = (glm::vec3{ eyePoses[0][3] } + glm::vec3{ eyePoses[1][3] }) / 2.f;
  objectModel[3] = glm::vec4{ cameraPosition, 1.f };

  // Draw on window surface
  const auto frameIndex = frameIndex_ % 3;
  device_.waitForFences(renderFinishedFences_[frameIndex], true, UINT64_MAX);
  device_.resetFences(renderFinishedFences_[frameIndex]);

  const auto swapchain = swapchain_.getSwapchain();
  const auto acquireNextImageResult = device_.acquireNextImageKHR(swapchain, UINT64_MAX, imageAvailableSemaphores_[frameIndex]);
  if (acquireNextImageResult.result == vk::Result::eErrorOutOfDateKHR)
  {
    recreateSwapchain();
    return;
  }
  else if (acquireNextImageResult.result != vk::Result::eSuccess && acquireNextImageResult.result != vk::Result::eSuboptimalKHR)
    throw std::runtime_error("Failed to acquire next swapchain image");

  const auto imageIndex = acquireNextImageResult.value;

  // Update uniform
  renderer_.updateDescriptorSet(imageIndex);

  // Draw command
  auto drawCommandBuffer = drawCommandBuffers_[imageIndex];
  drawCommandBuffer.reset();
  drawCommandBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

  const auto framebuffer = framebuffer_.getFramebuffers()[imageIndex];

  vk::Rect2D renderArea{ {0u, 0u}, {width_, height_} };

  std::array<float, 4> clearColor = { 0.75f, 0.75f, 0.75f, 1.f };
  std::vector<vk::ClearValue> clearValues = {
    vk::ClearColorValue{clearColor},
    vk::ClearDepthStencilValue{1.f, 0u},
  };

  vk::RenderPassBeginInfo renderPassBeginInfo;
  renderPassBeginInfo
    .setClearValues(clearValues)
    .setRenderArea(renderArea)
    .setRenderPass(renderPass_)
    .setFramebuffer(framebuffer);
  drawCommandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

  // TODO: bind pipeline via renderer
  drawCommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, renderer_.getPipeline());

  vk::Viewport viewport{ 0.f, 0.f, static_cast<float>(width_), static_cast<float>(height_), 0.f, 1.f };

  drawCommandBuffer.setViewport(0, viewport);
  drawCommandBuffer.setScissor(0, renderArea);

  drawCommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
    renderer_.getPipelineLayout(), 0,
    renderer_.getDescriptorSets()[imageIndex], {});

  drawMesh(drawCommandBuffer, renderer_.getPipelineLayout(), objectModel);

  // Eyes: scale by sphere size
  for (int i = 0; i < 2; i++)
  {
    constexpr float scaleLong = 0.05f;
    constexpr float scaleShort = 0.025f;
    glm::mat4 scaledModel{ 1.f };
    scaledModel[0][0] = scaleLong;
    scaledModel[1][1] = scaleLong;
    scaledModel[2][2] = scaleShort;
    scaledModel = eyePoses[i] * scaledModel;
    drawMesh(drawCommandBuffer, renderer_.getPipelineLayout(), scaledModel);
  }

  drawCommandBuffer.endRenderPass();

  drawCommandBuffer.end();

  std::vector<vk::Semaphore> waitSemaphores = {
    imageAvailableSemaphores_[frameIndex],
  };
  std::vector<vk::PipelineStageFlags> waitMasks = {
    vk::PipelineStageFlagBits::eColorAttachmentOutput
  };

  vk::SubmitInfo submitInfo;
  submitInfo
    .setWaitSemaphores(waitSemaphores)
    .setWaitDstStageMask(waitMasks)
    .setCommandBuffers(drawCommandBuffer)
    .setSignalSemaphores(renderFinishedSemaphores_[imageIndex]);
  queue_.submit(submitInfo, renderFinishedFences_[frameIndex]);

  // Present
  vk::PresentInfoKHR presentInfo;
  presentInfo
    .setWaitSemaphores(renderFinishedSemaphores_[imageIndex])
    .setSwapchains(swapchain)
    .setImageIndices(imageIndex);
  const auto presentResult = presentQueue_.presentKHR(presentInfo);

  if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR)
    recreateSwapchain();
  else if (presentResult != vk::Result::eSuccess)
    throw std::runtime_error("Failed to present swapchain image");

  frameIndex_++;
}

glm::quat Engine::getObjectOrientation()
{
  std::lock_guard<std::mutex> guard{ objectOrientationMutex_ };
  return objectOrientation_;
}

void Engine::setObjectOrientation(const glm::quat& objectOrientation)
{
  std::lock_guard<std::mutex> guard{ objectOrientationMutex_ };
  objectOrientation_ = objectOrientation;
}

void Engine::drawMesh(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const glm::mat4& model)
{
  std::array<float, 16> modelArray;
  std::memcpy(&modelArray, glm::value_ptr(model), sizeof(modelArray));

  glm::mat4 modelInverseTranspose = glm::transpose(glm::inverse(model));
  std::array<float, 16> modelInverseTransposeArray;
  std::memcpy(&modelInverseTransposeArray, glm::value_ptr(modelInverseTranspose), sizeof(modelInverseTransposeArray));

  commandBuffer.pushConstants<float>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, modelArray);
  commandBuffer.pushConstants<float>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(modelArray), modelInverseTransposeArray);

  commandBuffer.bindVertexBuffers(0, { meshBuffer_ }, { 0 });
  commandBuffer.bindIndexBuffer(meshBuffer_, meshIndexOffset_, vk::IndexType::eUint32);
  commandBuffer.drawIndexed(meshIndexCount_, 1, 0, 0, 0);
}
}
}
