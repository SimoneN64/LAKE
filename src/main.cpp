#include <cstdint>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

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

CommunicationMode commMode = KSTDLUNGO;

struct LineData {
  void SetAbsTime(const std::string &line) { absTime = GetTime(line); }

  void SetDeltaTime(const double &val) { deltaTime = val; }

  void SetIdentifier(const std::string &line) {
    identifier = GetDataByte(line);
  }

  void SetLen(const std::string &line) {
    if (commMode == KSTDLUNGO) {
      auto val = GetDataByte(line);
      len = val - (val > 0xf ? 0x80 : 0);
    } else if (commMode == KLUNGO_EXLEN) {
      len = GetDataByte(line);
    }
  }

  void SetCks(const std::string &line) { cks = GetDataByte(line); }

  double GetTime(const std::string &line) {
    return std::stod(line.substr(0, line.find_first_of(',')));
  }

  uint8_t GetDataByte(const std::string &line) {
    return std::stoi(
        line.substr(line.find_first_of(',') + 1,
                    line.find_first_of(',', line.find_first_of(',') + 1)),
        nullptr, 16);
  }

  std::vector<uint8_t> bytes{};
  double absTime{}, deltaTime{};
  uint8_t identifier{}, len{}, cks{};
};

CommunicationMode StrToCommMode(const std::string &param) {
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

int main(int argc, char **argv) {
  if (argc < 2) {
    return -1;
  }

  if (argc >= 3) {
    commMode = StrToCommMode(argv[2]);
    if (commMode == INVALID) {
      fmt::print("Inexistent communication mode. Available options "
                 "are:\n\t{}\n\t{}\n\t{}\n\t{}\n\t{}\n\t{}\n\t{}\n\t{}\n\t{}"
                 "\n\t{}\n\t{}",
                 "KSTDLUNGO", "KSTDCORTO", "KLUNGO_EXLEN", "KCORTOZERO",
                 "KLUNGOBMW", "KSTD686A", "KSTD33F1", "KLUNGO_LEN",
                 "KLUNGO_BENCH", "KCORTO_BENCH", "KLUNGO_EDC15");
      return -2;
    }
  }

  std::ofstream output("out.txt");
  std::ifstream inputFile(argv[1]);
  std::vector<std::string> input{};
  std::string lineFile;

  std::getline(inputFile, lineFile); // skippa prima riga "Time [s],Value,Parity
                                     // Error,Framing Error"

  while (std::getline(inputFile, lineFile)) {
    input.push_back(lineFile);
  }

  LineData previousData;

  output << fmt::format("{:^24}|{:^24}|{:^12}|{:^6}|{:^8}|{:^24}\n",
                        "Absolute Time (s)", "Delta Time (s)", "Identifier",
                        "Length", "Checksum", "Data");

  for (size_t i = 0; i < input.size();) {
    LineData data;
    data.SetAbsTime(input[i]);
    data.SetDeltaTime(
        previousData.absTime == 0 ? 0 : data.absTime - previousData.absTime);
    if (commMode == KSTDLUNGO) {
      data.SetLen(input[i]);
    }
    i += 2;
    data.SetIdentifier(input[i]);

    if (commMode == KLUNGO_EXLEN) {
      data.SetLen(input[++i]);
    }

    if (commMode == KSTDLUNGO && data.len == 0) {
      data.len = data.GetDataByte(input[++i]);
    }

    i++;

    std::string dataStr;
    for (size_t j = i, count = 0; j < data.len + i; j++, count++) {
      auto byte = data.GetDataByte(input[j]);
      if (count > 0 && (count % 8) == 0) {
        dataStr += "\n                        |                        |       "
                   "     |      |        |";
      }
      dataStr += fmt::format("{:02X} ", byte);
      data.bytes.push_back(byte);
    }

    i += data.len;

    data.SetCks(input[i++]);

    output << fmt::format("{:<24}|{:<24}|{:<12}|{:<6}|{:<8}|{:<24}\n",
                          data.absTime, data.deltaTime,
                          fmt::format("{:02X}", data.identifier),
                          fmt::format("{:02X}", data.len),
                          fmt::format("{:02X}", data.cks), dataStr);

    previousData = data;
  }

  output.close();
  return 0;
}
