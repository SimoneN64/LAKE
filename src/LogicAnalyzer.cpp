#include <nfd.hpp>
#include <fmt/core.h>
#include <LogicAnalyzer.hpp>
#include <Popup.hpp>

void LogicAnalyzer::OpenDialog() noexcept {
  nfdchar_t *outpath;
  constexpr nfdfilteritem_t filters[] = {{"Saleae project file", "sal"}, {"DSView project file", "dsl"}};
  auto result = NFD::OpenDialog(outpath, filters, 2);
  if (result == NFD_ERROR) {
    popupHandler.ScheduleErrorPopup("An error occurred",
                                    fmt::format("Could not open {}. Error: {}\n", outpath, NFD::GetError()));
    return;
  }

  OpenFile(outpath);
}

void LogicAnalyzer::OpenFile(const fs::path &path) noexcept {
  file.open(path);
  if (!file.good() || !file.is_open()) {
    popupHandler.ScheduleErrorPopup("An error occurred", fmt::format("Could not open {}\n", path.string()));
    return;
  }

  fileIsLoaded = true;
  filePath = path;
}

CommunicationMode LogicAnalyzer::StrToCommMode(const std::string &param) noexcept {
  if (param == "KSTDLUNGO")
    return KSTDLUNGO;
  if (param == "KSTDCORTO")
    return KSTDCORTO;
  if (param == "KLUNGO_EXLEN")
    return KLUNGO_EXLEN;
  if (param == "KCORTOZERO")
    return KCORTOZERO;
  if (param == "KLUNGOBMW")
    return KLUNGOBMW;
  if (param == "KSTD686A")
    return KSTD686A;
  if (param == "KSTD33F1")
    return KSTD33F1;
  if (param == "KLUNGO_LEN")
    return KLUNGO_LEN;
  if (param == "KLUNGO_BENCH")
    return KLUNGO_BENCH;
  if (param == "KCORTO_BENCH")
    return KCORTO_BENCH;
  if (param == "KLUNGO_EDC15")
    return KLUNGO_EDC15;

  return INVALID;
}

std::vector<LineData> LogicAnalyzer::ParseFile(std::ifstream &inputFile) noexcept {
  std::vector<LineData> result{};

  isFinishedParsing = true;

  return result;
}
