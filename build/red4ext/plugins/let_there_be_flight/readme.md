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

## Dependencies

* [RED4ext](https://github.com/WopsS/RED4ext)
* [TweakXL](https://github.com/psiberx/cp2077-tweak-xl)
* [Input Loader](https://github.com/jackhumbert/cyberpunk2077-input-loader)
* [Redscript](https://github.com/jac3km4/redscript)

## Configuration

[`r6/input/let_there_be_flight.xml`](https://github.com/jackhumbert/let_there_be_flight/tree/main/build/r6/input/let_there_be_flight.xml) contains all the keybindings for the keyboard & controller - you can customize these to your liking. See all possibilities here: https://redscript.redmodding.org/#5993

Other settings can be found in [`r6/scripts/let_there_be_flight.reds`](https://github.com/jackhumbert/let_there_be_flight/tree/main/build/r6/scripts/let_there_be_flight.reds) for the time being - search for `FlightSettings.SetFloat` to see all the variables used by the mod.

## Bugs

After a crash or error, please save your `red4ext/logs/let_there_be_flight.log` file - this will get written over the next time you start the game, and may contain helpful information regarding the issue.

If you come across something that doesn't work quite right, or interferes with another mod, [search for an issue!](https://github.com/jackhumbert/let_there_be_flight/issues) I have a lot of things on a private TODO list still, but can start to move things to Github issues.

**New issues/pull requests are disabled until I get things closer to a release and can manage things better.**

## Uninstallation

There's an installation script at `red4ext/plugins/let_there_be_flight/uninstall.bat` - if you run this, all LTBF mod files (including codeware and FMOD files) will be deleted, but its dependencies will remain.

Special thanks to @psiberx for [Codeware Lib](https://github.com/psiberx/cp2077-codeware/), [InkPlayground Demo](https://github.com/psiberx/cp2077-playground), and Redscript & CET examples on Discord, @WopsS for [RED4ext](https://github.com/WopsS/RED4ext), @jac3km4 for [Redscript toolkit](https://github.com/jac3km4/redscript), @yamashi for [CET](https://github.com/yamashi/CyberEngineTweaks) and the [E-mode mod](https://www.nexusmods.com/cyberpunk2077/mods/3207?tab=description) (very helpful in figuring out how to work with FMOD), @rfuzzo & team (especially @seberoth!) for [WolvenKit](https://github.com/WolvenKit/WolvenKit), and all of them for being helpful on Discord.

Interested in adding flying vehicles or a similar mechanic to an existing game? Let me know!

Audio system made with [FMOD Studio](https://www.fmod.com/) by Firelight Technologies Pty Ltd.
