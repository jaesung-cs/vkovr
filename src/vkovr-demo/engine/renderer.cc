#include <vkovr-demo/engine/renderer.h>

#include <fstream>

#include <vkovr-demo/engine/memory_pool.h>
#include <vkovr-demo/engine/render_pass.h>

namespace demo
{
namespace engine
{
namespace
{
auto align(vk::DeviceSize offset, vk::DeviceSize alignment)
{
  return (offset + alignment - 1) & ~(alignment - 1);
}

vk::ShaderModule createShaderModule(vk::Device device, const std::string& filepath)
{
  std::ifstream file(filepath, std::ios::ate | std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error("Failed to open file: " + filepath);

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  std::vector<uint32_t> code;
  auto* intPtr = reinterpret_cast<uint32_t*>(buffer.data());
  for (int i = 0; i < fileSize / 4; i++)
    code.push_back(intPtr[i]);

  vk::ShaderModuleCreateInfo shaderModuleCreateInfo;
  shaderModuleCreateInfo.setCode(code);
  return device.createShaderModule(shaderModuleCreateInfo);
}
}

Renderer createRenderer(const RendererCreateInfo& createInfo)
{
  const auto device = createInfo.device;
  const auto physicalDevice = createInfo.physicalDevice;
  auto& renderPass = *createInfo.pRenderPass;
  const auto samples = renderPass.getSamples();
  const auto descriptorPool = createInfo.descriptorPool;
  const auto imageCount = createInfo.imageCount;
  const auto textureImageView = createInfo.textureImageView;
  const auto sampler = createInfo.sampler;
  auto& memoryPool = *createInfo.pMemoryPool;

  // Pipeline cache
  const auto pipelineCache = device.createPipelineCache({});

  // Descriptor set layout
  std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings(3);
  descriptorSetLayoutBindings[0]
    .setBinding(0)
    .setDescriptorType(vk::DescriptorType::eUniformBuffer)
    .setDescriptorCount(1)
    .setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

  descriptorSetLayoutBindings[1]
    .setBinding(1)
    .setDescriptorType(vk::DescriptorType::eUniformBuffer)
    .setDescriptorCount(1)
    .setStageFlags(vk::ShaderStageFlagBits::eFragment);

  descriptorSetLayoutBindings[2]
    .setBinding(2)
    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
    .setDescriptorCount(1)
    .setStageFlags(vk::ShaderStageFlagBits::eFragment);

  vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
  descriptorSetLayoutCreateInfo
    .setBindings(descriptorSetLayoutBindings);
  const auto descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

  // Uniform buffer
  const auto alignment = physicalDevice.getProperties().limits.minUniformBufferOffsetAlignment;
  const auto cameraSize = align(sizeof(CameraUbo), alignment);
  const auto lightSize = align(sizeof(LightUbo), alignment);
  const auto stride = cameraSize + lightSize;
  const auto lightOffset = cameraSize;
  const auto bufferSize = stride * imageCount;

  vk::BufferCreateInfo bufferCreateInfo;
  bufferCreateInfo
    .setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
    .setSize(bufferSize);
  const auto uniformBuffer = device.createBuffer(bufferCreateInfo);

  const auto uniformBufferMemory = memoryPool.allocatePersistentlyMappedMemory(uniformBuffer);
  device.bindBufferMemory(uniformBuffer, uniformBufferMemory.memory, uniformBufferMemory.offset);
  
  // Descriptor sets
  std::vector<vk::DescriptorSetLayout> setLayouts(imageCount, descriptorSetLayout);
  vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo;
  descriptorSetAllocateInfo
    .setDescriptorPool(descriptorPool)
    .setSetLayouts(setLayouts);
  const auto descriptorSets = device.allocateDescriptorSets(descriptorSetAllocateInfo);

  for (int i = 0; i < descriptorSets.size(); i++)
  {
    std::vector<vk::DescriptorBufferInfo> bufferInfos(2);
    bufferInfos[0]
      .setBuffer(uniformBuffer)
      .setOffset(stride * i)
      .setRange(cameraSize);

    bufferInfos[1]
      .setBuffer(uniformBuffer)
      .setOffset(stride * i + lightOffset)
      .setRange(lightSize);

    std::vector<vk::DescriptorImageInfo> imageInfos(1);
    imageInfos[0]
      .setImageView(textureImageView)
      .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
      .setSampler(sampler);

    std::vector<vk::WriteDescriptorSet> descriptorWrites(3);
    descriptorWrites[0]
      .setDstSet(descriptorSets[i])
      .setDstBinding(0)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setBufferInfo(bufferInfos[0]);

    descriptorWrites[1]
      .setDstSet(descriptorSets[i])
      .setDstBinding(1)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eUniformBuffer)
      .setBufferInfo(bufferInfos[1]);

    descriptorWrites[2]
      .setDstSet(descriptorSets[i])
      .setDstBinding(2)
      .setDstArrayElement(0)
      .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
      .setImageInfo(imageInfos[0]);

    device.updateDescriptorSets(descriptorWrites, {});
  }

