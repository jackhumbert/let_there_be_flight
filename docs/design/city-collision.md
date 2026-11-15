# City collision

The goal: the city should be flyable as it looks. Two sides to it:

- Hidden colliders (collision with nothing visible) are removed. `tools/world/blockers.py` finds
  invisible vehicle blockers in the streaming sectors; the list ships as `src/navigation/blockers.txt`
  and `hidden_nodes.txt`, and the plugin moves them out of the world as sectors load
  (`src/red4ext/Navigation/Blockers.cpp`, the `StreamingSector_OnPostLoad` hook). The bake leaves them
  out too (`tools/navmap_bake/spans.py`). Hidden colliders with other presets (a World Static wall
  nothing draws) are listed by hand in `tools/world/hidden_colliders.json`, which the export adds to
  `blockers.txt` and the bake leaves out by preset and bounds. `tools/world/sector_edits_2.5.json`
  holds the older invisible wall removals ported to 2.5.
- Collision is added where the game draws something solid but gives it none: proxy-only geometry
  (for example the raised complex at (-1600, -680), 150 to 350 m up). `tools/world/proxy_colliders.py`
  cuts the proxy meshes not covered by real collision, terrain or a finer proxy into one collider
  entity per 128 m tile (566 tiles), cooked with `tools/packaging/physx_cook`, placed by
  `tools/pads/pad_sector.py` from `tools/world/proxy_collider_tiles.json`. The bake takes the same
  triangles so the Autopilot does not plan through them. Steps: `tools/navmap_bake/README.md`.
- A proxy is only what the game draws from afar when it hides up close (`nearAutoHideDistance` above
  0; 208 of the 1,489 placements with colliders). There a collider triangle needs something drawn
  up close within 4 m (a mesh node, an instanced group, a proxy that never hides, an entity's pivot
  within 15 m, or an entity of the proxy's name), else it is a hidden collider of our own: the
  Pacifica stadium supports proxy left 87,000 m2 of solid air round the stadium. `proxy_colliders.py
  detail` and `drawn` find them and `build --drawn` leaves them out (0.10 of 1.01 km2).

To see what the planner treats as solid along a route: draw it in game
(`tools/ingame/debug_draw/ltbf_draw.reds`, `fly_route.py --draw --draw-solid` for the map as
magenta boxes) or over a recorded run (`tools/navplan/navviz.py`). Magenta or an outline in empty
air is a hidden collider or a bake mistake; something drawn with no outline has no collision.

## Checking a hidden collider

Each place found goes into `tools/world/collider_cases.json` with what is there in game, the
box, and rays through it with the answer the game should give (clear or hit); real collision
next to it goes in too, so a check is tested both ways.

1. `tools/world/collision_at.py --case <name> [--actors]`: the collision the bake read in the box,
   by sector and preset, and with `--actors` the collision node, group (nodeData index for
   ArchiveXL), actor and stored position behind it. Nothing listed means the bake made it.
2. In game: V is put down near the box and the rays are cast against the game's collision, once
   a ray down from V has hit (collision streamed in) and V stands next to the box.
3. What it is: a blocker preset that should have been moved (`blockers.py`), real collision with
   nothing drawn (not caught by the scan yet: wanted, a check of collision shapes against the
   visible meshes), or a bake mistake (the game's rays are clear).

The other way round, drawn with no collision: `proxy_colliders.py` finds proxy-only geometry, and
buildings its coverage rule misses are listed in `tools/world/proxy_collider_force.json`. The
rule counts a surface under the top real surface of its column as covered, so a proxy-only
building under a roof that has collision keeps open walls; `proxy_colliders.py walls` finds
those (walls drawn with a roof near above, nothing behind them and open air beside them,
checked with rays from outside) and writes them as `walls_only` entries to read over and add:
only their near-vertical faces get collision, since the roof has its own. To find more after a bake change, `tools/world/opened_patches.py --old <spans> --new <spans>`
lists the space that opened and, per patch, how much of the drawn surface bounding it has no
collision (a hollow building) or none does (open air the old bake closed).
