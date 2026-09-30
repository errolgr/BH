## Evidence: Werewolf/Werebear IAS breakpoints in the advanced stats panel

Before this change BH's advanced stats panel prints `IAS (Frames): N/A (Work in Progress)` for a shapeshifted Druid (StatsDisplay.cpp, monstats 430/431 early return). This change computes the wereform line (WereSpeed = floor(256*NU/Delay), Delay from the human-form weapon animation).

Verified in the **live PD2 client** (Season 13 CDN mirror; ProjectDiablo.dll linked Wed May 13 11:40:49 2026) by an automated in-game scenario (bh-harness `evidence/scenarios/F1-wereform-ias.sh`). The run enters single player as a Druid, equips each weapon, sets IAS, morphs, opens BH's panel through `BHInteract(BH_CONFIG_ADVANCEDSTATS)` and reads the text BH draws. Each line is compared with an independent oracle (bh-harness `logic/oracle`, validated against the Amazon Basin and PD2 wiki tables).

| | BH.dll | result |
|---|---|---|
| before | shipped PD2 BH.dll `cba04ff9e6f94494` | **XFAIL**: 12 human-form lines = oracle, 24 wereform lines = `N/A (Work in Progress)` |
| after | this branch (`harness/evidence-f1` @ `e188b7a44e`, clang-cl build `179fc94fcb5f79ac`) | **XPASS**: all 36 lines = oracle (24 wereform + 12 human-form control), 0 mismatches |

### Wereform lines: BH before, BH after, oracle

Werewolf skill level 1; items: normal quality, no IAS mods (weapon IAS 0). 'IAS' is item IAS (stat 93); `(n)` marks the current frame count.

