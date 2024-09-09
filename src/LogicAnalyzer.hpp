#pragma once
#include <atomic>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <vector>

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
  uint32_t bitrate{10416};
  uint8_t bitsPerFrame{8};

  enum StopBitType { One, OneAndAHalf, Two } stopBitType = One;

  enum ParityBitType { None, Even, Odd } parityBitType = None;

  enum SignificantBitType { LSB, MSB } significantBitType = LSB;

  bool signalInversion = false;
};

struct LineData {
  void SetAbsTime(const std::string &line) { absTime = GetTime(line); }
  void SetDeltaTime(const double &val) { deltaTime = val; }
  void SetIdentifier(const std::string &line) { identifier = GetDataByte(line); }
  void SetLen(const CommunicationMode &commMode, const std::string &line);
  void SetCks(const std::string &line) { cks = GetDataByte(line); }

  static double GetTime(const std::string &line);
  static uint8_t GetDataByte(const std::string &line);

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

  static CommunicationMode StrToCommMode(const std::string &param) noexcept;

private:
  std::atomic_bool isFinished = false;
  PopupHandler &popupHandler;
  bool couldOpenCsv = false;
  std::ifstream csv;
  fs::path csvPath;
};
