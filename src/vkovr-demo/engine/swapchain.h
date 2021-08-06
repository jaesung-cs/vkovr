#ifndef VKOVR_DEMO_ENGINE_SWAPCHAIN_H_
#define VKOVR_DEMO_ENGINE_SWAPCHAIN_H_

#include <array>

#include <vulkan/vulkan.hpp>

namespace demo
{
namespace engine
{
class Swapchain;
class SwapchainCreateInfo;

Swapchain createSwapchain(const SwapchainCreateInfo& createInfo);

class Swapchain
{
  friend Swapchain createSwapchain(const SwapchainCreateInfo& createInfo);

public:
  Swapchain();
  ~Swapchain();

  auto getSwapchain() const { return swapchain_; }
  auto getImageFormat() const { return swapchainImageFormat_; }
  const auto& getImages() const { return swapchainImages_; }
  const auto& getImageViews() const { return swapchainImageViews_; }
  auto getImageCount() const { return swapchainImageCount_; }

  void destroy();

private:
  vk::Device device_;

  uint32_t width_ = 0;
  uint32_t height_ = 0;

  vk::SwapchainKHR swapchain_;
  uint32_t swapchainImageCount_ = 0;
  vk::Format swapchainImageFormat_ = vk::Format::eUndefined;
  std::vector<vk::Image> swapchainImages_;
  std::vector<vk::ImageView> swapchainImageViews_;
};

class SwapchainCreateInfo
{
public:
  vk::Device device;
  vk::SurfaceKHR surface;
  vk::PhysicalDevice physicalDevice;
  uint32_t width;
  uint32_t height;
};
}
}

#endif // VKOVR_DEMO_ENGINE_SWAPCHAIN_H_
