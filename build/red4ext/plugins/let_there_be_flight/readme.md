# Let There Be Flight

This is a mod (currently in a beta state) for Cyberpunk 2077 that adds a flying mechanism to all cars, with a couple different modes and options.

Other features include:
* Imminent explosion audio indicator
* Visual explosions happen more readily with other cars
* Quick-eject in flight mode (hold exit)

[Video preview of the Drone/Acrobat mode](https://www.youtube.com/watch?v=U9t2JWMY1-k)

![preview](preview.png)

## Installation

For an up-to-date version of the mod, you can [download just the `build/` folder](https://downgit.github.io/#/home?url=https://github.com/jackhumbert/let_there_be_flight/tree/main/build), and install that into your game's directory.

[Official, numbered releases are available here](https://github.com/jackhumbert/let_there_be_flight/releases) - `*packed*.zip` in the release contains all of the dependencies listed below at their most up-to-date versions (at the time of release). Simply extract it and copy the contents in your game's installation folder. If you're upgrading from v0.0.9, you'll need to delete the `r6/scripts/flight_control` folder, since files may have been renamed/removed. All files named `flight_control` can safely be removed.

## Dependencies (available in `/prereqs`)

* [RED4ext](https://github.com/WopsS/RED4ext)
* [TweakXL](https://github.com/psiberx/cp2077-tweak-xl)
* [Input Loader](https://github.com/jackhumbert/cyberpunk2077-input-loader)
* [Redscript](https://github.com/jac3km4/redscript)
* [Mod Settings](https://github.com/jackhumbert/mod_settings)

## Configuration

[`r6/input/let_there_be_flight.xml`](https://github.com/jackhumbert/let_there_be_flight/tree/main/build/r6/input/let_there_be_flight.xml) contains all the keybindings for the keyboard & controller - you can customize these to your liking. See all possibilities here: https://redscript.redmodding.org/#5993

Other settings can be found in [`r6/scripts/let_there_be_flight.reds`](https://github.com/jackhumbert/let_there_be_flight/tree/main/build/r6/scripts/let_there_be_flight.reds) for the time being - search for `FlightSettings.SetFloat` to see all the variables used by the mod.

## Troubleshooting

To better report crashes, click on "What does this report contain?" in the crash handler window:

<img width="420" alt="CrashReporter_ZpKyOJoSbT" src="https://user-images.githubusercontent.com/141431/186788162-9898b344-a22c-42b8-9ed3-e21518e17179.png">

And find the Cyberpunk2077.dmp file, shown here:

<img width="584" alt="explorer_iuXThrg7iB" src="https://user-images.githubusercontent.com/141431/186788246-eaa77ba1-5891-4f93-9abd-2f033b7e6f1a.png">

Copy this file, along with your `red4ext/logs/let_there_be_flight.log` file *before* you run the game again - it will get overwritten on each launch:

<img width="584" alt="explorer_vV4IdIzcT7" src="https://user-images.githubusercontent.com/141431/186788320-f909c7b4-ca0d-4fcc-b77a-14b3021fe45b.png">

If you can include both of these files in any report, it should make it a lot easier to track down bugs.

## Uninstallation

There's an installation script at `red4ext/plugins/let_there_be_flight/uninstall.bat` - if you run this, all LTBF mod files (including codeware and FMOD files) will be deleted, but its dependencies will remain.

Special thanks to @psiberx for [Codeware Lib](https://github.com/psiberx/cp2077-codeware/), [InkPlayground Demo](https://github.com/psiberx/cp2077-playground), and Redscript & CET examples on Discord, @WopsS for [RED4ext](https://github.com/WopsS/RED4ext), @jac3km4 for [Redscript toolkit](https://github.com/jac3km4/redscript), @yamashi for [CET](https://github.com/yamashi/CyberEngineTweaks) and the [E-mode mod](https://www.nexusmods.com/cyberpunk2077/mods/3207?tab=description) (very helpful in figuring out how to work with FMOD), @rfuzzo & team (especially @seberoth!) for [WolvenKit](https://github.com/WolvenKit/WolvenKit), and all of them for being helpful on Discord.

Interested in adding flying vehicles or a similar mechanic to an existing game? Let me know!

Audio system made with [FMOD Studio](https://www.fmod.com/) by Firelight Technologies Pty Ltd.
