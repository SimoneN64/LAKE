#pragma once
#include <functional>
#include <imgui.h>
#include <string>
#include <vector>

struct PopupData {
  std::string title;
  std::string message;
  std::function<void()> impl = nullptr;
  bool open = false;
};

struct Window;

struct PopupHandler {
  PopupHandler(Window &window) : window(window) {}
  void ScheduleErrorPopup(const std::string &title, const std::string &msg) noexcept;
  void ScheduleErrorPopup(const std::string &title, const std::function<void()> &impl) noexcept;
  bool RunPopups() noexcept;
  void MakePopup(const char *title, bool *open, const char *message) const noexcept;
  void MakePopup(const char *title, bool *open, const std::function<void()> &impl) const noexcept;

  std::vector<PopupData> popups;
  Window &window;
};
