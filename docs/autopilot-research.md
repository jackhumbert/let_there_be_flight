# Autopilot research: navigation, trajectories, guidance, avoidance

Light research pass (about 40 minutes) for the LTBF autopilot. Assumptions: an offline-baked span map of the city (2 m cells, each cell a sorted list of solid height intervals, so the free space in a cell is a list of gaps), and a car flown through our own force/torque channels (surge, sway, lift, yaw, pitch, roll, linear brake). Sources are listed at the end; bracketed numbers refer to them.

## 1. Representation and graph for multi-level flight

**Options surveyed.**

- **Recast heightfield spans** [1]. Recast voxelizes geometry into per-column solid spans, then builds a "compact heightfield" of the open space above each span. Neighbor connectivity is decided per pair of open spans: they connect if the floor height difference is within `walkableClimb` and the vertical overlap of the two gaps is at least `walkableHeight`. Agent radius is applied by `rcErodeWalkableArea`, which runs a 2D distance transform over the open spans and drops any closer than the radius to an edge. DetourTileCache adds `rcBuildHeightfieldLayers`, which splits a tile into non-overlapping 2D layers so overpasses and the road under them become separate layers. This is exactly our data shape, minus the "must stand on a floor" assumption.
- **Multi-level surface (MLS) maps** [2]. Triebel, Pfaff and Burgard store several surface patches per grid cell (each with height and depth) to handle bridges and underpasses. Conceptually the same as Recast spans, coming from robotics mapping.
- **Sparse voxel octrees** [3, 4]. Brewer's Warframe system stores free space in an octree with 64-voxel leaf bricks (one `uint64` per leaf) and runs A* over octree nodes of varying size, with a scaled heuristic to trade optimality for speed. Mercuna [5] is commercial middleware built on the same idea. OctoMap [6] is a probabilistic octree for robot mapping. Octrees shine in truly 3D spaces (caves, space stations, asteroid fields).

**Fit for Night City.** The city is mostly 2.5D: almost every column is "ground, maybe some overhangs, then open sky up to the ceiling". A span map stores that in two or three intervals per column, which is far more compact and easier to query than an octree, and it already encodes overpasses, tunnels and megabuilding underpasses as separate gaps. An octree would mostly be spending memory on the huge empty sky. Keep the span map as the source of truth.

**Building a graph from gaps.**

1. *Inflate solids, do not shrink free space by hand.* Dilate every solid interval vertically by half the car height plus a margin (about 1 m), and horizontally by the car radius: for each cell, union the solid intervals of all cells within radius r (a disc of cells, or a separable square approximation). Recompute the free gaps from the inflated solids. A gap that survives is guaranteed to hold the car's center. This is the configuration-space version of Recast's erosion and works for overhangs, which Recast's floor-based erosion does not.
2. *Nodes.* One node per (cell, gap). Tall gaps (open sky) should be split into vertical bands (for example 8 to 16 m) so climbs have a cost and the planner can prefer a cruise altitude.
3. *Edges.* Connect node A to a node B in one of the 8 neighbor columns if their (inflated) gap intervals overlap vertically. Within one column, adjacent bands connect vertically. Diagonals additionally need the two orthogonal neighbors to be passable to avoid corner cutting.
4. *Costs.* Euclidean length, plus a climb penalty, plus a soft penalty for low clearance (distance to nearest solid) and for flying in street canyons below a preferred altitude if the route style wants sky lanes.
5. *Scale.* At 2 m a 6 km city is about 9 million columns, too many for per-request A* in redscript. Use a hierarchy: a coarse grid (16 or 32 m) whose cells record which gap-bands connect across their borders (HPA*-style portals), plan on the coarse graph, refine only the next few hundred meters. First try a straight line at cruise altitude with a line-of-sight test against the span map; most trips above the rooftops need no search at all.

Open source: recastnavigation [1] (C++, the compact heightfield and layer code is worth reading even if we do not use Detour), svo-rs [4] (Rust SVO after Brewer), OctoMap [6] (C++), PythonRobotics [7] (Python reference implementations of A*, hybrid A*, RRT*, Dubins, pure pursuit).

