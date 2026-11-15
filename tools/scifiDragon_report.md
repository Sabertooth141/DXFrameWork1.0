# Sci-Fi Dragon 4Wings: phase-1 inspection

Measured with a temporary in-engine tool (now removed) that loads the mesh with `ModelReader`, the clips
with `LoadAnimationClip`, and samples poses with the same S·R·T / parent-chain / `meshSpaceInverse` math as
`SkeletalAnimatorComponent` and `HitVolumeComponent`. As a check, the same method reproduced the Usurper's
hand-measured offsets exactly (Spine01 → `{1.14, 0, 0}`, Spine02 → `{1.28, …}`, Neck01 → `{1.22, …}`).

**All meters below are at `data.scale = 0.01` (the model's native size: 1 unit = 1 cm).** Every length
scales linearly: at scale `s`, multiply by `s / 0.01`.

Supporting files:
- `tools/scifiDragon_skeleton.txt`: full hierarchy with each node's bind-pose child offsets (local frame, meters).
- `tools/scifiDragon_travel_raw.tsv`: per-node travel / peak speed / fast window for every attack clip (all 107 nodes).

---

## 1. Mesh and bone count

Mesh: `assets/models/scifiDragon/Mesh/Sci-Fi Dragon 4Wings Skin 1.fbx`
(Skins 1-4 are identical in geometry and skeleton: same 12 meshes, 38,109 verts, same AABB. They differ only in texture/material.)

```
[ModelReader] ../../assets/models/scifiDragon/Mesh\Sci-Fi Dragon 4Wings Skin 1.fbx: 107 nodes, 90 bones (maxBones 128)
```

**90 bones ≤ 128, so no change to `maxBones` / `MAX_BONES` is needed.** (The Usurper has 117.)

## 2. Weights, units, size

| | Sci-Fi 4Wings | Usurper (reference) |
|---|---|---|
| max bone influences / vertex before `LimitBoneWeights` | **6** (2,282 of 38,109 verts have > 4) | 7 (833 of 3,313) |
| FBX `UnitScaleFactor` | **1.0 → centimeters** | 100 (meters), but its `Dragon`/`Root` nodes carry scale 0.01 |
| mesh node scale | all mesh nodes (`polySurface*` under `Armor`/`Body`) and `root`/`pelvis`: scale 1, identity | `Dragon` mesh node scale 0.01 |
| engine model units | cm | cm (same) |
| skinned meshes | 12 (armor + body pieces), all skinned | 1 |

So the 4Wings uses the same engine units as the Usurper, and `data.scale = 0.01` gives true size.

Bind-pose size at scale 0.01 (vertex AABB, wings spread):

| | width (X, wingspan) | height (Y) | length (Z, head → tail tip) |
|---|---|---|---|
| **4Wings** | **12.05 m** | **3.99 m** | **9.83 m** |
| Usurper at its current 0.01 | 16.38 m | 8.75 m | 19.49 m |

4Wings landmarks at 0.01: pelvis 1.79 m (bind) / 1.60 m (Idle1), back (spine_03) 2.18 m, head 3.36 m, feet at
~0.1 m (so it stands on y = 0, no vertical offset needed). It faces **−Z** like the Usurper (head z −2.07, tail tips z +5.62).

**Picking `data.scale`:**
- `0.01`: native size, about 10 m long with a 12 m wingspan. Next to the 1.8 m player stand-in, the head is at about 3.4 m.
- `0.015`: roughly matches the Usurper's in-game presence (back about 2.7 m like the Usurper's 2.65 m; 14.7 m long; 18 m wingspan).
  **My recommendation** if the stage, camera and player spacing were tuned around the Usurper.
- `0.02`: matches the Usurper's overall length (19.7 m) but is taller (head about 6.7 m).

## 3. Skeleton

Full dump: `tools/scifiDragon_skeleton.txt`. Structure:

