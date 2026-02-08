#include "OrbitDSPDeployment/Topology.hpp"
#include <cstdio>

int main(int argc, char** argv) {
  (void)argc; (void)argv;

  std::printf("Starting OrbitDSPDeployment...\n");

  OrbitDSPDeployment::initTopology();
  OrbitDSPDeployment::startTopology();

  // For now, keep it simple. Later you can block on a signal or run forever.
  std::printf("OrbitDSPDeployment started (stub). Exiting.\n");
  OrbitDSPDeployment::stopTopology();
  return 0;
}