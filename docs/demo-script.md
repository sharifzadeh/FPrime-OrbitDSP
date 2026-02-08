# OrbitDSP demo script (2–3 minutes)

1) Set scenario/mode
2) Configure noise + filter
3) Start loop and observe telemetry
4) Inject fault (auto-clear) and observe events/telemetry

Example command sequence (concept):
- CMD_SET_SCENARIO(BURN_MONITOR)
- CMD_SET_FILTER(MEDIAN, alpha=0.15, win=7, cutoff=0.7)
- CMD_SET_NOISE(vib_amp=0.0, vib_hz=0.0, spike_rate=2.0, rand_sigma=0.5)
- CMD_INJECT_FAULT(OUT_OF_RANGE, duration_ms=3000, level=0)
