module Components {

  import Components.ImuStatusPort

  @ Morse LED Blinker component
  active component MorseBlinker {

    async command BLINK_STRING(
      message: string size 80
    )

    event Blinking(
      message: string size 80
    ) severity activity high format "Morse Blinking: {}"

    time get port timeCaller
    command reg port cmdRegOut
    command recv port cmdIn
    command resp port cmdResponseOut
    text event port logTextOut
    event port logOut
    telemetry port tlmOut

    @ Status code to blink as Morse: 0="F", 1="T", 2="N", 3="E"
    async input port imuStatusIn: Components.ImuStatusPort
  }

}