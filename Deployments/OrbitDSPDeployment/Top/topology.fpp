module OrbitDSPDeployment {

  topology OrbitDSPDeployment {

    import OrbitDSPDeployment.instances

    # ------------------------------------------------------------------------
    # Connections: Commands
    # ------------------------------------------------------------------------
    connections Command {

      # Incoming commands from GDS -> dispatcher
      cmdDisp.seqCmdStatus -> cmdSeq.cmdResponseIn
      cmdSeq.comCmdOut -> cmdDisp.seqCmdBuff

      # Route dispatcher outputs to components
      cmdDisp.compCmdOut -> orbitDSP.cmdIn
      cmdDisp.compCmdOut -> morseBlinker.cmdIn

      # Command registration
      orbitDSP.cmdRegOut -> cmdDisp.compCmdRegIn
      morseBlinker.cmdRegOut -> cmdDisp.compCmdRegIn
      cmdSeq.cmdRegOut -> cmdDisp.compCmdRegIn
    }

    # ------------------------------------------------------------------------
    # Connections: Telemetry
    # ------------------------------------------------------------------------
    connections Telemetry {
      orbitDSP.tlmOut -> tlmChan.tlmIn
      morseBlinker.tlmOut -> tlmChan.tlmIn
      cmdSeq.tlmOut -> tlmChan.tlmIn
    }

    # ------------------------------------------------------------------------
    # Connections: Events (Log)
    # ------------------------------------------------------------------------
    connections Events {
      orbitDSP.eventOut -> eventLogger.eventIn
      morseBlinker.eventOut -> eventLogger.eventIn
      cmdSeq.eventOut -> eventLogger.eventIn

      eventLogger.textEventOut -> textLogger.textIn
    }

    # ------------------------------------------------------------------------
    # Connections: Time
    # ------------------------------------------------------------------------
    connections Time {
      time.timeOut -> orbitDSP.timeGetIn
      time.timeOut -> morseBlinker.timeGetIn
      time.timeOut -> cmdSeq.timeGetIn
    }

    # ------------------------------------------------------------------------
    # Connections: Rate Groups (deterministic scheduling)
    # ------------------------------------------------------------------------
    connections RateGroups {

      # Driver triggers rate groups
      rateGroupDriver.cycleOut -> rateGroup1.cycleIn
      rateGroupDriver.cycleOut -> rateGroup2.cycleIn

      # Rate group members (put your periodic work here)
      rateGroup1.RateGroupMemberOut[0] -> orbitDSP.schedIn
      rateGroup1.RateGroupMemberOut[1] -> morseBlinker.schedIn

      # If you have a slower loop, you can wire it here
      # rateGroup2.RateGroupMemberOut[0] -> orbitDspFilter.schedIn
    }
  }
}