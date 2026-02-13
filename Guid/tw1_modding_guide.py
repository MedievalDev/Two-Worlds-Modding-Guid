#!/usr/bin/env python3
"""
TW1 Modding Guide v1.2
Interactive step-by-step tutorial for Two Worlds 1 modding beginners.
Clean, readable UI — one step at a time, 12 steps.
"""

import tkinter as tk
from tkinter import filedialog, messagebox
import os
import sys
import json
import glob
import subprocess
import threading
import time
import webbrowser

# ============================================================
# OPTIONAL IMPORTS
# ============================================================
try:
    import pyautogui
    pyautogui.FAILSAFE = True
    HAS_PYAUTOGUI = True
except ImportError:
    HAS_PYAUTOGUI = False

try:
    import pygetwindow as gw
    HAS_PYGETWINDOW = True
except ImportError:
    HAS_PYGETWINDOW = False

try:
    import winreg
    HAS_WINREG = True
except ImportError:
    HAS_WINREG = False

# ============================================================
# CONSTANTS
# ============================================================
VERSION = "1.2"
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CONFIG_FILE = os.path.join(SCRIPT_DIR, "tw1_guide_config.json")
TOTAL_STEPS = 12

URL_SDK = "https://www.moddb.com/downloads/two-worlds-software-development-kit-tools-13-installer"
URL_GITHUB = "https://github.com/MedievalDev/Two-Worlds-Modding-Guid"

# ============================================================
# COLORS
# ============================================================
BG = "#1a1a2e"
BG2 = "#16213e"
BG3 = "#0f3460"
FG = "#e8e8e8"
FG_DIM = "#999999"
ACCENT = "#e94560"
GREEN = "#4ecca3"
YELLOW = "#ffaa00"
RED = "#ff6b6b"
CARD_BG = "#16213e"
LINK_COLOR = "#64b5f6"

# ============================================================
# TRANSLATIONS
# ============================================================
# Steps:
#  1  SDK installieren
#  2  Spiel finden
#  3  Saves-Ordner
#  4  Tools besorgen
#  5  Editor starten          (was 6)
#  6  Map speichern           (was 7)
#  7  PhysX kochen            (was 8)
#  8  LevelHeaders            (was 9)
#  9  Kopieren & Umbenennen   (was 10)
# 10  .wd packen              (was 11)
# 11  Mod aktivieren          (was 12)
# 12  Spielen!                (was 13)

_current_lang = "DE"

