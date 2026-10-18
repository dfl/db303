# Factory presets

Every `*.json` file in this folder is built into the plugin as a factory preset, in file name order
(`00-init.json` stays first). Add or remove a file and rebuild; CMake picks up the change on its own.

```json
{
    "name": "Hoover",
    "description": "What the sound is and how it is made.",
    "knobs": ["superDetune", "lfoDepth"],
    "values": { "switchModState": true, "oscModel": "Alpha Juno", "lfoRate": 0.4 },
    "notes": { "lfoRate": "0.8 Hz" }
}
```

- `values`: parameter ID -> value. Choice parameters take the name shown in the plugin (`"Phaseshaper"`,
  `"Env+"`; an index also works), switches take `true` / `false`, and floats take the raw parameter value
  (0..1 for most knobs). Every sound parameter not listed goes back to its default.
- `knobs`: the two performance knobs (the mod wheel drives the first). Default: `cutoff`, `lfoDepth`.
- `description` and `notes` are for people reading the file; the plugin ignores them.
- The sequencer, volume and tuning are never changed by a preset.

`preset_test` checks that every ID and knob exists, every choice name is real and every value is in range, and that each preset
renders cleanly and near the level of the rest of the bank.
