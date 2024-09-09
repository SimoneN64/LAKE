#pragma once
#include <fstream>
#include <filesystem>
#include <cstdint>

namespace fs = std::filesystem;

struct PopupHandler;
enum CommunicationMode {
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

struct AnalyzerSettings {
  uint32_t bitrate;
  uint8_t bitsPerFrame;

  enum StopBitType {
    One,
    OneAndAHalf,
    Two
  } stopBitType = One;

  enum ParityBitType {
    None,
    Even,
    Odd
  } parityBitType = None;

  enum SignificantBitType {
    LSB,
    MSB
  } significantBitType = LSB;

  bool signalInversion = false;
};

struct LineData {
  void SetAbsTime(const std::string &line) { absTime = GetTime(line); }

  void SetDeltaTime(const double &val) { deltaTime = val; }

  void SetIdentifier(const std::string &line) { identifier = GetDataByte(line); }

  void SetLen(const CommunicationMode &commMode, const std::string &line) {
    if (commMode == KSTDLUNGO) {
      auto val = GetDataByte(line);
      len = val - (val > 0xf ? 0x80 : 0);
    } else if (commMode == KLUNGO_EXLEN) {
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

struct LogicAnalyzer {
  explicit LogicAnalyzer(PopupHandler &popupHandler) : popupHandler(popupHandler) {}

  void OpenDialog() noexcept;
  void OpenFile(const fs::path &path) noexcept;
  std::vector<LineData> ParseFile(std::ifstream &inputFile) const noexcept;

  CommunicationMode commMode = KSTDLUNGO;

  CommunicationMode StrToCommMode(const std::string &param) const noexcept;

private:
  PopupHandler &popupHandler;
  bool couldOpenCsv = false;
  std::ifstream csv;
  fs::path csvPath;
};
