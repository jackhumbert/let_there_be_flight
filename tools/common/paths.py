"""Machine paths for the offline tools.

Each path comes from, in order: its environment variable, then tools/common/paths.local.json (not
tracked; copy paths.example.json beside this file and fill it in), then a default where one makes
sense. A path with no default that is not set raises a KeyError naming the variable and the json
key, only when a tool reads it, so tools that do not need the game or the depot run without them.

    from common import paths
    paths.WORK      # work folder for bakes, caches and the hangar build
"""

import json
import os
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
# this repo, and its tools folder (tools/<folder>/ scripts reach each other's data through it)
REPO = os.path.dirname(os.path.dirname(HERE))
TOOLS = os.path.dirname(HERE)

LOCAL_JSON = os.path.join(HERE, "paths.local.json")

# name: (environment variable, default or None, what it is)
SETTINGS = {
    "GAME": ("CP2077_GAME_DIR", None, "the game install (the folder holding bin/ and archive/)"),
    "WOLVENKIT": ("WOLVENKIT_CLI10", None, "WolvenKit.CLI.exe 10.x (reads 2.5 sectors)"),
    "WOLVENKIT_PACK": ("WOLVENKIT_CLI", None,
                       "WolvenKit.CLI.exe 9.0.1, which packs the archive; the game's oo2ext_7_win64.dll beside it"),
    "WORK": ("LTBF_WORK", os.path.join(REPO, "build", "tools_work"),
             "work folder for bakes, caches and the hangar build (kept between runs)"),
    "TMP": ("CP2077_TMP", os.path.join(tempfile.gettempdir(), "ltbf_tools"),
            "scratch output of one run (benchmarks and the like); delete it when done"),
    "DEPOT": ("CP2077_DEPOT", None, "the game's archives unbundled (tools/kitbash/make_depot.ps1)"),
    "DEPOT_TEX": ("CP2077_DEPOT_TEX", None, "the game's textures exported for Blender (the depot's textures)"),
    "BLENDER": ("BLENDER_EXE", "blender", "blender.exe (default: the one on PATH)"),
    "FFMPEG": ("FFMPEG", "ffmpeg", "ffmpeg.exe (default: the one on PATH)"),
    "PHYSX_SRC": ("LTBF_PHYSX_SRC", None, "a PhysX 3.4 source checkout (tools/packaging/physx_cook)"),
}

_local = None


def _local_json():
    global _local
    if _local is None:
        _local = {}
        if os.path.exists(LOCAL_JSON):
            with open(LOCAL_JSON, encoding="utf-8") as f:
                _local = json.load(f)
    return _local


def get(name):
    """The path for one SETTINGS name; KeyError naming where to set it if it has no value."""
    env, default, what = SETTINGS[name]
    value = os.environ.get(env) or _local_json().get(name) or default
    if not value:
        raise KeyError(f"{name} is not set: {what}. Set the environment variable {env}, or \"{name}\" in {LOCAL_JSON} "
                       f"(template: paths.example.json)")
    return value


def __getattr__(name):
    if name in SETTINGS:
        return get(name)
    if name == "ARCHIVES":
        return os.path.join(get("GAME"), "archive", "pc")
    if name == "LTBF_LOG":
        return os.path.join(get("GAME"), "red4ext", "logs", "let_there_be_flight.log")
    raise AttributeError(name)


def scratch(prefix):
    """A new folder under TMP for one run's files."""
    tmp = get("TMP")
    os.makedirs(tmp, exist_ok=True)
    return tempfile.mkdtemp(prefix=prefix, dir=tmp)
