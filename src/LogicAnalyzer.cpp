#include <nfd.hpp>
#include <fmt/core.h>
#include <LogicAnalyzer.hpp>
#include <Popup.hpp>

void LogicAnalyzer::OpenDialog() noexcept {
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

void LogicAnalyzer::OpenFile(const fs::path &path) noexcept {
  csv.open(path);
  if (!csv.good() || !csv.is_open()) {
    popupHandler.ScheduleErrorPopup("An error occurred", fmt::format("Could not open {}\n", path.string()));
    return;
  }

  csvPath = path;
  couldOpenCsv = true;
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

std::vector<LineData> LogicAnalyzer::ParseFile(std::ifstream &inputFile) const noexcept {
  std::vector<LineData> result{};
  std::vector<std::string> input{};
  std::string lineFile;

  std::getline(inputFile, lineFile); // skippa prima riga "Time [s],Value,Parity
  // Error,Framing Error"

  while (std::getline(inputFile, lineFile)) {
    input.push_back(lineFile);
  }

  LineData previousData;

  for (size_t i = 0; i < input.size();) {
    LineData data;
    data.SetAbsTime(input[i]);
    data.SetDeltaTime(previousData.absTime == 0 ? 0 : data.absTime - previousData.absTime);
    if (commMode == KSTDLUNGO) {
      data.SetLen(commMode, input[i]);
    }
    i += 2;
    data.SetIdentifier(input[i]);

    if (commMode == KLUNGO_EXLEN) {
      data.SetLen(commMode, input[++i]);
    }

    if (commMode == KSTDLUNGO && data.len == 0) {
      data.len = LineData::GetDataByte(input[++i]);
    }

    i++;

    std::string dataStr;
    for (size_t j = i, count = 0; j < data.len + i; j++, count++) {
      auto byte = LineData::GetDataByte(input[j]);
      if (count > 0 && (count % 8) == 0) {
        dataStr += "\n                        |                        |       "
                   "     |      |        |";
      }
      dataStr += fmt::format("{:02X} ", byte);
      data.bytes.push_back(byte);
    }

    i += data.len;

    data.SetCks(input[i++]);
    result.push_back(data);

    previousData = data;
  }

  return result;
}
