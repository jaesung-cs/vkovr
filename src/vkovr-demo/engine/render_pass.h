#ifndef VKOVR_DEMO_RENDER_PASS_H_
#define VKOVR_DEMO_RENDER_PASS_H_

#include <vulkan/vulkan.hpp>

namespace demo
{
namespace engine
{
class RenderPass;
class RenderPassCreateInfo;

RenderPass createRenderPass(const RenderPassCreateInfo& createInfo);

class RenderPass
{
  friend RenderPass createRenderPass(const RenderPassCreateInfo& createInfo);;

public:
  RenderPass();
  ~RenderPass();

  operator vk::RenderPass () const { return renderPass_; }

  auto getFormat() const { return format_; }
  auto getDepthFormat() const { return depthFormat_; }
  auto getSamples() const { return samples_; }

  void destroy();

private:
  vk::Device device_;

  vk::Format format_;
  vk::Format depthFormat_;
  vk::SampleCountFlagBits samples_;
  vk::ImageLayout finalLayout_;
  vk::RenderPass renderPass_;
};

class RenderPassCreateInfo
{
public:
  vk::Device device;
  vk::Format format;
  vk::SampleCountFlagBits samples;
  vk::ImageLayout finalLayout;
};
}
}

#endif // VKOVR_DEMO_RENDER_PASS_H_
