// The Autopilot's telemetry row (AutopilotTelemetry.hpp), as FlightModeAutopilot.WriteTelemetry built it.

#include "AutopilotTelemetry.hpp"

#include "AutopilotDetail.hpp"

#include <cmath>

namespace Navigation {

using namespace AutopilotDetail;

const char *const kAutopilotTelemetryHeader =
    "t,x,y,z,vx,vy,vz,speed,speed2d,target,rate,cmd,off,following,maneuver,on_line,segment,"
    "plan_x,plan_y,plan_z,cmd_fwd_x,cmd_fwd_y,cmd_fwd_z,cmd_up_x,cmd_up_y,cmd_up_z,fwd_x,fwd_y,fwd_z,up_x,up_y,up_z,"
    "attitude_error,plan_cmd,cmd_car,up_cmd_car,plan_pitch,cmd_pitch,car_pitch,collision_timer,"
    "drone,cmd_tilt,car_tilt,thrust_right,thrust_fwd,thrust_up,surge_held,drift,audio_surge";

namespace {

// AngleBetween: the angle between two directions, degrees (-1 if either is zero)
float AngleBetween(const Vec3 &a, const Vec3 &b) {
  const float la = std::sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
  const float lb = std::sqrt(b.x * b.x + b.y * b.y + b.z * b.z);
  if (la < 0.0001f || lb < 0.0001f) {
    return -1.0f;
  }
  return Rad2Deg(std::acos(Clamp((a.x * b.x + a.y * b.y + a.z * b.z) / (la * lb), -1.0f, 1.0f)));
}

// PitchOf: the pitch of a direction above the horizontal, degrees
float PitchOf(const Vec3 &d) {
  const float l = std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
  return l < 0.0001f ? 0.0f : Rad2Deg(std::asin(Clamp(d.z / l, -1.0f, 1.0f)));
}

} // namespace

std::vector<float> AutopilotTelemetryRow(const Autopilot &aAp, float aClock, const Vec3 &aP,
                                         const Flight::StatsState &aStats, float aCollisionTimer) {
  const auto &t = aAp.tracker;
  const int32_t n = aAp.RouteSize();
  Vec3 plan{0.0f, 0.0f, 0.0f};
  if (t.segment + 1 < n) {
    plan = Sub(aAp.route.Point(t.segment + 1), aAp.route.Point(t.segment));
  }
  const Vec3 fwd = N(aAp.control.targetForward);
  const Vec3 up = N(aAp.control.targetUp);
  const Vec3 v = N(aStats.d_velocity);
  const Vec3 carForward = N(aStats.d_forward);
  const Vec3 carUp = N(aStats.d_up);
  const Vec3 worldUp{0.0f, 0.0f, 1.0f};
  const float drift = aAp.FollowsTrajectory() ? Rad2Deg(SampleAt(aAp.trajectory, t.closestS).drift) : 0.0f;
  return {aClock,
          aP.x,
          aP.y,
          aP.z,
          v.x,
          v.y,
          v.z,
          aStats.d_speed,
          aStats.d_speed2D,
          t.targetSpeed,
          t.speedRate,
          t.speedCmd,
          t.offRouteNow,
          aAp.status == "following" ? 1.0f : 0.0f,
          0.0f, // maneuver
          aAp.onLine ? 1.0f : 0.0f,
          float(t.segment),
          plan.x,
          plan.y,
          plan.z,
          fwd.x,
          fwd.y,
          fwd.z,
          up.x,
          up.y,
          up.z,
          carForward.x,
          carForward.y,
          carForward.z,
          carUp.x,
          carUp.y,
          carUp.z,
          Rad2Deg(aAp.attitudeError),
          AngleBetween(plan, fwd),
          AngleBetween(fwd, carForward),
          AngleBetween(up, carUp),
          PitchOf(plan),
          PitchOf(fwd),
          PitchOf(carForward),
          aCollisionTimer,
          1.0f, // drone
          AngleBetween(up, worldUp),
          AngleBetween(carUp, worldUp),
          aAp.thrustCmd.x,
          aAp.thrustCmd.y,
          aAp.thrustCmd.z,
          t.surge.held ? 1.0f : 0.0f,
          drift,
          aAp.commandSurge};
}

} // namespace Navigation
