"""Machine paths the tools default to, each overridable by an environment variable. The defaults are
Jack's PC (kevin); set the variables on another machine instead of editing the tools."""

import os

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))

# the game install
GAME = os.environ.get("CP2077_GAME_DIR", r"D:\Games\SteamLibrary\steamapps\common\Cyberpunk 2077")
ARCHIVES = os.path.join(GAME, "archive", "pc")
LTBF_LOG = os.path.join(GAME, "red4ext", "logs", "let_there_be_flight.log")
# WolvenKit.Console 10 (reads 2.5 sectors; pack_archive.py has its own 9.0.1 default)
WOLVENKIT = os.environ.get("WOLVENKIT_CLI10", r"D:\cp2077_work\build\tools\wolvenkit-10.0.0-beta.1\WolvenKit.CLI.exe")
# work folder for bakes, caches and the hangar build (not in the repo)
WORK = os.environ.get("LTBF_WORK", r"D:\cp2077_work\build")
# extracted game files (tools/kitbash/make_depot.ps1)
DEPOT = os.environ.get("CP2077_DEPOT", r"E:\cp2077_depot")
BLENDER = os.environ.get("BLENDER_EXE", r"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe")
# the shared game-driving tools (bridge.py, game.py)
CP2077_TOOLS = os.environ.get("CP2077_TOOLS", os.path.join(os.path.dirname(REPO), "cp2077_tools"))
# this repo's tools folder (tools/<folder>/ scripts reach each other's data through it)
TOOLS = os.path.dirname(HERE)
