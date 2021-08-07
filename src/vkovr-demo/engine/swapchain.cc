#include <vkovr-demo/engine/swapchain.h>

namespace demo
{
namespace engine
{
Swapchain createSwapchain(const SwapchainCreateInfo& createInfo)
{
  const auto device = createInfo.device;
  const auto physicalDevice = createInfo.physicalDevice;
  const auto surface = createInfo.surface;
  const auto width = createInfo.width;
  const auto height = createInfo.height;

  const auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

  // Triple buffering
  auto imageCount = capabilities.minImageCount + 1;
  if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
    imageCount = capabilities.maxImageCount;

  if (imageCount != 3)
    throw std::runtime_error("Triple buffering is not supported");

  // Present mode: use mailbox if available. Limit fps in draw call
  vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
  const auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
  for (auto availableMode : presentModes)
  {
    if (availableMode == vk::PresentModeKHR::eMailbox)
    {
      presentMode = vk::PresentModeKHR::eMailbox;
      break;
    }
  }

  // Format
  const auto availableFormats = physicalDevice.getSurfaceFormatsKHR(surface);
  auto format = availableFormats[0];
  for (const auto& availableFormat : availableFormats)
  {
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
      availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
      format = availableFormat;
  }

  // Extent
  vk::Extent2D extent;
  if (capabilities.currentExtent.width != UINT32_MAX)
    extent = capabilities.currentExtent;
  else
  {
    VkExtent2D actualExtent = { width, height };

    actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

    extent = actualExtent;
  }

  // Create swapchain
  vk::SwapchainCreateInfoKHR swapchainCreateInfo;
  swapchainCreateInfo
    .setSurface(surface)
    .setMinImageCount(imageCount)
    .setImageFormat(format.format)
    .setImageColorSpace(format.colorSpace)
    .setImageExtent(extent)
    .setImageArrayLayers(1)
    .setImageUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eColorAttachment)
    .setImageSharingMode(vk::SharingMode::eExclusive)
    .setPreTransform(capabilities.currentTransform)
    .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
    .setPresentMode(presentMode)
    .setClipped(true);
  const auto swapchain = device.createSwapchainKHR(swapchainCreateInfo);

  const auto swapchainImageFormat = format.format;
  const auto swapchainImageCount = imageCount;

  const auto swapchainImages = device.getSwapchainImagesKHR(swapchain);

  // Create image view for swapchain
  std::vector<vk::ImageView> swapchainImageViews(swapchainImages.size());
  for (int i = 0; i < swapchainImages.size(); i++)
  {
    vk::ImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo
      .setImage(swapchainImages[i])
      .setViewType(vk::ImageViewType::e2D)
      .setFormat(swapchainImageFormat)
      .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

    swapchainImageViews[i] = device.createImageView(imageViewCreateInfo);
  }

  Swapchain resultSwapchain;
  resultSwapchain.device_ = device;
  resultSwapchain.width_ = width;
  resultSwapchain.height_ = height;
  resultSwapchain.swapchain_ = swapchain;
  resultSwapchain.swapchainImageCount_ = imageCount;
  resultSwapchain.swapchainImageFormat_ = swapchainImageFormat;
  resultSwapchain.swapchainImages_ = swapchainImages;
  resultSwapchain.swapchainImageViews_ = swapchainImageViews;
  return resultSwapchain;
}

Swapchain::Swapchain()
{
}

Swapchain::~Swapchain()
{
}

void Swapchain::destroy()
{
  for (auto& imageView : swapchainImageViews_)
    device_.destroyImageView(imageView);
  swapchainImageViews_.clear();

  device_.destroySwapchainKHR(swapchain_);
}
}
}
