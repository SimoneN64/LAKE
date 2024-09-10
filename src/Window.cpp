#include <fmt/core.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_internal.h>
#include <implot.h>
#include <SDL3/SDL_opengl.h>
#include <Window.hpp>
#include <LogicAnalyzer.hpp>

Window::~Window() noexcept {
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImPlot::DestroyContext();
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
  ImPlot::CreateContext();
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

  io.Fonts->AddFontFromFileTTF("resources/FiraMono-Regular.ttf", fontSize);
  io.Fonts->AddFontFromFileTTF("resources/FiraMono-Bold.ttf", fontSize);

  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);
}

void Window::HandleEvents() noexcept {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL3_ProcessEvent(&event);
    done = (event.type == SDL_EVENT_QUIT) ||
      (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window));
  }
}

void Window::NewFrame() noexcept {
  if (fontSizeChanged) {
    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->Clear();

    io.Fonts->AddFontFromFileTTF("resources/FiraMono-Regular.ttf", fontSize);
    io.Fonts->AddFontFromFileTTF("resources/FiraMono-Bold.ttf", fontSize);

    ImGui_ImplSDLRenderer3_CreateFontsTexture();

    fontSizeChanged = false;
  }

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

void Window::MakeFrame(const char *name, ImVec2 size, const std::function<void()> &func, float &scrollAmount,
                       bool sameLine, bool scrollbar) noexcept {
  if (!scrollbar)
    ImGui::SetNextWindowScroll({0.f, scrollAmount});

  if (ImGui::BeginChild(name, size, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeX * !scrollbar,
                        ImGuiWindowFlags_NoScrollbar * !scrollbar)) {
    if (scrollbar)
      scrollAmount = ImGui::GetScrollY();

    func();
    ImGui::EndChild();
  }

  if (sameLine) {
    ImGui::SameLine();
  }
}

void Window::ShowMainMenuBar(LogicAnalyzer &logicAnalyzer) noexcept {
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
}

void Window::MainView(LogicAnalyzer &logicAnalyzer) noexcept {
  popupHandler.RunPopups();

  ShowMainMenuBar(logicAnalyzer);

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

    ImGui::SliderFloat("Font size:", &fontSize, 4.f, 96.f, "%.0f");
    if (fontSize != prevFontSize) {
      fontSizeChanged = true;
      prevFontSize = fontSize;
    }
  });

  static bool plotVisible = false;
  static auto bordersWidth = ImGui::GetStyle().FramePadding.x + ImGui::GetStyle().ItemSpacing.x * 2;
  static auto identifierWidth = ImGui::CalcTextSize("Identifier").x + bordersWidth;
  static auto lengthWidth = ImGui::CalcTextSize("Length").x + bordersWidth;
  static auto dataBytesWidth =
    ImGui::CalcTextSize("FF FF FF FF FF FF FF FF").x + ImGui::CalcTextSize("........").x * 2 + bordersWidth;
  static auto cksWidth = ImGui::CalcTextSize("Checksum").x + bordersWidth;
  static auto absTimeWidth = ImGui::CalcTextSize("Absolute time (s)").x + bordersWidth;
  static auto deltaTimeWidth = ImGui::CalcTextSize("Delta time (ms)").x + bordersWidth;

  ImGui::SetNextWindowPos({0.f, menuBarHeight});
  ImGui::SetNextWindowSize({static_cast<float>(Width()), static_cast<float>(Height()) - menuBarHeight});
  if (ImGui::Begin("Data View", nullptr,
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {

    static auto scrollbar = 0.f;
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

    MakeFrame(
      "##identifier", {identifierWidth, -1},
      [&]() {
        for (int i = 0; i < 100; i++) {
          ImGui::Text("%02X", 0xFF);
        }
      },
      scrollbar);

    MakeFrame(
      "##length", {lengthWidth, -1},
      [&]() {
        for (int i = 0; i < 100; i++) {
          ImGui::Text("%01X", 0xF);
        }
      },
      scrollbar);

    MakeFrame(
      "##dataBytes", {dataBytesWidth, -1},
      [&]() {
        for (int i = 0; i < 100; i++) {
          for (int j = 0; j < 8; j++) {
            ImGui::Text("FF");
            if (ImGui::IsItemClicked()) {
              plotVisible = true;
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
          for (int j = 0; j < 8; j++) {
            ImGui::Text("%c", '.');
            if (j < 7) {
              ImGui::SameLine();
            }
          }
        }
      },
      scrollbar);

    MakeFrame(
      "##cks", {cksWidth, -1},
      [&]() {
        for (int i = 0; i < 100; i++) {
          ImGui::Text("%02X", 0xFF);
        }
      },
      scrollbar);

    MakeFrame(
      "##absTime", {absTimeWidth, -1},
      [&]() {
        for (int i = 0; i < 100; i++) {
          ImGui::Text("%.6f", 10000.0);
        }
      },
      scrollbar);

    MakeFrame(
      "##deltaTime", {deltaTimeWidth, -1},
      [&]() {
        for (int i = 0; i < 100; i++) {
          ImGui::Text("%.6f", 10000.0);
        }
      },
      scrollbar, true, true);

    if (plotVisible) {
      if (ImPlot::BeginPlot("Square wave")) {
        static constexpr float time[] = {0.001, 0.002, 0.003, 0.004, 0.005, 0.006, 0.007, 0.008};
        static constexpr float bits[] = {1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f};
        ImPlot::SetupAxisLimits(ImAxis_Y1, -0.3f, 1.3f);

        ImPlot::PlotStairs("##squareWave", time, bits, 8);

        ImPlot::EndPlot();
      }

      plotVisible = !ImGui::CloseButton(ImGui::GetID("Data View"), {ImGui::GetWindowSize().x - 40.f, 40.f});
    }

    ImGui::End();
  }
}

void Window::ShowLoading(LogicAnalyzer &logicAnalyzer) noexcept {
  ImGui::OpenPopup("Loading");
  if (ImGui::BeginPopupModal("Loading", nullptr,
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
    ImGui::Text("Parsing %s...", logicAnalyzer.GetPath());
    ImGui::EndPopup();
  }

  ShowMainMenuBar(logicAnalyzer);
}

void Window::AskForFileAndLineSettings(LogicAnalyzer &logicAnalyzer) noexcept {
  ImGui::OpenPopup("Load a file");
  if (ImGui::BeginPopupModal("Load a file", nullptr,
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
    static const char* parity[] = {"None", "Even", "Odd "};
    static int currParity = 0;
    static const char *significantBit[] = {"LSB", "MSB"};
    static int currSignificantBit = 0;
    ImGui::InputScalar("Bitrate", ImGuiDataType_U32, &logicAnalyzer.settings.bitrate);
    ImGui::InputScalar("Bits per frame", ImGuiDataType_U8, &logicAnalyzer.settings.bitsPerFrame);
    ImGui::InputFloat("Stop bit type", &logicAnalyzer.settings.stopBitType);
    if (ImGui::BeginCombo("Parity bit", parity[currParity])) {
      ImGui::Combo("##parities", &currParity, parity, 3);
      ImGui::EndCombo();
    }
    if (ImGui::BeginCombo("Significant bit", significantBit[currSignificantBit])) {
      ImGui::Combo("##significantBits", &currSignificantBit, significantBit, 2);
      ImGui::EndCombo();
    }
    ImGui::InputScalar("Signal inversion", ImGuiDataType_Bool, &logicAnalyzer.settings.signalInversion);

    if (ImGui::Button("Select file")) {
      logicAnalyzer.OpenDialog();
    }

    ImGui::EndPopup();
  }

  ShowMainMenuBar(logicAnalyzer);
}