| case | before | after | oracle |
|---|---|---|---|
| werewolf unarmed (HTH WSM 0) IAS 0 (Werewolf lvl 1) | `N/A (Work in Progress)` | `0 (22) / 7 / 14 / 23 / 34 / 46 / 65 / 92` | `0 (22) / 7 / 14 / 23 / 34 / 46 / 65 / 92` |
| werewolf unarmed (HTH WSM 0) IAS 20 (Werewolf lvl 1) | `N/A (Work in Progress)` | `0 / 7 / 14 (20) / 23 / 34 / 46 / 65 / 92` | `0 / 7 / 14 (20) / 23 / 34 / 46 / 65 / 92` |
| werewolf unarmed (HTH WSM 0) IAS 40 (Werewolf lvl 1) | `N/A (Work in Progress)` | `0 / 7 / 14 / 23 / 34 (18) / 46 / 65 / 92` | `0 / 7 / 14 / 23 / 34 (18) / 46 / 65 / 92` |
| werebear unarmed (HTH WSM 0) IAS 0 | `N/A (Work in Progress)` | `0 (22) / 6 / 11 / 18 / 26 / 37 / 52 / 70 / 95 / 142` | `0 (22) / 6 / 11 / 18 / 26 / 37 / 52 / 70 / 95 / 142` |
| werebear unarmed (HTH WSM 0) IAS 20 | `N/A (Work in Progress)` | `0 / 6 / 11 / 18 (19) / 26 / 37 / 52 / 70 / 95 / 142` | `0 / 6 / 11 / 18 (19) / 26 / 37 / 52 / 70 / 95 / 142` |
| werebear unarmed (HTH WSM 0) IAS 40 | `N/A (Work in Progress)` | `0 / 6 / 11 / 18 / 26 / 37 (17) / 52 / 70 / 95 / 142` | `0 / 6 / 11 / 18 / 26 / 37 (17) / 52 / 70 / 95 / 142` |
| werewolf hax (1HS WSM 0) IAS 0 (Werewolf lvl 1) | `N/A (Work in Progress)` | `0 (22) / 7 / 14 / 23 / 34 / 46 / 65 / 92` | `0 (22) / 7 / 14 / 23 / 34 / 46 / 65 / 92` |
| werewolf hax (1HS WSM 0) IAS 20 (Werewolf lvl 1) | `N/A (Work in Progress)` | `0 / 7 / 14 (20) / 23 / 34 / 46 / 65 / 92` | `0 / 7 / 14 (20) / 23 / 34 / 46 / 65 / 92` |
| werewolf hax (1HS WSM 0) IAS 40 (Werewolf lvl 1) | `N/A (Work in Progress)` | `0 / 7 / 14 / 23 / 34 (18) / 46 / 65 / 92` | `0 / 7 / 14 / 23 / 34 (18) / 46 / 65 / 92` |
| werebear hax (1HS WSM 0) IAS 0 | `N/A (Work in Progress)` | `0 (22) / 6 / 11 / 18 / 26 / 37 / 52 / 70 / 95 / 142` | `0 (22) / 6 / 11 / 18 / 26 / 37 / 52 / 70 / 95 / 142` |
| werebear hax (1HS WSM 0) IAS 20 | `N/A (Work in Progress)` | `0 / 6 / 11 / 18 (19) / 26 / 37 / 52 / 70 / 95 / 142` | `0 / 6 / 11 / 18 (19) / 26 / 37 / 52 / 70 / 95 / 142` |
| werebear hax (1HS WSM 0) IAS 40 | `N/A (Work in Progress)` | `0 / 6 / 11 / 18 / 26 / 37 (17) / 52 / 70 / 95 / 142` | `0 / 6 / 11 / 18 / 26 / 37 (17) / 52 / 70 / 95 / 142` |
| werewolf spr (2HT WSM -10) IAS 0 (Werewolf lvl 1) | `N/A (Work in Progress)` | `0 (22) / 4 / 10 / 19 / 30 / 42 / 63` | `0 (22) / 4 / 10 / 19 / 30 / 42 / 63` |
| werewolf spr (2HT WSM -10) IAS 20 (Werewolf lvl 1) | `N/A (Work in Progress)` | `0 / 4 / 10 / 19 (19) / 30 / 42 / 63` | `0 / 4 / 10 / 19 (19) / 30 / 42 / 63` |
| werewolf spr (2HT WSM -10) IAS 40 (Werewolf lvl 1) | `N/A (Work in Progress)` | `0 / 4 / 10 / 19 / 30 (18) / 42 / 63` | `0 / 4 / 10 / 19 / 30 (18) / 42 / 63` |
| werebear spr (2HT WSM -10) IAS 0 | `N/A (Work in Progress)` | `0 (21) / 6 / 13 / 20 / 30 / 44 / 60 / 89 / 129` | `0 (21) / 6 / 13 / 20 / 30 / 44 / 60 / 89 / 129` |
| werebear spr (2HT WSM -10) IAS 20 | `N/A (Work in Progress)` | `0 / 6 / 13 / 20 (18) / 30 / 44 / 60 / 89 / 129` | `0 / 6 / 13 / 20 (18) / 30 / 44 / 60 / 89 / 129` |
| werebear spr (2HT WSM -10) IAS 40 | `N/A (Work in Progress)` | `0 / 6 / 13 / 20 / 30 (17) / 44 / 60 / 89 / 129` | `0 / 6 / 13 / 20 / 30 (17) / 44 / 60 / 89 / 129` |
| werewolf sst (STF WSM -10) IAS 0 (Werewolf lvl 1) | `N/A (Work in Progress)` | `0 (16) / 7 / 19 / 34 / 56` | `0 (16) / 7 / 19 / 34 / 56` |
| werewolf sst (STF WSM -10) IAS 20 (Werewolf lvl 1) | `N/A (Work in Progress)` | `0 / 7 / 19 (14) / 34 / 56` | `0 / 7 / 19 (14) / 34 / 56` |
| werewolf sst (STF WSM -10) IAS 40 (Werewolf lvl 1) | `N/A (Work in Progress)` | `0 / 7 / 19 / 34 (13) / 56` | `0 / 7 / 19 / 34 (13) / 56` |
| werebear sst (STF WSM -10) IAS 0 | `N/A (Work in Progress)` | `0 (16) / 4 / 13 / 24 / 40 / 63 / 102` | `0 (16) / 4 / 13 / 24 / 40 / 63 / 102` |
| werebear sst (STF WSM -10) IAS 20 | `N/A (Work in Progress)` | `0 / 4 / 13 (14) / 24 / 40 / 63 / 102` | `0 / 4 / 13 (14) / 24 / 40 / 63 / 102` |
| werebear sst (STF WSM -10) IAS 40 | `N/A (Work in Progress)` | `0 / 4 / 13 / 24 / 40 (12) / 63 / 102` | `0 / 4 / 13 / 24 / 40 (12) / 63 / 102` |

### Human-form control (unchanged, equal to the oracle before and after)

