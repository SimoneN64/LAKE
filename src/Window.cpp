#include <fmt/core.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_internal.h>
#include <SDL3/SDL_opengl.h>
#include <Window.hpp>
#include <LogicAnalyzer.hpp>

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
    SDL_CreateWindow("LAKE", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_HIGH_PIXEL_DENSITY);

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

  io.Fonts->AddFontFromFileTTF("resources/FiraMono-Regular.ttf", 20.f);
  io.Fonts->AddFontFromFileTTF("resources/FiraMono-Bold.ttf", 20.f);
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
                        ImGuiWindowFlags_NoScrollbar * (!scrollBar))) {
    func();
    ImGui::EndChild();
  }

  if (sameLine) {
    ImGui::SameLine();
  }
}

void Window::MainView(LogicAnalyzer &logicAnalyzer) noexcept {
  popupHandler.RunPopups();

  static float menuBarHeight = 0;
  static bool openSettings = false;
  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Open")) {
        logicAnalyzer.OpenDialog();
      }

      if (ImGui::MenuItem("Exit")) {
        Quit();
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Options")) {
      if (ImGui::MenuItem("Settings")) {
        openSettings = true;
      }
      ImGui::EndMenu();
    }
    menuBarHeight = ImGui::GetWindowHeight();
    ImGui::EndMainMenuBar();
  }

  PopupHandler::MakePopup("Settings", &openSettings, [&]() {
    ImGui::Text("Style:");
    ImGui::SameLine(0, 2);
    if (ImGui::RadioButton("Dark", themeDark)) {
      themeDark = true;
      ImGui::StyleColorsDark();
    }

    ImGui::SameLine();

    if (ImGui::RadioButton("Light", !themeDark)) {
      themeDark = false;
      ImGui::StyleColorsLight();
    }
  });

  static auto scrollbar = 0.f;
  static auto bordersWidth = ImGui::GetStyle().FramePadding.x + ImGui::GetStyle().ItemSpacing.x * 2;
  static auto identifierWidth = ImGui::CalcTextSize("Identifier").x + bordersWidth;
  static auto lengthWidth = ImGui::CalcTextSize("Length").x + bordersWidth;
  static auto dataBytesWidth =
    ImGui::CalcTextSize("FF FF FF FF FF FF FF FF").x + ImGui::CalcTextSize("........").x * 2 + bordersWidth;
  static auto cksWidth = ImGui::CalcTextSize("Checksum").x + bordersWidth;
  static auto absTimeWidth = ImGui::CalcTextSize("Absolute time (s)").x + bordersWidth;
  static auto deltaTimeWidth = ImGui::CalcTextSize("Delta time (ms)").x + bordersWidth;
  SDL_SetWindowSize(window, identifierWidth + lengthWidth + dataBytesWidth + cksWidth + absTimeWidth + deltaTimeWidth +
                      ImGui::GetStyle().ScrollbarSize, Height());

  ImGui::SetNextWindowPos({0.f, menuBarHeight});
  ImGui::SetNextWindowSize({static_cast<float>(Width()), static_cast<float>(Height()) - menuBarHeight});
  if (ImGui::Begin("Data View", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {

    ImGui::Text("Identifier");
    ImGui::SameLine(0, bordersWidth);
    ImGui::Text("Length");
    ImGui::SameLine(0, bordersWidth);
    ImGui::Text("Data bytes");
    ImGui::SameLine(0, dataBytesWidth - ImGui::CalcTextSize("Data bytes").x + bordersWidth);
    ImGui::Text("Checksum");
    ImGui::SameLine(0, bordersWidth);
    ImGui::Text("Absolute time (s)");
    ImGui::SameLine(0, bordersWidth);
    ImGui::Text("Delta time (ms)");

    Window::MakeFrame("##identifier", {identifierWidth, -1}, [&]() {
      ImGui::SetScrollY(scrollbar);
      for (int i = 0; i < 100; i++) {
        ImGui::Text("%02X", 0xFF);
      }
    });

    Window::MakeFrame("##length", {lengthWidth, -1}, [&]() {
      ImGui::SetScrollY(scrollbar);
      for (int i = 0; i < 100; i++) {
        ImGui::Text("%01X", 0xF);
      }
    });

    Window::MakeFrame("##dataBytes", {dataBytesWidth, -1}, [&]() {
      ImGui::SetScrollY(scrollbar);
      for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 8; j++) {
          ImGui::Text("FF");
          if (ImGui::IsItemHovered()) {
            if (ImGui::BeginTooltip()) {
              ImGui::Text("Immagina... una forma d'onda quadra");
              ImGui::EndTooltip();
            }
          }
          ImGui::SameLine();
          ImGui::Text(" ");
          if (ImGui::IsItemHovered()) {
            if (ImGui::BeginTooltip()) {
              ImGui::Text("Interbyte delay (us): %.3f", 0);
              ImGui::EndTooltip();
            }
          }
          ImGui::SameLine();
        }
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize("F").x * 8);
        ImGui::Text("%c%c%c%c%c%c%c%c", '.', '.', '.', '.', '.', '.', '.', '.');
      }
    });

    Window::MakeFrame("##cks", {cksWidth, -1}, [&]() {
      ImGui::SetScrollY(scrollbar);
      for (int i = 0; i < 100; i++) {
        ImGui::Text("%02X", 0xFF);
      }
    });

    Window::MakeFrame("##absTime", {absTimeWidth, -1}, [&]() {
      ImGui::SetScrollY(scrollbar);
      for (int i = 0; i < 100; i++) {
        ImGui::Text("%.6f", 10000.0);
      }
    });

    Window::MakeFrame(
      "##deltaTime", {deltaTimeWidth, -1},
      [&]() {
        scrollbar = ImGui::GetScrollY();
        ImGui::SetScrollY(scrollbar);

        for (int i = 0; i < 100; i++) {
          ImGui::Text("%.3f", 10000.0);
        }
      },
      false, true);

    ImGui::End();
  }
}
