#ifndef COMPONENTS_MORSEBLINKER_MORSEBLINKER_HPP
#define COMPONENTS_MORSEBLINKER_MORSEBLINKER_HPP

#include "OrbitDSP/Components/MorseBlinker/MorseBlinkerComponentAc.hpp"
#include <Fw/Types/BasicTypes.hpp>
#include <Fw/Cmd/CmdString.hpp>   // Fw::CmdStringArg

namespace Components {

  class MorseBlinker : public MorseBlinkerComponentBase {
    public:
      explicit MorseBlinker(const char* const compName);
      ~MorseBlinker() override;

    private:
      void BLINK_STRING_cmdHandler(
          FwOpcodeType opCode,
          U32 cmdSeq,
          const Fw::CmdStringArg& message
      ) override;

      void imuStatusIn_handler(
          FwIndexType portNum,
          U8 status
      ) override;
  };

} // namespace Components

#endif // COMPONENTS_MORSEBLINKER_MORSEBLINKER_HPP