#include <vkovr/session.h>

#include <iostream>

#include <OVR_CAPI.h>
#include <OVR_CAPI_Vk.h>

#include <vkovr/swapchain.h>

namespace vkovr
{
Session::Session()
{
}

Session::~Session() = default;

Session createSession(const SessionCreateInfo& createInfo)
{
  // Populate session info
  Session session;
  if (!OVR_SUCCESS(ovr_Create(&session.session_, &session.luid_)))
    throw std::runtime_error("Failed to create ovr session");
  session.hmdDesc_ = ovr_GetHmdDesc(session.session_);

  // FloorLevel will give tracking poses where the floor height is 0
  if (!OVR_SUCCESS(ovr_SetTrackingOriginType(session.session_, ovrTrackingOrigin_FloorLevel)))
    throw std::runtime_error("Failed to set tracking origin type: floor");

  return session;
}

Swapchain Session::createSwapchain(const SwapchainCreateInfo& createInfo)
{
  const auto eye = createInfo.eye;
  const auto device = createInfo.device;

  const auto ovrExtent = ovr_GetFovTextureSize(session_, eye, hmdDesc_.DefaultEyeFov[eye], 1);
  const auto extent = vk::Extent2D(ovrExtent.w, ovrExtent.h);

  // Color
  ovrTextureSwapChainDesc colorDesc = {};
  colorDesc.Type = ovrTexture_2D;
  colorDesc.ArraySize = 1;
  colorDesc.Format = OVR_FORMAT_B8G8R8A8_UNORM_SRGB;
  colorDesc.Width = extent.width;
  colorDesc.Height = extent.height;
  colorDesc.MipLevels = 1;
  colorDesc.SampleCount = 1;
  colorDesc.MiscFlags = ovrTextureMisc_DX_Typeless;
  colorDesc.BindFlags = ovrTextureBind_DX_RenderTarget;
  colorDesc.StaticImage = ovrFalse;

  ovrTextureSwapChain colorSwapchain;
  if (!OVR_SUCCESS(ovr_CreateTextureSwapChainVk(session_, device, &colorDesc, &colorSwapchain)))
    throw std::runtime_error("Failed to create ovr color swapchain");

  // Depth
  ovrTextureSwapChainDesc depthDesc = {};
  depthDesc.Type = ovrTexture_2D;
  depthDesc.ArraySize = 1;
  depthDesc.Format = OVR_FORMAT_D24_UNORM_S8_UINT;
  depthDesc.Width = extent.width;
  depthDesc.Height = extent.height;
  depthDesc.MipLevels = 1;
  depthDesc.SampleCount = 1;
  depthDesc.MiscFlags = ovrTextureMisc_DX_Typeless;
  depthDesc.BindFlags = ovrTextureBind_DX_DepthStencil;
  depthDesc.StaticImage = ovrFalse;

  ovrTextureSwapChain depthSwapchain;
  if (!OVR_SUCCESS(ovr_CreateTextureSwapChainVk(session_, device, &depthDesc, &depthSwapchain)))
    throw std::runtime_error("Failed to create ovr depth swapchain");

  int colorImageCount;
  if (!OVR_SUCCESS(ovr_GetTextureSwapChainLength(session_, colorSwapchain, &colorImageCount)))
    throw std::runtime_error("Failed to get color swapchain length");

  int depthImageCount;
  if (!OVR_SUCCESS(ovr_GetTextureSwapChainLength(session_, depthSwapchain, &depthImageCount)))
    throw std::runtime_error("Failed to get depth swapchain length");

  if (colorImageCount != depthImageCount)
    throw std::runtime_error("Assert: colorImageCount != depthImageCount");

  std::vector<vk::Image> colorImages;
  std::vector<vk::Image> depthImages;
  for (int i = 0; i < colorImageCount; i++)
  {
    VkImage colorImage;
    if (!OVR_SUCCESS(ovr_GetTextureSwapChainBufferVk(session_, colorSwapchain, i, &colorImage)))
      throw std::runtime_error("Failed to get color swapchain buffer, calling ovr_GetTextureSwapChainBufferVk()");

    VkImage depthImage;
    if (!OVR_SUCCESS(ovr_GetTextureSwapChainBufferVk(session_, depthSwapchain, i, &depthImage)))
      throw std::runtime_error("Failed to get depth swapchain buffer, calling ovr_GetTextureSwapChainBufferVk()");

    colorImages.push_back(colorImage);
    depthImages.push_back(depthImage);
  }

  std::vector<vk::ImageView> colorImageViews;
  std::vector<vk::ImageView> depthImageViews;
  for (int i = 0; i < colorImageCount; i++)
  {
    vk::ImageViewCreateInfo imageViewCreateInfo;
    imageViewCreateInfo
      .setViewType(vk::ImageViewType::e2D)
      .setImage(colorImages[i])
      .setFormat(vk::Format::eB8G8R8A8Srgb)
      .setComponents({})
      .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
    const auto colorImageView = device.createImageView(imageViewCreateInfo);

    // Depth
    imageViewCreateInfo
      .setImage(depthImages[i])
      .setFormat(vk::Format::eD24UnormS8Uint)
      .setSubresourceRange({ vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 });
    const auto depthImageView = device.createImageView(imageViewCreateInfo);

    colorImageViews.push_back(colorImageView);
    depthImageViews.push_back(depthImageView);
  }

  Swapchain swapchain;
  swapchain.session_ = *this;
  swapchain.device_ = device;
  swapchain.extent_ = extent;
  swapchain.colorSwapchain_ = colorSwapchain;
  swapchain.depthSwapchain_ = depthSwapchain;
  swapchain.colorImages_ = colorImages;
  swapchain.depthImages_ = depthImages;
  swapchain.colorImageViews_ = colorImageViews;
  swapchain.depthImageViews_ = depthImageViews;
  return swapchain;
}

bool Session::opened()
{
  return session_ != nullptr;
}

ovrSessionStatus Session::getStatus() const
{
  ovrSessionStatus sessionStatus;
  if (!OVR_SUCCESS(ovr_GetSessionStatus(session_, &sessionStatus)))
    throw std::runtime_error("Failed to get session status");
  return sessionStatus;
}

ovrInputState Session::getInputState() const
{
  ovrInputState inputState;
  if (!OVR_SUCCESS(ovr_GetInputState(session_, ovrControllerType_Active, &inputState)))
    throw std::runtime_error("Failed to get input state");
  return inputState;
}

std::vector<std::string> Session::getInstanceExtensions()
{
  char extensionNames[4096];
  uint32_t extensionNamesSize = sizeof(extensionNames);

  if (!OVR_SUCCESS(ovr_GetInstanceExtensionsVk(luid_, extensionNames, &extensionNamesSize)))
    return {};

  std::vector<std::string> extensions = { "" };
  for (int i = 0; extensionNames[i] != 0; i++)
  {
    if (extensionNames[i] == ' ')
      extensions.emplace_back();
    else
      extensions.back().push_back(extensionNames[i]);
  }

  return extensions;
}

vk::PhysicalDevice Session::getPhysicalDevice(vk::Instance instance)
{
  VkPhysicalDevice physicalDevice;
  if (!OVR_SUCCESS(ovr_GetSessionPhysicalDeviceVk(session_, luid_, instance, &physicalDevice)))
    throw std::runtime_error("Failed to get physical device, calling ovr_GetSessionPhysicalDeviceVk()");
  return physicalDevice;
}

std::vector<std::string> Session::getDeviceExtensions()
{
  char extensionNames[4096];
  uint32_t extensionNamesSize = sizeof(extensionNames);

  if (!OVR_SUCCESS(ovr_GetDeviceExtensionsVk(luid_, extensionNames, &extensionNamesSize)))
    throw std::runtime_error("Failed to get device extensions, calling ovr_GetDeviceExtensionsVk()");

  std::vector<std::string> extensions = { "" };
  for (int i = 0; extensionNames[i] != 0; i++)
  {
    if (extensionNames[i] == ' ')
      extensions.emplace_back();
    else
      extensions.back().push_back(extensionNames[i]);
  }

  return extensions;
}

void Session::synchronizeWithQueue(vk::Queue queue)
{
  ovr_SetSynchronizationQueueVk(session_, queue);
}

std::vector<ovrPosef> Session::getEyePoses()
{
  ovrEyeRenderDesc eyeRenderDescs[ovrEye_Count];
  for (auto eye : { ovrEye_Left, ovrEye_Right })
    eyeRenderDescs[eye] = ovr_GetRenderDesc(session_, eye, hmdDesc_.DefaultEyeFov[eye]);

  ovrPosef hmdToEyePoses[ovrEye_Count] = {
    eyeRenderDescs[ovrEye_Left].HmdToEyePose,
    eyeRenderDescs[ovrEye_Right].HmdToEyePose
  };
  std::vector<ovrPosef> eyeRenderPoses(ovrEye_Count);
  ovr_GetEyePoses(session_, frameIndex_, ovrTrue, hmdToEyePoses, &eyeRenderPoses[0], &sensorSampleTime_);

  eyeRenderPoses_ = eyeRenderPoses;

  return eyeRenderPoses;
}

void Session::getTouchPoses()
{
  const auto hmdFrameTiming = ovr_GetPredictedDisplayTime(session_, frameIndex_);
  const auto trackingState = ovr_GetTrackingState(session_, hmdFrameTiming, false);
  const auto& p = trackingState.HandPoses[ovrHand_Left].ThePose.Position;

  const auto input = getInputState();
  std::cout << std::boolalpha << static_cast<bool>(input.ControllerType & ovrControllerType_LTouch) << ' '
    << p.x << ' ' << p.y << ' ' << p.z << std::endl;
}

ovrMatrix4f Session::getEyeProjection(ovrEyeType eye, float near, float far)
{
  auto projection = ovrMatrix4f_Projection(hmdDesc_.DefaultEyeFov[eye], near, far, ovrProjection_None);
  posTimewarpProjectionDesc_ = ovrTimewarpProjectionDesc_FromProjection(projection, ovrProjection_None);
  return projection;
}

void Session::beginFrame()
{
  frameIndex_++;

  if (!OVR_SUCCESS(ovr_WaitToBeginFrame(session_, frameIndex_)))
    throw std::runtime_error("Failed to wait for begin ovr frame");

  // TODO: predicted display time?
  /*
  HmdFrameTiming = ovr_GetPredictedDisplayTime(Session, frameIndex);
  */

  if (!OVR_SUCCESS(ovr_BeginFrame(session_, frameIndex_)))
    throw std::runtime_error("Failed to begin ovr frame");
}

void Session::endFrame(const std::vector<Swapchain>& swapchains)
{
  ovrLayerEyeFovDepth ld = {};
  ld.Header.Type = ovrLayerType_EyeFovDepth;
  ld.Header.Flags = 0;
  ld.ProjectionDesc = posTimewarpProjectionDesc_;
  ld.SensorSampleTime = sensorSampleTime_;

  for (auto eye : { ovrEye_Left, ovrEye_Right })
  {
    const auto& extent = swapchains[eye].getExtent();
    ld.ColorTexture[eye] = swapchains[eye].getColorSwapchain();
    ld.DepthTexture[eye] = swapchains[eye].getDepthSwapchain();
    ld.Viewport[eye] = { 0, 0, static_cast<int>(extent.width), static_cast<int>(extent.height) };
    ld.Fov[eye] = hmdDesc_.DefaultEyeFov[eye];
    ld.RenderPose[eye] = eyeRenderPoses_[eye];
  }

  ovrLayerHeader* layers = &ld.Header;
  if (!OVR_SUCCESS(ovr_EndFrame(session_, frameIndex_, nullptr, &layers, 1)))
    throw std::runtime_error("Failed to submit ovr frame");
}

void Session::recenter()
{
  if (!OVR_SUCCESS(ovr_RecenterTrackingOrigin(session_)))
    throw std::runtime_error("Failed to recenter tracking origin");
}

void Session::destroy()
{
  ovr_Destroy(session_);
  session_ = nullptr;
}
}
