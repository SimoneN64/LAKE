#include <Window.hpp>
#include <fmt/core.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_internal.h>
#include <SDL3/SDL_opengl.h>

Window::~Window() noexcept {
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

Window::Window() noexcept {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fmt::print("Could not initialize video system! (SDL error: {})\n", SDL_GetError());
    errorState = VideoSystemFailure;

    return;
  }

  window =
    SDL_CreateWindow("LAKE", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

  if (window == nullptr) {
    fmt::print("Could not initialize window! (SDL error: {})\n", SDL_GetError());
    errorState = VideoSystemFailure;

    return;
  }

  renderer = SDL_CreateRenderer(window, nullptr);
  if (renderer == nullptr) {
    fmt::print("Could not initialize renderer! (SDL error: {})\n", SDL_GetError());
    errorState = VideoSystemFailure;

    return;
  }

  if (!SDL_SetRenderVSync(renderer, SDL_WINDOW_SURFACE_VSYNC_ADAPTIVE)) {
    fmt::print("Adaptive VSync not available. Falling back to frame-swapping\n");
    SDL_SetRenderVSync(renderer, true);
  }

  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
  io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
  io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;
  io.FontAllowUserScaling = true;

  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 0.0f;
  style.WindowPadding = {0.f, 0.4f};
  style.ChildBorderSize = 0;
  style.ItemSpacing = {2.f, 2.f};

  fontRegular = io.Fonts->AddFontFromFileTTF("resources/FiraMono-Regular.ttf", 24.f);
  fontBold = io.Fonts->AddFontFromFileTTF("resources/FiraMono-Bold.ttf", 24.f);
  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);
}

void Window::HandleEvents() noexcept {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL3_ProcessEvent(&event);
    if (event.type == SDL_EVENT_QUIT)
      done = true;
    if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
      done = true;
  }
}

void Window::NewFrame() noexcept {
  ImGui_ImplSDLRenderer3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}

void Window::Render() const noexcept {
  ImGui::Render();
  SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 0);
  SDL_RenderClear(renderer);
  ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
  SDL_RenderPresent(renderer);

  ImGui::UpdatePlatformWindows();
  ImGui::RenderPlatformWindowsDefault();
}

void Window::MakeFrame(const char *name, ImVec2 size, const std::function<void()> &func, bool sameLine,
                       bool scrollBar) noexcept {
  if (ImGui::BeginChild(name, size, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeX,
                        ImGuiWindowFlags_NoScrollbar * (!scrollBar) |
                          ImGuiWindowFlags_NoScrollWithMouse * (!scrollBar))) {
    func();
    ImGui::EndChild();
  }

  if (sameLine) {
    ImGui::SameLine();
  }
}
