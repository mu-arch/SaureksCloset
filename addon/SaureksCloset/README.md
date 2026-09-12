<div align="center">

# Saurek's Closet

**A new look for your adventures in Azeroth.**

Customize your armor, change your character's appearance, and build a collection of saved looks—all from a wardrobe designed to feel at home in Vanilla WoW.

**World of Warcraft 1.12.1 · Build 5875 · Windows · Version 3.4.15**

Made by **Saurek**

</div>

## Your character, your look

Saurek's Closet changes how your character looks on your own client. Your actual equipment, stats, and gameplay stay the same; other players do not see these appearance changes.

- **Dress your character:** browse the item database, filter by quality and type, and preview an item before activating it.
- **Choose what shows:** customize an armor slot, hide it, or pass through your real equipment.
- **Customize your body:** change race, gender, skin, face, hair, and available features. Cycle through options with arrows or choose directly from a dropdown.
- **Arrange your weaponry:** select appearances for both sides of the waist, both sides of the back, a shield, a ranged weapon, and a quiver.
- **Save your favorites:** name new looks, update existing ones, and switch between them from the quick-selection dropdown.

## Take a look inside

### Wardrobe & item browser

![Wardrobe and searchable item appearance browser](Screenshots/1.png)

The wardrobe shows your character in a live 3D preview, with armor slots arranged along either side. Click a slot to choose a custom item, hide it, or pass through your real armor. The item browser opens alongside the wardrobe: search the database, narrow the list by quality and type, and click an item to preview it. **Activate Item** applies your selection. The crossed-out eye marks a hidden slot, and **Toggle** switches your local appearance overrides on or off.

### Body customization

![Body and appearance controls beside the character preview](Screenshots/2.png)

The **Body** view keeps your character visible while you change race and gender, then fine-tune skin, face, hairstyle, hair color, and available facial features. Use the left and right arrows to cycle through options, or click a value to choose from its dropdown. Changes apply immediately. **Reset body** restores your character's original body appearance.

### Weaponry

![Waist, back, shield, ranged weapon, and quiver appearance slots](Screenshots/3.png)

The **Weaponry** view groups placements by position: left and right waist, left and right back, shield, ranged weapon, and quiver. Each slot has its own appearance selection, so several weapons can be displayed together. Matching weapon appearances move into your hands when drawn according to the type actually equipped; other placements stay stored. See the weapon behavior notes below for the current limitations.

### Saved Looks

![Saved looks with portraits, customized-slot counts, and an active-look indicator](Screenshots/4.png)

**Saved Looks** brings your collection together in a scrollable list. Each tile shows a race-and-gender portrait and the number of customized slots; the green eye identifies the active look. Edits appear in the single **(Unsaved)** entry until you save them. Click a tile to open its outfit preview, where you can activate, rename, or delete a saved look. Open **(Unsaved)** to save a new look or save your changes to an existing outfit.

*Screenshots show a recent build; some control positions have since been refined.*

## Installation

### What you need

- The supported **Windows 1.12.1 client, build 5875**. This is not an addon for modern WoW Classic or Retail.
- **VanillaFixes** configured to launch your game.
- **VanillaHelpers.dll** installed and enabled in `dlls.txt`.
- Both parts of Saurek's Closet: the **`SaureksCloset` addon folder** and **`SaureksCloset.dll`** from the release download.

### Install the release

1. Fully close World of Warcraft and extract your download. In a GitHub source download, the addon folder is inside `addon/`.
2. Copy the `SaureksCloset` folder into `Interface/AddOns/`.
3. Open the addon's **[Installation instructions](Installation%20instructions/)** folder. Copy the included `SaureksCloset.dll` into your main game folder, beside `WoW.exe`. The short [setup guide](Installation%20instructions/READ%20ME.txt) explains where to drag it.
4. Add `SaureksCloset.dll` to `dlls.txt`, after `VanillaHelpers.dll`. Keep any other DLL entries you already use.
5. Launch through VanillaFixes, enable **Saurek's Closet** in the AddOns list, and log in.

Your installation should contain:

```text
World of Warcraft/
├── WoW.exe
├── VanillaHelpers.dll
├── SaureksCloset.dll
├── dlls.txt
└── Interface/
    └── AddOns/
        └── SaureksCloset/
            ├── SaureksCloset.toc
            ├── Textures/
            └── ...
```

**Don't forget the DLL.** Copying only the addon folder is not a complete installation. A UI reload cannot load a new DLL; fully restart the game after replacing one.

The full release also includes an optional Python 3 installer, which verifies the supported client and backs up the files it replaces:

```powershell
py -3 install.py "C:\Games\World of Warcraft"
```

For upgrades from the old VanityStudio or numbered addon folders, use this installer to migrate settings. Back up your `WTF` folder before a manual migration.

## Make your first look

1. Click the minimap button or type **`/closet`**.
2. In **Wardrobe → Outfit**, click an armor slot and choose **Custom Item**, **Hide Slot**, or **Passthrough**.
3. Search or filter the item list. Click an item to preview it, then choose **Activate Item** to apply it.
4. Use the bottom dropdown to visit **Body** or **Weaponry** and continue customizing.
5. Open **Saved Looks** and select **(Unsaved)**. Enter a name and click **Save new**, or use **Save to existing outfit** to update a favorite.

Open any saved look to preview, rename, delete, or activate it. The green eye marks the active look. The dropdown at the top of the window lets you switch saved looks quickly, and **Toggle** turns your local appearance overrides on or off.

### How weapon placements work

Several custom weapons can be displayed at once. When you draw weapons, matching appearances move into your hands according to the type of weapon actually equipped. A cosmetic gun stays stored if you don't have a gun equipped; unmatched placements and quivers also stay stored.

Weaponry is still being refined. Some combinations may clip, and attachment positions cannot yet be adjusted manually. The **Bags** view is a placeholder; bag appearance customization is not available yet.

## Useful commands

| Command | Action |
| --- | --- |
| `/closet` | Open or close the wardrobe |
| `/closet body` | Open Body customization |
| `/closet outfits` | Open Saved Looks |
| `/closet next` / `/closet prev` | Cycle through saved looks |
| `/closet on` / `/closet off` | Enable or disable local appearances |
| `/closet diagnose` | Show diagnostic information for troubleshooting |

## Feedback & support

Hello and welcome to my Saurek's Closet addon! If you have any feedback or problems, please [open an issue](https://github.com/mu-arch/SaureksCloset/issues). No question is too stupid. Reach out to me and I'll get back to you when I have a chance.

When reporting a problem, include your **About-page version**, what you were doing, and a screenshot or the exact error message. If the interface updates but customization does not work, check that you also copied the release's DLL and fully restarted through VanillaFixes.

— **Saurek**

## Credits & source

Created by **Saurek**. Built for the original Vanilla interface, with support from VanillaFixes and VanillaHelpers.

The addon lives in `addon/SaureksCloset/`. DLL source is in `native/`, with build tools in `tools/` and tests in `tests/`. See [the build guide](../../BUILDING.md) and the included license notices. Catalog attribution is documented in [CATALOG-COPYRIGHT.md](CATALOG-COPYRIGHT.md) and [CATALOG-LICENSE.md](CATALOG-LICENSE.md); artwork provenance is recorded in [ARTWORK.json](ARTWORK.json).
