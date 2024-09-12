#pragma once
#include <atomic>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <vector>
#include <thread>

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
  float stopBitType = 1.f;

  enum ParityBitType : uint8_t { None, Even, Odd } parityBitType = None;
  enum SignificantBitType : uint8_t { LSB, MSB } significantBitType = LSB;

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
  ~LogicAnalyzer() { stopThread(); }

  void test() {}
  void OpenDialog() noexcept;
  auto &GetPath() const noexcept { return filePath; }
  std::vector<LineData> ParseFile(std::ifstream &inputFile) noexcept;

  void stopThread() {
    if (parserThread.joinable())
      parserThread.join();
  }

  void startThread() {
    stopThread();
    parserThread = std::thread([&]() { ParseFile(file); });
  }

  CommunicationMode commMode = KSTDLUNGO;

  static CommunicationMode StrToCommMode(const std::string &param) noexcept;

  enum State {
    None,
    FileSelected,
    FileConfirmed,
    FileParsed,
    ParseError
  };

  std::atomic<State> state = None;
  AnalyzerSettings settings{};
  std::thread parserThread{};

private:
  void MakePopupErrorParsing(const std::string& title, const std::string& msg);

  PopupHandler &popupHandler;
  std::ifstream file{};
  fs::path filePath{};
};
