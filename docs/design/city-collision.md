# City collision

Jack's goal: the city should be flyable as it looks. Two sides to it:

- Hidden colliders (collision with nothing visible) are removed. `tools/world/blockers.py` finds
  invisible vehicle blockers in the streaming sectors; the list ships as `src/navigation/blockers.txt`
  and `hidden_nodes.txt`, and the plugin moves them out of the world as sectors load
  (`src/red4ext/Navigation/Blockers.cpp`, the `StreamingSector_PostLoad` hook). The bake leaves them
  out too (`tools/navmap_bake/spans.py`). `tools/world/sector_edits_2.5.json` holds the older
  invisible wall removals ported to 2.5.
- Collision is added where the game draws something solid but gives it none: proxy-only geometry
  (for example the raised complex at (-1600, -680), 150 to 350 m up). `tools/world/proxy_colliders.py`
  cuts the proxy meshes not covered by real collision, terrain or a finer proxy into one collider
  entity per 128 m tile (548 tiles), cooked with `tools/packaging/physx_cook`, placed by
  `tools/pads/pad_sector.py` from `tools/world/proxy_collider_tiles.json`. The bake takes the same
  triangles so the Autopilot does not plan through them. Steps: `tools/navmap_bake/README.md`.

To see what the planner treats as solid along a route: draw it in game
(`tools/ingame/debug_draw/ltbf_draw.reds`) or over a recorded run (`tools/navplan/navviz.py`). An
outline in empty air is a hidden collider; something drawn with no outline has no collision.
