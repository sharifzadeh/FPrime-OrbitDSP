#include "OrbitDSPDeployment/Topology.hpp"
#include "OrbitDSPDeployment/Top/OrbitDSPDeploymentTopology.hpp"

namespace OrbitDSPDeployment {

  void initTopology() {
    constructTopology();
    configureTopology();
  }

  void startTopology() {
    OrbitDSPDeployment::startTopology();
  }

  void stopTopology() {
    teardownTopology();
  }

}