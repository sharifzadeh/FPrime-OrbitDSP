# Deployments/OrbitDSPDeployment/CMakeLists.txt

cmake_minimum_required(VERSION 3.16)

project(OrbitDSPDeployment C CXX)

set(ORBITDSP_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE PATH "OrbitDSP repo root")

set(FPRIME_ROOT "${ORBITDSP_ROOT}/fprime" CACHE PATH "Path to F Prime framework")

include("${FPRIME_ROOT}/cmake/FPrime.cmake")
include("${FPRIME_ROOT}/cmake/FPrime-Code.cmake")

add_fprime_subdirectory("${ORBITDSP_ROOT}/OrbitDspFilter")
add_fprime_subdirectory("${ORBITDSP_ROOT}/Components/Ports")
add_fprime_subdirectory("${ORBITDSP_ROOT}/Components/MorseBlinker")
add_fprime_subdirectory("${ORBITDSP_ROOT}/Components/OrbitDSP")

add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/Top")