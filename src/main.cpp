#include <cstdint>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <Window.hpp>
#include <Parser.hpp>
#include <nfd.hpp>
#include <filesystem>
#include <LogicAnalyzer.hpp>


int main() {
  Window window;
  LogicAnalyzer logicAnalyzer(window.GetPopupHandler());

  if (window.errorState == Window::VideoSystemFailure) {
    return -1;
  }

  while (!window.ShouldQuit()) {
    window.HandleEvents();

    if (window.IsMinimized())
      continue;

    window.NewFrame();

    switch (logicAnalyzer.state) {
      case LogicAnalyzer::None:
      case LogicAnalyzer::FileSelected:
      case LogicAnalyzer::ParseError:
        window.AskForFileAndLineSettings(logicAnalyzer);
        break;
      case LogicAnalyzer::FileConfirmed:
        logicAnalyzer.startThread();
        window.ShowLoading(logicAnalyzer);
        break;
      case LogicAnalyzer::FileParsed:
        logicAnalyzer.stopThread();
        window.MainView(logicAnalyzer);
        break;
    }

    window.Render();
  }

  return 0;
}
