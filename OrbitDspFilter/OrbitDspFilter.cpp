#include "OrbitDspFilter.hpp"

namespace OrbitDsp {

void OrbitDspFilter::configure(const FilterConfig& cfg) { cfg_ = cfg; }
void OrbitDspFilter::reset() { state_ = 0.0f; }

float OrbitDspFilter::step(float x) {
  switch (cfg_.type) {
    case FilterType::EMA:    return ema(x);
    case FilterType::MEDIAN: return median(x);
    case FilterType::LPF1:   return lpf1(x);
    default:                 return ema(x);
  }
}

float OrbitDspFilter::ema(float x) {
  // y[n] = alpha*x + (1-alpha)*y[n-1]
  state_ = cfg_.alpha * x + (1.0f - cfg_.alpha) * state_;
  return state_;
}

float OrbitDspFilter::lpf1(float x) {
  // Placeholder: treat as EMA for now
  return ema(x);
}

float OrbitDspFilter::median(float x) {
  // Placeholder: no window buffer yet — pass-through for now
  // (later: maintain a ring buffer of size cfg_.win and return median)
  return x;
}

} // namespace OrbitDsp