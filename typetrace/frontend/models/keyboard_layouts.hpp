#pragma once

#include <string>
#include <tuple>
#include <vector>

namespace typetrace::frontend {

// Keyboard key: (scancode, label, is_expanded)
using KeyInfo = std::tuple<int, std::string, bool>;
using KeyboardRow = std::vector<KeyInfo>;
using KeyboardLayout = std::vector<KeyboardRow>;

// US QWERTY Layout
constexpr KeyboardLayout US_QWERTY = {
  // Function keys row
  {{1, "Esc", true},
   {59, "F1", true},
   {60, "F2", true},
   {61, "F3", true},
   {62, "F4", true},
   {63, "F5", true},
   {64, "F6", true},
   {65, "F7", true},
   {66, "F8", true},
   {67, "F9", true},
   {68, "F10", true},
   {87, "F11", true},
   {88, "F12", true}},
  // Number row
  {{41, "`", false},
   {2, "1", false},
   {3, "2", false},
   {4, "3", false},
   {5, "4", false},
   {6, "5", false},
   {7, "6", false},
   {8, "7", false},
   {9, "8", false},
   {10, "9", false},
   {11, "0", false},
   {12, "-", false},
   {13, "=", false},
   {14, "Backspace", true}},
  // QWERTY row
  {{15, "Tab", true},
   {16, "Q", false},
   {17, "W", false},
   {18, "E", false},
   {19, "R", false},
   {20, "T", false},
   {21, "Y", false},
   {22, "U", false},
   {23, "I", false},
   {24, "O", false},
   {25, "P", false},
   {26, "[", false},
   {27, "]", false},
   {43, "\\", true}},
  // ASDF row
  {{58, "Caps", true},
   {30, "A", false},
   {31, "S", false},
   {32, "D", false},
   {33, "F", false},
   {34, "G", false},
   {35, "H", false},
   {36, "J", false},
   {37, "K", false},
   {38, "L", false},
   {39, ";", false},
   {40, "'", false},
   {28, "Enter", true}},
  // ZXCV row
  {{42, "Shift", true},
   {44, "Z", false},
   {45, "X", false},
   {46, "C", false},
   {47, "V", false},
   {48, "B", false},
   {49, "N", false},
   {50, "M", false},
   {51, ",", false},
   {52, ".", false},
   {53, "/", false},
   {54, "Shift", true}},
  // Bottom row
  {{29, "Ctrl", false},
   {56, "Alt", false},
   {57, "Space", true},
   {100, "Alt", false},
   {97, "Ctrl", false},
   {105, "←", false},
   {103, "↑", false},
   {108, "↓", false},
   {106, "→", false}}
};

} // namespace typetrace::frontend

