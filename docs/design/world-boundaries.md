# World boundaries

The Autopilot keeps out of the areas the game keeps the player out of: the edges of the playable map
(the Badlands, the hills, the sea) and, while a quest has them up, the Act 1 lockdown of Watson.

## The vanilla analogue

The game marks those areas with `gameWorldBoundaryNode`, a `worldAreaShapeNode` (an outline, extruded
upwards by a height) placed in the ordinary exterior streaming sectors. The native
`gameWorldBoundarySystem` acts on them, and the game's own autodrive stops when it reaches one
(`AutoDriveSystem.OnReachedWorldBoundary` in `core/systems/autoDriveSystem.script`). The Autopilot,
the flying autodrive, treats them the same way: as places it does not fly into.

They are fences, not one closed outline: about 220 oriented boxes, tens to a few hundred metres thick
and up to a kilometre high, along the Badlands and the hills north east of the city and out to sea
west of Watson and Pacifica. Terrain and the sea close the gaps between them for a car on the ground.

## Data: `src/navigation/boundaries.tsv`

`tools/navmap_bake/boundaries.py` reads every boundary node from the exterior sectors and writes one
line per area: name, gate, z range, and the outline in world metres. The plugin ships the file
(`CMakeLists.txt`, `configure_mod_file`) and loads it at startup (`Main.cpp`).

### Gates: areas a quest switches

Some areas belong to a sector variant that a quest shows or hides (`questTogglePrefabVariant`). The
streaming block lists each sector's variants (`worldStreamingSectorDescriptor.variants`: the prefab,
the variant's name, the range of the sector's nodes it holds, whether it is on by default). The game
gives a plugin no way to ask for a variant's state, so each variant is mapped to the quest fact that
decides it, read from the quest phases that toggle it (`VARIANT_GATES` in the tool):

| Variant | Areas | Gate | Why |
|---|---|---|---|
| `prolog_blockades` | 25, Watson | `watson_prolog_lock>0` | `open_world_content` starts `lch_prolog_roadblocks` while the fact is above 0; that phase shows the variant at its start and hides it once the fact is 0. `q001_ride_to_mb` sets it to 1, `q101` to 0 |
| `boundaries_spaceport_invisible` | 7, the spaceport | `ep1_installed==0` | on by default; `ep1_open_world` hides it once Phantom Liberty is installed |
| `var_vr_game` | 5, sq021's trailer park | left out | off by default, and every quest node naming it hides it |

A new game patch can add variants: the tool stops on a boundary in a variant it has no gate for.

## In the plugin

- `Navigation::Boundaries` (game-free) parses the file, works out which areas are in force from the
  facts, and builds a `ColumnMask` of them: every 2 m column whose centre is inside an outline or
  within the car's radius of one (as the bake grows solids). Masks are kept for the session, one per
  set of areas in force.
- `NavMap` reads a column under the mask as blocked at every height (`ColumnAt`, `View::Column`), so
  the planner, free space (`IsFree`, `SegmentClear`, `Gaps`) and everything built on them keep out.
  A `View` takes the mask once, so a plan running while a fact changes sees one set throughout.
- The doorway layer keeps a set per mask (the mask's key: a hash of the areas in force and the map's
  size). Baked doorways carry the key they were baked under (version 2); a set baked under another
  mask, or before masks (version 1), is not used, and blocks are worked out as searches reach them.
  `doorways.py` bakes under the mask `navplan` applies, which defaults to after the prologue with
  Phantom Liberty (`NAVPLAN_FACTS`).
- Free roam does not pick a goal or a gate when the straight line to it passes a masked column
  (`World::CrossesBoundary`).
- The game side follows the facts as `uiInventoryScriptableSystem` follows its own.
  `FlightBoundariesSystem` (a scriptable system, `src/redscript/Navigation/FlightBoundaries.reds`)
  registers a listener per gate fact (`QuestsSystem.RegisterListener`) once the player is attached,
  when a save's facts are in, and drops them in `OnDetach`. On attach and from each listener it calls
  the native `FlightBoundaries.Refresh`, which reads the facts from the quests system and applies the
  mask. The listeners live in script because the quests system calls them back as script functions.
  `GetAreasInForce`, `GetMaskKey` and `IsBlocked` are read-only accessors for checks in the game.

### Closures: the open ends

The fences end open where terrain or the sea closes the gap for a car on the ground, and a few of the
game's own boxes leave slits of a few metres where they meet. A car in the air would fly round an
open end over the hills or out over the sea. `tools/navmap_bake/boundary_closures.tsv` holds polylines
drawn by hand to close them; `boundaries.py` widens each into 40 m areas (one per segment, lengthened
at both ends so the segments overlap at a bend) and adds them, always in force, to `boundaries.tsv`:

- the north shore past Watson, outside the prologue lockdown's own north fence;
- north of the spaceport, continuing the sea fence's line north across the runway's far end, then east
  over the water to the fence west of Watson;
- the Pacifica coast west from where the sea fence meets it, then down the west edge of the southern
  Badlands to the nav map's south edge (which ends the map there anyway);
- the south fence's end straight down to the nav map's edge;
- the north east, through the mountains from the end of the hills fence to the top of the eastern fence;
- four slits between the game's boxes (north of the North Oak hills, three where the sea fences cross).

They were placed from the game's own extents: the minimap's tiles (`MinimapDataNode`, every 128 m over
the playable area and a little past it) and the fences either side. With Phantom Liberty, after the
prologue, the area left inside is 67 km2 and holds 94% of the minimap's tiles; the rest are the
minimap's margins (out over the northern sea, a little past the eastern fence, past the nav map's
south edge, and inside the hills lobe the game's own fences leave out). Checked by a flood fill at the
mask's resolution (2 m columns, the car's radius, diagonal steps as the planner takes them) from the
city to points beyond every edge.

## Limits

- The z range of an area is not used: a masked column is blocked at every height.
- A destination outside the areas has no route; the planner takes its whole search budget (about
  20 s for a goal past the south fence) before it says so.
