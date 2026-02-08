# OrbitDSP Architecture

## High-level flow (conceptual)

Scheduler (Svc.Sched)
  -> OrbitDSP::runCycle()
       - acquire raw measurement (simulated or input)
       - apply noise model (optional)
       - apply filter (EMA/Median/LPF)
       - evaluate fault rules (optional)
       - publish telemetry: raw, filtered, noise metrics, fault code
       - log events on mode changes / fault injection / command execution
       - (optional) update hardware LED status

## Observability

- Commands control mode/filter/noise/fault injection
- Telemetry exposes raw vs filtered and key counters
- Events explain *why* state changed