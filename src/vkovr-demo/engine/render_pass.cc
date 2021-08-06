#include <vkovr-demo/engine/render_pass.h>

namespace demo
{
namespace engine
{
RenderPass createRenderPass(const RenderPassCreateInfo& createInfo)
{
  const auto device = createInfo.device;
  const auto format = createInfo.format;
  const auto samples = createInfo.samples;
  const auto finalLayout = createInfo.finalLayout;

  std::vector<vk::AttachmentReference> attachmentReferences;
  std::vector<vk::AttachmentDescription> attachments;
  std::vector<vk::SubpassDescription> subpasses;

  const auto multisampling = samples != vk::SampleCountFlagBits::e1;
  if (multisampling)
  {
    // Multisampling requires resolve
    attachments.resize(3);
    attachments[0]
      .setFormat(format)
      .setSamples(samples)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

    attachments[1]
      .setFormat(vk::Format::eD24UnormS8Uint)
      .setSamples(samples)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    attachments[2]
      .setFormat(format)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(finalLayout);

    attachmentReferences.resize(3);
    attachmentReferences[0]
      .setAttachment(0)
      .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    attachmentReferences[1]
      .setAttachment(1)
      .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    attachmentReferences[2]
      .setAttachment(2)
      .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    subpasses.resize(1);
    subpasses[0]
      .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
      .setColorAttachments(attachmentReferences[0])
      .setPDepthStencilAttachment(&attachmentReferences[1])
      .setResolveAttachments(attachmentReferences[2]);
  }
  else
  {
    // Don't need resolve
    attachments.resize(2);
    attachments[0]
      .setFormat(format)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(finalLayout);

    attachments[1]
      .setFormat(vk::Format::eD24UnormS8Uint)
      .setSamples(vk::SampleCountFlagBits::e1)
      .setLoadOp(vk::AttachmentLoadOp::eClear)
      .setStoreOp(vk::AttachmentStoreOp::eStore)
      .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
      .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
      .setInitialLayout(vk::ImageLayout::eUndefined)
      .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    attachmentReferences.resize(2);
    attachmentReferences[0]
      .setAttachment(0)
      .setLayout(finalLayout);

    attachmentReferences[1]
      .setAttachment(1)
      .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    subpasses.resize(1);
    subpasses[0]
      .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
      .setColorAttachments(attachmentReferences[0])
      .setPDepthStencilAttachment(&attachmentReferences[1]);
  }

  // Dependencies
  std::vector<vk::SubpassDependency> dependencies(1);
  dependencies[0]
    .setSrcSubpass(VK_SUBPASS_EXTERNAL)
    .setDstSubpass(0)
    .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
    .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests)
    .setSrcAccessMask({})
    .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite);

  // Render pass
  vk::RenderPassCreateInfo renderPassCreateInfo;
  renderPassCreateInfo
    .setAttachments(attachments)
    .setSubpasses(subpasses)
    .setDependencies(dependencies);
  const auto renderPass = device.createRenderPass(renderPassCreateInfo);

  RenderPass result;
  result.device_ = device;
  result.format_ = format;
  result.samples_ = samples;
  result.finalLayout_ = finalLayout;
  result.renderPass_ = renderPass;
  return result;
}

RenderPass::RenderPass()
{
}

RenderPass::~RenderPass()
{
}

void RenderPass::destroy()
{
  device_.destroyRenderPass(renderPass_);
}
}
}
