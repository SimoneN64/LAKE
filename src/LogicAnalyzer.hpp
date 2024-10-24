#pragma once
#include <atomic>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <vector>
#include <thread>
#include <mutex>

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

struct LogicAnalyzer {
  explicit LogicAnalyzer(PopupHandler &popupHandler) : popupHandler(popupHandler) {}
  ~LogicAnalyzer() { stopThread(); }

  void test() {}
  void OpenDialog() noexcept;
  auto &GetPath() const noexcept { return filePath; }

  std::vector<double> ParseFile() noexcept;

  void stopThread() {
    if (parserThread.joinable())
      parserThread.join();
  }

  void startThread() {
    stopThread();
    parserThread = std::thread([&]() { ParseFile(); });
  }

  CommunicationMode commMode = KSTDLUNGO;

  static CommunicationMode StrToCommMode(const std::string &param) noexcept;

  enum State {
    None,
    FileOpenError,
    FileSelected,
    FileConfirmed,
    ParseError,
    FileParsed,
  };

  std::atomic<State> state = None;
  AnalyzerSettings settings{};
  std::thread parserThread{};
  int initialState{};
  double beginTime{}, endTime{};
  uint64_t numTransitions{};

private:
  void MakePopupError(const std::string &title, const std::string &msg, State newState);
  std::vector<double> ParseSaleae(const std::vector<uint8_t> &buffer);
  std::vector<double> ParseDSLogic(const std::vector<uint8_t> &buffer);

  PopupHandler &popupHandler;
  fs::path filePath{};
};
