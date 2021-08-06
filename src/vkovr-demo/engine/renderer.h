#ifndef VKOVR_DEMO_ENGINE_RENDERER_H_
#define VKOVR_DEMO_ENGINE_RENDERER_H_

#include <vulkan/vulkan.hpp>

#include <vkovr-demo/engine/ubo/camera_ubo.h>
#include <vkovr-demo/engine/ubo/light_ubo.h>

namespace demo
{
namespace engine
{
class MemoryPool;
class RenderPass;

class Renderer;
class RendererCreateInfo;

Renderer createRenderer(const RendererCreateInfo& createInfo);

class Renderer
{
  friend Renderer createRenderer(const RendererCreateInfo& createInfo);

public:
  Renderer();
  ~Renderer();

  auto getDescriptorSetLayout() const { return descriptorSetLayout_; }
  auto getPipeline() const { return pipeline_; }
  auto getPipelineLayout() const { return pipelineLayout_; }
  const auto& getDescriptorSets() const { return descriptorSets_; }

  void updateDescriptorSet(int imageIndex);
  void updateCamera(const CameraUbo& camera);
  void updateLight(const LightUbo& light);

  void destroy();

private:
  vk::Device device_;

  vk::DescriptorSetLayout descriptorSetLayout_;
  vk::PipelineLayout pipelineLayout_;
  vk::Pipeline pipeline_;
  std::vector<vk::DescriptorSet> descriptorSets_;

  vk::Buffer uniformBuffer_;
  uint8_t* uniformBufferMap_ = nullptr;
  vk::DeviceSize uniformBufferLightOffset_ = 0;
  vk::DeviceSize uniformBufferStride_ = 0;
  CameraUbo cameraUbo_;
  LightUbo lightUbo_;
};

class RendererCreateInfo
{
public:
  vk::Device device;
  vk::PhysicalDevice physicalDevice;
  vk::DescriptorPool descriptorPool;
  uint32_t imageCount;
  vk::ImageView textureImageView;
  vk::Sampler sampler;
  MemoryPool* pMemoryPool;
  RenderPass* pRenderPass;
};
}
}

#endif // VKOVR_DEMO_ENGINE_RENDERER_H_
