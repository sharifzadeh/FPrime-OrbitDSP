#pragma once
#include <cstdint>

namespace OrbitDsp {

enum class FilterType : uint8_t {
  EMA = 0,
  MEDIAN = 1,
  LPF1 = 2
};

struct FilterConfig {
  FilterType type{FilterType::EMA};
  float alpha{0.15f};     // for EMA / LPF placeholder
  uint32_t win{7};        // for median placeholder
  float cutoff{0.7f};     // placeholder
};

class OrbitDspFilter {
public:
  OrbitDspFilter() = default;

  void configure(const FilterConfig& cfg);
  void reset();

  float step(float x);

private:
  FilterConfig cfg_{};
  float state_{0.0f};

  float ema(float x);
  float lpf1(float x);
  float median(float x); // placeholder
};

} // namespace OrbitDsp