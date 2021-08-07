#include <vkovr-demo/engine/vr_worker.h>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <chrono>
#include <queue>

#include <vkovr-demo/engine/engine.h>
#include <vkovr-demo/engine/memory_pool.h>
#include <vkovr-demo/engine/texture.h>
#include <vkovr-demo/engine/sampler.h>

namespace demo
{
namespace engine
{
VrWorker::VrWorker(Engine* engine)
  : engine_{ engine }
{
}

VrWorker::~VrWorker()
{
}

void VrWorker::join()
{
  terminate();
  thread_.join();
}

void VrWorker::updateLight(const LightUbo& light)
{
  std::lock_guard<std::mutex> guard{ lightMutex_ };
  light_ = light;
}

std::array<glm::mat4, 2> VrWorker::getEyePoses()
{
  std::lock_guard<std::mutex> guard{ eyeMutex_ };
  return eyePoses_;
}

void VrWorker::run(const VrWorkerRunInfo& runInfo)
{
  instance_ = runInfo.instance;
  physicalDevice_ = runInfo.physicalDevice;
  device_ = runInfo.device;
  pMemoryPool_ = runInfo.pMemoryPool;
  queue_ = runInfo.queue;
  queueIndex_ = runInfo.queueIndex;

  meshBuffer_ = runInfo.meshBuffer;
  meshIndexOffset_ = runInfo.meshIndexOffset;
  meshIndexCount_ = runInfo.meshIndexCount;
  pTexture_ = runInfo.pTexture;
  pSampler_ = runInfo.pSampler;

  thread_ = std::thread([&] { loop(); });
}

void VrWorker::loop()
{
  // Command pool
  vk::CommandPoolCreateInfo commandPoolCreateInfo;
  commandPoolCreateInfo
    .setQueueFamilyIndex(queueIndex_)
    .setFlags(vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
  commandPool_ = device_.createCommandPool(commandPoolCreateInfo);

  // Descriptor pool
  constexpr auto descriptorCount = 100;
  constexpr auto setCount = 100;
  std::vector<vk::DescriptorPoolSize> poolSizes = {
    {vk::DescriptorType::eUniformBuffer, descriptorCount},
    {vk::DescriptorType::eStorageBuffer, descriptorCount},
    {vk::DescriptorType::eCombinedImageSampler, descriptorCount},
  };

  // VR command buffers
  constexpr auto imageCount = 3; // TODO: is it fixed?
  vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
  commandBufferAllocateInfo
    .setLevel(vk::CommandBufferLevel::ePrimary)
    .setCommandPool(commandPool_)
    .setCommandBufferCount(imageCount);
  commandBuffers_ = device_.allocateCommandBuffers(commandBufferAllocateInfo);

  vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo;
  descriptorPoolCreateInfo
    .setMaxSets(setCount)
    .setPoolSizes(poolSizes);
  descriptorPool_ = device_.createDescriptorPool(descriptorPoolCreateInfo);

  using namespace std::chrono_literals;
  using Clock = std::chrono::high_resolution_clock;
  using Duration = std::chrono::duration<double>;
  using Timestamp = Clock::time_point;

  std::deque<Timestamp> deque;
  int64_t recentSeconds = 0;
  const auto startTime = Clock::now();
  Timestamp previousTime = startTime;

  while (!shouldTerminate_)
  {
    const auto currentTime = Clock::now();

    constexpr auto scale = 0.5f;
    glm::mat4 objectModel{ 1.f };
    objectModel[0][0] = scale;
    objectModel[1][1] = scale;
    objectModel[2][2] = scale;

    // Check ovr session
    if (session_.opened() && session_.getStatus().ShouldQuit)
    {
      for (auto& swapchain : swapchains_)
        swapchain.destroy();
      swapchains_.clear();

      renderPass_.destroy();

      for (auto& framebuffer : framebuffers_)
        framebuffer.destroy();
      framebuffers_.clear();

      renderer_.destroy();

      session_.destroy();
    }

    // Create session if not opened
    bool sessionCreated = false;
    if (!session_.opened())
    {
      // TODO: try open ovr session in thread. It takes about 0.5s to receive response on failure
      try
      {
        session_ = vkovr::createSession({});
        sessionCreated = true;

        // Check if physical device needs to be changed
        const auto vrPhysicalDevice = session_.getPhysicalDevice(instance_);
        if (vrPhysicalDevice != physicalDevice_)
          throw std::runtime_error("VR device requires another GPU");

        // OVR render pass
        RenderPassCreateInfo renderPassCreateInfo;
        renderPassCreateInfo.device = device_;
        renderPassCreateInfo.format = vk::Format::eB8G8R8A8Srgb;
        renderPassCreateInfo.samples = vk::SampleCountFlagBits::e4;
        renderPassCreateInfo.finalLayout = vk::ImageLayout::ePresentSrcKHR;
        renderPass_ = engine::createRenderPass(renderPassCreateInfo);

        // OVR swapchains, framebuffers and renderers
        // TODO: use the same pipeline, bind different render pass
        swapchains_.resize(ovrEye_Count);
        framebuffers_.resize(ovrEye_Count);
        for (const auto eye : { ovrEye_Left, ovrEye_Right })
        {
          vkovr::SwapchainCreateInfo swapchainCreateInfo;
          swapchainCreateInfo.device = device_;
          swapchainCreateInfo.eye = eye;
          swapchains_[eye] = session_.createSwapchain(swapchainCreateInfo);

          // Create framebuffers targeting swapchain images
          const auto& extent = swapchains_[eye].getExtent();
          FramebufferCreateInfo framebufferCreateInfo;
          framebufferCreateInfo.device = device_;
          framebufferCreateInfo.width = extent.width;
          framebufferCreateInfo.height = extent.height;
          framebufferCreateInfo.maxWidth = extent.width;
          framebufferCreateInfo.maxHeight = extent.height;
          framebufferCreateInfo.colorImageViews = swapchains_[eye].getColorImageViews();
          framebufferCreateInfo.depthImageViews = swapchains_[eye].getDepthImageViews();
          framebufferCreateInfo.pMemoryPool = pMemoryPool_;
          framebufferCreateInfo.pRenderPass = &renderPass_;
          framebuffers_[eye] = engine::createFramebuffer(framebufferCreateInfo);
        }

        // OVR renderer
        // TODO: swapchain image count from left eye
        const auto imageCount = swapchains_[0].getColorImageViews().size();
        RendererCreateInfo rendererCreateInfo;
        rendererCreateInfo.device = device_;
        rendererCreateInfo.physicalDevice = physicalDevice_;
        rendererCreateInfo.pRenderPass = &renderPass_;
        rendererCreateInfo.descriptorPool = descriptorPool_;
        rendererCreateInfo.imageCount = imageCount * 2; // For left/right eye descriptor sets
        rendererCreateInfo.textureImageView = pTexture_->getImageView();
        rendererCreateInfo.sampler = *pSampler_;
        rendererCreateInfo.pMemoryPool = pMemoryPool_;
        renderer_ = engine::createRenderer(rendererCreateInfo);

        session_.synchronizeWithQueue(queue_);
      }
      catch (const std::exception& e)
      {
        std::cerr << e.what() << std::endl;
      }
    }

    if (session_.opened())
    {
      session_.beginFrame();

      LightUbo light;
      {
        std::lock_guard<std::mutex> guard{ lightMutex_ };
        light = light_;
      }

      // Y-up to Z-up
      glm::mat4 coordinateSystem{ 0.f };
      coordinateSystem[0][0] = 1.f;
      coordinateSystem[1][2] = 1.f;
      coordinateSystem[2][1] = -1.f;
      coordinateSystem[3][3] = 1.f;

      glm::quat coordinateSystemOrientation{ coordinateSystem };

      const auto eyePoses = session_.getEyePoses();
      {
        std::lock_guard<std::mutex> guard{ eyeMutex_ };
        for (int i = 0; i < 2; i++)
        {
          const auto& ovrQ = eyePoses[i].Orientation;
          const auto& ovrP = eyePoses[i].Position;

          glm::quat q{ ovrQ.w, ovrQ.x, ovrQ.y, ovrQ.z };
          const glm::vec3 p{ ovrP.x, ovrP.y, ovrP.z };

          eyePoses_[i] = glm::mat3{ q };
          eyePoses_[i][3] = { p, 1.f };
          eyePoses_[i] = coordinateSystem * eyePoses_[i];
        }
      }

      const auto status = session_.getStatus();
      if (status.HasInputFocus)
      {
        const auto input = session_.getInputState();

        // Average eye quaternions
        glm::quat qs{ 0.f, 0.f, 0.f, 0.f };
        glm::vec3 ps{ 0.f, 0.f, 0.f };
        for (const auto eye : { ovrEye_Left, ovrEye_Right })
        {
          const auto& ovrQuat = eyePoses[eye].Orientation;
          glm::quat q{ ovrQuat.w, ovrQuat.x, ovrQuat.y, ovrQuat.z };
          q = coordinateSystemOrientation * q;
          qs += q;

          const auto& ovrPosition = eyePoses[eye].Position;
          glm::vec3 p{ ovrPosition.x, ovrPosition.y, ovrPosition.z };
          p = coordinateSystem * glm::vec4{ p, 1.f };
          ps += p;
        }
        qs = glm::normalize(qs);
        ps = ps / 2.f;

        auto objectOrientation = engine_->getObjectOrientation();

        constexpr float rotationSpeed = 3.f;
        glm::vec3 v = qs * glm::vec3{ input.Thumbstick[1].x, input.Thumbstick[1].y, 0.f } * rotationSpeed;
        glm::vec3 z = qs * glm::vec3{ 0.f, 0.f, 1.f };
        glm::vec3 w{ glm::cross(z, v) };
        glm::quat dq = 0.5f * glm::quat{ 0.f, w } * objectOrientation;
        constexpr float dt = 1.f / 72.f; // TODO
        static float animationTime = 0.f;
        animationTime += dt;
        objectOrientation = glm::normalize(objectOrientation + dq * dt);

        engine_->setObjectOrientation(objectOrientation);
        
        objectModel = objectModel * glm::mat4{ objectOrientation };
        objectModel[3][0] = ps.x;
        objectModel[3][1] = ps.y + 2.f;
        objectModel[3][2] = ps.z - 2.f + std::sin(animationTime * 2.f) * 1.f;
      }

      if (status.IsVisible)
      {
        const auto vrFrameIndex = session_.getFrameIndex() % 3;

        // Draw to vr device
        auto& commandBuffer = commandBuffers_[vrFrameIndex];

        commandBuffer.reset();
        commandBuffer.begin({ vk::CommandBufferUsageFlagBits::eOneTimeSubmit });

        constexpr auto near = 0.2f;
        constexpr auto far = 1000.f;
        for (const auto eye : { ovrEye_Left, ovrEye_Right })
        {
          const auto imageIndex = swapchains_[eye].acquireNextImageIndex();

          auto projection = session_.getEyeProjection(eye, near, far);

          CameraUbo camera;
          for (int r = 0; r < 4; r++)
          {
            for (int c = 0; c < 4; c++)
              camera.projection[c][r] = projection.M[r][c];
          }

          // Convert to Vulkan coordinate space
          for (int c = 0; c < 4; c++)
            camera.projection[c][1] *= -1.f;

          const auto& ovrQuat = eyePoses[eye].Orientation;
          glm::quat q{ ovrQuat.w, ovrQuat.x, ovrQuat.y, ovrQuat.z };
          glm::mat3 rot{ q };

          // -Z direction is the forward direction
          const glm::vec3 forward = glm::mat3{ coordinateSystem } * -rot[2];
          const glm::vec3 up = glm::mat3{ coordinateSystem } * rot[1];

          const auto& eyePosition = eyePoses[eye].Position;
          auto eyeVec = glm::vec3{ eyePosition.x, eyePosition.y, eyePosition.z };
          camera.eye = glm::vec3{ coordinateSystem * glm::vec4{eyeVec, 1.f} };
          camera.view = glm::lookAt(camera.eye, camera.eye + forward, up);

          // Update uniforms
          renderer_.updateCamera(camera);
          renderer_.updateLight(light);
          renderer_.updateDescriptorSet(imageIndex * 2 + eye);

          // Draw
          const auto& framebuffer = framebuffers_[eye];
          const auto& extent = swapchains_[eye].getExtent();

          vk::Rect2D renderArea{ {0u, 0u}, {extent.width, extent.height} };

          constexpr float dt = 1.f / 72.f; // TODO
          static float animationTime = 0.f;
          animationTime += dt;

          std::array<float, 4> clearColor = { 0.75f, 0.75f, 0.75f, 1.f };
          std::vector<vk::ClearValue> clearValues = {
            vk::ClearColorValue{clearColor},
            vk::ClearDepthStencilValue{1.f, 0u},
          };

          vk::RenderPassBeginInfo renderPassBeginInfo;
          renderPassBeginInfo
            .setClearValues(clearValues)
            .setRenderArea(renderArea)
            .setRenderPass(renderPass_)
            .setFramebuffer(framebuffer.getFramebuffers()[imageIndex]);
          commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

          commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, renderer_.getPipeline());

          vk::Viewport viewport{ 0.f, 0.f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.f, 1.f };
          commandBuffer.setViewport(0, viewport);
          commandBuffer.setScissor(0, renderArea);

          commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
            renderer_.getPipelineLayout(), 0,
            renderer_.getDescriptorSets()[imageIndex * 2 + eye], {});

          auto copyModel = objectModel;
          for (int i = -5; i < 5; i++)
          {
            for (int j = 0; j < 10; j++)
            {
              copyModel[3].x = objectModel[3].x + i;
              copyModel[3].y = objectModel[3].y + j;
              drawMesh(commandBuffer, renderer_.getPipelineLayout(), copyModel);
            }
          }

          commandBuffer.endRenderPass();
        }

        commandBuffer.end();

        // Submit command buffers
        vk::SubmitInfo submitInfo;
        submitInfo
          .setCommandBuffers(commandBuffer);
        queue_.submit(submitInfo);

        for (auto& swapchain : swapchains_)
          swapchain.commit();
      }

      session_.endFrame(swapchains_);

      if (status.ShouldRecenter)
        session_.recenter();
    }

    else
    {
      // If session is not opened
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1s);
    }