LANG = {
    "EN": {
        "title": "TW1 Modding Guide",
        "step_of": "Step {} of {}",
        "btn_next": "Next  →",
        "btn_prev": "←  Back",
        "btn_verify": "✔  Check",
        "btn_browse": "Browse...",
        "btn_open": "Open",
        "btn_open_editor": "🖥  Open Editor",
        "btn_send_physx": "⚡  Send PhysX Commands",
        "font_size": "Font:",
        "not_found": "Not found",

        # Step 1 - SDK
        "s1_title": "Install the SDK",
        "s1_what": "The SDK contains the map editor and all tools you need for modding Two Worlds 1.",
        "s1_do": "Download the SDK from ModDB and extract it to a simple location like:",
        "s1_example": "C:\\Games\\TwoWorldsSDK\\",
        "s1_note": "Keep it OUTSIDE the game folder, in a short path.",
        "s1_select": "Select your SDK folder:",
        "s1_ok": "SDK found! TwoWorldsEditor.exe is present.",
        "s1_fail": "TwoWorldsEditor.exe not found in this folder.",

        # Step 2 - Game
        "s2_title": "Find Your Game",
        "s2_what": "You need Two Worlds - Epic Edition installed. Steam version works fine.",
        "s2_do": "Find your game folder and make sure it has a 'Mods' subfolder (capital M!). If not, create one.",
        "s2_example": "Two Worlds - Epic Edition\\Mods\\",
        "s2_note": "'Mods' must be spelled with a capital M!",
        "s2_select": "Select your game folder:",
        "s2_ok": "Game found! Mods folder present.",
        "s2_fail_exe": "TwoWorlds.exe not found here.",
        "s2_fail_mods": "No 'Mods' folder! Create one inside the game folder.",

        # Step 3 - Saves
        "s3_title": "Saves Folder",
        "s3_what": "The game saves maps and editor files here:",
        "s3_path": "%USERPROFILE%\\Saved Games\\Two Worlds Saves\\",
        "s3_do": "If this folder doesn't exist yet, start Two Worlds once, begin a new game, save, and exit.",
        "s3_select": "Saves folder:",
        "s3_ok": "Saves folder found!",
        "s3_fail": "Saves folder not found. Start the game once to create it.",

        # Step 4 - Tools
        "s4_title": "Get the Tools",
        "s4_what": "You need these community tools (by Buglord):",
        "s4_tool1": "TwoWorlds1 Mod Selector.exe",
        "s4_tool1_desc": "→ Copy into your game folder (next to TwoWorlds.exe)",
        "s4_tool2": "Tw1WDRepacker.exe",
        "s4_tool2_desc": "→ Keep somewhere accessible (needed later)",
        "s4_do": "Download from the GitHub modding guide:",
        "s4_ok": "Mod Selector: ✅\nRepacker: ✅",
        "s4_fail_sel": "Mod Selector: ❌ not in game folder",

        # Step 5 - Editor
        "s5_title": "Start the Editor",
        "s5_what": "Time to open the editor!",
        "s5_controls": "Basic Controls:\n  C key   →  Open/close console\n  WASD    →  Move camera\n  Mouse   →  Look around",
        "s5_do": "Click the button below or start TwoWorldsEditor.exe manually.\nWait for it to fully load.",
        "s5_note": "The editor takes a while to start. Be patient!",
        "s5_ok": "Editor window found!",
        "s5_fail": "Editor not detected. Is it running?",

        # Step 6 - Save Map
        "s6_title": "Save Your Map",
        "s6_what": "When you save in the editor, files get an 's' suffix:",
        "s6_example": "Original:    Map_F01      (vanilla game map)\nYour save:   Map_F01s     ('s' = editor save)",
        "s6_do": "1. Make changes in the editor (place objects, edit terrain)\n2. Save the map\n3. Check Saves\\Levels\\ for a Map_*s.lnd file",
        "s6_note": "Later you'll need to REMOVE the 's' when packing the mod.",
        "s6_ok": "Map files found: {}",
        "s6_fail": "No editor maps (Map_*s.lnd) found in Saves\\Levels\\.",

        # Step 7 - PhysX
        "s7_title": "Cook PhysX",
        "s7_what": "PhysX = collision data. Without it, you fall through the ground!\n\n4 console commands to cook it:",
        "s7_cmds": "editor.cookphysx.mode geomipmap\neditor.cookphysx.strength = 1.0\neditor.cookphysx.overwrite = 1\neditor.cookphysx.pc out",
        "s7_do": "Open the console (C key) and enter the commands above.\nOr click the button to send them automatically.",
        "s7_warn": "The .phx file must NEVER be compressed!\nOnly use Buglord's tools for packing.",
        "s7_ok": "PhysX found: {}",
        "s7_fail": "No .phx files in Saves\\Levels\\Physic\\.",

        # Step 8 - LevelHeaders
        "s8_title": "Generate LevelHeaders",
        "s8_what": "Map_LevelHeaders.lhc = index of all maps.\nMust be regenerated after every map change.",
        "s8_do": "1. Open the .bat file in a text editor first\n2. Check the game path inside is correct\n3. Double-click LevelHeadersCacheGen.bat to run it",
        "s8_note": "The .bat is in your SDK\\Tools\\ folder.",
        "s8_ok": "Map_LevelHeaders.lhc found!",
        "s8_fail": "Not found. Did you run the batch file?",

        # Step 9 - Copy & Rename
        "s9_title": "Copy & Rename Files",
        "s9_what": "Prepare files for packing — rename and organize:",
        "s9_structure": "Mods\\YourModName\\\n  └── Levels\\\n      ├── Map_F01.lnd       ← was Map_F01s.lnd\n      ├── Map_F01.bmp       ← was Map_F01s.bmp\n      ├── Map_LevelHeaders.lhc\n      └── physic\\           ← LOWERCASE!\n          └── Map_F01.phx   ← was Map_F01s.phx",
        "s9_do": "1. Create Mods\\YourModName\\Levels\\ and Levels\\physic\\\n2. Copy files from Saves\\Levels\\ → rename (remove 's'!)\n3. Copy .phx from Saves\\Levels\\Physic\\\n4. Copy Map_LevelHeaders.lhc into Levels\\",
        "s9_warn": "Remove the 's'!  Map_F01s.lnd → Map_F01.lnd",
        "s9_ok": "Mod folder found: {}",
        "s9_fail": "No mod folders with Levels\\ subfolder found in Mods\\.",

        # Step 10 - Pack .wd
        "s10_title": "Pack into .wd",
        "s10_what": "Pack your mod folder into a .wd archive using Tw1WDRepacker.",
        "s10_do": "Open Tw1WDRepacker.exe:\n\n   SOURCE:        Your mod folder\n   DESTINATION:   Game\\Mods\\ folder\n   → Click PACK",
        "s10_source_explain": "  ✅ CORRECT:  Mods\\YourMod        (contains Levels\\)\n  ❌ WRONG:    Mods\\YourMod\\Levels  (too deep!)\n  ❌ WRONG:    Mods\\YourMod.wd       (file, not folder!)",
        "s10_note": "Folder name becomes .wd name:  YourMod → YourMod.wd",
        "s10_ok": ".wd files in Mods: {}",
        "s10_fail": "No custom .wd files in Mods folder.",

        # Step 11 - Activate
        "s11_title": "Activate Your Mod",
        "s11_what": "Use the Mod Selector to activate your .wd file.",
        "s11_do": "1. Run 'TwoWorlds1 Mod Selector.exe' from your game folder\n2. Find your mod → press ENTER (Red → Green)",
        "s11_note": "The Mod Selector writes to the Windows Registry:\nHKCU\\SOFTWARE\\Reality Pump\\TwoWorlds\\Mods\nEach .wd = 1 (active) or 0 (inactive)\nOnly one map-replacing mod active at a time!",
        "s11_ok": "Active mods: {}",
        "s11_fail": "No active mods in registry.",

        # Step 12 - Play
        "s12_title": "Play! 🎮",
        "s12_what": "🎉  You did it!  Your mod is ready!",
        "s12_do": "Start Two Worlds → NEW GAME (not Load!)\n\nYour modded map should load.\nAlways start a NEW game — saves use old map data!",
        "s12_summary": "What you've learned:\n  ✅ SDK & folder structure\n  ✅ How mods work (.wd + registry)\n  ✅ Editor & console\n  ✅ PhysX cooking\n  ✅ LevelHeaders\n  ✅ File renaming (Map_F01s → Map_F01)\n  ✅ Packing with Repacker\n  ✅ Activating with Mod Selector",
        "s12_next": "Next steps:\n  → TW Editor CMD Injector (faster workflows)\n  → TW1 Map-to-Mod Converter (one-click conversion)\n  → Join the modding community on Discord!",
    },
    "DE": {
        "title": "TW1 Modding Guide",
        "step_of": "Schritt {} von {}",
        "btn_next": "Weiter  →",
        "btn_prev": "←  Zurueck",
        "btn_verify": "✔  Pruefen",
        "btn_browse": "Durchsuchen...",
        "btn_open": "Oeffnen",
        "btn_open_editor": "🖥  Editor oeffnen",
        "btn_send_physx": "⚡  PhysX-Befehle senden",
        "font_size": "Schrift:",
        "not_found": "Nicht gefunden",

        # Step 1
        "s1_title": "SDK installieren",
        "s1_what": "Das SDK enthaelt den Map-Editor und alle Werkzeuge die du zum Modden von Two Worlds 1 brauchst.",
        "s1_do": "Lade das SDK von ModDB herunter und entpacke es an einen einfachen Ort wie:",
        "s1_example": "C:\\Games\\TwoWorldsSDK\\",
        "s1_note": "AUSSERHALB des Spielordners, kurzer Pfad.",
        "s1_select": "Waehle deinen SDK-Ordner:",
        "s1_ok": "SDK gefunden! TwoWorldsEditor.exe ist vorhanden.",
        "s1_fail": "TwoWorldsEditor.exe nicht in diesem Ordner gefunden.",

        # Step 2
        "s2_title": "Spiel finden",
        "s2_what": "Du brauchst Two Worlds - Epic Edition. Steam-Version funktioniert.",
        "s2_do": "Finde deinen Spielordner und stelle sicher dass ein 'Mods'-Unterordner existiert (grosses M!). Falls nicht, erstelle einen.",
        "s2_example": "Two Worlds - Epic Edition\\Mods\\",
        "s2_note": "'Mods' muss mit grossem M geschrieben sein!",
        "s2_select": "Waehle deinen Spielordner:",
        "s2_ok": "Spiel gefunden! Mods-Ordner vorhanden.",
        "s2_fail_exe": "TwoWorlds.exe nicht gefunden.",
        "s2_fail_mods": "Kein 'Mods'-Ordner! Erstelle einen im Spielordner.",

        # Step 3
        "s3_title": "Saves-Ordner",
        "s3_what": "Das Spiel speichert Maps und Editor-Dateien hier:",
        "s3_path": "%USERPROFILE%\\Saved Games\\Two Worlds Saves\\",
        "s3_do": "Falls dieser Ordner noch nicht existiert: Starte Two Worlds einmal, beginne ein neues Spiel, speichere und beende.",
        "s3_select": "Saves-Ordner:",
        "s3_ok": "Saves-Ordner gefunden!",
        "s3_fail": "Saves-Ordner nicht gefunden. Starte das Spiel einmal.",

        # Step 4
        "s4_title": "Tools besorgen",
        "s4_what": "Du brauchst diese Community-Tools (von Buglord):",
        "s4_tool1": "TwoWorlds1 Mod Selector.exe",
        "s4_tool1_desc": "→ Kopiere in deinen Spielordner (neben TwoWorlds.exe)",
        "s4_tool2": "Tw1WDRepacker.exe",
        "s4_tool2_desc": "→ Behalte griffbereit (brauchst du spaeter)",
        "s4_do": "Herunterladen vom GitHub Modding Guide:",
        "s4_ok": "Mod Selector: ✅\nRepacker: ✅",
        "s4_fail_sel": "Mod Selector: ❌ nicht im Spielordner",

        # Step 5
        "s5_title": "Editor starten",
        "s5_what": "Zeit den Editor zu oeffnen!",
        "s5_controls": "Grundsteuerung:\n  C-Taste  →  Konsole oeffnen/schliessen\n  WASD     →  Kamera bewegen\n  Maus     →  Umschauen",
        "s5_do": "Klicke den Button unten oder starte TwoWorldsEditor.exe manuell.\nWarte bis er vollstaendig geladen ist.",
        "s5_note": "Der Editor braucht eine Weile zum Starten. Geduld!",
        "s5_ok": "Editor-Fenster gefunden!",
        "s5_fail": "Editor nicht erkannt. Laeuft er?",

        # Step 6
        "s6_title": "Map speichern",
        "s6_what": "Beim Speichern im Editor bekommen Dateien ein 's'-Suffix:",
        "s6_example": "Original:    Map_F01      (Vanilla-Spielkarte)\nDein Save:   Map_F01s     ('s' = Editor-Speicherstand)",
        "s6_do": "1. Aenderungen im Editor machen (Objekte platzieren, Terrain)\n2. Map speichern\n3. Pruefe Saves\\Levels\\ auf eine Map_*s.lnd Datei",
        "s6_note": "Spaeter musst du das 's' ENTFERNEN beim Mod-Packen.",
        "s6_ok": "Map-Dateien gefunden: {}",
        "s6_fail": "Keine Editor-Maps (Map_*s.lnd) in Saves\\Levels\\ gefunden.",

        # Step 7
        "s7_title": "PhysX kochen",
        "s7_what": "PhysX = Kollisionsdaten. Ohne sie faellst du durch den Boden!\n\n4 Konsolen-Befehle zum Kochen:",
        "s7_cmds": "editor.cookphysx.mode geomipmap\neditor.cookphysx.strength = 1.0\neditor.cookphysx.overwrite = 1\neditor.cookphysx.pc out",
        "s7_do": "Konsole oeffnen (C-Taste) und Befehle eingeben.\nOder Button klicken fuer automatische Eingabe.",
        "s7_warn": "Die .phx-Datei darf NIEMALS komprimiert werden!\nNur Buglords Tools zum Packen verwenden.",
        "s7_ok": "PhysX gefunden: {}",
        "s7_fail": "Keine .phx-Dateien in Saves\\Levels\\Physic\\.",

        # Step 8
        "s8_title": "LevelHeaders",
        "s8_what": "Map_LevelHeaders.lhc = Index aller Maps.\nMuss nach jeder Map-Aenderung neu erstellt werden.",
        "s8_do": "1. Oeffne die .bat-Datei zuerst im Texteditor\n2. Pruefe ob der Spielpfad darin stimmt\n3. Doppelklick auf LevelHeadersCacheGen.bat",
        "s8_note": "Die .bat liegt im SDK\\Tools\\-Ordner.",
        "s8_ok": "Map_LevelHeaders.lhc gefunden!",
        "s8_fail": "Nicht gefunden. Hast du die Batch-Datei ausgefuehrt?",

        # Step 9
        "s9_title": "Kopieren & Umbenennen",
        "s9_what": "Dateien vorbereiten — umbenennen und organisieren:",
        "s9_structure": "Mods\\DeinModName\\\n  └── Levels\\\n      ├── Map_F01.lnd       ← war Map_F01s.lnd\n      ├── Map_F01.bmp       ← war Map_F01s.bmp\n      ├── Map_LevelHeaders.lhc\n      └── physic\\           ← KLEINGESCHRIEBEN!\n          └── Map_F01.phx   ← war Map_F01s.phx",
        "s9_do": "1. Erstelle Mods\\DeinModName\\Levels\\ und Levels\\physic\\\n2. Kopiere aus Saves\\Levels\\ → umbenennen ('s' weg!)\n3. Kopiere .phx aus Saves\\Levels\\Physic\\\n4. Kopiere Map_LevelHeaders.lhc in Levels\\",
        "s9_warn": "'s' entfernen!  Map_F01s.lnd → Map_F01.lnd",
        "s9_ok": "Mod-Ordner gefunden: {}",
        "s9_fail": "Keine Mod-Ordner mit Levels\\-Unterordner in Mods\\ gefunden.",

        # Step 10
        "s10_title": ".wd packen",
        "s10_what": "Deinen Mod-Ordner in ein .wd-Archiv packen mit Tw1WDRepacker.",
        "s10_do": "Oeffne Tw1WDRepacker.exe:\n\n   SOURCE:        Dein Mod-Ordner\n   DESTINATION:   Spiel\\Mods\\ Ordner\n   → Klicke PACK",
        "s10_source_explain": "  ✅ RICHTIG:  Mods\\DeinMod          (enthaelt Levels\\)\n  ❌ FALSCH:   Mods\\DeinMod\\Levels    (zu tief!)\n  ❌ FALSCH:   Mods\\DeinMod.wd         (Datei, kein Ordner!)",
        "s10_note": "Ordnername wird .wd-Name:  DeinMod → DeinMod.wd",
        "s10_ok": ".wd-Dateien in Mods: {}",
        "s10_fail": "Keine eigenen .wd-Dateien im Mods-Ordner.",

        # Step 11
        "s11_title": "Mod aktivieren",
        "s11_what": "Aktiviere deine Mod mit dem Mod Selector.",
        "s11_do": "1. Mod Selector aus dem Spielordner starten\n2. Deine Mod finden → ENTER (Rot → Gruen)",
        "s11_note": "Der Mod Selector schreibt in die Windows-Registry:\nHKCU\\SOFTWARE\\Reality Pump\\TwoWorlds\\Mods\nJede .wd = 1 (aktiv) oder 0 (inaktiv)\nNur eine Map-Mod gleichzeitig aktiv!",
        "s11_ok": "Aktive Mods: {}",
        "s11_fail": "Keine aktiven Mods in der Registry.",

        # Step 12
        "s12_title": "Spielen! 🎮",
        "s12_what": "🎉  Geschafft!  Deine Mod ist fertig!",
        "s12_do": "Two Worlds starten → NEUES SPIEL (nicht Laden!)\n\nDeine modifizierte Map sollte laden.\nImmer NEUES Spiel — Saves nutzen alte Map-Daten!",
        "s12_summary": "Was du gelernt hast:\n  ✅ SDK & Ordnerstruktur\n  ✅ Wie Mods funktionieren (.wd + Registry)\n  ✅ Editor & Konsole\n  ✅ PhysX-Cooking\n  ✅ LevelHeaders\n  ✅ Dateien umbenennen (Map_F01s → Map_F01)\n  ✅ Packen mit Repacker\n  ✅ Aktivieren mit Mod Selector",
        "s12_next": "Naechste Schritte:\n  → TW Editor CMD Injector (schnellere Workflows)\n  → TW1 Map-to-Mod Converter (Ein-Klick-Konvertierung)\n  → Tritt der Modding-Community auf Discord bei!",
    },
}


