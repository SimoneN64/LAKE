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

    // if(logicAnalyzer.doneParsing) window.MainView(logicAnalyzer);
    // else window.ShowLoading();

    window.MainView(logicAnalyzer);

    window.Render();
  }

  return 0;
}