```
root                       (animated; carries root motion in some clips, see §4)
└ pelvis
  ├ spine_01 ─ spine_02 ─ spine_03 ─ neck_01 ─ neck_02 ─ head ─ jaw ─ Tongue…
  │                       ├ upperarm_High_l/r ─ lowerarm_High ─ Hand_High ─ Wings_l/r ─ Wings_1…4   (upper wing, leading edge → tip)
  │                       │                    └ Wings_5…8                                         (upper wing, inner finger)
  │            upperarm_Low_l/r (on spine_01) ─ lowerarm_Low ─ Hand_Low ─ Wings_9…13               (lower wing, leading edge → tip)
  │                                           └ Wings_14…17                                        (lower wing, inner finger)
  ├ thigh_l/r ─ calf ─ calf1 ─ foot ─ ball ─ ball1        (hind legs only)
  └ tail ─ Tail_l1…l9   and   tail ─ Tail_r1…r9           (TWO parallel tail chains, ~0.2-1.0 m apart)
```

**Anatomy differs from the Usurper. Please check these against your phase-2 plan:**
- **No front legs.** It's a wyvern layout: the "arms" are the wings. The phase-2 request "front and hind legs (L/R)"
  can only map to the **hind legs** (`thigh_* / calf_* / calf_*1 / foot_*`). The wings get their own hurtboxes, as planned.
- **Wing naming:** `wingL1`/`wingR1` = upper pair (`upperarm_High_*` chain), `wingL2`/`wingR2` = lower pair (`upperarm_Low_*` chain).
- **Two tails.** `Tail_l1-9` and `Tail_r1-9` both hang off `tail` and run in parallel, diverging from 0.44 m apart at the base
  to 2.0 m at the tips. A single "tail" hurtbox chain would need both, or names like `tailL` / `tailR`.
- Bone names are all lowercase-first (`spine_01`, `upperarm_High_l`). There are no typos like the Usurper's.

Key child offsets (local frame, meters at 0.01; full list in the dump):

| node → child | offset (local) |
|---|---|
| pelvis → spine_01 | {0.31, 0.00, −0.01} |
| spine_01 → spine_02 | {0.64, 0, 0} |
| spine_02 → spine_03 | {0.61, 0.00, −0.08} |
| spine_03 → neck_01 | {0.56, 0.00, 0.04} |
| neck_01 → neck_02 / neck_02 → head | {0.53, 0, 0} / {0.55, 0, 0} |
| thigh_l → calf_l / calf_l → calf_l1 / calf_l1 → foot_l | {0.48, 0, 0} / {0.48, 0, 0} / {0.60, 0, 0} |
| pelvis → tail | {−0.70, 0.00, 0.28} |
| tail → Tail_l1 / Tail_r1 | {±0.22, 0.02, 0.36} |
| Tail_l1→l2 … l8→l9 | {0, 0, 0.37}, {0, 0, 0.36}, {0, 0, 0.55}, {0, 0, 0.39}, {0, 0, 0.57}, {0, 0, 0.56}, {0, 0, 0.57}, {0, 0, 0.62} |

Spine, neck and leg bones point along local **+X** (like the Usurper). **The tail segments point along local +Z**,
so their "along the bone" capsule ends are `{0, 0, length}`.

## 4. Clips (41 in `Animations/`, rootmotion/ excluded)

All 30 fps, all 91/91 channels map to the skeleton (no "channel has no node" warnings).
**Loop?** compares the first and last pose (max local rotation difference, plus max joint-position difference relative to the root).
**Root motion** is the top animated node, `root` (the node the whole skeleton hangs under). `pelvis` moves with it.

