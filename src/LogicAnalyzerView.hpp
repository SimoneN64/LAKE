#pragma once
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

struct PopupHandler;

struct LogicAnalyzerView {
  explicit LogicAnalyzerView(PopupHandler &popupHandler) : popupHandler(popupHandler) {}

  void OpenDialog() noexcept;
  void OpenFile(const fs::path &path) noexcept;

private:
  PopupHandler &popupHandler;
  bool couldOpenCsv = false;
  std::ifstream csv;
  fs::path csvPath;
};