| case | BH (before = after) |
|---|---|
| none unarmed (HTH WSM 0) IAS 0 | `0 (15) / 9 / 18 / 30 / 48 / 75 / 125` |
| none unarmed (HTH WSM 0) IAS 20 | `0 / 9 / 18 (13) / 30 / 48 / 75 / 125` |
| none unarmed (HTH WSM 0) IAS 40 | `0 / 9 / 18 / 30 (12) / 48 / 75 / 125` |
| none hax (1HS WSM 0) IAS 0 | `0 (18) / 7 / 15 / 23 / 35 / 52 / 78 / 117 / 194` |
| none hax (1HS WSM 0) IAS 20 | `0 / 7 / 15 (16) / 23 / 35 / 52 / 78 / 117 / 194` |
| none hax (1HS WSM 0) IAS 40 | `0 / 7 / 15 / 23 / 35 (14) / 52 / 78 / 117 / 194` |
| none spr (2HT WSM -10) IAS 0 | `0 (20) / 7 / 14 / 23 / 34 / 48 / 70 / 102` |
| none spr (2HT WSM -10) IAS 20 | `0 / 7 / 14 (18) / 23 / 34 / 48 / 70 / 102` |
| none spr (2HT WSM -10) IAS 40 | `0 / 7 / 14 / 23 / 34 (16) / 48 / 70 / 102` |
| none sst (STF WSM -10) IAS 0 | `0 (15) / 5 / 14 / 26 / 44 / 72 / 125` |
| none sst (STF WSM -10) IAS 20 | `0 / 5 / 14 (13) / 26 / 44 / 72 / 125` |
| none sst (STF WSM -10) IAS 40 | `0 / 5 / 14 / 26 (12) / 44 / 72 / 125` |

### Screenshots (live PD2 client, IAS 40)

| | before (shipped BH) | after (this branch) |
|---|---|---|
| werebear-sst | ![before](before-werebear-sst-ias40.jpg) | ![after](after-werebear-sst-ias40.jpg) |
| werewolf-hax | ![before](before-werewolf-hax-ias40.jpg) | ![after](after-werewolf-hax-ias40.jpg) |
| werebear-spr | ![before](before-werebear-spr-ias40.jpg) | ![after](after-werebear-spr-ias40.jpg) |
| werewolf-unarmed | ![before](before-werewolf-unarmed-ias40.jpg) | ![after](after-werewolf-unarmed-ias40.jpg) |
| none-sst | ![before](before-none-sst-ias40.jpg) | ![after](after-none-sst-ias40.jpg) |

All 24 pairs: `evidence/pr-f1/{before,after}-<form>-<weapon>-ias40.jpg`.

### Unit tests (L1, BH logic compiled natively against the same oracle)

- base `harness/clang-build` 60cd0cf: `logic: 46 passed, 8 xfail, 0 failed`; `BH IAS breakpoints == oracle: Druid Werewolf/Werebear attack` are XFAIL (BH-IAS-WEREFORM).
- this branch e188b7a: `logic: 46 passed, 6 xfail, 2 failed`. The 2 'failed' are those two tests reporting **XPASS** (the pinned bug no longer reproduces). No other test changed. The PR removes `doctest::should_fail()` from them.

### Runtime validity

- `build/parity.sh` on this branch's DLL (clean build, sha256 `a9b0ac66959fde69…`): **PASS**, live PD2 10/10 and vanilla 10/10 launches, 0 access violations at the menu, shift-click survived. The after run used a warm relink of the same objects (`179fc94f…`; build-bh.sh relinks on every call, so the PE timestamp changes the hash). An earlier attempt of the gate, run while the machine was at load average 20-60, had 4 of 20 launches time out before the menu ("no-menu"; the gate does not inspect those launches for faults). The reference build/BH.dll passed 4/4 under the same load.
- Full in-game suite on this DLL in the live PD2 client (`./check.sh --full --pd2 --l3-bh built`): H1-boot, H2-nav, H3-pd2-boot, H4-sp-enter and A1-aether-smoke PASS; F1 XPASS.

### Provenance

- before run: `runs/F1-wereform-ias/2026-09-30T07-19-11` (bh-harness 5ae1c3141a), BH sha256 `cba04ff9e6f944942d69e4b610eeb0a8434bc780d312970ea922877fd134ac74` (shipped PD2 BH (pd2-cdn-live, MSVC))
- after run: `runs/F1-wereform-ias/2026-09-30T07-56-28` (bh-harness a90478647f), BH sha256 `179fc94fcb5f79ac542ed034b2874ff5339b0b297abadfb414f7e26a4cbe9775` (build-bh.sh (clang-cl + lld-link), harness/evidence-f1 @ e188b7a44ea940b3f255d21a463f7d27a704d1d6)
- PD2 client: pd2-cdn-live, ProjectDiablo.dll sha256 `538a77b7ccef3d5334e56c4e9e57a4d8fc69a1e27c46beb694c0dedfcfbf9cb3`
