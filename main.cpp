#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <vector>
#include <fmt/core.h>

struct LineData {
	void SetAbsTime(const std::string& line) {
		absTime = GetTime(line);
	}

	void SetDeltaTime(const double& val) {
		deltaTime = val;
	}

	void SetIdentifier(const std::string& line) {
		identifier = GetDataByte(line);
	}

	void SetLen(const std::string& line) {
		len = GetDataByte(line) & 0x7f;
	}

	void SetCks(const std::string& line) {
		cks = GetDataByte(line);
	}

	double GetTime(const std::string& line) {
		return std::stod(line.substr(0, line.find_first_of(',')));
	}
	
	uint8_t GetDataByte(const std::string& line) {
		return std::stoi(line.substr(line.find_first_of(',') + 1, line.find_first_of(',', line.find_first_of(',') + 1)), nullptr, 16);
	}

	std::vector<uint8_t> bytes{};
	double absTime{}, deltaTime{};
	uint8_t identifier{}, len{}, cks{};
};

int main(int argc, char** argv) {
	if (argc < 2) {
		return -1;
	}

	std::ofstream output("out.txt");
	std::ifstream inputFile(argv[1]);
	std::vector<std::string> input{};
	std::string lineFile;
	
	std::getline(inputFile, lineFile); // skippa prima riga "Time [s],Value,Parity Error,Framing Error"

	while (std::getline(inputFile, lineFile)) {
		input.push_back(lineFile);
	}

	LineData previousData;

	output << fmt::format("{:^24}|{:^24}|{:^12}|{:^6}|{:^8}|{:^24}\n", "Absolute Time (s)", "Delta Time (s)", "Identifier", "Length", "Checksum", "Data");

	for (size_t i = 0; i < input.size();) {
		LineData data;
		data.SetAbsTime(input[i]);
		data.SetDeltaTime(previousData.absTime == 0 ? 0 : data.absTime - previousData.absTime);
		data.SetLen(input[i]);
		i+=2;
		data.SetIdentifier(input[i]);
		
		if (data.len == 0) {
			i++;
			data.SetLen(input[i]);
		}

		i++;

		std::string dataStr;
		for (size_t j = i, count = 0; j < data.len + i; j++, count++) {
			auto byte = data.GetDataByte(input[j]);
			if (count > 0 && (count % 8) == 0) {
				dataStr += "\n                        |                        |            |      |        |";
			}
			dataStr += fmt::format("{:02X} ", byte);
			data.bytes.push_back(byte);
		}

		i += data.len;

		data.SetCks(input[i++]);

		output << fmt::format("{:<24}|{:<24}|{:<12}|{:<6}|{:<8}|{:<24}\n", data.absTime, data.deltaTime, fmt::format("{:02X}", data.identifier), fmt::format("{:02X}", data.len), fmt::format("{:02X}", data.cks), dataStr);

		previousData = data;
	}

	output.close();
	return 0;
}
