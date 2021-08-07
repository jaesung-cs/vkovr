#include <vkovr-demo/application.h>

#include <thread>
#include <chrono>
#include <queue>
#include <iostream>

#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>

#include <vkovr-demo/engine/ubo/camera_ubo.h>
#include <vkovr-demo/engine/ubo/light_ubo.h>
#include <vkovr-demo/scene/light.h>
#include <vkovr-demo/scene/camera.h>
#include <vkovr-demo/scene/camera_control.h>

namespace demo
{
namespace
{
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  double x, y;
  glfwGetCursorPos(window, &x, &y);
  auto module_window = static_cast<Application*>(glfwGetWindowUserPointer(window));
  module_window->mouseButton(button, action, mods, x, y);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  auto module_window = static_cast<Application*>(glfwGetWindowUserPointer(window));
  module_window->key(key, scancode, action, mods);
}

void cursor_pos_callback(GLFWwindow* window, double x, double y)
{
  auto module_window = static_cast<Application*>(glfwGetWindowUserPointer(window));
  module_window->cursorPos(x, y);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  auto module_window = static_cast<Application*>(glfwGetWindowUserPointer(window));
  module_window->scroll(yoffset);
}

void resize_callback(GLFWwindow* window, int width, int height)
{
  auto module_window = static_cast<Application*>(glfwGetWindowUserPointer(window));
  module_window->resize(width, height);
}
}

Application::Application()
{
  if (!glfwInit())
    throw std::runtime_error("Failed to initialize GLFW");

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  window_ = glfwCreateWindow(width_, height_, "Superlucent", NULL, NULL);

  constexpr int maxWidth = 1920;
  constexpr int maxHeight = 1080;
  glfwSetWindowSizeLimits(window_, 100, 100, maxWidth, maxHeight);

  glfwSetWindowUserPointer(window_, this);
  glfwSetMouseButtonCallback(window_, mouse_button_callback);
  glfwSetCursorPosCallback(window_, cursor_pos_callback);
  glfwSetKeyCallback(window_, key_callback);
  glfwSetScrollCallback(window_, scroll_callback);
  glfwSetWindowSizeCallback(window_, resize_callback);

  glfwSetWindowPos(window_, 1000, 500);

  // Initialize ovr
  vkovr::initialize();

  engine_ = std::make_unique<engine::Engine>(window_, width_, height_);

  // Camera
  const auto camera = std::make_shared<scene::Camera>();
  camera->setScreenSize(width_, height_);
  cameraControl_ = std::make_unique<scene::CameraControl>(camera);

  // Lights
  auto light = std::make_shared<scene::Light>();
  light->setDirectionalLight();
  light->setPosition(glm::vec3{ 0.f, 0.f, 1.f });
  light->setAmbient(glm::vec3{ 0.1f, 0.1f, 0.1f });
  light->setDiffuse(glm::vec3{ 0.2f, 0.2f, 0.2f });
  light->setSpecular(glm::vec3{ 0.1f, 0.1f, 0.1f });
  lights_.push_back(light);

  light = std::make_shared<scene::Light>();
  light->setPointLight();
  light->setPosition(glm::vec3{ 0.f, -5.f, 3.f });
  light->setAmbient(glm::vec3{ 0.2f, 0.2f, 0.2f });
  light->setDiffuse(glm::vec3{ 0.8f, 0.8f, 0.8f });
  light->setSpecular(glm::vec3{ 1.f, 1.f, 1.f });
  lights_.push_back(light);

  // Initialize events
  for (int i = 0; i < keyPressed_.size(); i++)
    keyPressed_[i] = false;
  for (int i = 0; i < mouseButtons_.size(); i++)
    mouseButtons_[i] = false;
}

Application::~Application()
{
  engine_ = nullptr;

  if (window_)
    glfwDestroyWindow(window_);

  glfwTerminate();

  vkovr::terminate();
}

void Application::run()
{
  using namespace std::chrono_literals;
  using Clock = std::chrono::high_resolution_clock;
  using Duration = std::chrono::duration<double>;
  using Timestamp = Clock::time_point;

  std::deque<Timestamp> deque;

  // Start vr worker
  engine_->startVr();

  int64_t recentSeconds = 0;
  const auto startTime = Clock::now();
  Timestamp previousTime = startTime;
  double animationTime = 0.;
  while (!glfwWindowShouldClose(window_))
  {
    glfwPollEvents();

    const auto currentTime = Clock::now();

    // Update keyboard
    const auto dt = std::chrono::duration<double>(currentTime - previousTime).count();
    updateKeyboard(dt);

    // Light position updated to camera eye
    const auto camera = cameraControl_->camera();
    lights_[0]->setPosition(camera->eye() - camera->center());

    updateLight();
    updateCamera();
    engine_->drawFrame();

    while (!deque.empty() && deque.front() < currentTime - 1s)
      deque.pop_front();
    deque.push_back(currentTime);

    // Update animation time
    if (isAnimated_)
      animationTime += dt;

    const auto seconds = static_cast<int64_t>(Duration(Clock::now() - startTime).count());
    if (seconds > recentSeconds)
    {
      const auto fps = deque.size();
      std::cout << "Application: " << fps << std::endl;

      recentSeconds = seconds;
    }

    previousTime = currentTime;
  }

  engine_->terminateVr();
}

