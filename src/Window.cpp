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

  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;
  io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
  io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;
  io.FontAllowUserScaling = true;

  ImGui::StyleColorsDark();

  ImGuiStyle &style = ImGui::GetStyle();

  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.0f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }

  io.Fonts->AddFontFromFileTTF("resources/FiraMono-Regular.ttf", fontSize);
  io.Fonts->AddFontFromFileTTF("resources/FiraMono-Bold.ttf", fontSize);

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

  ImGui::UpdatePlatformWindows();
  ImGui::RenderPlatformWindowsDefault();

  SDL_RenderPresent(renderer);
}

void Window::MakeFrame(const char *name, const std::function<void()> &func, bool *visible) noexcept {
  ImGui::SetNextWindowScroll({0.f, scrollAmount});
  if (ImGui::Begin(name, visible)) {
    scrollAmount = ImGui::GetScrollY();
    func();
    ImGui::End();
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

  auto style = ImGui::GetStyle();
  static auto scrollbar = 0.f;
  static bool plotVisible = false;

  MakeFrame("Identifier", [&]() {
    for (int i = 0; i < 100; i++) {
      ImGui::Text("%02X", 0xFF);
    }
  });

  MakeFrame("Length", [&]() {
    for (int i = 0; i < 100; i++) {
      ImGui::Text("%01X", 0xF);
    }
  });

  MakeFrame("Data bytes", [&]() {
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
      ImGui::SameLine(ImGui::GetWindowWidth() - ((ImGui::CalcTextSize("F").x + style.ItemSpacing.x) * 8));
      for (int j = 0; j < 8; j++) {
        ImGui::Text("%c", '.');
        if (j < 7) {
          ImGui::SameLine();
        }
      }
    }
  });

  MakeFrame("Checksum", [&]() {
    for (int i = 0; i < 100; i++) {
      ImGui::Text("%02X", 0xFF);
    }
  });

  MakeFrame("Absolute time (s)", [&]() {
    for (int i = 0; i < 100; i++) {
      ImGui::Text("%.6f", 10000.0);
    }
  });

  MakeFrame("Delta time (us)", [&]() {
    for (int i = 0; i < 100; i++) {
      ImGui::Text("%.6f", 10000.0);
    }
  });

  if (plotVisible) {
    MakeFrame(
      "Square wave",
      [&]() {
        if (ImPlot::BeginPlot("##plotSquare")) {
          static constexpr float time[] = {0.001, 0.002, 0.003, 0.004, 0.005, 0.006, 0.007, 0.008};
          static constexpr float bits[] = {1.f, 0.f, 1.f, 0.f, 1.f, 0.f, 1.f, 0.f};
          ImPlot::SetupAxisLimits(ImAxis_Y1, -0.3f, 1.3f);

          ImPlot::PlotStairs("##squareWave", time, bits, 8);

          ImPlot::EndPlot();
        }
      },
      &plotVisible);
  }
}

void Window::ShowLoading(LogicAnalyzer &logicAnalyzer) noexcept {
  ImGui::OpenPopup("Loading");
  if (ImGui::BeginPopupModal("Loading", nullptr,
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
    ImGui::Text("Parsing \"%s\"", logicAnalyzer.GetPath().string().c_str());
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImGui::SetWindowPos({Width() / 2 - windowSize.x / 2, Height() / 2 - windowSize.y / 2});
    ImGui::EndPopup();
  }

  ShowMainMenuBar(logicAnalyzer);
}

const std::string &Window::MakeCombo(const std::string &label, const std::vector<std::string> &items) noexcept {
  int current = 0;
  if (ImGui::BeginCombo(label.c_str(), items[current].c_str())) {
    for (int i = 0; i < items.size(); i++) {
      bool is_selected = (current == i);
      if (ImGui::Selectable(items[i].c_str(), is_selected))
        current = i;
      if (is_selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  return items[current];
}

static inline AnalyzerSettings::ParityBitType StrToParity(const std::string &str) noexcept {
  if (str == "None") {
    return AnalyzerSettings::None;
  } else if (str == "Even") {
    return AnalyzerSettings::Even;
  } else if (str == "Odd") {
    return AnalyzerSettings::Odd;
  }
}

static inline AnalyzerSettings::SignificantBitType StrToSignificantBit(const std::string &str) noexcept {
  if (str == "LSB") {
    return AnalyzerSettings::LSB;
  } else if (str == "MSB") {
    return AnalyzerSettings::MSB;
  }
}

static inline float StrToStopBit(const std::string &str) noexcept {
  if (str == "1.0") {
    return 1.f;
  } else if (str == "1.5") {
    return 1.5f;
  } else if (str == "2.0") {
    return 2.f;
  }
}

void Window::AskForFileAndLineSettings(LogicAnalyzer &logicAnalyzer) noexcept {
  ImGui::OpenPopup("Load a file");
  if (ImGui::BeginPopupModal("Load a file", nullptr,
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
    ImGui::InputScalar("Bitrate", ImGuiDataType_U32, &logicAnalyzer.settings.bitrate);
    ImGui::InputScalar("Bits per frame", ImGuiDataType_U8, &logicAnalyzer.settings.bitsPerFrame);

    logicAnalyzer.settings.stopBitType = StrToStopBit(MakeCombo("Stop bit", {"1.0", "1.5", "2.0"}));
    logicAnalyzer.settings.parityBitType = StrToParity(MakeCombo("Parity bit", {"None", "Even", "Odd"}));
    logicAnalyzer.settings.significantBitType = StrToSignificantBit(MakeCombo("Significant bit", {"LSB", "MSB"}));

    ImGui::Checkbox("Signal inversion", &logicAnalyzer.settings.signalInversion);

    if (ImGui::Button("Select file")) {
      logicAnalyzer.OpenDialog();
    }

    ImGui::EndPopup();
  }

  ShowMainMenuBar(logicAnalyzer);
}
