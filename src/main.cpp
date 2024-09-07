#include <cstdint>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#include <imgui_internal.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <FileHelper.hpp>

enum class CommunicationMode {
  KSTDLUNGO,
  KSTDCORTO,
  KLUNGO_EXLEN,
  KCORTOZERO,
  KLUNGOBMW,
  KSTD686A,
  KSTD33F1,
  KLUNGO_LEN,
  KLUNGO_BENCH,
  KCORTO_BENCH,
  KLUNGO_EDC15,
  INVALID,
  COMM_COUNT
};

CommunicationMode commMode = CommunicationMode::KSTDLUNGO;

struct LineData {
  void SetAbsTime(const std::string &line) { absTime = GetTime(line); }

  void SetDeltaTime(const double &val) { deltaTime = val; }

  void SetIdentifier(const std::string &line) { identifier = GetDataByte(line); }

  void SetLen(const std::string &line) {
    if (commMode == CommunicationMode::KSTDLUNGO) {
      auto val = GetDataByte(line);
      len = val - (val > 0xf ? 0x80 : 0);
    } else if (commMode == CommunicationMode::KLUNGO_EXLEN) {
      len = GetDataByte(line);
    }
  }

  void SetCks(const std::string &line) { cks = GetDataByte(line); }

  double GetTime(const std::string &line) { return std::stod(line.substr(0, line.find_first_of(','))); }

  uint8_t GetDataByte(const std::string &line) {
    return std::stoi(line.substr(line.find_first_of(',') + 1, line.find_first_of(',', line.find_first_of(',') + 1)),
                     nullptr, 16);
  }

  std::vector<uint8_t> bytes{};
  double absTime{}, deltaTime{};
  uint8_t identifier{}, len{}, cks{};
};

CommunicationMode StrToCommMode(const std::string &param) {
  if (param == "KSTDLUNGO")
    return CommunicationMode::KSTDLUNGO;
  if (param == "KSTDCORTO")
    return CommunicationMode::KSTDCORTO;
  if (param == "KLUNGO_EXLEN")
    return CommunicationMode::KLUNGO_EXLEN;
  if (param == "KCORTOZERO")
    return CommunicationMode::KCORTOZERO;
  if (param == "KLUNGOBMW")
    return CommunicationMode::KLUNGOBMW;
  if (param == "KSTD686A")
    return CommunicationMode::KSTD686A;
  if (param == "KSTD33F1")
    return CommunicationMode::KSTD33F1;
  if (param == "KLUNGO_LEN")
    return CommunicationMode::KLUNGO_LEN;
  if (param == "KLUNGO_BENCH")
    return CommunicationMode::KLUNGO_BENCH;
  if (param == "KCORTO_BENCH")
    return CommunicationMode::KCORTO_BENCH;
  if (param == "KLUNGO_EDC15")
    return CommunicationMode::KLUNGO_EDC15;

  return CommunicationMode::INVALID;
}

void ParseFile(std::ifstream &inputFile) {
  std::vector<std::string> input{};
  std::string lineFile;

  std::getline(inputFile, lineFile); // skippa prima riga "Time [s],Value,Parity
  // Error,Framing Error"

  while (std::getline(inputFile, lineFile)) {
    input.push_back(lineFile);
  }

  LineData previousData;

  /*output << fmt::format("{:^24}|{:^24}|{:^12}|{:^6}|{:^8}|{:^24}\n",
                        "Absolute Time (s)", "Delta Time (s)", "Identifier",
                        "Length", "Checksum", "Data");
  */
  for (size_t i = 0; i < input.size();) {
    LineData data;
    data.SetAbsTime(input[i]);
    data.SetDeltaTime(previousData.absTime == 0 ? 0 : data.absTime - previousData.absTime);
    if (commMode == CommunicationMode::KSTDLUNGO) {
      data.SetLen(input[i]);
    }
    i += 2;
    data.SetIdentifier(input[i]);

    if (commMode == CommunicationMode::KLUNGO_EXLEN) {
      data.SetLen(input[++i]);
    }

    if (commMode == CommunicationMode::KSTDLUNGO && data.len == 0) {
      data.len = data.GetDataByte(input[++i]);
    }

    i++;

    std::string dataStr;
    for (size_t j = i, count = 0; j < data.len + i; j++, count++) {
      auto byte = data.GetDataByte(input[j]);
      if (count > 0 && (count % 8) == 0) {
        dataStr += "\n                        |                        |       "
                   "     |      |        |";
      }
      dataStr += fmt::format("{:02X} ", byte);
      data.bytes.push_back(byte);
    }

    i += data.len;

    data.SetCks(input[i++]);

    /* output << fmt::format("{:<24}|{:<24}|{:<12}|{:<6}|{:<8}|{:<24}\n",
                          data.absTime, data.deltaTime,
                          fmt::format("{:02X}", data.identifier),
                          fmt::format("{:02X}", data.len),
                          fmt::format("{:02X}", data.cks), dataStr);
    */
    previousData = data;
  }

  // output.close();
}

