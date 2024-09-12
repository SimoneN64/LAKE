#include <Popup.hpp>
#include <algorithm>
#include <Window.hpp>

void PopupHandler::ScheduleErrorPopup(const std::string &title, const std::string &msg) noexcept {
  popups.push_back({title, msg, nullptr, true});
}

void PopupHandler::ScheduleErrorPopup(const std::string &title, const std::function<void()> &impl) noexcept {
  popups.push_back({title, "", impl, true});
}

bool PopupHandler::RunPopups() noexcept {
  for (auto &popup : popups) {
    if (popup.impl)
      MakePopup(popup.title.c_str(), &popup.open, popup.impl);
    else
      MakePopup(popup.title.c_str(), &popup.open, popup.message.c_str());
  }

  bool anyOpen = std::any_of(popups.begin(), popups.end(), [](const PopupData &data) { return data.open; });

  std::erase_if(popups, [](const PopupData &data) { return !data.open; });

  return anyOpen;
}

void PopupHandler::MakePopup(const char *title, bool *open, const char *message) const noexcept {
  if (*open) {
    ImGui::OpenPopup(title);
    ImGui::SetNextWindowPos({window.PosX() + window.Width() / 2.f, window.PosY() + window.Height() / 2.f}, 0,
                            {0.5f, 0.5f});
    if (ImGui::BeginPopupModal(
          title, open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
      ImGui::Text(message);
      ImGui::EndPopup();
    }
  }
}

void PopupHandler::MakePopup(const char *title, bool *open, const std::function<void()> &impl) const noexcept {
  if (*open) {
    ImGui::OpenPopup(title);
    ImGui::SetNextWindowPos({window.PosX() + window.Width() / 2.f, window.PosY() + window.Height() / 2.f}, 0,
                            {0.5f, 0.5f});
    if (ImGui::BeginPopupModal(
          title, open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
      impl();
      ImGui::EndPopup();
    }
  }
}