def T(key):
    return LANG.get(_current_lang, LANG["EN"]).get(key, LANG["EN"].get(key, key))


# ============================================================
# HELPERS
# ============================================================

def find_editor_window():
    if not HAS_PYGETWINDOW:
        return None
    for kw in ["twoworldseditor", "two worlds editor", "twoworlds", "editor"]:
        try:
            for w in gw.getAllWindows():
                if w.title and w.visible and kw in w.title.lower():
                    return w
        except Exception:
            pass
    return None


def clipboard_paste(root, text):
    done = threading.Event()
    def _do():
        try:
            root.clipboard_clear()
            root.clipboard_append(text)
            root.update()
        except Exception:
            pass
        done.set()
    root.after(0, _do)
    done.wait(timeout=2.0)
    time.sleep(0.01)
    if HAS_PYAUTOGUI:
        pyautogui.hotkey("ctrl", "v")


def auto_detect_saves():
    user = os.environ.get("USERPROFILE", "")
    if user:
        p = os.path.join(user, "Saved Games", "Two Worlds Saves")
        if os.path.isdir(p):
            return p
    return ""


def auto_detect_game():
    for drive in ["C", "D", "E", "F"]:
        for path in [
            f"{drive}:\\Program Files (x86)\\Steam\\steamapps\\common\\Two Worlds - Epic Edition",
            f"{drive}:\\Steam\\steamapps\\common\\Two Worlds - Epic Edition",
            f"{drive}:\\Games\\Two Worlds - Epic Edition",
        ]:
            if os.path.isdir(path):
                return path
    return ""


