#include <nfd.hpp>
#include <fmt/core.h>
#include <LogicAnalyzerView.hpp>
#include <Popup.hpp>

void LogicAnalyzerView::OpenDialog() noexcept {
  nfdchar_t *outpath;
  constexpr nfdfilteritem_t filters[] = {{"Saleae/DSView TXT file", "txt"}, {"Saleae/DSView CSV file", "csv"}};
  auto result = NFD::OpenDialog(outpath, filters, 2);
  if (result == NFD_ERROR) {
    popupHandler.ScheduleErrorPopup("An error occurred",
                                    fmt::format("Could not open {}. Error: {}\n", outpath, NFD::GetError()));
    return;
  }

  OpenFile(outpath);
}

void LogicAnalyzerView::OpenFile(const fs::path &path) noexcept {
  csv.open(path);
  if (!csv.good() || !csv.is_open()) {
    popupHandler.ScheduleErrorPopup("An error occurred", fmt::format("Could not open {}\n", path.string()));
    return;
  }

  csvPath = path;
  couldOpenCsv = true;
}
