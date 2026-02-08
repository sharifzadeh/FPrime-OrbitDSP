#ifndef ORBITDSP_COMPONENTS_ORBITDSP_ORBITDSP_HPP
#define ORBITDSP_COMPONENTS_ORBITDSP_ORBITDSP_HPP

#include <OrbitDSP/Components/OrbitDSP/OrbitDSPComponentAc.hpp>
#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Time/Time.hpp>

namespace OrbitDSP {

  class OrbitDSP : public OrbitDSPComponentBase {
   public:
    explicit OrbitDSP(const char* compName);
    ~OrbitDSP() override;

   private:
    // ---- Command handlers ----
    void CMD_SET_SCENARIO_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, Scenario scenario) override;

    void CMD_SET_FILTER_cmdHandler(
      FwOpcodeType opCode, U32 cmdSeq,
      FilterType filterType, F32 ema_alpha, U32 median_win, F32 lpf_cutoff_hz
    ) override;

    void CMD_SET_NOISE_cmdHandler(
      FwOpcodeType opCode, U32 cmdSeq,
      F32 vib_amp, F32 vib_hz, F32 spike_rate, F32 rand_sigma
    ) override;

    void CMD_INJECT_FAULT_cmdHandler(
      FwOpcodeType opCode, U32 cmdSeq,
      FaultType faultType, U32 duration_ms, F32 level
    ) override;

    void CMD_SET_FUEL_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, F32 fuel_kg) override;
    void CMD_START_BURN_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, F32 burn_rate_kg_s, U32 duration_ms) override;
    void CMD_STOP_BURN_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    void CMD_SET_MEAS_cmdHandler(FwOpcodeType opCode, U32 cmdSeq, F32 value) override;
    void CMD_RESET_DEMO_cmdHandler(FwOpcodeType opCode, U32 cmdSeq) override;

    // ---- Scheduler ----
    void schedIn_handler(FwIndexType portNum, U32 context) override;

    // ---- Helpers ----
    void sendStatus(U8 status);
    U8 computeStatus() const;

    Fw::Time getNowTime();
    U64 toUsec(const Fw::Time& t) const;

    void clearFaultIfExpired(U64 nowUsec);

    void resetFilterState();
    F32 applyFilter(F32 x_raw, F32 dt);

    F32 clampF32(F32 v, F32 lo, F32 hi) const;

    // ---- Median filter helpers ----
    static constexpr U32 MED_MAX = 21U;   // fixed buffer capacity
    void medianPush(F32 x);
    bool medianReady() const;
    F32 medianCompute(U32 win) const;

    // ---- State ----
    Scenario   m_scenario;
    FilterType m_filterType;
    FaultType  m_faultType;

    // Noise config
    F32 m_vibAmp;
    F32 m_vibHz;
    F32 m_spikeRate;
    F32 m_randSigma;

    // Filter params/state
    F32 m_emaAlpha;
    U32 m_medianWin;
    F32 m_lpfCutoffHz;

    F32 m_emaState;
    F32 m_lpfState;
    bool m_filterInit;

    // Median buffer
    F32 m_medBuf[MED_MAX];
    U32 m_medCount;
    U32 m_medHead;

    // Burn/Fuel
    F32 m_fuelKg;
    bool m_burnActive;
    F32 m_burnRateKgS;
    U64 m_burnEndUsec;

    // External measurement for IMU_STREAM
    F32 m_measValue;

    // Fault expiry
    U64 m_faultEndUsec;

    // Time bookkeeping
    bool m_haveLastTime;
    U64  m_lastUsec;

    // Diagnostics
    U32 m_spikeCount;

    // Status edge detect
    U8  m_lastStatus;
    bool m_sentStartS;

    // RNG (was static in schedIn before)
    U32 m_rng;
  };

}  // namespace OrbitDSP

#endif