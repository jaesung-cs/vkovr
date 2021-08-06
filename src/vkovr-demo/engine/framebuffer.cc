#include <vkovr-demo/engine/framebuffer.h>

#include <vkovr-demo/engine/memory_pool.h>
#include <vkovr-demo/engine/render_pass.h>

namespace demo
{
namespace engine
{
Framebuffer createFramebuffer(const FramebufferCreateInfo& createInfo)
{
  const auto device = createInfo.device;
  auto& renderPass = *createInfo.pRenderPass;
  const auto imageFormat = renderPass.getFormat();
  const auto depthFormat = renderPass.getDepthFormat();
  const auto samples = renderPass.getSamples();
  const auto& colorImageViews = createInfo.colorImageViews;
  const auto& depthImageViews = createInfo.depthImageViews;
  const auto imageCount = colorImageViews.size();
  const auto width = createInfo.width;
  const auto height = createInfo.height;
  const auto maxWidth = createInfo.maxWidth;
  const auto maxHeight = createInfo.maxHeight;
  auto& memoryPool = *createInfo.pMemoryPool;

  const auto multisampling = samples != vk::SampleCountFlagBits::e1;

  // Transient render targets
  vk::Image colorImage;
  vk::ImageView colorImageView;
  vk::Image depthImage;
  vk::ImageView depthImageView;
  if (multisampling)
  {
    vk::ImageCreateInfo imageCreateInfo;
    imageCreateInfo
      .setImageType(vk::ImageType::e2D)
      .setFormat(imageFormat)
      .setExtent(vk::Extent3D{ maxWidth, maxHeight, 1 })
      .setMipLevels(1)
      .setArrayLayers(1)
      .setSamples(samples)
      .setTiling(vk::ImageTiling::eOptimal)
      .setUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransientAttachment)
      .setSharingMode(vk::SharingMode::eExclusive)
      .setInitialLayout(vk::ImageLayout::eUndefined);
    const auto tempColorImage = device.createImage(imageCreateInfo);
    const auto colorMemory = memoryPool.allocateDeviceMemory(tempColorImage);
    device.destroyImage(tempColorImage);

    imageCreateInfo
      .setExtent(vk::Extent3D{ width, height, 1u });
    colorImage = device.createImage(imageCreateInfo);
    device.bindImageMemory(colorImage, colorMemory.memory, colorMemory.offset);

    vk::ImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo
      .setImage(colorImage)
      .setViewType(vk::ImageViewType::e2D)
      .setFormat(imageFormat)
      .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
    colorImageView = device.createImageView(imageViewCreateInfo);

    imageCreateInfo
      .setImageType(vk::ImageType::e2D)
      .setFormat(depthFormat)
      .setExtent(vk::Extent3D{ maxWidth, maxHeight, 1 })
      .setMipLevels(1)
      .setArrayLayers(1)
      .setSamples(samples)
      .setTiling(vk::ImageTiling::eOptimal)
      .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment)
      .setSharingMode(vk::SharingMode::eExclusive)
      .setInitialLayout(vk::ImageLayout::eUndefined);
    const auto tempDepthImage = device.createImage(imageCreateInfo);
    const auto depthMemory = memoryPool.allocateDeviceMemory(tempDepthImage);
    device.destroyImage(tempDepthImage);

    imageCreateInfo
      .setExtent(vk::Extent3D{ width, height, 1u });
    depthImage = device.createImage(imageCreateInfo);
    device.bindImageMemory(depthImage, depthMemory.memory, depthMemory.offset);

    imageViewCreateInfo
      .setImage(depthImage)
      .setViewType(vk::ImageViewType::e2D)
      .setFormat(depthFormat)
      .setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 });
    depthImageView = device.createImageView(imageViewCreateInfo);
  }
  
  // Framebuffers
  std::vector<vk::Framebuffer> framebuffers(imageCount);
  for (uint32_t i = 0; i < imageCount; i++)
  {
    std::vector<vk::ImageView> attachments;
    if (multisampling)
    {
      attachments.push_back(colorImageView);
      attachments.push_back(depthImageView);
      attachments.push_back(colorImageViews[i]);
    }
    else
    {
      attachments.push_back(colorImageViews[i]);
      attachments.push_back(depthImageViews[i]);
    }

    vk::FramebufferCreateInfo framebufferCreateInfo;
    framebufferCreateInfo
      .setRenderPass(renderPass)
      .setAttachments(attachments)
      .setWidth(width)
      .setHeight(height)
      .setLayers(1);
    framebuffers[i] = device.createFramebuffer(framebufferCreateInfo);
  }

  Framebuffer framebuffer;
  framebuffer.device_ = device;
  framebuffer.colorImage_ = colorImage;
  framebuffer.colorImageView_ = colorImageView;
  framebuffer.depthImage_ = depthImage;
  framebuffer.depthImageView_ = depthImageView;
  framebuffer.framebuffers_ = framebuffers;
  return framebuffer;
}

Framebuffer::Framebuffer()
{
}

Framebuffer::~Framebuffer()
{
}

void Framebuffer::destroy()
{
  if (colorImage_)
  {
    device_.destroyImage(colorImage_);
    device_.destroyImageView(colorImageView_);
  }

  if (depthImage_)
  {
    device_.destroyImage(depthImage_);
    device_.destroyImageView(depthImageView_);
  }

  for (auto framebuffer : framebuffers_)
    device_.destroyFramebuffer(framebuffer);
  framebuffers_.clear();
}
}
}