enum ErrorCodes {
  eError_None,
  eError_VideoSystemFailure,
};

template <class... Args>
void DisplayErrorPopup(const char *fmt, Args... args) {
  ImGui::PushStyleColor(ImGuiCol_Text, 0xFF5D67);
  ImGui::OpenPopup("An error occurred");
  if (ImGui::BeginPopupModal("An error occurred", nullptr)) {
    ImGui::PopStyleColor();
    ImGui::Text(fmt, args...);
    ImGui::EndPopup();
  } else {
    ImGui::PopStyleColor();
  }
}

struct LogicViewArea {
  void OpenDialog() {
    fs::path resultPath;
    if (!Helpers::FileDialog::Get().OpenDialog(resultPath) &&
        Helpers::FileDialog::Get().GetErrorState() == Helpers::FileDialog::Error) {
      DisplayErrorPopup(Helpers::FileDialog::Get().GetError().c_str());
    } else {
      OpenFile(resultPath);
    }
  }

  void OpenFile(const fs::path &path) {
    csv.open(path);
    if (!csv.good() || !csv.is_open()) {
      DisplayErrorPopup("Could not open %s\n", csvPath.c_str());
    }

    csvPath = path;
    couldOpenCsv = true;
  }

private:
  bool couldOpenCsv = false;
  std::ifstream csv;
  fs::path csvPath;
};

int main() {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fmt::print("Could not initialize video system! (SDL error: {})\n", SDL_GetError());
    return eError_VideoSystemFailure;
  }

  SDL_Window *window = SDL_CreateWindow(
    "LAKE", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

  if (window == nullptr) {
    fmt::print("Could not initialize window! (SDL error: {})\n", SDL_GetError());
    return eError_VideoSystemFailure;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
  if (renderer == nullptr) {
    fmt::print("Could not initialize renderer! (SDL error: {})\n", SDL_GetError());
    return eError_VideoSystemFailure;
  }

  if (!SDL_SetRenderVSync(renderer, SDL_WINDOW_SURFACE_VSYNC_ADAPTIVE)) {
    fmt::print("Adaptive VSync not available. Falling back to frame-swapping\n");
    SDL_SetRenderVSync(renderer, true);
  }

  SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
  SDL_ShowWindow(window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;

  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGuiStyle &style = ImGui::GetStyle();
  style.WindowRounding = 0.0f;
  style.Colors[ImGuiCol_WindowBg].w = 1.0f;

  auto firaMonoRegular = io.Fonts->AddFontFromFileTTF("resources/FiraMono-Regular.ttf", 16.f);
  auto firaMonoBold = io.Fonts->AddFontFromFileTTF("resources/FiraMono-Bold.ttf", 16.f);
  ImGui::StyleColorsDark();

  ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
  ImGui_ImplSDLRenderer3_Init(renderer);

  bool done = false;
  bool themeDark = true;
  LogicViewArea logicViewArea;

  while (!done) {
    int windowW, windowH;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);
      if (event.type == SDL_EVENT_QUIT)
        done = true;
      if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
        done = true;
    }

    if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
      SDL_Delay(10);
      continue;
    }

    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    SDL_GetWindowSizeInPixels(window, &windowW, &windowH);

    {
      ImGui::SetNextWindowPos({});
      ImGui::SetNextWindowSize({static_cast<float>(windowW), static_cast<float>(windowH)});
      if (ImGui::Begin("Main view", nullptr,
                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoScrollWithMouse)) {
        static bool openSettings = false;
        if (ImGui::BeginMenuBar()) {
          if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open")) {
              logicViewArea.OpenDialog();
            }

            done = ImGui::MenuItem("Exit");
            ImGui::EndMenu();
          }

          if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("Settings")) {
              openSettings = true;
            }
            ImGui::EndMenu();
          }
          ImGui::EndMenuBar();
        }

        if (ImGui::BeginPopup("Test")) {
          ImGui::Text("Ciao ciao");
          ImGui::EndPopup();
        }

        if (openSettings) {
          ImGui::OpenPopup("##settings");
        }

        if (ImGui::BeginPopupModal("##settings", &openSettings)) {
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

          ImGui::EndPopup();
        }

        ImGui::SetNextWindowSizeConstraints({float(windowW) / 4, float(windowH)},
                                            {float(windowW) * 0.75f, float(windowH)});
        if (ImGui::BeginChild("KLine Messages", {}, ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders)) {
          for (int i = 0; i < 100; i++) {
            ImGui::Text("Lorem ipsum");
          }
          ImGui::EndChild();
        }

        ImGui::SameLine();

        ImGui::SetNextWindowSizeConstraints({float(windowW) / 4, float(windowH)},
                                            {float(windowW) * 0.75f, float(windowH)});
        if (ImGui::BeginChild("Graph View")) {
          ImGui::Text("Lorem ipsum");
          ImGui::EndChild();
        }

        ImGui::End();
      }
    }
    ImGui::Render();
    SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
    SDL_RenderPresent(renderer);

    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }

  // Cleanup
  ImGui_ImplSDLRenderer3_Shutdown();
  ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return eError_None;
}
