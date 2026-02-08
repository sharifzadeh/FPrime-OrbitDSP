# OrbitDSP architecture (concept)

Scheduler (Svc.Sched)
  -> OrbitDSP component compute/update
  -> Publish TLM (raw, filtered, metrics, fault)
  -> Emit events (mode change, fault injected/cleared)

Extensible: can mirror status to hardware (LED/GPIO).
