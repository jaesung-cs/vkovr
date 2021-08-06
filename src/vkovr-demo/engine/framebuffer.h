#ifndef VKOVR_DEMO_ENGINE_FRAMEBUFFER_H_
#define VKOVR_DEMO_ENGINE_FRAMEBUFFER_H_

#include <vulkan/vulkan.hpp>

namespace demo
{
namespace engine
{
class MemoryPool;
class RenderPass;

class Framebuffer;
class FramebufferCreateInfo;

Framebuffer createFramebuffer(const FramebufferCreateInfo& createInfo);

class Framebuffer
{
public:
  friend Framebuffer createFramebuffer(const FramebufferCreateInfo& createInfo);

  Framebuffer();
  ~Framebuffer();

  auto getFramebuffers() const { return framebuffers_; }

  void destroy();

private:
  vk::Device device_;

  // Pipeline
  std::vector<vk::Framebuffer> framebuffers_;
  vk::Image colorImage_;
  vk::ImageView colorImageView_;
  vk::Image depthImage_;
  vk::ImageView depthImageView_;
};

class FramebufferCreateInfo
{
public:
  vk::Device device;
  std::vector<vk::ImageView> colorImageViews;
  std::vector<vk::ImageView> depthImageViews;
  uint32_t width;
  uint32_t height;
  uint32_t maxWidth;
  uint32_t maxHeight;
  RenderPass* pRenderPass = nullptr;
  MemoryPool* pMemoryPool = nullptr;
};
}
}

#endif // VKOVR_DEMO_ENGINE_FRAMEBUFFER_H_
