#include <nfd.hpp>
#include <fmt/core.h>
#include <LogicAnalyzer.hpp>
#include <Popup.hpp>
#include <algorithm>
#include <unarr.h>

void LogicAnalyzer::OpenDialog() noexcept {
  state = None;

  nfdchar_t *outpath;
  constexpr nfdfilteritem_t filters[] = {{"Saleae project file", "sal"}, {"DSView project file", "dsl"}};
  auto result = NFD::OpenDialog(outpath, filters, 2);
  if (result == NFD_ERROR) {
    MakePopupError("An error occurred", fmt::format("Could not open \"{}\". Error: {}\n", outpath, NFD::GetError()),
                   FileOpenError);
    return;
  }

  filePath = outpath;
  state = FileSelected;
}

void LogicAnalyzer::MakePopupError(const std::string &title, const std::string &msg, LogicAnalyzer::State newState) {
  std::mutex m;
  std::lock_guard guard(m);
  state = newState;
  filePath.clear();
  popupHandler.ScheduleErrorPopup(title, msg);
}

template <int Size>
static inline std::string ReadAsciiFromBuffer(const std::vector<uint8_t> &buffer, int index = 0) {
  std::string result{};

  for (int i = index; i < index + Size; i++) {
    result.push_back(buffer[i]);
  }

  return result;
}

void LogicAnalyzer::ParseSaleae(const std::vector<ArchiveEntry> &files) {
  auto digitalIndex = std::find_if(files.begin(), files.end(),
                                   [](const ArchiveEntry &entry) { return entry.name.starts_with("digital"); });

  if (digitalIndex == files.end()) {
    MakePopupError(
      "An error occurred",
      fmt::format("Could not parse \"{}\". No files seem to be the expected \"digital-X.bin\"\n", filePath.string()),
      ParseError);
    return;
  }

  auto identifier = ReadAsciiFromBuffer<8>(digitalIndex->fileBuffer);
  if (identifier != "<SALEAE>") {
    MakePopupError(
      "An error occurred",
      fmt::format("Could not parse \"{}\". Incorrect header; <SALEAE> marker not present\n", filePath.string()),
      ParseError);
  }

  MakePopupError("An error occurred", fmt::format("Could not parse \"{}\". Error opening archive\n", filePath.string()),
                 ParseError);
}

void LogicAnalyzer::ParseDSLogic(const std::vector<ArchiveEntry> &files) {
  MakePopupError("An error occurred", fmt::format("Could not parse \"{}\". Error opening archive\n", filePath.string()),
                 ParseError);
}

std::vector<LineData> LogicAnalyzer::ParseFile(std::ifstream &inputFile) noexcept {
  std::vector<LineData> result{};

  auto stream = ar_open_file(filePath.string().c_str());

  if (!stream) {
    MakePopupError("An error occurred",
                   fmt::format("Could not open \"{}\". Error opening archive\n", filePath.string()), FileOpenError);
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
    MakePopupError(
      "An error occurred",
      fmt::format("Could not open \"{}\". Error unzipping file. Is this a valid Saleae or DSLogic project?\n",
                  filePath.string()),
      FileOpenError);
    return {};
  }

  std::vector<ArchiveEntry> files;
  bool isSaleae = false;
  // find "meta.json" -> isSaleae = true
  while (ar_parse_entry(archive)) {
    auto filename = std::string(ar_entry_get_name(archive));
    isSaleae = filename == "meta.json";
    std::vector<uint8_t> buffer;
    buffer.resize(ar_entry_get_size(archive));
    ar_entry_uncompress(archive, buffer.data(), buffer.size());
    files.push_back({filename, buffer});
  }

  ar_close_archive(archive);
  ar_close(stream);

  isSaleae ? ParseSaleae(files) : ParseDSLogic(files);

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
