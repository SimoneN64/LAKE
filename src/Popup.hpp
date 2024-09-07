#pragma once
#include <algorithm>
#include <functional>
#include <imgui.h>
#include <string>
#include <vector>

struct PopupData {
  std::string title;
  std::string message;
  bool open = false;
};

struct PopupHandler {
  void ScheduleErrorPopup(const std::string &title, const std::string &msg) noexcept {
    popups.push_back({title, msg, true});
  }

  void RunPopups() noexcept {
    for (auto &popup : popups) {
      MakePopup(popup.title.c_str(), &popup.open, popup.message.c_str());
    }

    std::erase_if(popups, [](const PopupData &data) { return !data.open; });
  }

  static void MakePopup(const char *title, bool *open, const char *message) noexcept {
    if (*open) {
      ImGui::OpenPopup(title);
    }

    if (ImGui::BeginPopupModal(title, open)) {
      ImGui::Text(message);
      ImGui::EndPopup();
    }
  }

  static void MakePopup(const char *title, bool *open, const std::function<void()> &impl) noexcept {
    if (*open) {
      ImGui::OpenPopup(title);
    }

    if (ImGui::BeginPopupModal(title, open)) {
      impl();
      ImGui::EndPopup();
    }
  }

  std::vector<PopupData> popups;
};
