#pragma once
#include <SDL3/SDL.h>

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
  static void NewFrame() noexcept;
  [[nodiscard]] int Width() const noexcept {
    int w, h;
    SDL_GetRenderOutputSize(renderer, &w, &h);
    return w;
  }

  [[nodiscard]] int Height() const noexcept {
    int w, h;
    SDL_GetRenderOutputSize(renderer, &w, &h);
    return h;
  }

  [[nodiscard]] bool IsMinimized() const noexcept { return SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED; }

  void Render() const noexcept;

private:
  SDL_Window *window{};
  SDL_Renderer *renderer{};
  bool done = false;
};
