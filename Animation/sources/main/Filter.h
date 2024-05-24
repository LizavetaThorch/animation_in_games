#pragma once

namespace ImGui
{
  bool ComboWithFilter(const char* label, int* current_item, const std::vector<std::string>& items);
}
