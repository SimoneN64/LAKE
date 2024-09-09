#pragma once
#include <SDL3/SDL.h>
#include <functional>
#include <Popup.hpp>

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

  [[nodiscard]] bool IsMinimized() const noexcept { return SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED; }

  void Render() const noexcept;
  static void MakeFrame(const char *name, ImVec2 size, const std::function<void()> &func, float &scrollAmount,
                        bool sameLine = true, bool scrollBar = false) noexcept;

  void MainView(LogicAnalyzer &) noexcept;

  auto &GetPopupHandler() noexcept { return popupHandler; }

private:
  PopupHandler popupHandler;
  SDL_Window *window{};
  SDL_Renderer *renderer{};
  bool done = false;
  bool themeDark = true;
  bool fontSizeChanged = false;
  float fontSize = 20.f, prevFontSize = fontSize;
};
