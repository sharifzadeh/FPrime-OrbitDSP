#include "OrbitDSP/Components/OrbitDSP/OrbitDSP.hpp"

#include <cmath>
#include <cstdint>

namespace OrbitDSP {

  static U8 fault_to_status(FaultType t) {
    switch (t) {
      case FaultType::NONE:          return 1U; // T
      case FaultType::SATURATE_HIGH: return 0U; // F
      case FaultType::SATURATE_LOW:  return 0U; // F
      case FaultType::STUCK_AT:      return 3U; // E
      case FaultType::OUT_OF_RANGE:  return 3U; // E
      case FaultType::DROPOUT:       return 0U; // F
      default:                       return 3U; // E
    }
  }

  // Tiny deterministic PRNG
  static U32 lcg(U32& s) {
    s = 1664525U * s + 1013904223U;
    return s;
  }

  static F32 pseudo_gauss(U32& s) {
    F32 acc = 0.0F;
    for (int i = 0; i < 6; ++i) {
      U32 r = lcg(s);
      F32 u = (r / 4294967295.0F);
      acc += u;
    }
    return (acc - 3.0F);
  }

  OrbitDSP::OrbitDSP(const char* compName)
  : OrbitDSPComponentBase(compName),
    m_scenario(Scenario::BURN_MONITOR),
    m_filterType(FilterType::EMA),
    m_faultType(FaultType::NONE),
    m_vibAmp(0.0F),
    m_vibHz(0.0F),
    m_spikeRate(0.0F),
    m_randSigma(0.0F),
    m_emaAlpha(0.1F),
    m_medianWin(5U),
    m_lpfCutoffHz(1.0F),
    m_emaState(0.0F),
    m_lpfState(0.0F),
    m_filterInit(false),
    m_medBuf{0.0F},
    m_medCount(0U),
    m_medHead(0U),
    m_fuelKg(10.0F),
    m_burnActive(false),
    m_burnRateKgS(0.0F),
    m_burnEndUsec(0U),
    m_measValue(0.0F),
    m_faultEndUsec(0U),
    m_haveLastTime(false),
    m_lastUsec(0U),
    m_spikeCount(0U),
    m_lastStatus(255U),
    m_sentStartS(false),
    m_rng(0x12345678U)   // <-- rng as a member + initialized in ctor
  {
    this->tlmWrite_TLM_SCENARIO(static_cast<U8>(m_scenario));
    this->tlmWrite_TLM_FILTER_TYPE(static_cast<U8>(m_filterType));
    this->tlmWrite_TLM_SPIKE_COUNT(m_spikeCount);
    this->tlmWrite_TLM_FAULT_CODE(static_cast<U8>(m_faultType));

    this->tlmWrite_TLM_FUEL_KG(m_fuelKg);
    this->tlmWrite_TLM_BURN_ACTIVE(m_burnActive ? 1U : 0U);
    this->tlmWrite_TLM_BURN_RATE(m_burnRateKgS);

    this->tlmWrite_TLM_MEAS_VALUE(m_measValue);
  }

  OrbitDSP::~OrbitDSP() = default;

  Fw::Time OrbitDSP::getNowTime() {
    return this->getTime();
  }

  U64 OrbitDSP::toUsec(const Fw::Time& t) const {
    const U64 s  = static_cast<U64>(t.getSeconds());
    const U64 us = static_cast<U64>(t.getUSeconds());
    return s * 1000000ULL + us;
  }

  F32 OrbitDSP::clampF32(F32 v, F32 lo, F32 hi) const {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
  }

  void OrbitDSP::sendStatus(U8 status) {
    if (status == m_lastStatus) return;
    if (this->isConnected_dspStatusOut_OutputPort(0)) {
      this->dspStatusOut_out(0, status);
      m_lastStatus = status;
    }
  }

  U8 OrbitDSP::computeStatus() const {
    if (m_faultType != FaultType::NONE) {
      return fault_to_status(m_faultType);
    }
    const bool noisy =
      (m_randSigma > 5.0F) || (m_spikeRate > 0.0F) || (m_vibAmp != 0.0F);
    if (noisy) return 2U; // N
    return 1U;            // T
  }

  void OrbitDSP::clearFaultIfExpired(U64 nowUsec) {
    if (m_faultType == FaultType::NONE) return;
    if (m_faultEndUsec == 0U) return; // 0 => "infinite" until changed
    if (nowUsec < m_faultEndUsec) return;

    const FaultType prev = m_faultType;
    m_faultType = FaultType::NONE;
    m_faultEndUsec = 0U;
    this->tlmWrite_TLM_FAULT_CODE(static_cast<U8>(m_faultType));
    this->log_ACTIVITY_HI_FaultCleared(prev);
  }

