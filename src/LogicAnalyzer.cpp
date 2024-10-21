#include <nfd.hpp>
#include <fmt/core.h>
#include <LogicAnalyzer.hpp>
#include <Popup.hpp>
#include <algorithm>

void LogicAnalyzer::OpenDialog() noexcept {
  state = None;

  nfdchar_t *outpath;
  constexpr nfdfilteritem_t filters[] = {{"Saleae or DSLogic raw data file", "bin"}};
  auto result = NFD::OpenDialog(outpath, filters, 1);
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
  return {reinterpret_cast<const char *>(buffer.data() + index), Size};
}

template <class T>
static inline T ReadScalarFromBuffer(const std::vector<uint8_t> &buffer, int index = 0) {
  if constexpr (std::is_same_v<T, uint64_t>) {
    uint32_t high = *reinterpret_cast<const uint32_t *>(&buffer[index + 0]);
    uint32_t low = *reinterpret_cast<const uint32_t *>(&buffer[index + 4]);

    return ((T)high << 32) | (T)low;
  }

  return *reinterpret_cast<const T *>(&buffer[index]);
}

std::vector<double> LogicAnalyzer::ParseSaleae(const std::vector<uint8_t> &buffer) {
  auto index = 0;
  auto identifier = ReadAsciiFromBuffer<8>(buffer);
  if (identifier != "<SALEAE>") {
    return {};
  }

  index += 8;

  auto version = ReadScalarFromBuffer<uint32_t>(buffer, index);
  index += sizeof(uint32_t);
  auto datatype = ReadScalarFromBuffer<uint32_t>(buffer, index);
  index += sizeof(uint32_t);

  if (version != 0 && datatype != 0) {
    return {};
  }

  initialState = ReadScalarFromBuffer<int>(buffer, index);
  index += sizeof(int);
  beginTime = ReadScalarFromBuffer<double>(buffer, index);
  index += sizeof(double);
  endTime = ReadScalarFromBuffer<double>(buffer, index);
  index += sizeof(double);
  numTransitions = ReadScalarFromBuffer<uint64_t>(buffer, index);
  index += sizeof(uint64_t);

  std::vector<double> result{};
  result.resize(buffer.size() - index);

  for (int i = index; i < buffer.size(); i += sizeof(double)) {
    result[i - index] = ReadScalarFromBuffer<double>(buffer, i);
  }

  return result;
}

std::vector<double> LogicAnalyzer::ParseDSLogic(const std::vector<uint8_t> &files) {
  MakePopupError("An error occurred",
                 fmt::format("Could not parse \"{}\". DSLogic not implemented!\n", filePath.string()), ParseError);

  return {};
}

std::vector<double> LogicAnalyzer::ParseFile() noexcept {
  auto file = std::ifstream(filePath, std::ios::binary);

  if (!file) {
    MakePopupError("An error occurred", fmt::format("Could not open \"{}\". Error opening file\n", filePath.string()),
                   FileOpenError);
    return {};
  }

  std::vector<uint8_t> buffer{std::istreambuf_iterator{file}, {}};

  file.close();

  auto result = ParseSaleae(buffer);
  if (result.empty()) {
    result = ParseDSLogic(buffer);
    if (result.empty()) {
      MakePopupError("An error occurred",
                     fmt::format("Could not parse \"{}\". This is neither a Saleae nor a DSLogic raw data file\n",
                                 filePath.string()),
                     ParseError);

      return {};
    }
  }

  state = FileParsed;

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
