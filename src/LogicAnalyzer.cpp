#include <nfd.hpp>
#include <fmt/core.h>
#include <LogicAnalyzer.hpp>
#include <Popup.hpp>
#include <unarr.h>

void LogicAnalyzer::OpenDialog() noexcept {
  nfdchar_t *outpath;
  constexpr nfdfilteritem_t filters[] = {{"Saleae project file", "sal"}, {"DSView project file", "dsl"}};
  auto result = NFD::OpenDialog(outpath, filters, 2);
  if (result == NFD_ERROR) {
    popupHandler.ScheduleErrorPopup("An error occurred",
                                    fmt::format("Could not open {}. Error: {}\n", outpath, NFD::GetError()));
    return;
  }

  filePath = outpath;
  fileIsSelected = true;
}

std::vector<LineData> LogicAnalyzer::ParseFile(std::ifstream &inputFile) noexcept {
  std::vector<LineData> result{};

  auto stream = ar_open_file(filePath.string().c_str());

  if (!stream) {
    errorParsing = true;
    isFinishedParsing = false;
    fileIsSelected = false;
    popupHandler.ScheduleErrorPopup("An error occurred",
                                    fmt::format("Could not open {}. Error: {}\n", filePath.string(), NFD::GetError()));
    return {};
  }


  ar_archive *archive = ar_open_zip_archive(stream, false);

  if (!archive)
    archive = ar_open_rar_archive(stream);
  if (!archive)
    archive = ar_open_7z_archive(stream);
  if (!archive)
    archive = ar_open_tar_archive(stream);

  if (!archive) {
    ar_close(stream);
    errorParsing = true;
    isFinishedParsing = false;
    fileIsSelected = false;
    popupHandler.ScheduleErrorPopup("An error occurred",
                                    fmt::format("Could not open {}. Error: {}\n", filePath.string(), NFD::GetError()));
    return {};
  }

  isFinishedParsing = true;

  ar_close_archive(archive);
  ar_close(stream);
  return result;
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