  void OrbitDSP::resetFilterState() {
    m_filterInit = false;
    m_emaState = 0.0F;
    m_lpfState = 0.0F;
    m_medCount = 0U;
    m_medHead = 0U;
    for (U32 i = 0; i < MED_MAX; ++i) m_medBuf[i] = 0.0F;
  }

  void OrbitDSP::medianPush(F32 x) {
    m_medBuf[m_medHead] = x;
    m_medHead = (m_medHead + 1U) % MED_MAX;
    if (m_medCount < MED_MAX) m_medCount++;
  }

  bool OrbitDSP::medianReady() const {
    return (m_medCount > 0U);
  }

  F32 OrbitDSP::medianCompute(U32 win) const {
    // win clamped to [1, MED_MAX] and also <= m_medCount
    if (win < 1U) win = 1U;
    if (win > MED_MAX) win = MED_MAX;
    if (win > m_medCount) win = m_medCount;

    // Collect last "win" samples into tmp
    F32 tmp[MED_MAX];
    for (U32 i = 0; i < win; ++i) {
      const U32 idx = (m_medHead + MED_MAX - 1U - i) % MED_MAX;
      tmp[i] = m_medBuf[idx];
    }

    // Insertion sort (win <= 21 so cheap)
    for (U32 i = 1U; i < win; ++i) {
      F32 key = tmp[i];
      U32 j = i;
      while (j > 0U && tmp[j - 1U] > key) {
        tmp[j] = tmp[j - 1U];
        --j;
      }
      tmp[j] = key;
    }

    const U32 mid = win / 2U;
    if ((win % 2U) == 1U) return tmp[mid];
    return 0.5F * (tmp[mid - 1U] + tmp[mid]);
  }

  F32 OrbitDSP::applyFilter(F32 x_raw, F32 dt) {
    if (!m_filterInit) {
      m_emaState = x_raw;
      m_lpfState = x_raw;
      m_filterInit = true;
    }

    switch (m_filterType) {
      case FilterType::EMA: {
        const F32 a = clampF32(m_emaAlpha, 0.0F, 1.0F);
        m_emaState = a * x_raw + (1.0F - a) * m_emaState;
        return m_emaState;
      }

      case FilterType::LPF: {
        const F32 fc = (m_lpfCutoffHz <= 0.0F) ? 0.1F : m_lpfCutoffHz;
        const F32 rc = 1.0F / (2.0F * 3.1415926F * fc);
        const F32 k = dt / (rc + dt);
        m_lpfState = m_lpfState + k * (x_raw - m_lpfState);
        return m_lpfState;
      }

      case FilterType::MEDIAN:
      default: {
        medianPush(x_raw);
        if (!medianReady()) return x_raw;
        return medianCompute(m_medianWin);
      }
    }
  }

  // ---------------- Commands ----------------