# ============================================================
# APP
# ============================================================

class App:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title(T("title"))
        self.root.geometry("820x700")
        self.root.minsize(700, 550)
        self.root.configure(bg=BG)

        self.current_step = 0
        self.step_done = [False] * TOTAL_STEPS
        self.font_size = 14

        self.sdk_path = tk.StringVar()
        self.game_path = tk.StringVar()
        self.saves_path = tk.StringVar()

        self._load_config()
        self._build_chrome()
        self._show_step(self.current_step)

    def _load_config(self):
        global _current_lang
        if os.path.exists(CONFIG_FILE):
            try:
                with open(CONFIG_FILE, "r") as f:
                    cfg = json.load(f)
                _current_lang = cfg.get("lang", "DE")
                self.sdk_path.set(cfg.get("sdk", ""))
                self.game_path.set(cfg.get("game", ""))
                self.saves_path.set(cfg.get("saves", ""))
                self.current_step = min(cfg.get("step", 0), TOTAL_STEPS - 1)
                self.step_done = cfg.get("done", [False] * TOTAL_STEPS)
                self.font_size = cfg.get("font_size", 14)
                # Handle config from old 13-step version
                if len(self.step_done) > TOTAL_STEPS:
                    self.step_done = self.step_done[:TOTAL_STEPS]
                elif len(self.step_done) < TOTAL_STEPS:
                    self.step_done += [False] * (TOTAL_STEPS - len(self.step_done))
            except Exception:
                pass
        if not self.saves_path.get():
            self.saves_path.set(auto_detect_saves())
        if not self.game_path.get():
            self.game_path.set(auto_detect_game())

    def _save_config(self):
        try:
            with open(CONFIG_FILE, "w") as f:
                json.dump({
                    "lang": _current_lang, "sdk": self.sdk_path.get(),
                    "game": self.game_path.get(), "saves": self.saves_path.get(),
                    "step": self.current_step, "done": self.step_done,
                    "font_size": self.font_size,
                }, f, indent=2)
        except Exception:
            pass

    # ---- TOP BAR, DOTS, BOTTOM NAV ----
    def _build_chrome(self):
        top = tk.Frame(self.root, bg=BG3, height=50)
        top.pack(fill="x")
        top.pack_propagate(False)

        tk.Label(top, text=T("title"), font=("Segoe UI", 14, "bold"),
                 bg=BG3, fg=FG).pack(side="left", padx=20)

        self.step_label = tk.Label(top, font=("Segoe UI", 11), bg=BG3, fg=GREEN)
        self.step_label.pack(side="left", padx=20)

        right = tk.Frame(top, bg=BG3)
        right.pack(side="right", padx=15)

        tk.Label(right, text=T("font_size"), font=("Segoe UI", 9),
                 bg=BG3, fg=FG_DIM).pack(side="left")
        tk.Button(right, text="−", font=("Segoe UI", 10, "bold"), width=2,
                 bg=BG2, fg=FG, relief="flat", cursor="hand2",
                 command=lambda: self._change_font(-1)).pack(side="left", padx=2)
        self.font_label = tk.Label(right, text=str(self.font_size),
                                    font=("Segoe UI", 10), bg=BG3, fg=FG, width=3)
        self.font_label.pack(side="left")
        tk.Button(right, text="+", font=("Segoe UI", 10, "bold"), width=2,
                 bg=BG2, fg=FG, relief="flat", cursor="hand2",
                 command=lambda: self._change_font(1)).pack(side="left", padx=2)

        tk.Frame(right, bg=FG_DIM, width=1, height=24).pack(side="left", padx=10)

        self.lang_btn = tk.Button(right, text=_current_lang, width=4,
                                   font=("Segoe UI", 10, "bold"),
                                   bg=BG2, fg=FG, relief="flat", cursor="hand2",
                                   command=self._toggle_lang)
        self.lang_btn.pack(side="left")

        # Progress dots
        self.dots_frame = tk.Frame(self.root, bg=BG)
        self.dots_frame.pack(fill="x", pady=(8, 4))
        self.dots = []
        inner = tk.Frame(self.dots_frame, bg=BG)
        inner.pack()
        for i in range(TOTAL_STEPS):
            dot = tk.Canvas(inner, width=16, height=16, bg=BG, highlightthickness=0)
            dot.pack(side="left", padx=3)
            dot.bind("<Button-1>", lambda e, idx=i: self._dot_click(idx))
            self.dots.append(dot)

        # Content
        self.content_outer = tk.Frame(self.root, bg=BG)
        self.content_outer.pack(fill="both", expand=True, padx=40, pady=(5, 5))

        # Bottom nav
        nav = tk.Frame(self.root, bg=BG2, height=55)
        nav.pack(fill="x", side="bottom")
        nav.pack_propagate(False)

        self.btn_prev = tk.Button(nav, text=T("btn_prev"), font=("Segoe UI", 11),
                                   bg=BG3, fg=FG, relief="flat", padx=25, pady=6,
                                   cursor="hand2", command=self._prev)
        self.btn_prev.pack(side="left", padx=20, pady=10)

        self.btn_next = tk.Button(nav, text=T("btn_next"), font=("Segoe UI", 11, "bold"),
                                   bg=ACCENT, fg=FG, relief="flat", padx=25, pady=6,
                                   cursor="hand2", command=self._next)
        self.btn_next.pack(side="right", padx=20, pady=10)

    def _change_font(self, delta):
        self.font_size = max(9, min(20, self.font_size + delta))
        self.font_label.config(text=str(self.font_size))
        self._save_config()
        self._show_step(self.current_step)

    def _toggle_lang(self):
        global _current_lang
        _current_lang = "EN" if _current_lang == "DE" else "DE"
        self.lang_btn.config(text=_current_lang)
        self.btn_prev.config(text=T("btn_prev"))
        self.btn_next.config(text=T("btn_next"))
        self._save_config()
        self._show_step(self.current_step)

    def _update_dots(self):
        for i, dot in enumerate(self.dots):
            dot.delete("all")
            if i == self.current_step:
                dot.create_oval(2, 2, 14, 14, fill=ACCENT, outline="")
            elif self.step_done[i]:
                dot.create_oval(2, 2, 14, 14, fill=GREEN, outline="")
            else:
                dot.create_oval(3, 3, 13, 13, fill="", outline=FG_DIM, width=1)
        self.step_label.config(text=T("step_of").format(self.current_step + 1, TOTAL_STEPS))
        self.btn_prev.config(state="normal" if self.current_step > 0 else "disabled")

    def _dot_click(self, idx):
        if idx <= self.current_step or self.step_done[idx]:
            self._show_step(idx)

    def _prev(self):
        if self.current_step > 0:
            self._show_step(self.current_step - 1)

    def _next(self):
        if self.current_step < TOTAL_STEPS - 1:
            self.step_done[self.current_step] = True
            self._show_step(self.current_step + 1)
            self._save_config()

    # ---- CONTENT ----
    def _show_step(self, idx):
        self.current_step = idx
        self._update_dots()

        for w in self.content_outer.winfo_children():
            w.destroy()

        canvas = tk.Canvas(self.content_outer, bg=BG, highlightthickness=0)
        vsb = tk.Scrollbar(self.content_outer, orient="vertical", command=canvas.yview)
        frame = tk.Frame(canvas, bg=BG)
        frame.bind("<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.create_window((0, 0), window=frame, anchor="nw", tags="inner")

        def _resize(e):
            canvas.itemconfig("inner", width=e.width)
        canvas.bind("<Configure>", _resize)
        canvas.configure(yscrollcommand=vsb.set)
        vsb.pack(side="right", fill="y")
        canvas.pack(side="left", fill="both", expand=True)

        def _scroll(e):
            canvas.yview_scroll(int(-1 * (e.delta / 120)), "units")
        canvas.bind_all("<MouseWheel>", _scroll)

        s = idx + 1

        # Title
        self._label(frame, T(f"s{s}_title"), size=8, bold=True, pady=(15, 12))
        # Description
        self._label(frame, T(f"s{s}_what"), fg=FG_DIM, pady=(0, 10))
        # Step content
        getattr(self, f"_step{s}", lambda f: None)(frame)
        # Bottom spacer
        tk.Frame(frame, bg=BG, height=30).pack()

    # ---- UI HELPERS ----
    def _label(self, parent, txt, fg=FG, bold=False, mono=False, size=0, pady=(4, 4)):
        fs = self.font_size + size
        fam = "Consolas" if mono else "Segoe UI"
        wt = "bold" if bold else "normal"
        tk.Label(parent, text=txt, font=(fam, fs, wt), bg=BG, fg=fg,
                 justify="left", anchor="w", wraplength=700).pack(anchor="w", pady=pady)

    def _card(self, parent, txt, fg=FG, bg_c=CARD_BG):
        fs = self.font_size
        cf = tk.Frame(parent, bg=bg_c, padx=16, pady=12)
        cf.pack(fill="x", pady=(6, 6))
        tk.Label(cf, text=txt, font=("Consolas", fs), bg=bg_c, fg=fg,
                 justify="left", anchor="w", wraplength=660).pack(anchor="w")

    def _note(self, parent, txt, color=YELLOW, bg_c="#2a2a1a"):
        fs = self.font_size
        nf = tk.Frame(parent, bg=bg_c, padx=14, pady=10)
        nf.pack(fill="x", pady=(8, 4))
        tk.Label(nf, text="⚠  " + txt, font=("Segoe UI", fs - 1), bg=bg_c, fg=color,
                 justify="left", anchor="w", wraplength=650).pack(anchor="w")

    def _warn(self, parent, txt):
        self._note(parent, txt, color=RED, bg_c="#2e1a1a")

    def _link(self, parent, text, url):
        fs = self.font_size
        tk.Button(parent, text=f"🔗  {text}", font=("Segoe UI", fs - 1, "underline"),
                 bg=BG, fg=LINK_COLOR, relief="flat", padx=0, pady=4,
                 cursor="hand2", anchor="w", activeforeground=FG, activebackground=BG,
                 command=lambda: webbrowser.open(url)).pack(anchor="w", pady=(4, 2))

    def _action(self, parent, text, command, color=GREEN, enabled=True):
        fs = self.font_size
        tk.Button(parent, text=text, font=("Segoe UI", fs, "bold"),
                 bg=color if enabled else FG_DIM, fg=BG, relief="flat",
                 padx=20, pady=8, cursor="hand2" if enabled else "",
                 state="normal" if enabled else "disabled",
                 command=command).pack(anchor="w", pady=(10, 4))

    def _verify(self, parent, check_func):
        fs = self.font_size
        result_var = tk.StringVar()
        rl = tk.Label(parent, textvariable=result_var, font=("Segoe UI", fs - 1),
                      bg=BG, fg=FG, justify="left", anchor="w", wraplength=700)

        def _do():
            ok, msg = check_func()
            result_var.set(msg)
            rl.config(fg=GREEN if ok else RED)
            if ok:
                self.step_done[self.current_step] = True
                self._update_dots()
                self._save_config()

        tk.Button(parent, text=T("btn_verify"), font=("Segoe UI", fs - 1, "bold"),
                 bg=BG3, fg=FG, relief="flat", padx=20, pady=6,
                 cursor="hand2", command=_do).pack(anchor="w", pady=(14, 4))
        rl.pack(anchor="w", pady=(2, 8))

    def _path_row(self, parent, var, label_text):
        fs = self.font_size
        tk.Label(parent, text=label_text, font=("Segoe UI", fs - 1, "bold"),
                 bg=BG, fg=FG).pack(anchor="w", pady=(12, 4))
        row = tk.Frame(parent, bg=BG)
        row.pack(fill="x")
        tk.Entry(row, textvariable=var, font=("Consolas", fs - 1),
                 bg=CARD_BG, fg=FG, insertbackground=FG, relief="flat"
                 ).pack(side="left", fill="x", expand=True, ipady=6, padx=(0, 8))
        tk.Button(row, text=T("btn_browse"), font=("Segoe UI", fs - 2),
                 bg=BG3, fg=FG, relief="flat", padx=12, pady=4, cursor="hand2",
                 command=lambda: self._browse(var)).pack(side="left")
        if var.get():
            tk.Button(row, text=T("btn_open"), font=("Segoe UI", fs - 2),
                     bg=BG3, fg=FG, relief="flat", padx=12, pady=4, cursor="hand2",
                     command=lambda: self._open_dir(var.get())).pack(side="left", padx=(4, 0))

    def _browse(self, var):
        p = filedialog.askdirectory(initialdir=var.get() or "C:\\")
        if p:
            var.set(p)
            self._save_config()
            self._show_step(self.current_step)

    def _open_dir(self, path):
        if path and os.path.isdir(path):
            if sys.platform == "win32":
                os.startfile(path)
            else:
                subprocess.Popen(["xdg-open", path])

    # ============================================================
    # STEPS
    # ============================================================

    def _step1(self, f):
        self._label(f, T("s1_do"))
        self._card(f, T("s1_example"), fg=GREEN)
        self._note(f, T("s1_note"))
        self._link(f, "SDK Download (ModDB)", URL_SDK)
        self._link(f, "Modding Guide (GitHub)", URL_GITHUB)
        self._path_row(f, self.sdk_path, T("s1_select"))
        self._verify(f, self._chk_sdk)

    def _step2(self, f):
        self._label(f, T("s2_do"))
        self._card(f, T("s2_example"), fg=GREEN)
        self._note(f, T("s2_note"))
        self._path_row(f, self.game_path, T("s2_select"))
        self._verify(f, self._chk_game)

    def _step3(self, f):
        self._card(f, T("s3_path"), fg=YELLOW)
        self._label(f, T("s3_do"))
        self._path_row(f, self.saves_path, T("s3_select"))
        self._verify(f, self._chk_saves)

    def _step4(self, f):
        for i in range(1, 3):
            self._card(f, f"{T(f's4_tool{i}')}\n{T(f's4_tool{i}_desc')}")
        self._label(f, T("s4_do"))
        self._link(f, "Modding Guide & Tools (GitHub)", URL_GITHUB)
        if self.game_path.get():
            self._action(f, T("btn_open") + "  Game\\Mods\\",
                        lambda: self._open_dir(os.path.join(self.game_path.get(), "Mods")), color=BG3)
        self._verify(f, self._chk_tools)

    def _step5(self, f):
        self._card(f, T("s5_controls"), fg=FG_DIM)
        self._label(f, T("s5_do"))
        self._note(f, T("s5_note"))
        sdk = self.sdk_path.get()
        can = sdk and os.path.exists(os.path.join(sdk, "TwoWorldsEditor.exe"))
        self._action(f, T("btn_open_editor"), self._do_open_editor, enabled=can)
        self._verify(f, self._chk_editor)

    def _step6(self, f):
        self._card(f, T("s6_example"), fg=YELLOW)
        self._card(f, T("s6_do"))
        self._note(f, T("s6_note"))
        if self.saves_path.get():
            self._action(f, T("btn_open") + "  Saves\\Levels\\",
                        lambda: self._open_dir(os.path.join(self.saves_path.get(), "Levels")), color=BG3)
        self._verify(f, self._chk_maps)

    def _step7(self, f):
        self._card(f, T("s7_cmds"), fg=GREEN)
        self._label(f, T("s7_do"))
        self._warn(f, T("s7_warn"))
        has = HAS_PYAUTOGUI and HAS_PYGETWINDOW
        self._action(f, T("btn_send_physx"), self._do_send_physx, enabled=has)
        if not has:
            self._label(f, "pip install pyautogui pygetwindow", fg=FG_DIM, mono=True)
        self._verify(f, self._chk_physx)

    def _step8(self, f):
        self._card(f, T("s8_do"))
        self._note(f, T("s8_note"))
        if self.sdk_path.get():
            tools = os.path.join(self.sdk_path.get(), "Tools")
            if not os.path.isdir(tools):
                tools = self.sdk_path.get()
            self._action(f, T("btn_open") + "  SDK\\Tools\\",
                        lambda: self._open_dir(tools), color=BG3)
        self._verify(f, self._chk_lhc)

    def _step9(self, f):
        self._card(f, T("s9_structure"), fg=YELLOW)
        self._card(f, T("s9_do"))
        self._warn(f, T("s9_warn"))
        row = tk.Frame(f, bg=BG)
        row.pack(fill="x", pady=(8, 0))
        fs = self.font_size
        if self.saves_path.get():
            tk.Button(row, text=T("btn_open") + "  Saves", font=("Segoe UI", fs - 1),
                     bg=BG3, fg=FG, relief="flat", padx=12, pady=5, cursor="hand2",
                     command=lambda: self._open_dir(
                         os.path.join(self.saves_path.get(), "Levels"))).pack(side="left", padx=(0, 8))
        if self.game_path.get():
            tk.Button(row, text=T("btn_open") + "  Mods", font=("Segoe UI", fs - 1),
                     bg=BG3, fg=FG, relief="flat", padx=12, pady=5, cursor="hand2",
                     command=lambda: self._open_dir(
                         os.path.join(self.game_path.get(), "Mods"))).pack(side="left")
        self._verify(f, self._chk_mod_folder)

    def _step10(self, f):
        self._card(f, T("s10_do"))
        self._card(f, T("s10_source_explain"), fg=YELLOW)
        self._note(f, T("s10_note"))
        if self.game_path.get():
            self._action(f, T("btn_open") + "  Mods",
                        lambda: self._open_dir(os.path.join(self.game_path.get(), "Mods")), color=BG3)
        self._verify(f, self._chk_wd)

    def _step11(self, f):
        self._card(f, T("s11_do"))
        self._note(f, T("s11_note"))
        self._verify(f, self._chk_registry)

    def _step12(self, f):
        self._card(f, T("s12_do"), fg=GREEN)
        tk.Frame(f, bg=BG3, height=2).pack(fill="x", pady=(18, 12))
        self._label(f, T("s12_summary"), fg=FG_DIM)
        tk.Frame(f, bg=BG3, height=2).pack(fill="x", pady=(12, 12))
        self._label(f, T("s12_next"), fg=FG_DIM)
        self._link(f, "Modding Guide (GitHub)", URL_GITHUB)

    # ============================================================
    # CHECKS
    # ============================================================

    def _chk_sdk(self):
        s = self.sdk_path.get()
        if s and os.path.exists(os.path.join(s, "TwoWorldsEditor.exe")):
            return True, T("s1_ok")
        return False, T("s1_fail")

    def _chk_game(self):
        g = self.game_path.get()
        if not g or not os.path.exists(os.path.join(g, "TwoWorlds.exe")):
            return False, T("s2_fail_exe")
        if not os.path.isdir(os.path.join(g, "Mods")):
            return False, T("s2_fail_mods")
        return True, T("s2_ok")

    def _chk_saves(self):
        s = self.saves_path.get()
        return (True, T("s3_ok")) if s and os.path.isdir(s) else (False, T("s3_fail"))

    def _chk_tools(self):
        g = self.game_path.get()
        if not g:
            return False, T("not_found")
        ms = any(os.path.exists(os.path.join(g, p)) for p in [
            "TwoWorlds1 Mod Selector.exe", "TwoWorlds1_Mod_Selector.exe",
            "Mod Selector.exe", "ModSelector.exe"])
        if ms:
            return True, T("s4_ok")
        return False, T("s4_fail_sel")

    def _chk_editor(self):
        if not HAS_PYGETWINDOW:
            return True, T("s5_ok")
        ew = find_editor_window()
        return (True, T("s5_ok") + f" ({ew.title})") if ew else (False, T("s5_fail"))

    def _chk_maps(self):
        s = self.saves_path.get()
        ld = os.path.join(s, "Levels") if s else ""
        if ld and os.path.isdir(ld):
            maps = glob.glob(os.path.join(ld, "Map_*s.lnd"))
            if maps:
                names = [os.path.splitext(os.path.basename(m))[0] for m in maps]
                return True, T("s6_ok").format(", ".join(names[:5]))
        return False, T("s6_fail")

    def _chk_physx(self):
        s = self.saves_path.get()
        if s:
            for sub in ["Physic", "physic"]:
                pd = os.path.join(s, "Levels", sub)
                if os.path.isdir(pd):
                    phx = glob.glob(os.path.join(pd, "*.phx"))
                    if phx:
                        return True, T("s7_ok").format(", ".join(os.path.basename(f) for f in phx[:5]))
        return False, T("s7_fail")

    def _chk_lhc(self):
        for base in [self.sdk_path.get(), self.game_path.get()]:
            if base:
                for sub in ["Levels", "Tools", ""]:
                    p = os.path.join(base, sub, "Map_LevelHeaders.lhc") if sub else os.path.join(base, "Map_LevelHeaders.lhc")
                    if os.path.exists(p):
                        return True, T("s8_ok")
        return False, T("s8_fail")

    def _chk_mod_folder(self):
        g = self.game_path.get()
        mods = os.path.join(g, "Mods") if g else ""
        if mods and os.path.isdir(mods):
            dirs = [d for d in os.listdir(mods)
                   if os.path.isdir(os.path.join(mods, d)) and d != "_converter_temp"
                   and os.path.isdir(os.path.join(mods, d, "Levels"))]
            if dirs:
                return True, T("s9_ok").format(", ".join(dirs[:3]))
        return False, T("s9_fail")

    def _chk_wd(self):
        g = self.game_path.get()
        mods = os.path.join(g, "Mods") if g else ""
        if mods and os.path.isdir(mods):
            wds = [f for f in os.listdir(mods) if f.endswith(".wd") and f != "Map Test EX.wd"]
            if wds:
                return True, T("s10_ok").format(", ".join(wds[:5]))
        return False, T("s10_fail")

    def _chk_registry(self):
        if not HAS_WINREG:
            return True, T("s11_ok").format("(n/a)")
        try:
            k = winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"SOFTWARE\Reality Pump\TwoWorlds\Mods")
            active = []
            i = 0
            while True:
                try:
                    name, val, _ = winreg.EnumValue(k, i)
                    if val == 1:
                        active.append(name)
                    i += 1
                except OSError:
                    break
            winreg.CloseKey(k)
            if active:
                return True, T("s11_ok").format(", ".join(active))
        except Exception:
            pass
        return False, T("s11_fail")

    # ============================================================
    # ACTIONS
    # ============================================================

    def _do_open_editor(self):
        sdk = self.sdk_path.get()
        exe = os.path.join(sdk, "TwoWorldsEditor.exe")
        if os.path.exists(exe):
            subprocess.Popen([exe], cwd=sdk)

    def _do_send_physx(self):
        if not HAS_PYAUTOGUI or not HAS_PYGETWINDOW:
            return
        def _go():
            ew = find_editor_window()
            if not ew:
                return
            try:
                ew.activate()
                time.sleep(0.3)
                pyautogui.press("c")
                time.sleep(0.5)
                try:
                    for w in gw.getAllWindows():
                        if w.title and w.visible and "console" in w.title.lower():
                            w.activate()
                            time.sleep(0.2)
                            break
                except Exception:
                    pass
                for cmd in [
                    "editor.cookphysx.mode geomipmap",
                    "editor.cookphysx.strength = 1.0",
                    "editor.cookphysx.overwrite = 1",
                    "editor.cookphysx.pc out",
                ]:
                    clipboard_paste(self.root, cmd)
                    time.sleep(0.05)
                    pyautogui.press("enter")
                    time.sleep(0.1)
            except Exception:
                pass
        threading.Thread(target=_go, daemon=True).start()

    def run(self):
        self.root.mainloop()


if __name__ == "__main__":
    app = App()
    app.run()
