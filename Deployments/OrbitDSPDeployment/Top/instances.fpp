module OrbitDSPDeployment {

  # --------------------------------------------------------------------------
  # Core services
  # --------------------------------------------------------------------------
  instance tlmChan   : Svc.TlmChan base id 0x1000
  instance cmdDisp   : Svc.CmdDispatcher base id 0x1100
  instance cmdSeq    : Svc.CmdSequencer base id 0x1200
  instance eventLogger : Svc.ActiveLogger base id 0x1300
  instance textLogger  : Svc.PassiveTextLogger base id 0x1400

  instance time       : Svc.Time base id 0x1500

  # Rate groups (deterministic scheduling)
  instance rateGroupDriver : Svc.RateGroupDriver base id 0x1600
  instance rateGroup1      : Svc.ActiveRateGroup base id 0x1700
  instance rateGroup2      : Svc.ActiveRateGroup base id 0x1800

  # --------------------------------------------------------------------------
  # App components
  # --------------------------------------------------------------------------
  instance orbitDSP   : OrbitDSP.OrbitDSP base id 0x2000
  instance morseBlinker : MorseBlinker.MorseBlinker base id 0x2100

  # Optional: if you want OrbitDspFilter as a separate component later:
  # instance orbitDspFilter : OrbitDspFilter.OrbitDspFilter base id 0x2200

}