    while (!deque.empty() && deque.front() < currentTime - 1s)
      deque.pop_front();
    deque.push_back(currentTime);

    const auto seconds = static_cast<int64_t>(Duration(Clock::now() - startTime).count());
    if (seconds > recentSeconds)
    {
      const auto fps = deque.size();
      std::cout << "VR worker: " << fps << std::endl;

      recentSeconds = seconds;
    }

    previousTime = currentTime;

    // TODO: need a short delay to remove flickering with two rendering thread. What is the best sleep duration?
    std::this_thread::sleep_for(0.005s);
  }

  destroy();
}

void VrWorker::destroy()
{
  commandBuffers_.clear();
  device_.destroyCommandPool(commandPool_);
  device_.destroyDescriptorPool(descriptorPool_);

  // Close session
  if (session_.opened())
  {
    for (auto& swapchain : swapchains_)
      swapchain.destroy();
    swapchains_.clear();

    renderPass_.destroy();

    for (auto& framebuffer : framebuffers_)
      framebuffer.destroy();
    framebuffers_.clear();

    renderer_.destroy();

    session_.destroy();
  }
}

void VrWorker::terminate()
{
  shouldTerminate_ = true;
}

void VrWorker::drawMesh(vk::CommandBuffer commandBuffer, vk::PipelineLayout pipelineLayout, const glm::mat4& model)
{
  std::array<float, 16> modelArray;
  std::memcpy(&modelArray, glm::value_ptr(model), sizeof(modelArray));

  glm::mat4 modelInverseTranspose = glm::transpose(glm::inverse(model));
  std::array<float, 16> modelInverseTransposeArray;
  std::memcpy(&modelInverseTransposeArray, glm::value_ptr(modelInverseTranspose), sizeof(modelInverseTransposeArray));

  commandBuffer.pushConstants<float>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, modelArray);
  commandBuffer.pushConstants<float>(pipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(modelArray), modelInverseTransposeArray);

  commandBuffer.bindVertexBuffers(0, { meshBuffer_ }, { 0 });
  commandBuffer.bindIndexBuffer(meshBuffer_, meshIndexOffset_, vk::IndexType::eUint32);
  commandBuffer.drawIndexed(meshIndexCount_, 1, 0, 0, 0);
}
}
}
