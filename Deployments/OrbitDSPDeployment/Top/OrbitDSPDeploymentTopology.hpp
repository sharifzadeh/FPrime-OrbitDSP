#ifndef ORBITDSPDEPLOYMENT_TOPOLOGY_HPP
#define ORBITDSPDEPLOYMENT_TOPOLOGY_HPP

#include "OrbitDSPDeployment/Top/OrbitDSPDeploymentTopologyDefs.hpp"

namespace OrbitDSPDeployment {

  // Construct + connect all instances (generated-style API)
  void constructTopology();

  // Optional: configure drivers, rate group periods, etc.
  void configureTopology();

  // Start active components if needed (rate group driver, etc.)
  void startTopology();

  // Teardown (optional)
  void teardownTopology();

}

#endif