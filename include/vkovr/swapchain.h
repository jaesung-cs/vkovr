#ifndef VKOVR_SWAPCHAIN_H_
#define VKOVR_SWAPCHAIN_H_

#include <vulkan/vulkan.hpp>

#include <OVR_CAPI.h>

#include <vkovr/session.h>

namespace vkovr
{
class Session;

class Swapchain
{
  friend class Session;

public:
  Swapchain();
  ~Swapchain();

  auto getColorSwapchain() const { return colorSwapchain_; }
  auto getDepthSwapchain() const { return depthSwapchain_; }
  const auto& getColorImages() const { return colorImages_; }
  const auto& getDepthImages() const { return depthImages_; }
  const auto& getColorImageViews() const { return colorImageViews_; }
  const auto& getDepthImageViews() const { return depthImageViews_; }
  const auto& getExtent() const { return extent_; }

  int acquireNextImageIndex();
  void commit();

  void destroy();

private:
  Session session_;
  vk::Device device_;
  vk::Extent2D extent_;
  ovrTextureSwapChain colorSwapchain_ = nullptr;
  ovrTextureSwapChain depthSwapchain_ = nullptr;
  std::vector<vk::Image> colorImages_;
  std::vector<vk::Image> depthImages_;
  std::vector<vk::ImageView> colorImageViews_;
  std::vector<vk::ImageView> depthImageViews_;
};

class SwapchainCreateInfo
{
public:
  vk::Device device;
  ovrEyeType eye;
};
}

#endif // VKOVR_SWAPCHAIN_H_