  // Pipeline layout
  vk::PushConstantRange pushConstantRange;
  pushConstantRange
    .setStageFlags(vk::ShaderStageFlagBits::eVertex)
    .setOffset(0)
    .setSize(sizeof(float) * 16 + sizeof(float) * 16);

  vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
  pipelineLayoutCreateInfo
    .setSetLayouts(descriptorSetLayout)
    .setPushConstantRanges(pushConstantRange);
  const auto pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);

  // Shader modules
  const std::string baseDir = "C:\\workspace\\vkovr\\src\\vkovr-demo\\shader";
  vk::ShaderModule vertModule = createShaderModule(device, baseDir + "\\mesh.vert.spv");
  vk::ShaderModule fragModule = createShaderModule(device, baseDir + "\\mesh.frag.spv");

  // Shader stages
  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages(2);
  shaderStages[0]
    .setStage(vk::ShaderStageFlagBits::eVertex)
    .setModule(vertModule)
    .setPName("main");

  shaderStages[1]
    .setStage(vk::ShaderStageFlagBits::eFragment)
    .setModule(fragModule)
    .setPName("main");

  // Vertex input
  std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions(1);
  vertexBindingDescriptions[0]
    .setBinding(0)
    .setStride(sizeof(float) * 8)
    .setInputRate(vk::VertexInputRate::eVertex);

  std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions(3);
  vertexAttributeDescriptions[0]
    .setLocation(0)
    .setBinding(0)
    .setFormat(vk::Format::eR32G32B32Sfloat)
    .setOffset(0);

  vertexAttributeDescriptions[1]
    .setLocation(1)
    .setBinding(0)
    .setFormat(vk::Format::eR32G32B32Sfloat)
    .setOffset(sizeof(float) * 3);

  vertexAttributeDescriptions[2]
    .setLocation(2)
    .setBinding(0)
    .setFormat(vk::Format::eR32G32Sfloat)
    .setOffset(sizeof(float) * 6);

  vk::PipelineVertexInputStateCreateInfo vertexInputState;
  vertexInputState
    .setVertexBindingDescriptions(vertexBindingDescriptions)
    .setVertexAttributeDescriptions(vertexAttributeDescriptions);

  // Input assembly
  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState;
  inputAssemblyState
    .setTopology(vk::PrimitiveTopology::eTriangleList)
    .setPrimitiveRestartEnable(false);

  // Viewport (dynamic state)
  vk::Viewport viewport;
  viewport
    .setX(0.f)
    .setY(0.f)
    .setWidth(256.f)
    .setHeight(256.f)
    .setMinDepth(0.f)
    .setMaxDepth(1.f);

  vk::Rect2D scissor{ { 0u, 0u }, { 256u, 256u } };

  vk::PipelineViewportStateCreateInfo viewportState;
  viewportState
    .setViewports(viewport)
    .setScissors(scissor);

  // Rasterization
  vk::PipelineRasterizationStateCreateInfo rasterizationState;
  rasterizationState
    .setDepthClampEnable(false)
    .setRasterizerDiscardEnable(false)
    .setPolygonMode(vk::PolygonMode::eFill)
    .setCullMode(vk::CullModeFlagBits::eBack)
    .setFrontFace(vk::FrontFace::eCounterClockwise)
    .setDepthBiasEnable(false)
    .setLineWidth(1.f);

