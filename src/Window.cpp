#include <fmt/core.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <implot.h>
#include <SDL3/SDL_opengl.h>
#include <Window.hpp>
#include <LogicAnalyzer.hpp>

Window::~Window() noexcept {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();

  SDL_GL_DestroyContext(glContext);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

Window::Window() noexcept {
  auto settingsResult = OpenOrCreateSettings();

  switch (settingsResult.error) {
  case JsonParseResult::ParseError:
    fmt::print("Failed to parse settings!\n");
    fmt::print("Consider deleting the file \"settings.json\" you find next to the executable\n");
    exit(1);
    break;
  case JsonParseResult::OpenError:
    fmt::print("The settings file may be corrupted or incorrect!\n");
    fmt::print("Consider deleting the file \"settings.json\" you find next to the executable\n");
    exit(1);
    break;
  default:
    break;
  }

  settings = settingsResult.json;
  theme = settings["style"]["theme"].get<std::string>();
  fontSize = settings["font"]["size"].get<float>();

  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fmt::print("Could not initialize video system! (SDL error: {})\n", SDL_GetError());
    errorState = VideoSystemFailure;

    return;
  }

  const char *glsl_version = "#version 130";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

  window = SDL_CreateWindow("LAKE", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

  if (window == nullptr) {
    fmt::print("Could not initialize window! (SDL error: {})\n", SDL_GetError());
    errorState = VideoSystemFailure;

    return;
  }

  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

  glContext = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, glContext);
  SDL_GL_SetSwapInterval(1); // Enable vsync

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

  if (theme == "dark") {
    ImGui::StyleColorsDark();
  } else if (theme == "light") {
    ImGui::StyleColorsLight();
  }

  ImGuiStyle &style = ImGui::GetStyle();

  style.WindowRounding = 0.0f;
  style.Colors[ImGuiCol_WindowBg].w = 1.0f;

  io.Fonts->AddFontFromFileTTF("resources/FiraMono-Regular.ttf", fontSize);
  io.Fonts->AddFontFromFileTTF("resources/FiraMono-Bold.ttf", fontSize);

  ImGui_ImplSDL3_InitForOpenGL(window, glContext);
  ImGui_ImplOpenGL3_Init(glsl_version);
}

Window::JsonParseResult Window::OpenOrCreateSettings() {
  if (fs::exists("settings.json")) {
    auto file = std::fstream("settings.json", std::fstream::in | std::fstream::out);
    if (!nlohmann::json::accept(file)) {
      return {{}, JsonParseResult::OpenError};
    }

    file.seekg(0);
    auto json = nlohmann::json::parse(file, nullptr, false);
    file.close();

    if (json.is_discarded()) {
      return {{}, JsonParseResult::ParseError};
    }

    return {json, JsonParseResult::None};
  }

  auto file = std::fstream("settings.json", std::fstream::in | std::fstream::out | std::fstream::trunc);
  nlohmann::json json;
  json["style"]["theme"] = "dark";
  json["font"]["size"] = 20.f;

  file << json;
  file.close();

  return {json, JsonParseResult::None};
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

    std::ofstream file("settings.json");
    settings["font"]["size"] = fontSize;
    file << settings;
    file.close();

    io.Fonts->AddFontFromFileTTF("resources/FiraMono-Regular.ttf", fontSize);
    io.Fonts->AddFontFromFileTTF("resources/FiraMono-Bold.ttf", fontSize);

    ImGui_ImplOpenGL3_CreateFontsTexture();

    fontSizeChanged = false;
  }

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}

void Window::Render() const noexcept {
  ImGui::Render();
  glViewport(0, 0, Width(), Height());
  glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  SDL_Window *backup_current_window = SDL_GL_GetCurrentWindow();
  SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
  ImGui::UpdatePlatformWindows();
  ImGui::RenderPlatformWindowsDefault();
  SDL_GL_MakeCurrent(backup_current_window, backup_current_context);

  SDL_GL_SwapWindow(window);
}

void Window::MakeFrame(const char *name, const std::function<void()> &func, bool *visible) noexcept {
  ImGui::Begin(name, visible);
  bool isHovered = ImGui::IsWindowHovered();
  if (!isHovered) {
    ImGui::SetScrollY(scrollAmount);
  } else {
    scrollAmount = ImGui::GetScrollY();
  }
  func();
  ImGui::End();
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

    ImGui::EndMainMenuBar();
  }
}

void Window::MainView(LogicAnalyzer &logicAnalyzer) noexcept {
  popupHandler.RunPopups();

  ShowMainMenuBar(logicAnalyzer);

  PopupHandler::MakePopup("Settings", &openSettings, [&]() {
    static bool themeDark = theme == "dark";
    static bool themeChanged = themeDark;
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

    if (themeChanged != themeDark) {
      themeChanged = themeDark;

      std::ofstream file("settings.json");
      settings["style"]["theme"] = themeDark ? "dark" : "light";
      file << settings;
      file.close();
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
  popupHandler.RunPopups();

  ImGui::OpenPopup("Loading");
  ImGui::SetNextWindowPos({PosX() + Width() / 2.f, PosY() + Height() / 2.f}, 0, {0.5f, 0.5f});
  if (ImGui::BeginPopupModal("Loading", nullptr,
                             ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
    ImGui::Text("Parsing \"%s\"", logicAnalyzer.GetPath().string().c_str());
    ImGui::ProgressBar(-1.f * ImGui::GetTime());
    ImGui::EndPopup();
  }

  ShowMainMenuBar(logicAnalyzer);
}

static inline const std::string &MakeCombo(const std::string &label, const std::vector<std::string> &items) noexcept {
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
  popupHandler.RunPopups();

  ImGui::OpenPopup("Load a file");
  ImGui::SetNextWindowPos({PosX() + Width() / 2.f, PosY() + Height() / 2.f}, 0, {0.5f, 0.5f});
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

    ImGui::SameLine();
    ImGui::Text("%s",
                logicAnalyzer.fileIsSelected ? ("\"" + logicAnalyzer.GetPath().string() + "\"").c_str()
                                             : std::string("No file selected").c_str());

    float size = ImGui::CalcTextSize("Analyze!").x + ImGui::GetStyle().FramePadding.x * 2.0f;
    float avail = ImGui::GetContentRegionAvail().x;
    float off = (avail - size) * 0.5f;

    if (off > 0.0f)
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

    fileIsConfirmed = ImGui::Button("Analyze!");

    ImGui::EndPopup();
  }

  ShowMainMenuBar(logicAnalyzer);
}
