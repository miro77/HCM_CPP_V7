// ARC-007 — Tabellen der Feldstaerke-Kurven (Index 0 = F01).
#pragma once

#include <array>
#include <cstddef>

namespace hcmms {

inline constexpr std::size_t kFigureCount = 24;
inline constexpr std::size_t kFigureValues = 702;

extern const std::array<std::array<float, kFigureValues>, kFigureCount> kFigures;

}  // namespace hcmms