| clip | dur (s) | pelvis h t0→end (m) | first↔last pose | loop candidate | root X/Z t0 → end (m) | root yaw end |
|---|---|---|---|---|---|---|
| Attk1 | 3.500 | 1.60 → 1.60 | 0.04° | pose-closed (returns to idle) | 0,0 → 0,0 | 0° |
| Attk2 | 3.500 | 1.60 | 0.04° | pose-closed | 0 → 0 | 0° |
| Attk3 | 3.500 | 1.60 | 0.06° | pose-closed | 0 → 0 | 0° |
| Attk4 | 4.333 | 1.60 | 0.06° | pose-closed | 0 → 0 | 0° |
| Attk5 | 2.433 | 1.60 | 0.04° | pose-closed | 0 → 0 | 0° |
| Death1 | 1.767 | 1.60 → 0.63 | 64° | no | 0 → 0 | 0° |
| Death2 | 2.067 | 1.60 → 0.72 | 72° | no | 0 → 0 | 0° |
| FlyAccelStart | 1.167 | 2.66 → 2.55 | 60° | no (→ FlyAccelLoop) | 0 → 0 | 0° |
| FlyAccelLoop | 2.000 | 2.56 | 0.04° | **yes** | 0 → 0 | 0° |
| FlyAccelEnd | 1.167 | 2.55 → 2.66 | 60° | no (→ hover) | 0 → 0 | 0° |
| FlyAttk1 | 3.333 | 2.66 | 2.3° | pose-closed (hover) | 0 → 0 | 0° |
| FlyAttk2 | 3.333 | 2.66 | 0.06° | pose-closed (hover) | 0 → 0 | 0° |
| FlyF | 3.333 | 2.94 | 0.06° | **yes** | 0 → 0 | 0° |
| FlyB | 3.333 | 2.44 | **4.3°, 0.57 m** | **weak loop**: visible hitch at the wrap | 0 → 0 | 0° |
| FlyL | 3.333 | 2.94 | 0.06° | **yes** | 0 → 0 | 0° |
| FlyR | 3.333 | 2.94 | 0.10° | **yes** | 0 → 0 | 0° |
| FlyRollL | 2.000 | 2.56 | 0.24° | pose-closed (starts/ends in FlyAccelLoop pose) | 0 → 0 | 0° |
| FlyRollR | 2.033 | 2.56 | 0.06° | pose-closed (FlyAccelLoop pose) | 0 → 0 | 0° |
| FlyTurn90L | 1.667 | 2.66 | 0.06° | no | 0 → 0 | **−90°** |
| FlyTurn90R | 1.700 | 2.66 | 0.13° | no | 0 → 0 | **+90°** |
| FlyTurn180 | 2.300 | 2.66 → 2.87 | 6.9° | no | 0 → 0 | **180°** |
| FlyGetHit1 | 1.533 | 2.66 | 1.3° | pose-closed (hover) | 0 → 0 | 0° |
| FlyGetHit2 | 1.800 | 2.66 | 0.06° | pose-closed (hover) | 0 → 0 | 0° |
| GetHitB | 2.833 | 1.60 | 0.06° | pose-closed | 0,0 → **0, −1.26** | 0° |
| GetHitF | 2.667 | 1.60 | 0.06° | pose-closed | 0 → 0 | 0° |
| GetHitF2 | 2.000 | 1.60 | 0.04° | pose-closed | 0 → 0 | 0° |
| GetHitL | 2.333 | 1.60 | 0.06° | pose-closed | 0 → 0 | 0° |
| GetHitR | 2.367 | 1.60 | 0.66° | pose-closed | 0 → 0 | 0° |
| Idle1 | 6.667 | 1.60 | 0.06° | **yes** | 0 → 0 | 0° |
| Idle2 | 6.667 | 1.60 | 0.06° | **yes** | 0 → 0 | 0° |
| **Idle3** | 3.333 | **2.66** | 0.96° | **yes, but it's the in-air hover idle** | 0 → 0 | 0° |
| TakeOff | 3.300 | 1.60 → 2.66 | 60° | no (ends in hover) | 0 → 0 | 0° |
| Land | 2.000 | 2.66 → 1.60 | 60° | no (hover → ground idle) | 0 → 0 | 0° |
| Roar | 3.867 | 1.60 | 0.06° | pose-closed | 0 → 0 | 0° |
| Turn90L | 1.700 | 1.60 | 0.06° | no | 0 → 0 | **−90°** |
| Turn90R | 1.733 | 1.60 | **21.7°** | no; one or more bones end ~22° away from idle | 0 → 0 | **+90°** |
| Turn180 | 2.800 | 1.60 | 0.04° | no | 0 → 0 | **180°** |
| WalkF | 1.700 | 1.63 | 1.8° | **yes, but has root motion** | 0,0 → **0, −2.51** | 0° |
| WalkB | 1.667 | 1.68 | 0.04° | **yes** (in place) | 0 → 0 | 0° |
| WalkL | 1.667 | 1.61 | 0.5° | **yes** (in place) | 0 → 0 | 0° |
| WalkR | 1.667 | 1.61 | 0.5° | **yes** (in place) | 0 → 0 | 0° |

