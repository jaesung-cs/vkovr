#ifndef VKOVR_DEMO_ENGINE_ENGINE_H_
#define VKOVR_DEMO_ENGINE_ENGINE_H_

#include <vulkan/vulkan.hpp>

#include <glm/gtx/quaternion.hpp>

#include <vkovr/vkovr.hpp>

#include <vkovr-demo/engine/memory_pool.h>
#include <vkovr-demo/engine/swapchain.h>
#include <vkovr-demo/engine/render_pass.h>
#include <vkovr-demo/engine/framebuffer.h>
#include <vkovr-demo/engine/renderer.h>
#include <vkovr-demo/engine/texture.h>
#include <vkovr-demo/engine/sampler.h>
#include <vkovr-demo/engine/ubo/light_ubo.h>
#include <vkovr-demo/engine/vr_worker.h>
#include <vkovr-demo/scene/mesh.h>

struct GLFWwindow;

namespace demo
{
namespace engine
{
struct CameraUbo;
struct LightUbo;

class Engine
{
public:
  Engine() = delete;
  Engine(GLFWwindow* window, uint32_t width, uint32_t height);
  ~Engine();

  void resize(uint32_t width, uint32_t height);

  void updateCamera(const CameraUbo& camera);
  void updateLight(const LightUbo& light);

  glm::quat getObjectOrientation();
  void setObjectOrientation(const glm::quat& objectOrientation);

  void startVr();
  void terminateVr();

  void drawFrame();

private:
  void createInstance(GLFWwindow* window);
  void destroyInstance();

  void createDevice();
  void destroyDevice();

  void createResourcePools();
  void destroyResourcePools();

  void createSwapchain();
  void destroySwapchain();

  void createRenderPass();
  void destroyRenderPass();

  void createFramebuffer();
  void destroyFramebuffer();

  void createRenderer();
  void destroyRenderer();

  void prepareResources();
  void destroyResources();

  void createCommandBuffers();
  void destroyCommandBuffers();

  void createSynchronizationObjects();
  void destroySynchronizationObjects();

  void recreateSwapchain();

  void drawMesh(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const glm::mat4& model);

private:
  uint32_t width_ = 0;
  uint32_t height_ = 0;

  // Vr worker
  VrWorker vrWorker_;

  // Instance
  vk::Instance instance_;
  vk::DebugUtilsMessengerEXT messenger_;
  vk::SurfaceKHR surface_;

  // Device
  vk::PhysicalDevice physicalDevice_;
  vk::Device device_;
  uint32_t queueIndex_ = 0;
  vk::Queue queue_;
  vk::Queue vrQueue_;
  vk::Queue presentQueue_;

  vk::DeviceSize ssboAlignment_ = 0;
  vk::DeviceSize uboAlignment_ = 0;

  // Pools
  vk::CommandPool commandPool_;
  vk::DescriptorPool descriptorPool_;
  MemoryPool memoryPool_;

  // Swapchain
  Swapchain swapchain_;

  // Framebuffer
  RenderPass renderPass_;
  Framebuffer framebuffer_;

  // Renderer
  Renderer renderer_;

  // Meshes
  std::unique_ptr<scene::Mesh> mesh_;
  vk::Buffer meshBuffer_;
  vk::DeviceSize meshIndexOffset_ = 0;
  uint32_t meshIndexCount_ = 0;

  // VR object orientation
  std::mutex objectOrientationMutex_;
  glm::quat objectOrientation_{ 1.f, 0.f, 0.f, 0.f };

  // Texture
  Texture texture_;
  uint32_t mipLevel_ = 3;
  Sampler sampler_;

  // Staging buffer
  vk::Buffer stagingBuffer_;

  // Command buffers
  vk::CommandBuffer transferCommandBuffer_;
  std::vector<vk::CommandBuffer> drawCommandBuffers_;

  // Synchronization
  std::vector<vk::Semaphore> imageAvailableSemaphores_;
  std::vector<vk::Semaphore> renderFinishedSemaphores_;
  std::vector<vk::Fence> renderFinishedFences_;

  // Frame index
  uint64_t frameIndex_ = 0;
};
}
}

#endif // VKOVR_DEMO_ENGINE_ENGINE_H_
