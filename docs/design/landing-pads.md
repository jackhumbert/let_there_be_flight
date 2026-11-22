# Landing pads

The city's AV landing pads, found in the world sectors at bake time, shipped as a small list, used
as Autopilot landing spots, world-map pins (discovered by flying near them) and places with supplies.

## Data and code

- `tools/pads/pads.py` finds the pads in the raw sectors (FNV-1a hashes of the pad meshes), checks
  them against the nav map, applies `tools/pads/helipads_overrides.json` (`remove`, `names`, `add`,
  `specials`) and writes `src/navigation/helipads.tsv` (id, x, y, z of the deck, yaw, radius,
  clearance, name, number). After a rebake or an override edit: rerun `pads.py build --tsv ...` and commit
  the TSV.
- Ids: the prefab's `globalNodeId`, or `<sector>/<nodeData index>` for painted-only pads, or the
  override id. Stable for a given set of sectors; a patch that rebuilds sectors can change them.
- Plugin: `Navigation::Helipads` reads the TSV at load; scripts query it through `FlightNavMap`
  (`GetHelipad*`, `FindNearestHelipad`, `FindHelipadsInRange`; distances horizontal).
- `src/redscript/Navigation/FlightHelipads.reds` (`FlightHelipadSystem`): pads within 150 m are
  discovered (saved), registered as our own map pins (`Mappins.LTBF_LandingPadMappin`, the icon from
  `tools/packaging/icons/landing_pad_icon.py`), and the first touchdown on each pad gives supplies
  (saved too).