  void OrbitDSP::CMD_SET_SCENARIO_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, Scenario scenario) {
    m_scenario = scenario;
    this->tlmWrite_TLM_SCENARIO(static_cast<U8>(m_scenario));
    this->log_ACTIVITY_HI_ScenarioSet(m_scenario);

    // Send "S" start marker once when entering BURN_MONITOR for the first time
    if (scenario == Scenario::BURN_MONITOR && !m_sentStartS) {
      m_sentStartS = true;
      m_lastStatus = 255U;   // force send even if same
      this->sendStatus(4U);  // 4 = "S"
      this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
      return;
    }

    this->sendStatus(this->computeStatus());
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void OrbitDSP::CMD_SET_NOISE_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                         F32 vib_amp, F32 vib_hz, F32 spike_rate, F32 rand_sigma) {
    m_vibAmp = vib_amp;
    m_vibHz = vib_hz;
    m_spikeRate = spike_rate;
    m_randSigma = rand_sigma;

    this->log_ACTIVITY_HI_NoiseSet(vib_amp, vib_hz, spike_rate, rand_sigma);

    this->sendStatus(this->computeStatus());
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void OrbitDSP::CMD_SET_FILTER_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                          FilterType filterType, F32 ema_alpha, U32 median_win, F32 lpf_cutoff_hz) {
    m_filterType = filterType;
    m_emaAlpha = ema_alpha;
    m_medianWin = median_win;
    m_lpfCutoffHz = lpf_cutoff_hz;

    this->tlmWrite_TLM_FILTER_TYPE(static_cast<U8>(m_filterType));
    this->log_ACTIVITY_HI_FilterSet(m_filterType);

    resetFilterState();

    this->sendStatus(this->computeStatus());
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void OrbitDSP::CMD_INJECT_FAULT_cmdHandler(FwOpcodeType opCode, U32 cmdSeq,
                                            FaultType faultType, U32 duration_ms, F32 level) {
    (void)level;

    m_faultType = faultType;
    this->tlmWrite_TLM_FAULT_CODE(static_cast<U8>(m_faultType));
    this->log_WARNING_HI_FaultInjected(m_faultType, duration_ms, level);

    const U64 now = toUsec(getNowTime());
    if (duration_ms == 0U || faultType == FaultType::NONE) {
      m_faultEndUsec = 0U;
    } else {
      m_faultEndUsec = now + static_cast<U64>(duration_ms) * 1000ULL;
    }

    if (faultType == FaultType::NONE) {
      m_lastStatus = 255U;  // force re-send
    }

    this->sendStatus(this->computeStatus());
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void OrbitDSP::CMD_SET_FUEL_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, F32 fuel_kg) {
    if (fuel_kg < 0.0F) fuel_kg = 0.0F;
    m_fuelKg = fuel_kg;
    this->tlmWrite_TLM_FUEL_KG(m_fuelKg);
    this->log_ACTIVITY_HI_FuelSet(m_fuelKg);

    this->sendStatus(this->computeStatus());
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void OrbitDSP::CMD_START_BURN_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, F32 burn_rate_kg_s, U32 duration_ms) {
    if (burn_rate_kg_s < 0.0F) burn_rate_kg_s = 0.0F;
    m_burnRateKgS = burn_rate_kg_s;
    m_burnActive = (duration_ms > 0U) && (m_burnRateKgS > 0.0F);

    const U64 now = toUsec(getNowTime());
    m_burnEndUsec = now + static_cast<U64>(duration_ms) * 1000ULL;

    this->tlmWrite_TLM_BURN_RATE(m_burnRateKgS);
    this->tlmWrite_TLM_BURN_ACTIVE(m_burnActive ? 1U : 0U);
    this->log_ACTIVITY_HI_BurnStarted(m_burnRateKgS, duration_ms);

    this->sendStatus(this->computeStatus());
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void OrbitDSP::CMD_STOP_BURN_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    m_burnActive = false;
    m_burnRateKgS = 0.0F;
    m_burnEndUsec = 0U;

    this->tlmWrite_TLM_BURN_ACTIVE(0U);
    this->tlmWrite_TLM_BURN_RATE(0.0F);
    this->log_ACTIVITY_HI_BurnStopped();

    this->sendStatus(this->computeStatus());
    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  void OrbitDSP::CMD_SET_MEAS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, F32 value) {
    m_measValue = value;
    this->tlmWrite_TLM_MEAS_VALUE(m_measValue);
    this->log_ACTIVITY_LO_MeasSet(m_measValue);

    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

  // ---------------- Scheduler ----------------

  void OrbitDSP::schedIn_handler(FwIndexType portNum, U32 context) {
    (void)portNum;
    (void)context;

    const U64 now = toUsec(getNowTime());

    // dt
    F32 dt = 0.02F;
    if (m_haveLastTime) {
      const U64 dus = (now > m_lastUsec) ? (now - m_lastUsec) : 0U;
      dt = static_cast<F32>(dus) / 1000000.0F;
      if (dt <= 0.0F || dt > 1.0F) dt = 0.02F;
    }
    m_lastUsec = now;
    m_haveLastTime = true;

    // auto-clear injected fault if expired
    clearFaultIfExpired(now);

    // Base "true" / input
    const F32 tsec = static_cast<F32>(now % 10000000ULL) / 1000000.0F;

    F32 x_true = 0.0F;
    if (m_scenario == Scenario::BURN_MONITOR) {
      x_true = 0.5F * std::sin(2.0F * 3.1415926F * 0.2F * tsec);
    } else { // IMU_STREAM
      x_true = m_measValue;
    }

    // Noise injection
    F32 noise = 0.0F;

    if (m_vibAmp != 0.0F && m_vibHz != 0.0F) {
      noise += m_vibAmp * std::sin(2.0F * 3.1415926F * m_vibHz * tsec);
    }

    if (m_randSigma != 0.0F) {
      noise += m_randSigma * pseudo_gauss(m_rng);
    }

    if (m_spikeRate > 0.0F) {
      const F32 p = m_spikeRate * dt;
      const F32 u = (lcg(m_rng) / 4294967295.0F);
      if (u < p) {
        noise += 5.0F;
        m_spikeCount++;
        this->tlmWrite_TLM_SPIKE_COUNT(m_spikeCount);
      }
    }

    F32 x_raw = x_true + noise;

    // Clipping / auto-fault detect
    const F32 HI = 3.0F;
    const F32 LO = -3.0F;

    bool clipped = false;
    if (x_raw > HI) { x_raw = HI; clipped = true; }
    if (x_raw < LO) { x_raw = LO; clipped = true; }

    // Only set auto-fault if not manually forced
    if (m_faultType == FaultType::NONE) {
      if (clipped) {
        m_faultType = (x_raw >= HI) ? FaultType::SATURATE_HIGH : FaultType::SATURATE_LOW;
        this->tlmWrite_TLM_FAULT_CODE(static_cast<U8>(m_faultType));
      } else if (std::fabs(x_raw) > 2.5F) {
        m_faultType = FaultType::OUT_OF_RANGE;
        this->tlmWrite_TLM_FAULT_CODE(static_cast<U8>(m_faultType));
      }
    }

    // Filtering
    const F32 y = applyFilter(x_raw, dt);

    // Burn/Fuel update (only in burn scenario)
    if (m_scenario == Scenario::BURN_MONITOR) {
      if (m_burnActive) {
        if (now >= m_burnEndUsec || m_fuelKg <= 0.0F) {
          m_burnActive = false;
          m_burnRateKgS = 0.0F;
          this->tlmWrite_TLM_BURN_ACTIVE(0U);
          this->tlmWrite_TLM_BURN_RATE(0.0F);
        } else {
          const F32 df = m_burnRateKgS * dt;
          m_fuelKg = (m_fuelKg > df) ? (m_fuelKg - df) : 0.0F;
          this->tlmWrite_TLM_FUEL_KG(m_fuelKg);
          this->tlmWrite_TLM_BURN_ACTIVE(1U);
          this->tlmWrite_TLM_BURN_RATE(m_burnRateKgS);
        }
      }
    }

    // Telemetry
    this->tlmWrite_TLM_RAW_VALUE(x_raw);
    this->tlmWrite_TLM_FILT_VALUE(y);
    this->tlmWrite_TLM_NOISE_METRIC(std::fabs(noise));

    // Status to MorseBlinker
    this->sendStatus(this->computeStatus());
  }

  void OrbitDSP::CMD_RESET_DEMO_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) {
    // core state
    m_faultType = FaultType::NONE;
    m_faultEndUsec = 0U;
    this->tlmWrite_TLM_FAULT_CODE(static_cast<U8>(m_faultType));

    // noise
    m_vibAmp = 0.0F;
    m_vibHz = 0.0F;
    m_spikeRate = 0.0F;
    m_randSigma = 0.0F;

    // filter defaults
    m_filterType = FilterType::EMA;
    m_emaAlpha = 0.1F;
    m_medianWin = 5U;
    m_lpfCutoffHz = 1.0F;
    this->tlmWrite_TLM_FILTER_TYPE(static_cast<U8>(m_filterType));
    resetFilterState();

    // burn/fuel
    m_fuelKg = 10.0F;
    m_burnActive = false;
    m_burnRateKgS = 0.0F;
    m_burnEndUsec = 0U;
    this->tlmWrite_TLM_FUEL_KG(m_fuelKg);
    this->tlmWrite_TLM_BURN_ACTIVE(0U);
    this->tlmWrite_TLM_BURN_RATE(0.0F);

    // meas
    m_measValue = 0.0F;
    this->tlmWrite_TLM_MEAS_VALUE(m_measValue);

    // diagnostics
    m_spikeCount = 0U;
    this->tlmWrite_TLM_SPIKE_COUNT(m_spikeCount);

    // reset RNG as requested
    m_rng = 0x12345678U;

    // IMPORTANT: allow S again + force next status
    m_sentStartS = false;
    m_lastStatus = 255U;

    // push a clean status immediately
    this->sendStatus(this->computeStatus());

    // event
    this->log_ACTIVITY_HI_DemoReset();

    this->cmdResponse_out(opCode, cmdSeq, Fw::CmdResponse::OK);
  }

}  // namespace OrbitDSP