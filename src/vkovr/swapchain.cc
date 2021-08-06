#include <vkovr/swapchain.h>

#include <iostream>

namespace vkovr
{
Swapchain::Swapchain()
{
}

Swapchain::~Swapchain()
{
}

int Swapchain::acquireNextImageIndex()
{
  int index;
  if (!OVR_SUCCESS(ovr_GetTextureSwapChainCurrentIndex(session_, colorSwapchain_, &index)))
    throw std::runtime_error("Failed to get texture swapchain current index");
  return index;
}

void Swapchain::commit()
{
  if (!OVR_SUCCESS(ovr_CommitTextureSwapChain(session_, colorSwapchain_)))
    throw std::runtime_error("Failed to commit ovr color swapchain");

  if (!OVR_SUCCESS(ovr_CommitTextureSwapChain(session_, depthSwapchain_)))
    throw std::runtime_error("Failed to commit ovr depth swapchain");
}

void Swapchain::destroy()
{
  ovr_DestroyTextureSwapChain(session_, colorSwapchain_);
  ovr_DestroyTextureSwapChain(session_, depthSwapchain_);

  for (auto imageView : colorImageViews_)
    device_.destroyImageView(imageView);
  colorImageViews_.clear();

  for (auto imageView : depthImageViews_)
    device_.destroyImageView(imageView);
  depthImageViews_.clear();

  colorSwapchain_ = nullptr;
  depthSwapchain_ = nullptr;
}
}