**Recommendation:** keep the 2 m span map, inflate solids by the car's half extents offline, bake (cell, gap-band) nodes with overlap-based edges, and add a coarse portal layer for long routes. Skip octrees.

## 2. From grid path to a flyable trajectory

**Any-angle path first.** Grid A* paths zig-zag. String pulling via line-of-sight checks (Theta* and Lazy Theta*, the 3D variant uses far fewer LOS checks [8]) produces a short list of 3D corner points. A cheap post-pass that does the same: walk the path, keep the furthest point visible from the current anchor.

**Respecting momentum.** The literature options, from heavy to light:

- *Kinodynamic search.* Hybrid A* [9] (Stanford's Junior, DARPA Urban Challenge) searches over continuous (x, y, heading) states with motion primitives, then smooths numerically. Kinodynamic RRT* is similar in spirit. Both are too expensive for per-frame scripting and unnecessary for a vehicle that can slow down and hover.
- *Dubins and Dubins airplane.* Shortest paths with bounded curvature are arcs and straights (Dubins); Chitsaz and LaValle [10] add altitude with bounded climb rate. Owen, Beard and McLain [11] give a practical version for fixed-wing UAVs, including spirals for steep climbs. Good mental model; the useful takeaway is "corners become arcs of radius R, climbs that are too steep become helices or are spread out".
- *Clothoids.* Euler spirals make curvature (and therefore bank angle) change linearly, which removes the bank "snap" at arc entry. Bertolazzi and Frego's Clothoids library [12] does G1 fitting in C++.
- *Minimum snap* [13]. Piecewise polynomials through waypoints minimizing the 4th derivative of position; gives very smooth quadrotor-style trajectories but needs a QP solve per segment set.

**What games do.** Public material is thin. Racing games precompute a racing line as a spline, derive a speed profile from its curvature, and steer to a lookahead point on it [14]. Flying AI in open-world games typically flies waypoints at a set altitude with steering behaviors and a local avoidance probe; ship games (Elite-like) are commonly described as PID controllers chasing a target point. None of this needs a kinodynamic planner because the controller and the speed profile absorb the dynamics.

**Cheap pipeline.**

1. String-pull to corners.
2. Fillet each corner with an arc (or a clothoid-arc-clothoid blend) of radius R chosen from the desired speed, then sample the result every few meters. Check the samples against the inflated span map; where they collide, shrink R (and therefore speed) until clear. A car that can hover can always take a tight corner slowly.
3. Speed profile. Per sample: curvature from three points (Menger curvature, k = 4 * area / (a * b * c)); cap `v <= sqrt(a_lat_max / k)`; cap climb by `v <= vz_max / sin(gamma)` where gamma is the path's climb angle; cap by clearance. Then a forward pass `v[i+1]^2 <= v[i]^2 + 2 * a_accel * ds` and a backward pass `v[i]^2 <= v[i+1]^2 + 2 * a_brake * ds`. This is the standard racing-game and robotics approach and is O(n).

**Recommendation:** string pulling + arc/clothoid fillets checked against the span map + curvature-based forward/backward speed profile. Recompute only the next 300 to 500 m when the route changes. No hybrid A*, no minimum snap.

## 3. Path following that looks flown

**Guidance laws.** L1 nonlinear guidance [15] picks a reference point on the path at distance L1 ahead and commands lateral acceleration `a = 2 * V^2 / L1 * sin(eta)`, where eta is the angle between velocity and the line to the reference point. It naturally anticipates curves and tracks circles exactly. ArduPilot tunes it with a period and damping, `L1 = damping * period * V / pi` [16], so lookahead scales with speed. Pure pursuit [7] is the simpler cousin with a speed-scaled lookahead `L = clamp(k * V, L_min, L_max)`. Beard and McLain's vector-field following [11, 17] is another option. For 3D, apply the law to the vector from the car to the reference point, taking the component perpendicular to velocity.

**Cascade.** PX4's diagrams [18] show the standard structure: position (P) -> velocity (PID) -> acceleration -> attitude (quaternion) plus collective thrust -> body rates (PID) -> mixer. For us:

1. Desired acceleration = feedforward from the trajectory (tangential acceleration from the speed profile plus centripetal `V^2 * k` toward the curve center, using curvature sampled slightly ahead) + L1/PD correction for cross-track and vertical error + speed error along the tangent.
2. Map acceleration to attitude. *Plane mode* (high speed): yaw aligned with velocity, bank `phi = atan(a_lat / g)` (the coordinated-turn relation, equal to `atan(V^2 / (g * R))`), pitch tracks the flight-path angle plus whatever lift needs, surge handles the tangential component. *Hover mode* (low speed): level attitude with small tilt in the direction of travel, sway and lift provide lateral and vertical acceleration directly, yaw toward the path tangent or the destination. PX4's multicopter accel-to-tilt conversion is the model here.
3. Blend: `s = smoothstep(v_low, v_high, airspeed)` (for example 10 and 25 m/s); attitude targets and the sway/roll split are linear blends by s. VTOL transition logic in PX4 does essentially this.
4. Attitude to channels: PD on attitude error producing rate targets, then a rate P(I) loop producing the roll/pitch/yaw torque channel values.

**Anticipation and smoothness.** Use curvature at the point `V * tau` ahead (tau around the roll response time, 0.3 to 0.6 s) for the bank feedforward, so the car rolls in before the corner. Clothoid fillets make that roll gradual. Slew-limit bank and pitch targets, low-pass the acceleration command (one-pole, 0.1 to 0.2 s), clamp integrators, take derivatives on the measurement not the error, and schedule gains with speed (L1 does this for free). Oscillation usually means the outer loop is faster than the inner one; keep each loop roughly 3 to 5 times slower than the loop inside it. TECS [18] (trade pitch and thrust for total energy) is optional polish: it makes the car dive to gain speed and pull up to bleed it, which reads as "plane".

**Recommendation:** L1 guidance with speed-scaled lookahead in 3D, curvature feedforward sampled ahead, a three-level cascade (accel -> attitude -> rates) and a speed-based blend between hover-style (sway/lift, level) and coordinated-turn (bank, yaw on velocity) acceleration allocation.

## 4. Avoiding other flying vehicles

- **Velocity obstacles, RVO, ORCA** [19, 20]. Each agent computes the set of velocities that would collide within a time horizon and picks the closest allowed velocity to its preferred one. ORCA solves this as a small linear program over half-spaces and assumes the other agent takes half the responsibility. RVO2-3D [20] is the C++ 3D version; it does not handle static obstacles, so the span map must still be handled by the planner. ORCA is overkill for a handful of vehicles and is awkward when other vehicles are not running the same algorithm.
- **Reynolds steering** [21]. Separation and "unaligned collision avoidance": predict closest point of approach (CPA) and steer away. Cheap and good enough for a few agents.
- **Semicircular rule** [22]. In aviation, magnetic track 000 to 179 flies at odd thousands of feet and 180 to 359 at even thousands, so opposite-direction traffic is vertically separated. For us: pick the cruise band from heading quadrant (for example 4 bands 10 to 15 m apart above the rooftop level of the route). This removes most head-on conflicts before any local avoidance runs, and also makes AI traffic look organized.

A cheap local check per neighbor within 150 m: `t* = -dot(dp, dv) / |dv|^2`, `d* = |dp + dv * t*|`; if `0 < t* < 5 s` and `d* < r_sum + margin`, add an avoidance acceleration away from the CPA offset, preferring vertical separation (the layered free space makes climbing or descending usually safe; check the target altitude against the span gap) and scaled by `1 - t* / horizon`. Feed it into the same desired-acceleration input as guidance, so the attitude logic still banks and pitches naturally.

**Recommendation:** heading-based altitude bands for deconfliction plus a CPA-based Reynolds-style avoidance term that prefers vertical offsets, validated against the span map. Consider ORCA only if dense AI flying traffic is added later.

## Sources

1. recastnavigation (Recast/Detour/DetourTileCache), https://github.com/recastnavigation/recastnavigation ; API docs https://recastnav.com/group__recast.html
2. Triebel, Pfaff, Burgard, "Multi-Level Surface Maps for Outdoor Terrain Mapping and Loop Closing", IROS 2006, https://cvai.cit.tum.de/_media/spezial/bib/triebel06multi.pdf
3. Brewer, "3D Flight Navigation Using Sparse Voxel Octrees", Game AI Pro 3, ch. 21, https://www.gameaipro.com/GameAIPro3/GameAIPro3_Chapter21_3D_Flight_Navigation_Using_Sparse_Voxel_Octrees.pdf ; GDC 2015 "Getting off the NavMesh", https://gdcvault.com/play/1022016/
4. svo-rs (Rust SVO pathfinding after Brewer), https://github.com/mkmarek/svo-rs
5. Mercuna 3D Navigation, https://80.lv/articles/mercuna-3d-navigation-middleware-for-game-developers
6. OctoMap, https://octomap.github.io/
7. PythonRobotics (A*, hybrid A*, RRT*, Dubins, pure pursuit, clothoids), https://github.com/AtsushiSakai/PythonRobotics
8. Nash, Koenig, Tovey, "Lazy Theta*: Any-Angle Path Planning and Path Length Analysis in 3D", AAAI 2010, https://idm-lab.org/bib/abstracts/papers/aaai10b.pdf
9. Dolgov, Thrun, Montemerlo, Diebel, "Path Planning for Autonomous Vehicles in Unknown Semi-structured Environments", IJRR 2010, https://journals.sagepub.com/doi/10.1177/0278364909359210 ; workshop version https://cdn.aaai.org/Workshops/2008/WS-08-10/WS08-10-006.pdf
10. Chitsaz, LaValle, "Time-optimal paths for a Dubins airplane", CDC 2007, listed at https://lavalle.pl/optcon.html
11. Owen, Beard, McLain, "Implementing Dubins Airplane Paths on Fixed-wing UAVs", Handbook of Unmanned Aerial Vehicles, 2015; Beard and McLain, "Small Unmanned Aircraft" book site https://github.com/randybeard/uavbook
12. Bertolazzi, Frego, Clothoids library (G1 clothoid fitting), https://github.com/ebertolazzi/Clothoids
13. Mellinger, Kumar, "Minimum Snap Trajectory Generation and Control for Quadrotors", ICRA 2011, https://ieeexplore.ieee.org/abstract/document/5980409
14. Tomlinson, Melder, "An Architecture Overview for AI in Racing Games", Game AI Pro, ch. 38, https://www.gameaipro.com/GameAIPro/GameAIPro_Chapter38_An_Architecture_Overview_for_AI_in_Racing_Games.pdf
15. Park, Deyst, How, "A New Nonlinear Guidance Logic for Trajectory Tracking", AIAA GNC 2004 (AIAA 2004-4900); MIT ACL copy http://acl.mit.edu/papers/gnc_park_deyst_how.pdf
16. ArduPilot Plane navigation (L1) tuning, https://ardupilot.org/plane/docs/navigation-tuning.html
17. Beard, McLain, uavbook supplement (path following, Dubins chapters), https://github.com/randybeard/uavbook
18. PX4 controller diagrams (multicopter cascade, fixed-wing TECS, VTOL), https://docs.px4.io/main/en/flight_stack/controller_diagrams
19. van den Berg, Guy, Lin, Manocha, "Reciprocal n-body Collision Avoidance" (ORCA), ISRR 2009, https://gamma.cs.unc.edu/ORCA/
20. RVO2-3D library, https://github.com/snape/RVO2-3D ; docs https://gamma.cs.unc.edu/RVO2/documentation/3d-1.0/whatsnew.html
21. Reynolds, "Steering Behaviors For Autonomous Characters", GDC 1999, https://www.red3d.com/cwr/steer/gdc99/
22. Semicircular rule (hemispheric cruising levels), https://en.wikipedia.org/wiki/Flight_level#Semicircular/hemispheric_rule

Notes on source quality: the MIT ACL link for [15] and the uavbook location for [11, 17] were taken from citations and not opened in this pass; public detail on how commercial games fly vehicle AI is sparse, so the "what games do" paragraph is a summary of the racing chapter plus common practice rather than documented per-title.