  // Multisample
  vk::PipelineMultisampleStateCreateInfo multisampleState;
  multisampleState
    .setRasterizationSamples(samples);

  // Depth stencil
  vk::PipelineDepthStencilStateCreateInfo depthStencilState;
  depthStencilState
    .setDepthTestEnable(true)
    .setDepthWriteEnable(true)
    .setDepthCompareOp(vk::CompareOp::eLess)
    .setDepthBoundsTestEnable(false)
    .setStencilTestEnable(false);

  // Color blend
  vk::PipelineColorBlendAttachmentState colorBlendAttachment;
  colorBlendAttachment
    .setBlendEnable(true)
    .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
    .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
    .setColorBlendOp(vk::BlendOp::eAdd)
    .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
    .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
    .setColorBlendOp(vk::BlendOp::eAdd)
    .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

  vk::PipelineColorBlendStateCreateInfo colorBlendState;
  colorBlendState
    .setLogicOpEnable(false)
    .setAttachments(colorBlendAttachment)
    .setBlendConstants({ 0.f, 0.f, 0.f, 0.f });

  // Dynamic states
  std::vector<vk::DynamicState> dynamicStates{
    vk::DynamicState::eViewport,
    vk::DynamicState::eScissor,
  };
  vk::PipelineDynamicStateCreateInfo dynamicState;
  dynamicState
    .setDynamicStates(dynamicStates);

  vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
  pipelineCreateInfo
    .setStages(shaderStages)
    .setPVertexInputState(&vertexInputState)
    .setPInputAssemblyState(&inputAssemblyState)
    .setPViewportState(&viewportState)
    .setPRasterizationState(&rasterizationState)
    .setPMultisampleState(&multisampleState)
    .setPDepthStencilState(&depthStencilState)
    .setPColorBlendState(&colorBlendState)
    .setPDynamicState(&dynamicState)
    .setLayout(pipelineLayout)
    .setRenderPass(renderPass)
    .setSubpass(0);
  const auto pipelineCreateResult = device.createGraphicsPipeline(pipelineCache, pipelineCreateInfo);
  // TODO: error handling
  if (pipelineCreateResult.result != vk::Result::eSuccess)
    throw std::runtime_error("Failed to create graphics pipeline");
  const auto pipeline = pipelineCreateResult.value;

  // Destroy local objects
  device.destroyPipelineCache(pipelineCache);
  device.destroyShaderModule(vertModule);
  device.destroyShaderModule(fragModule);

  Renderer renderer;
  renderer.device_ = device;
  renderer.descriptorSetLayout_ = descriptorSetLayout;
  renderer.uniformBuffer_ = uniformBuffer;
  renderer.uniformBufferMap_ = uniformBufferMemory.map;
  renderer.uniformBufferLightOffset_ = lightOffset;
  renderer.uniformBufferStride_ = stride;
  renderer.descriptorSets_ = descriptorSets;
  renderer.pipelineLayout_ = pipelineLayout;
  renderer.pipeline_ = pipeline;
  return renderer;
}

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::updateDescriptorSet(int imageIndex)
{
  std::memcpy(uniformBufferMap_ + uniformBufferStride_ * imageIndex, &cameraUbo_, sizeof(CameraUbo));
  std::memcpy(uniformBufferMap_ + uniformBufferStride_ * imageIndex + uniformBufferLightOffset_, &lightUbo_, sizeof(LightUbo));
}

void Renderer::updateCamera(const CameraUbo& camera)
{
  cameraUbo_ = camera;
}

void Renderer::updateLight(const LightUbo& light)
{
  lightUbo_ = light;
}

void Renderer::destroy()
{
  device_.destroyDescriptorSetLayout(descriptorSetLayout_);
  device_.destroyPipelineLayout(pipelineLayout_);
  device_.destroyPipeline(pipeline_);

  device_.destroyBuffer(uniformBuffer_);
  descriptorSets_.clear();
}
}
}
