#ifndef VKOVR_DEMO_VR_WORKER_H_
#define VKOVR_DEMO_VR_WORKER_H_

#include <vector>
#include <thread>
#include <mutex>

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

#include <vkovr/vkovr.hpp>

#include <vkovr-demo/engine/render_pass.h>
#include <vkovr-demo/engine/framebuffer.h>
#include <vkovr-demo/engine/renderer.h>
#include <vkovr-demo/engine/ubo/light_ubo.h>

namespace demo
{
namespace engine
{
class Engine;
class MemoryPool;
class Texture;
class Sampler;

class VrWorkerRunInfo;

class VrWorker
{
public:
  VrWorker() = delete;
  explicit VrWorker(Engine* engine);
  ~VrWorker();

  void updateLight(const LightUbo& light);

  std::array<glm::mat4, 2> getEyePoses();

  void run(const VrWorkerRunInfo& runInfo);
  void terminate();
  void join();

private:
  void loop();

  void destroy();

  void drawMesh(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const glm::mat4& model);

  Engine* const engine_;

  std::atomic_bool shouldTerminate_ = false;

  std::thread thread_;

  // Passed from parent thread
  vk::Instance instance_;
  vk::PhysicalDevice physicalDevice_;
  vk::Device device_;
  vk::Queue queue_;
  int queueIndex_ = 0;
  MemoryPool* pMemoryPool_;

  // Create by this thread
  vk::CommandPool commandPool_;
  vk::DescriptorPool descriptorPool_;

  // Ovr
  vkovr::Session session_;
  std::vector<vkovr::Swapchain> swapchains_;
  std::vector<vk::CommandBuffer> commandBuffers_;
  RenderPass renderPass_;
  std::vector<Framebuffer> framebuffers_;
  // TODO: create renderer using same pipeline cache
  Renderer renderer_;
  LightUbo light_;
  std::array<glm::mat4, 2> eyePoses_ = { glm::mat4{1.f}, glm::mat4{1.f} };

  // Mesh
  // TODO: use shared mesh with engine
  vk::Buffer meshBuffer_;
  vk::DeviceSize meshIndexOffset_ = 0;
  vk::DeviceSize meshIndexCount_ = 0;
  Texture* pTexture_ = nullptr;
  Sampler* pSampler_ = nullptr;

  // Synchronizations
  std::mutex lightMutex_;
  std::mutex eyeMutex_;
};

class VrWorkerRunInfo
{
public:
  vk::Instance instance;
  vk::PhysicalDevice physicalDevice;
  vk::Device device;
  vk::Queue queue;
  int queueIndex;
  MemoryPool* pMemoryPool;

  // Mesh
  vk::Buffer meshBuffer;
  vk::DeviceSize meshIndexOffset = 0;
  vk::DeviceSize meshIndexCount = 0;
  Texture* pTexture = nullptr;
  Sampler* pSampler = nullptr;
};
}
}

#endif // VKOVR_DEMO_VR_WORKER_H_
