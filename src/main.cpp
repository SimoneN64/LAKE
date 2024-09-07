#include <cstdint>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <Window.hpp>
#include <Parser.hpp>
#include <Popup.hpp>
#include <nfd.hpp>
#include <filesystem>
#include <LogicAnalyzerView.hpp>

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

int main() {
  bool themeDark = true;
  PopupHandler popupHandler;
  LogicAnalyzerView logicAnalyzerView(popupHandler);
  Window window;
  if (window.errorState == Window::VideoSystemFailure) {
    return -1;
  }

  while (!window.ShouldQuit()) {
    window.HandleEvents();

    if (window.IsMinimized())
      continue;

    Window::NewFrame();

    {
      ImGui::SetNextWindowPos({});
      ImGui::SetNextWindowSize({static_cast<float>(window.Width()), static_cast<float>(window.Height())});
      if (ImGui::Begin("Main view", nullptr,
                       ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDecoration |
                         ImGuiWindowFlags_NoScrollWithMouse)) {

        popupHandler.RunPopups();

        static bool openSettings = false;
        if (ImGui::BeginMenuBar()) {
          if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open")) {
              logicAnalyzerView.OpenDialog();
            }

            if (ImGui::MenuItem("Exit")) {
              window.Quit();
            }
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

        PopupHandler::MakePopup("Settings", &openSettings, [&themeDark]() {
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

        static float identifierSize = ImGui::CalcTextSize("Identifier").x + 2;
        static float lengthSize = ImGui::CalcTextSize("Length").x + 2;
        static float messageDataSize = ImGui::CalcTextSize("DE AD BE EF EF BE AD DE").x + 2;
        static float checksumSize = ImGui::CalcTextSize("Checksum").x + 2;
        static float totalPadding = ImGui::GetStyle().WindowPadding.x + ImGui::GetStyle().FrameBorderSize + 2;
        static float totalW = identifierSize + totalPadding + lengthSize + totalPadding + messageDataSize +
          totalPadding + checksumSize + totalPadding * 3;

        ImGui::SetNextWindowSizeConstraints({totalW, -1}, {totalW, -1});
        if (ImGui::BeginChild("##messages", {}, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_Borders,
                              ImGuiWindowFlags_NoScrollbar)) {
          /*
          std::vector<uint8_t> bytes{};
          double absTime{}, deltaTime{};
          uint8_t identifier{}, len{}, cks{};
           */
          static float scroll = 0;

          ImGui::Text("Identifier");
          ImGui::SameLine(0, totalPadding);
          ImGui::Text("Length");
          ImGui::SameLine(0, totalPadding);
          ImGui::Text("Message");
          ImGui::SameLine(0, messageDataSize - 24);
          ImGui::Text("Checksum");

          if (ImGui::BeginChild("##identifier", {}, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeX,
                                ImGuiWindowFlags_NoScrollbar)) {
            ImGui::SetWindowSize({identifierSize, -1});
            ImGui::SetScrollY(scroll);
            for (int i = 0; i < 128; i++) {
              ImGui::Text("12");
            }
            ImGui::EndChild();
          }

          ImGui::SameLine();

          if (ImGui::BeginChild("##length", {}, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeX,
                                ImGuiWindowFlags_NoScrollbar)) {
            ImGui::SetWindowSize({lengthSize, -1});
            ImGui::SetScrollY(scroll);
            for (int i = 0; i < 128; i++) {
              ImGui::Text("8");
            }
            ImGui::EndChild();
          }

          ImGui::SameLine();

          if (ImGui::BeginChild("##messageData", {}, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeX)) {
            ImGui::SetWindowSize({messageDataSize + 24, -1});
            scroll = ImGui::GetScrollY();
            for (int i = 0; i < 128; i++) {
              ImGui::Text("DE AD BE EF EF BE AD DE");
            }
            ImGui::EndChild();
          }

          ImGui::SameLine();

          if (ImGui::BeginChild("##cks", {}, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeX,
                                ImGuiWindowFlags_NoScrollbar)) {
            ImGui::SetWindowSize({checksumSize, -1});
            ImGui::SetScrollY(scroll);
            for (int i = 0; i < 128; i++) {
              ImGui::Text("62");
            }
            ImGui::EndChild();
          }

          ImGui::EndChild();
        }

        ImGui::SameLine();

        if (ImGui::BeginChild("Graph", {}, ImGuiChildFlags_FrameStyle,
                              ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize)) {
          ImGui::Text("Lorem ipsum");
          ImGui::EndChild();
        }

        ImGui::End();
      }
    }
    window.Render();
  }

  return 0;
}
