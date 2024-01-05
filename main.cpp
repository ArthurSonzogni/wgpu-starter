#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>
#include <glm/glm.hpp>
#include <iostream>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#else
#include <webgpu/webgpu_glfw.h>
#endif

wgpu::Instance instance;
wgpu::Device device;
wgpu::RenderPipeline pipeline;
wgpu::SwapChain swapChain;
wgpu::Surface surface;
wgpu::Buffer buffer;
uint32_t kWidth = 512;
uint32_t kHeight = 512;
GLFWwindow* window = nullptr;

void OnError(WGPUErrorType type, char const* message, void* userdata) {
  std::cout << "Device error: " << std::endl;
  std::cout << "- type " << type << std::endl;
  if (message) {
    std::cout << " - message: " << message << std::endl;
  }
}

void SetupSwapChain(wgpu::Surface surface) {
  wgpu::SwapChainDescriptor scDesc{
      .usage = wgpu::TextureUsage::RenderAttachment,
      .format = wgpu::TextureFormat::BGRA8Unorm,
      .width = kWidth,
      .height = kHeight,
      .presentMode = wgpu::PresentMode::Fifo,
  };
  swapChain = device.CreateSwapChain(surface, &scDesc);
}

void OnDeviceFound(WGPURequestDeviceStatus status,
                   WGPUDevice dev,
                   char const* message,
                   void* pUserData) {
  if (status != WGPURequestDeviceStatus_Success) {
    std::cout << "Failed to create device: " << message << std::endl;
    exit(0);
    return;
  }

  device = wgpu::Device::Acquire(dev);
  std::cout << "Found device = " << device.Get() << std::endl;

  // Add an error callback for more debug info
  device.SetUncapturedErrorCallback(OnError, nullptr);

  auto callback = reinterpret_cast<void (*)()>(pUserData);
  callback();
}

void OnAdapterFound(WGPURequestAdapterStatus status,
                    WGPUAdapter adapter,
                    char const* message,
                    void* pUserData) {
  if (status != WGPURequestAdapterStatus_Success) {
    std::cout << "Failed to find an adapter: " << message << std::endl;
    exit(0);
    return;
  }

  wgpu::Adapter wgpu_adapter = wgpu::Adapter::Acquire(adapter);

  wgpu::AdapterProperties properties;
  wgpu_adapter.GetProperties(&properties);
  std::cout << " Adapter properties: " << std::endl;
  std::cout << " - vendorName: " << properties.vendorName << std::endl;
  std::cout << " - architecture: " << properties.architecture << std::endl;
  std::cout << " - name: " << properties.name << std::endl;
  std::cout << " - driverDescription: " << properties.driverDescription
            << std::endl;

  wgpu::DeviceDescriptor device_descriptor{
      .label = "My device",
  };
  wgpu_adapter.RequestDevice(&device_descriptor, OnDeviceFound, pUserData);
};

void GetDevice(void (*callback)()) {
  wgpu::RequestAdapterOptions options{
      .compatibleSurface = surface,
      .powerPreference = wgpu::PowerPreference::HighPerformance,
  };
  instance.RequestAdapter(&options, OnAdapterFound,
                          reinterpret_cast<void*>(callback));
}

