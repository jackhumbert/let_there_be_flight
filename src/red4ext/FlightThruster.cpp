#include "FlightThruster.hpp"

IFlightThruster::~IFlightThruster() {
  meshComponent.~Handle();
}