- Toasts (discovery, supplies): the game's "new area" notification (`notification_LocationAdded`,
  as `JournalNotificationQueue.OnNewLocationDiscovered` pushes it on a first visit to a
  district), with the landing pad icon. The header line ("Entering a new area", `NewQuest_txt` in
  `quest_update.inkwidget`) says what happened ("Landing pad discovered", "Landing pad
  supplies"), the big line is the pad's name. `FlightHelipadSystem` keeps the HUD's
  `JournalNotificationQueue` and pushes a `FlightLandingPadToastData` onto it;
  `NewLocationNotification` sets the icon's part name only, so our atlas and the header are set
  one tick after the widget initializes (the compiler will not wrap `SetNotificationData`).
- The Autopilot lands on a pad when the destination is one, or within `padRange` of one, on the
  pad's heading.
- A voice line on discovery or touchdown (Johnny's, or a custom recording): how the game triggers
  character lines is researched (four options); no option picked yet.

## Pad numbers

Every listed pad has a number for good, painted on its deck and shown in its pin. In vanilla every pad with a
numbered sign says "28": the sign is one mesh, `landing_pad_b_01.mesh`, a 5 x 5 m quad with a `diode_pavements`
material (an LED grid) and a 128 x 128 texture, `helipad_b_01.xbm`, with the 28 in it; 71 placements of it.

- **Numbers:** `tools/pads/pad_numbers.json` (pad id -> number), kept by `pad_numbers.py`: a new pad gets the
  next free number, in map reading order (500 m bands north to south, west to east), and a number is never
  reused. `pads.py build` and `pad_numbers.py number` write it into the TSV's `number` column; `FlightNavMap`
  gives it to scripts (`GetHelipadNumber`), and the pin title reads "AV Landing Pad <n>" ("<name> (<n>)" for a named pad).
- **Signs:** `pad_number_assets.py` makes the ring (the vanilla sign with its digits taken out) and one sign per
  digit, drawn in the vanilla sign's pixel style, each a copy of the vanilla mesh with our texture
  (`user\jackhumbert\pads\`). `pad_numbers.py signs` writes `ltbf_pad_number.ent` / `.app`, one appearance per
  number (the ring and its digits as mesh components, digits along the sign's local X, a multiple of the
  diode pitch apart), and `pad_number_signs.json`: one sign per pad at its vanilla marking, placed by
  `pad_sector.py` as a `worldEntityNode` with that appearance.
- **The vanilla 28:** hidden under each of ours by the generated block of `src/navigation/hidden_nodes.txt`.
  The 28 is a cooked static mesh with no `globalNodeId`, and it shares its x y with the square border, so it is
  listed by its nodeData index (`I <index> <x> <y>`; the plugin checks the record's x y before hiding it).
- **Which pads:** 69 have the vanilla 28 (ours goes in its place), 17 only the square border (ours goes in its
  middle, turned 90 degrees from it as the vanilla sign always is), 5 are `av_landing_pad_a_v3_small` decks with
  nothing painted (ours goes in the deck's middle, from the proxy mesh's bounds: the prefab's origin is at one
  end). Not painted: the 16 `av_landing_pad_a_v2` and 6 Charter Hill decks (grating, nowhere to paint), the v3
  and v4 pads that show only as their proxy (its texture has a 28 that cannot be hidden alone) and the 499 m
  tower's deck. The three spaceport spots had no marking at all and left the list.
- The sign sits 1.5 cm over the vanilla paint (the digits 3 cm): at 2 mm one deck drew over it.
- After a rebake or new pads: `pads.py build --tsv ...` (or `pad_numbers.py number`), then
  `pad_numbers.py signs --work <pads.py work dir>`, `pad_sector.py`, `tools/packaging/pack_archive.py`.

## Visual-only pads

Some pads are drawn but have no collision under the deck (`tools/pads/pad_collision.py` finds them).
We give them a real deck: a pad entity of our own built from a complete pad of the same prefab
(`pad_donor.py`, `pad_kit.py`, `pad_entity.py`), placed in our sector (`pad_sector.py`,
`tools/pads/pad_sectors.json`).

### Recipe: a real deck for a visual-only pad

1. Check that it is visual only, levels 0 to 3 (`pad_collision.py`, section 8). Levels 0 to 2
   alone call roofs visual that are not, and `--tol 4`: the nav map floor sits 1.5 to 3 m above
   the real surface. Confirm in game with rays over a wide area (below), run once with the
   committed archive so nothing of ours is in the way.
2. Kit for the prefab, from three or four complete placements whose surroundings differ (or that
   sit in a row: parts of the neighbours land at different local offsets and drop out):
   `pad_donor.py` (`--half` wide enough for the prefab), `pad_kit.py`, `pad_geometry.py`,
   `pad_appearances.py` (WolvenKit cannot write a hashed CName: without it `cr2w -d` of the
   `.app` fails with `NotResolvableException: val`), `pad_entity.py` (100 meshes per entity).
   `cr2w -d` the jsons into `src/wolvenkit/source/archive/user/jackhumbert/entities/`.
3. Add the pad to `pad_sectors.json` at the proxy's transform (`pads/placements.json`), streaming
   distance as far as the proxy drew; `pad_sector.py`; `tools/packaging/pack_archive.py`.
4. Hide the proxy: `src/navigation/hidden_nodes.txt` (sector, `globalNodeId`, x y).
5. List the pad: an `add` with `own_deck` in `helipads_overrides.json`, deck height from rays on
   our deck; `pads.py build --tsv ../../src/navigation/helipads.tsv`.
6. In game: rays, stand on it, Autopilot landing (snap: target within 25 m and 6 m of the deck).

Ways of looking that worked here:

- Rays from CET: `Game.GetSpatialQueriesSystem():SyncRaycastByCollisionGroup(from, to,
  CName.new('VehicleBlocker'), false, false)` returns `ok, result`, `result.position.z` is the
  hit. `Static` and `PlayerBlocker` groups see the same decks.
- High places: `Game.GetGodModeSystem():AddGodMode(player:GetEntityID(),
  gameGodModeType.Invulnerable, CName.new('x'))` first, then teleport onto known collision and
  wait 30 to 40 s before testing what streams in; teleporting straight onto a deck that has not
  streamed drops V through it, and a deck height read from where V landed then is the wrong one.
- A control: when something of ours does not show up, put a copy next to one that is known to
  work (here the Watson spot) in the same file. Both missing means the file or its registration,
  not the place.
- Not a test for our entities: `Game.GetTargetingSystem():GetTargetParts` (the
  `mount_nearest.lua` query) lists V, cars and NPCs but never the pad entities, also when they
  had loaded. Use rays.