void CreateRenderPipeline() {
  wgpu::ShaderModuleWGSLDescriptor wgslDesc;
  wgslDesc.code = R"(
    @vertex
    fn vs_main(@location(0) in_vertex_position: vec2f) -> @builtin(position) vec4f {
      return vec4f(in_vertex_position, 0.0, 1.0);
    }
  
    @fragment
    fn fs_main() -> @location(0) vec4f {
        return vec4f(0.0, 0.4, 1.0, 1.0);
    }
  )";

  wgpu::ShaderModuleDescriptor shaderModuleDescriptor{
      .nextInChain = &wgslDesc,
  };
  wgpu::ShaderModule shaderModule =
      device.CreateShaderModule(&shaderModuleDescriptor);

  std::vector<wgpu::VertexAttribute> vertexAttributes = {
      {
          .format = wgpu::VertexFormat::Float32x2,
          .offset = 0,
          .shaderLocation = 0,
      },
  };

  std::vector<wgpu::VertexBufferLayout> vertexBufferLayout = {
      {
          .arrayStride = sizeof(glm::vec2),
          .attributeCount = vertexAttributes.size(),
          .attributes = vertexAttributes.data(),
      },
  };

  std::vector<wgpu::ColorTargetState> colorTargetState = {
      {
          .format = wgpu::TextureFormat::BGRA8Unorm,
      },
  };

  wgpu::FragmentState fragmentState{
      .module = shaderModule,
      .entryPoint = "fs_main",
      .targetCount = colorTargetState.size(),
      .targets = colorTargetState.data(),
  };

  wgpu::RenderPipelineDescriptor descriptor{
      .vertex =
          {
              .module = shaderModule,
              .entryPoint = "vs_main",
              .bufferCount = vertexBufferLayout.size(),
              .buffers = vertexBufferLayout.data(),
          },
      .primitive =
          {
              .topology = wgpu::PrimitiveTopology::TriangleList,
              .stripIndexFormat = wgpu::IndexFormat::Undefined,
              .frontFace = wgpu::FrontFace::CCW,
              .cullMode = wgpu::CullMode::None,
          },
      .fragment = &fragmentState,
  };
  pipeline = device.CreateRenderPipeline(&descriptor);
}

void Render() {
  std::vector<wgpu::RenderPassColorAttachment> attachment = {
      {
          .view = swapChain.GetCurrentTextureView(),
          .loadOp = wgpu::LoadOp::Clear,
          .storeOp = wgpu::StoreOp::Store,
          .clearValue = {0.0f, 1.0f, 0.5f, 1.0f},
      },
  };

  wgpu::RenderPassDescriptor renderpass{
      .colorAttachmentCount = attachment.size(),
      .colorAttachments = attachment.data(),
  };

  wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
  wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
  pass.SetPipeline(pipeline);
  pass.SetVertexBuffer(0, buffer, 0, 6 * sizeof(glm::vec2));
  pass.Draw(6);
  pass.End();

  wgpu::CommandBuffer commands = encoder.Finish();
  device.GetQueue().Submit(1, &commands);
}

void Start() {
  std::cout << "Start" << std::endl;

  std::vector<glm::vec2> vertices = {
      {-0.5, -0.5},   {+0.5, -0.5},   {+0.0, +0.5},
      {-0.55f, -0.5}, {-0.05f, +0.5}, {-0.55f, +0.5},
  };
  wgpu::BufferDescriptor bufferDesc = {
      .label = "Some GPU-side data buffer",
      .usage = wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst,
      .size = vertices.size() * sizeof(glm::vec2),
      .mappedAtCreation = false,
  };
  buffer = device.CreateBuffer(&bufferDesc);
  device.GetQueue().WriteBuffer(buffer, 0, vertices.data(),
                                vertices.size() * sizeof(glm::vec2));

  SetupSwapChain(surface);
  CreateRenderPipeline();

#if defined(__EMSCRIPTEN__)
  emscripten_set_main_loop(Render, 0, false);
#else

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (width != kWidth || height != kHeight) {
      std::cout << "Resizing to " << width << "x" << height << std::endl;
      kWidth = width;
      kHeight = height;

      SetupSwapChain(surface);
      CreateRenderPipeline();
    }

    Render();
    swapChain.Present();

    device.Tick();
  }

#endif

  glfwDestroyWindow(window);
  glfwTerminate();
}

int main() {
  if (!glfwInit()) {
    std::cout << "Failed to initialize GLFW" << std::endl;
    return EXIT_FAILURE;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  window = glfwCreateWindow(kWidth, kHeight, "WebGPU window", nullptr, nullptr);
  if (!window) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return EXIT_FAILURE;
  }

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  kWidth = width;
  kHeight = height;

  instance = wgpu::CreateInstance();

#if defined(__EMSCRIPTEN__)
  wgpu::SurfaceDescriptorFromCanvasHTMLSelector canvasDesc;
  canvasDesc.selector = "#canvas";

  wgpu::SurfaceDescriptor surfaceDesc{
      .nextInChain = &canvasDesc,
  };
  surface = instance.CreateSurface(&surfaceDesc);
#else
  surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);
#endif
  std::cout << "Surface = " << surface.Get() << std::endl;

  GetDevice(Start);
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
