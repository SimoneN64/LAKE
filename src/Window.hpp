#pragma once
#include <nlohmann/json.hpp>
#include <SDL3/SDL.h>
#include <functional>
#include <Popup.hpp>
#include <array>
#include <fstream>

struct ImVec2;
struct ImFont;
struct LogicAnalyzer;

struct Window {
  enum ErrorState {
    None,
    VideoSystemFailure,
  } errorState = None;

  Window() noexcept;
  ~Window() noexcept;

  void Quit() noexcept { done = true; }
  [[nodiscard]] bool ShouldQuit() const noexcept { return done; }
  [[nodiscard]] SDL_Window *GetHandle() const noexcept { return window; }
  void HandleEvents() noexcept;
  void NewFrame() noexcept;
  [[nodiscard]] int Width() const noexcept {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    return w;
  }

  [[nodiscard]] int Height() const noexcept {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    return h;
  }

  [[nodiscard]] int PosX() const noexcept {
    int x, y;
    SDL_GetWindowPosition(window, &x, &y);
    return x;
  }

  [[nodiscard]] int PosY() const noexcept {
    int x, y;
    SDL_GetWindowPosition(window, &x, &y);
    return y;
  }

  [[nodiscard]] bool IsMinimized() const noexcept { return SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED; }

  void Render() const noexcept;
  void MakeFrame(const char *name, const std::function<void()> &func, bool *visible = nullptr) noexcept;

  void MainView(LogicAnalyzer &) noexcept;
  void ShowLoading(LogicAnalyzer &) noexcept;
  void AskForFileAndLineSettings(LogicAnalyzer &) noexcept;

  auto &GetPopupHandler() noexcept { return popupHandler; }

private:
  struct JsonParseResult {
    nlohmann::json json;
    enum {
      None,
      ParseError,
      OpenError,
    } error;
  };
  static JsonParseResult OpenOrCreateSettings();
  void ShowMainMenuBar(LogicAnalyzer &) noexcept;
  PopupHandler popupHandler;
  SDL_Window *window{};
  SDL_GLContext glContext{};
  bool done = false;
  bool fontSizeChanged = false;
  float fontSize = 20.f, prevFontSize = fontSize;
  bool openSettings = false;
  float scrollAmount = 0.f;
  std::string theme = "dark";
  nlohmann::json settings{};
};
