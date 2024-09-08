#include "fmt/compile.h"


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
      popupHandler.RunPopups();

      static float menuBarHeight = 0;
      static bool openSettings = false;
      if (ImGui::BeginMainMenuBar()) {
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

      ImGui::SetNextWindowPos({0.f, menuBarHeight});
      ImGui::SetNextWindowSize(
        {static_cast<float>(window.Width()), static_cast<float>(window.Height()) - menuBarHeight});
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
            ImGui::Text("FF");
          }
        });

        Window::MakeFrame("##length", {lengthWidth, -1}, [&]() {
          ImGui::SetScrollY(scrollbar);
          for (int i = 0; i < 100; i++) {
            ImGui::Text("F");
          }
        });

        Window::MakeFrame("##dataBytes", {dataBytesWidth, -1}, [&]() {
          ImGui::SetScrollY(scrollbar);
          for (int i = 0; i < 100; i++) {
            for (int j = 0; j < 8; j++) {
              ImGui::Text("FF ");
              if (ImGui::IsItemHovered()) {
                if (ImGui::BeginTooltip()) {
                  ImGui::Text("Immagina... una forma d'onda quadra");
                  ImGui::EndTooltip();
                }
              }
              ImGui::SameLine();
            }
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() - ImGui::CalcTextSize("F").x * 8);
            ImGui::Text("........");
          }
        });

        Window::MakeFrame("##cks", {cksWidth, -1}, [&]() {
          ImGui::SetScrollY(scrollbar);
          for (int i = 0; i < 100; i++) {
            ImGui::Text("FF");
          }
        });

        Window::MakeFrame("##absTime", {absTimeWidth, -1}, [&]() {
          ImGui::SetScrollY(scrollbar);
          for (int i = 0; i < 100; i++) {
            ImGui::Text("10000.000000");
          }
        });

        Window::MakeFrame(
          "##deltaTime", {deltaTimeWidth, -1},
          [&]() {
            scrollbar = ImGui::GetScrollY();

            for (int i = 0; i < 100; i++) {
              ImGui::Text("10000.000");
            }
          },
          false, true);

        ImGui::End();
      }
    }

    window.Render();
  }

  return 0;
}
