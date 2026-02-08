module OrbitDSP {

  enum Scenario : U8 {
    BURN_MONITOR = 1
    IMU_STREAM   = 2
  }

  enum FilterType : U8 {
    EMA    = 1
    MEDIAN = 2
    LPF    = 3
  }

  enum FaultType : U8 {
    NONE          = 0
    SATURATE_HIGH = 1
    SATURATE_LOW  = 2
    STUCK_AT      = 3
    OUT_OF_RANGE  = 4
    DROPOUT       = 5
  }

  active component OrbitDSP {

    # ----------------------------
    # Commands
    # ----------------------------
    async command CMD_SET_SCENARIO(scenario: Scenario)

    async command CMD_SET_FILTER(
      filterType: FilterType,
      ema_alpha: F32,
      median_win: U32,
      lpf_cutoff_hz: F32
    )

    async command CMD_SET_NOISE(
      vib_amp: F32,
      vib_hz: F32,
      spike_rate: F32,
      rand_sigma: F32
    )

    async command CMD_INJECT_FAULT(
      faultType: FaultType,
      duration_ms: U32,
      level: F32
    )

    async command CMD_SET_FUEL(fuel_kg: F32)
    async command CMD_START_BURN(burn_rate_kg_s: F32, duration_ms: U32)
    async command CMD_STOP_BURN()

    @ NEW: push an external measurement (e.g., IMU magnitude, thruster signal, etc.)
    async command CMD_SET_MEAS(value: F32)

    @ Reset all internal demo state (fault/noise/filter/burn/counters/status)
    async command CMD_RESET_DEMO()

    # ----------------------------
    # Events
    # ----------------------------
    event ScenarioSet(s: Scenario) severity activity high format "Scenario set to {}"
    event FilterSet(f: FilterType) severity activity high format "Filter set to {}"
    event NoiseSet(a: F32, hz: F32, spike: F32, sigma: F32) severity activity high format "Noise: amp={} hz={} spikeRate={} sigma={}"
    event FaultInjected(t: FaultType, duration_ms: U32, level: F32) severity warning high format "Fault: type={} duration_ms={} level={}"
    event FaultCleared(t: FaultType) severity activity high format "Fault cleared: {}"
    event FuelSet(fuel_kg: F32) severity activity high format "Fuel set to {} kg"
    event BurnStarted(rate: F32, duration_ms: U32) severity activity high format "Burn started: rate={} kg/s duration_ms={}"
    event BurnStopped() severity activity high format "Burn stopped"
    event MeasSet(v: F32) severity activity low format "Measurement set: {}"
    event DemoReset() severity activity high format "Demo state reset"

    # ----------------------------
    # Telemetry
    # ----------------------------
    telemetry TLM_SCENARIO: U8
    telemetry TLM_FILTER_TYPE: U8
    telemetry TLM_FAULT_CODE: U8
    telemetry TLM_SPIKE_COUNT: U32

    telemetry TLM_RAW_VALUE: F32
    telemetry TLM_FILT_VALUE: F32
    telemetry TLM_NOISE_METRIC: F32

    telemetry TLM_FUEL_KG: F32
    telemetry TLM_BURN_ACTIVE: U8
    telemetry TLM_BURN_RATE: F32

    telemetry TLM_MEAS_VALUE: F32

    # ----------------------------
    # Standard ports
    # ----------------------------
    time get port timeCaller
    command reg port cmdRegOut
    command recv port cmdIn
    command resp port cmdResponseOut
    text event port logTextOut
    event port logOut
    telemetry port tlmOut

    # ----------------------------
    # Scheduler input
    # ----------------------------
    async input port schedIn: Svc.Sched

    # ----------------------------
    # Status output to MorseBlinker
    # ----------------------------
    output port dspStatusOut: Components.ImuStatusPort
  }

}