void Application::mouseButton(int button, int action, int mods, double x, double y)
{
  int mouseButtonIndex = -1;
  switch (button)
  {
  case GLFW_MOUSE_BUTTON_LEFT: mouseButtonIndex = 0; break;
  case GLFW_MOUSE_BUTTON_RIGHT: mouseButtonIndex = 1; break;
  }

  int mouseButtonStateIndex = -1;
  switch (action)
  {
  case GLFW_RELEASE: mouseButtonStateIndex = 0; break;
  case GLFW_PRESS: mouseButtonStateIndex = 1; break;
  }

  if (mouseButtonIndex >= 0)
    mouseButtons_[mouseButtonIndex] = mouseButtonStateIndex;
}

void Application::key(int key, int scancode, int action, int mods)
{
  if (action == GLFW_PRESS)
  {
    if (key == GLFW_KEY_GRAVE_ACCENT)
    {
      glfwSetWindowShouldClose(window_, true);
      return;
    }

    if (key == GLFW_KEY_ENTER)
    {
      isAnimated_ = !isAnimated_;
    }
  }

  switch (action)
  {
  case GLFW_PRESS:
    keyPressed_[key] = true;
    break;
  case GLFW_RELEASE:
    keyPressed_[key] = false;
    break;
  }
}

void Application::cursorPos(double x, double y)
{
  const auto dx = static_cast<int>(x - mouseLastX_);
  const auto dy = static_cast<int>(y - mouseLastY_);

  if (mouseButtons_[0] && mouseButtons_[1])
  {
    cameraControl_->zoomByPixels(dx, dy);
    cameraControl_->update();
  }

  else if (mouseButtons_[0])
  {
    cameraControl_->rotateByPixels(dx, dy);
    cameraControl_->update();
  }

  else if (mouseButtons_[1])
  {
    cameraControl_->translateByPixels(dx, dy);
    cameraControl_->update();
  }

  mouseLastX_ = x;
  mouseLastY_ = y;
}

void Application::scroll(double scroll)
{
  cameraControl_->zoomByWheel(static_cast<int>(scroll));
  cameraControl_->update();
}

void Application::resize(int width, int height)
{
  cameraControl_->camera()->setScreenSize(width, height);
  engine_->resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

void Application::updateKeyboard(double dt)
{
  const auto dtf = static_cast<float>(dt);

  // Move camera
  if (keyPressed_['W'])
    cameraControl_->moveForward(dtf);
  if (keyPressed_['S'])
    cameraControl_->moveForward(-dtf);
  if (keyPressed_['A'])
    cameraControl_->moveRight(-dtf);
  if (keyPressed_['D'])
    cameraControl_->moveRight(dtf);
  if (keyPressed_[' '])
    cameraControl_->moveUp(dtf);

  cameraControl_->update();
}

void Application::updateCamera()
{
  auto camera = cameraControl_->camera();

  engine::CameraUbo cameraUbo;
  cameraUbo.view = camera->viewMatrix();
  cameraUbo.projection = camera->projectionMatrix();

  // Convert to Vulkan coordinate space
  for (int c = 0; c < 4; c++)
    cameraUbo.projection[c][1] *= -1.f;

  cameraUbo.eye = camera->eye();

  engine_->updateCamera(cameraUbo);
}

void Application::updateLight()
{
  engine::LightUbo lightUbo;
  for (int i = 0; i < lights_.size(); i++)
  {
    const auto light = lights_[i];
    engine::LightUbo::Light& lightData = lightUbo.lights[i];
    lightData.position = glm::vec4{ light->position(), light->isDirectionalLight() ? 0.f : 1.f };
    lightData.ambient = glm::vec4{ light->ambient(), 0.f };
    lightData.diffuse = glm::vec4{ light->diffuse(), 0.f };
    lightData.specular = glm::vec4{ light->specular(), 0.f };
  }
  engine_->updateLight(lightUbo);
}
}