### Root-motion findings (these matter for phase 2)
- **Turn90L/R, Turn180, FlyTurn90L/R, FlyTurn180 rotate `root` by ±90° / 180°, with no X/Z translation.**
  The engine doesn't extract root motion, so the dragon turns on screen and then **snaps back to its original heading**
  when the next clip starts. Proper turns need root yaw applied to the transform at clip end, or `root` stripped
  from these clips with the transform rotated in code. Until then the turns are cosmetic.
- **Attk1-5 have no root motion** (root and pelvis fixed). They play correctly in place.
- **WalkF translates `root` 2.51 m forward (−Z) per cycle.** WalkB/L/R are in place. Looping WalkF in place
  slides forward and **pops back 2.5 m every 1.7 s**. Options: zero `root` translation for WalkF, or move the object
  at 2.51 / 1.7 ≈ **1.48 m/s** (at 0.01; scale with data.scale) and cancel the root track.
- **GetHitB moves `root` 1.26 m backward** and pops back at the end.

### Clip state and loop findings
- **Idle3 is the flying hover idle, not a ground idle.** Its pelvis height (2.66 m) and pose match the start and end of
  FlyAttk1/2, FlyGetHit1/2, FlyTurn*, and the end of TakeOff / start of Land. The phase-2 loop list ("Idle1-3") stays
  correct, since Idle3 does loop. But the air state's idle is Idle3.
