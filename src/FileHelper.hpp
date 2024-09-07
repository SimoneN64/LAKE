#pragma once
#include <nfd.hpp>
#include <fstream>
#include <filesystem>
#include <optional>
#include <fmt/core.h>

namespace fs = std::filesystem;

namespace Helpers {
struct FileDialog {
  enum ErrorState {
    Good,
    Cancel,
    Error,
  };

  FileDialog() { NFD::Init(); }

  ~FileDialog() { NFD::Quit(); }

  static FileDialog &Get() noexcept {
    static FileDialog instance;
    return instance;
  }

  [[nodiscard]] ErrorState GetErrorState() const noexcept { return errorState; }
  [[nodiscard]] std::string GetError() const noexcept { return error; }

  [[nodiscard]] bool OpenDialog(fs::path &pathResult) noexcept {
    nfdu8char_t *outPath;

    constexpr nfdu8filteritem_t filters[] = {{"Saleae/DSLogic TXT file", "txt"}, {"Saleae/DSLogic CSV file", "csv"}};
    const nfdresult_t result = NFD::OpenDialog(outPath, filters, 2);

    if (result == NFD_CANCEL) {
      errorState = Cancel;
      fmt::print("Operation canceled.\n");
      return false;
    }

    if (result == NFD_ERROR) {
      errorState = Error;
      error = fmt::format("Could not open file {}. Error: {}\n", outPath, NFD::GetError());
      return false;
    }

    pathResult = outPath;
    return true;
  }

private:
  ErrorState errorState = Good;
  std::string error;
};
} // namespace Helpers
