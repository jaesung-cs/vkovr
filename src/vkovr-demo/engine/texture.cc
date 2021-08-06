#include <vkovr-demo/engine/texture.h>

#include <vkovr-demo/engine/memory_pool.h>

namespace demo
{
namespace engine
{
Texture createTexture(const TextureCreateInfo& createInfo)
{
  const auto device = createInfo.device;
  const auto width = createInfo.width;
  const auto height = createInfo.height;
  const auto srcBuffer = createInfo.srcBuffer;
  const auto srcOffset = createInfo.srcOffset;
  const auto mipLevel = createInfo.mipLevel;
  const auto commandBuffer = createInfo.commandBuffer;
  auto& memoryPool = createInfo.pMemoryPool;

  constexpr auto format = vk::Format::eR8G8B8A8Srgb;

  vk::ImageCreateInfo imageCreateInfo;
  imageCreateInfo
    .setImageType(vk::ImageType::e2D)
    .setFormat(format)
    .setExtent({ width, height, 1 })
    .setMipLevels(mipLevel)
    .setArrayLayers(1)
    .setSamples(vk::SampleCountFlagBits::e1)
    .setTiling(vk::ImageTiling::eOptimal)
    .setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled)
    .setSharingMode(vk::SharingMode::eExclusive)
    .setInitialLayout(vk::ImageLayout::eUndefined);
  auto image = device.createImage(imageCreateInfo);

  const auto imageMemory = memoryPool->allocateDeviceMemory(image);
  device.bindImageMemory(image, imageMemory.memory, imageMemory.offset);

  vk::ImageViewCreateInfo imageViewCreateInfo;
  imageViewCreateInfo
    .setViewType(vk::ImageViewType::e2D)
    .setImage(image)
    .setFormat(format)
    .setComponents({})
    .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, mipLevel, 0, 1 });
  auto imageView = device.createImageView(imageViewCreateInfo);

  // Transition to transfer dst
  vk::ImageMemoryBarrier barrier;
  barrier
    .setImage(image)
    .setOldLayout(vk::ImageLayout::eUndefined)
    .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
    .setSrcAccessMask({})
    .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
    .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
    .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
    .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, mipLevel, 0, 1 });
  commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {},
    {}, {}, barrier);

  // Copy to image
  vk::BufferImageCopy region;
  region
    .setBufferOffset(srcOffset)
    .setBufferRowLength(0)
    .setBufferImageHeight(0)
    .setImageOffset({ 0, 0, 0 })
    .setImageExtent({ width, height, 1 })
    .setImageSubresource({ vk::ImageAspectFlagBits::eColor, 0, 0, 1 });
  commandBuffer.copyBufferToImage(srcBuffer, image, vk::ImageLayout::eTransferDstOptimal, region);

  // Generate mipmap
  int32_t currentWidth = width;
  int32_t currentHeight = height;
  for (uint32_t i = 0; i + 1 < mipLevel; i++)
  {
    // Current level layout from transfer dst to transfer src
    barrier
      .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
      .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
      .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
      .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
      .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, i, 1, 0, 1 });
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {},
      {}, {}, barrier);

    // Blit to next level
    std::array<vk::Offset3D, 2> srcOffsets = {
      vk::Offset3D{0, 0, 0},
      vk::Offset3D{currentWidth, currentHeight, 1}
    };
    std::array<vk::Offset3D, 2> dstOffsets = {
      vk::Offset3D{0, 0, 0},
      vk::Offset3D{currentWidth / 2, currentHeight / 2, 1}
    };
    vk::ImageBlit blit;
    blit
      .setSrcOffsets(srcOffsets)
      .setSrcSubresource({ vk::ImageAspectFlagBits::eColor, i, 0, 1 })
      .setDstOffsets(dstOffsets)
      .setDstSubresource({ vk::ImageAspectFlagBits::eColor, i + 1, 0, 1 });
    commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image, vk::ImageLayout::eTransferDstOptimal,
      blit, vk::Filter::eLinear);

    currentWidth /= 2;
    currentHeight /= 2;

    // Layout from transfer src to shader read
    barrier
      .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
      .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
      .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
      .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
      .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, i, 1, 0, 1 });
    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {},
      {}, {}, barrier);
  }

  // Last level layout from transfer src to shader read
  barrier
    .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
    .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
    .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
    .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
    .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, mipLevel - 1, 1, 0, 1 });
  commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {},
    {}, {}, barrier);

  Texture result;
  result.device_ = device;
  result.mipLevel_ = mipLevel;
  result.image_ = image;
  result.imageView_ = imageView;
  return result;
}

Texture::Texture()
{
}

Texture::~Texture()
{
}

void Texture::destroy()
{
  device_.destroyImage(image_);
  device_.destroyImageView(imageView_);
}
}
}
