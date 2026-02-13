# Two-Worlds-Modding-Guid

Intro: A collection of community tools and guides for modding Two Worlds 1 (Epic Edition). These tools cover the essential steps needed to create, pack, and load custom map mods.

## Contents
Mod Selector — A lightweight tool by Buglord that toggles mods on/off via the Windows registry. Place it in your game directory, run it, and use Enter to switch mods between active (green) and inactive (red). Works by setting HKCU\SOFTWARE\Reality Pump\TwoWorlds\Mods\<filename.wd> to 1 or 0.
WD Repacker (Win32) — Buglord's GUI tool for packing and unpacking .wd archives (the mod format used by Two Worlds 1). Important: when packing, select the folder as source, not a .wd file. The repacker correctly handles .phx files by leaving them uncompressed — other packers may compress them, which causes crashes.


## Map to Mod Workflow (Overview)
Kurzes ASCII-Diagramm:
Editor Map (Saves/Levels/Map_F01s.lnd)
    → Cook PhysX (4 console commands in the editor)
    → Generate LevelHeaders (LevelHeadersCacheGen.bat)
    → Copy & rename files (Map_F01s → Map_F01)
    → Pack into .wd archive (WD Repacker)
    → Activate via Mod Selector or registry
    → Start a new game
Plus ein Hinweis: "The s suffix and any extra characters after the first two digits must be removed when renaming — mod files must match the vanilla filenames exactly."
## Credits

- Buglord — Mod Selector, WD Repacker, wdio.py, registry research --> https://github.com/buglord/Two-Worlds-1-Misc-Projects/tree/main
- JadetheReaper & smoothness — conversion process documentation
- Reality Pump Studios — Two Worlds SDK

## Links

- Two Worlds SDK v1.3 on ModDB  -->   https://www.moddb.com/downloads/two-worlds-software-development-kit-tools-13-installer
- Two Worlds on Steam 