- **TakeOff → FlyF:** TakeOff ends in the hover pose (pelvis 2.66). FlyF starts higher (pelvis 2.94), and the pose
  differs by about 14° on some bones, so the 0.2 s default fade will be noticeable. **TakeOff → Idle3** is the natural
  chain (same hover state; Idle3 plays the Usurper's FlyIdle role). I'll keep T → FlyF as you asked unless you change it.
- **FlyB is a weak loop** (4.3° and 0.57 m mismatch at the wrap). It still loops, with a small hitch.
- Pose-closed one-shots (attacks, GetHits, Roar) return to the idle pose within about 2°, so they blend cleanly back to Idle1.

## 5. Attack motion (for hitbox bones and HitboxOn/Off timing)

World-space path length over the clip, peak speed, and the **fast window**: the contiguous span around the peak
where speed stays at or above 50% of peak. That window is a reasonable first guess for HitboxOn → HitboxOff.
Candidate end bones: head, jaw, wing wrists (`Hand_High_*` upper, `Hand_Low_*` lower), wing tips (`Wings_*4` upper,
`Wings_*13` lower), tail tips (`Tail_l9` / `Tail_r9`), hind claws (`ball_*1`).

**Caveat:** the wings flap through every clip, so wing tips always top raw travel. Each attack therefore lists the
top 3 overall (as requested) **and** the top 3 non-wing-tip bones, which show what the attack actually swings.

| clip | top 3 overall: bone (travel m, peak m/s @ s, fast window s) | top 3 excluding wing tips | read |
|---|---|---|---|
| **Attk1** (3.50 s) | Wings_r4 (35.7, 44.5 @1.675, 1.37-1.80); Wings_l4 (23.6, 35.8 @1.642, 1.53-1.77); Wings_r13 (18.6, 24.6 @1.542, 1.37-1.77) | Hand_High_l (10.2, 17.4 @1.300, 1.17-1.40); Hand_High_r (9.6, 11.6 @1.275, 1.17-1.70); jaw (6.8, 13.8 @1.242, 1.17-1.37) | lunge / wing strike: wrists and head lead (~1.17-1.40), wing tips follow through (to ~1.80) |
| **Attk2** (3.50 s) | Wings_r4 (36.6, 64.0 @1.417, 1.33-1.53); Wings_l4 (35.4, 61.7 @1.433, 1.30-1.53); **Tail_r9 (31.4, 51.2 @1.333, 1.20-1.50)** | **Tail_r9; Tail_l9 (29.4, 47.7 @1.300, 1.20-1.50)**; Hand_High_l (14.8, 24.3 @1.333, 1.20-1.43) | **tail whip**, both tails ~1.20-1.50 |
| **Attk3** (3.50 s) | Wings_l4 (29.8, 31.7 @1.208, 1.10-1.50); Wings_r4 (22.4, 22.1 @1.808, 1.61-1.93); Wings_r13 (21.6, 34.9 @1.500, 1.37-1.63) | Hand_High_r (13.0, 19.0 @1.275, 1.13-1.40); **jaw (13.0, 19.6 @1.242, 1.13-1.37); head (11.7, 19.2 @1.242, 1.13-1.37)** | **bite**: head and jaw snap 1.13-1.37 |
| **Attk4** (4.33 s) | Wings_r4 (25.8, 20.8 @2.342, 2.03-2.63); Wings_l4 (22.8, 11.9 @2.133, 1.67-2.43); jaw (9.2, 6.3 @2.033, 1.67-2.27) | jaw; Hand_High_r (9.0, 4.7 @2.242, 1.73-2.53); Hand_High_l (9.0, 5.4 @1.933, 1.67-2.30) | slow, sustained head motion (peaks only ~6 m/s) over 1.6-2.3 s: looks like a **breath / sweep** attack, not a strike |
| **Attk5** (2.43 s) | Wings_l4 (68.0, **166.9** @0.967, 0.87-1.03); Wings_r4 (54.5, 101.9 @1.042, 1.00-1.27); Wings_l13 (52.0, 95.0 @0.933, 0.83-1.10) | Hand_High_l (25.0, 64.3 @0.967, 0.87-1.03); Hand_High_r (19.6, 33.0 @1.033, 0.87-1.13); Hand_Low_l (18.4, 38.3 @0.933, 0.83-1.07) | **wing slam**, everything fast 0.83-1.13 (tails and head also peak ~0.93). The 167 m/s wing-tip peak is extreme; check visually for a key snap |
| **FlyAttk1** (3.33 s) | Wings_l4 (86.0, 45.7 @1.742, 1.60-1.90); Wings_r4 (79.5, 40.8 @2.675, 2.50-2.83); Wings_r13 (71.7, 36.3 @2.700, 2.53-2.83) | Hand_High_l (28.6, 15.3 @0.867, 0.70-1.00); Hand_High_r (27.9, 15.7 @0.842, 0.70-1.00); Hand_Low_l (21.8, 11.2 @1.733, 1.57-1.90) | dominated by **flapping** (periodic wrist/tip peaks ~0.85 / 1.75 / 2.7). The strike isn't separable from travel; jaw peaks @1.975 (1.83-2.07), claws @0.375. Needs a visual check with the tester |
| **FlyAttk2** (3.33 s) | Wings_l4 (87.0, 47.6 @2.675, 2.50-2.83); Wings_r4 (81.6, 44.7 @1.867, 1.67-2.00); Wings_r13 (72.4, 36.8 @2.708, 2.57-2.87) | Hand_High_l (28.2, 16.2 @0.875, 0.73-1.03); Hand_High_r (27.7, 15.4 @1.742, 1.60-1.90); Hand_Low_r (21.5, 10.8 @2.667, 2.50-2.83) | flapping again; **head and jaw peak late @2.833 (2.63-2.97)**, a probable bite/breath at the end. Visual check advised |

Hind claws (`ball_*1`) barely move in the ground attacks (< 0.15 m). They're planted, so they're not hitbox candidates there.

---

## Decisions for you before phase 2
1. **`data.scale`**: 0.01 (native), **0.015 (Usurper-like, recommended)**, or another value. Hurtbox meters in the dump are at 0.01 and scale linearly.
2. **Legs**: there are no front legs, so the hurtboxes will cover hind legs `legL` / `legR` only (wings get `wingL1/L2/R1/R2`). OK?
3. **Tails**: hurtbox both chains as `tailL` / `tailR`, or one `tail` name for both?
4. **T key chain**: TakeOff → FlyF as written, or **TakeOff → Idle3** (the real hover idle)?
5. **Root motion** (Turn*/FlyTurn* yaw, WalkF and GetHitB translation): leave for later (turns snap back, WalkF pops), or handle in phase 2?
6. **Skin**: Skin 1 by default (all 4 are the same mesh).
