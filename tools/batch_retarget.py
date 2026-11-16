"""
batch_retarget.py
HumanIK batch retarget: UE-mannequin nodachi clips (assets/models/animations)
-> violinist clip files for DXFramework (assets/models/10e/animations)

Run inside characterAnimationTest.mb, with HumanIK already set up:
    Character1 = violinist  (target)
    Character2 = UE mannequin 'root' skeleton (source)
Open the HumanIK panel once before running, so its MEL procedures are loaded.

Usage (Script Editor, Python tab):
    import sys, importlib
    sys.path.append(r"<DXFramework>/tools")
    import batch_retarget as br
    importlib.reload(br)

    br.run(only=["Combo1Attk1"])     # load a clip to look at
    br.setup_sword()                 # ONCE: calibrate the sword grip (see its docstring)
    br.run(only=["Combo1Attk1", "RunLoop"])   # check two different grips
    br.run()                         # then every clip

What one clip goes through:
    1. clear the source skeleton's keys, import the clip onto it (FBX "exmerge" = update animation only)
    2. read the frame range from the imported keys
    3. connect HumanIK: Character1 <- Character2 (the violinist follows live)
    4. key Sword1 per frame from the source sword's grip (hand-local, see _key_sword)
    5. FBX export of the violinist Skeleton with "bake complex animation":
       the exporter samples every frame and writes keys for the HumanIK-driven joints
       and the keyed Sword1. No mesh, no skin, one take.
"""

import os
import json
import math
import maya.cmds as cmds
import maya.mel as mel
import maya.api.OpenMaya as om

# ---------------------------------------------------------------- config
# Paths are relative to THIS SCRIPT, so it works on any machine / clone.
# Script lives at  DXFramework/tools/batch_retarget.py
SCRIPT_TO_MODELS = "../assets/models"   # change if you move the script
SRC_SUBDIR = "animations"               # relative to assets/models
OUT_SUBDIR = "10e/animations"
SKIP = {"UAL2Standard"}                 # not a nodachi clip

TARGET_CHAR = "Character1"         # violinist
SOURCE_CHAR = "Character2"         # UE mannequin
SOURCE_ROOT = "root"               # source skeleton root joint
TARGET_SKEL = "Skeleton"           # violinist skeleton root (what gets exported)
SWORD = "Sword1"                   # violinist sword joint (child of hand_R)
SOURCE_WEAPON = "Nodachi_Weapon_R" # source sword joint (child of hand_r)
SWORD_CALIB_ATTR = "swordCalib"    # string attr on Sword1, holds the grip calibration

SCENE_TIME_UNIT = "ntscf"          # 60 fps = the source pack's rate (no resampling)
RM_THRESHOLD_CM = 2.0              # source root moves more than this -> root-motion clip
RM_SUFFIX = "_RM"                  # Combo1Attk1 -> Combo1Attk1_RM.fbx


# ---------------------------------------------------------------- scene / paths
def _check_scene():
    """Fail early (before touching any keys) if this isn't the retarget scene."""
    missing = [n for n in (SOURCE_ROOT, SOURCE_WEAPON, TARGET_SKEL, SWORD) if not cmds.objExists(n)]
    missing += [c for c in (TARGET_CHAR, SOURCE_CHAR)
                if not cmds.ls(c, type="HIKCharacterNode")]
    if missing:
        scene = cmds.file(query=True, sceneName=True) or "(unsaved scene)"
        raise RuntimeError(
            "Wrong scene? %s is missing: %s\n"
            "Open characterAnimationTest.mb (the HumanIK retarget scene) and run again."
            % (os.path.basename(scene), ", ".join(missing)))


def _models_dir():
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.normpath(os.path.join(here, SCRIPT_TO_MODELS))


# ---------------------------------------------------------------- helpers
def _hierarchy(root):
    """root + every joint under it (long names)."""
    kids = cmds.listRelatives(root, allDescendents=True, type="joint", fullPath=True) or []
    return cmds.ls(root, long=True) + kids


def _clear_keys(nodes):
    if nodes:
        cmds.cutKey(nodes, clear=True)


def _import_source(path):
    # Old keys must go first: exmerge only overwrites curves the new file has,
    # so a shorter clip would otherwise keep the tail of the previous one.
    _clear_keys(_hierarchy(SOURCE_ROOT))
    mel.eval("FBXResetImport")
    mel.eval("FBXImportMode -v exmerge")        # update animation on existing nodes only
    mel.eval("FBXImportFillTimeline -v false")  # we set the range ourselves
    mel.eval('FBXImport -f "%s"' % path.replace("\\", "/"))


def _key_range():
    nodes = _hierarchy(SOURCE_ROOT)
    first = cmds.findKeyframe(nodes, which="first")
    last = cmds.findKeyframe(nodes, which="last")
    return int(math.floor(first + 1e-3)), int(math.ceil(last - 1e-3))


def _connect_hik():
    mel.eval('hikSetCurrentCharacter("%s")' % TARGET_CHAR)
    mel.eval('hikSetCharacterInput("%s", "%s")' % (TARGET_CHAR, SOURCE_CHAR))


def _root_travel(start, end):
    """Largest distance the source root gets from its start position (cm).
    Max over every frame, not just start->end, so a clip that lunges out and
    comes back (end == start) still counts as root motion."""
    p0 = cmds.getAttr(SOURCE_ROOT + ".translate", time=start)[0]
    return max(math.dist(p0, cmds.getAttr(SOURCE_ROOT + ".translate", time=f)[0])
               for f in range(start, end + 1))


# ---------------------------------------------------------------- sword
# Matrices follow Maya's row-vector convention: world = local * parentWorld.
#
# Why not an orientConstraint: a constraint matches the sword's WORLD rotation to the
# source sword. The two hands are not oriented identically frame by frame after the
# retarget, so the sword turned inside her hand during a swing (up to ~60 deg in
# Combo1Attk1) even though the source sword never moves relative to its hand.
#
# Instead we copy the grip in HAND space:
#   N  = source sword rotation relative to hand_r      (read per frame)
#   S  = violinist sword rotation relative to hand_R   (what we key)
#   E  = Hs0 * Ht0^-1  converts source-hand space -> target-hand space (from calibration)
#   A  = S0 * E^-1 * N0^-1  (from calibration, so that S == S0 when N == N0)
#   S  = A * N * E
# If the source grip is fixed in a clip, S is fixed too: the sword stays rigid in her hand.
# If the source grip changes between clips (RunLoop vs Combo1Attk1), S changes the same way.

def _rot(m):
    """Rotation part of a matrix (drops translation and scale)."""
    return om.MTransformationMatrix(om.MMatrix(m)).rotation(asQuaternion=True).asMatrix()


def _world_rot(node, t=None):
    m = cmds.getAttr(node + ".worldMatrix[0]", time=t) if t is not None \
        else cmds.getAttr(node + ".worldMatrix[0]")
    return _rot(m)


def _parent(node):
    return cmds.listRelatives(node, parent=True, fullPath=True)[0]


def _local_rot(node, t=None):
    """Rotation of node relative to its parent (includes jointOrient etc.)."""
    return _world_rot(node, t) * _world_rot(_parent(node), t).inverse()


def _m_to_list(m):
    return [m.getElement(r, c) for r in range(4) for c in range(4)]


def _save_calib(A, E):
    if not cmds.attributeQuery(SWORD_CALIB_ATTR, node=SWORD, exists=True):
        cmds.addAttr(SWORD, longName=SWORD_CALIB_ATTR, dataType="string")
    cmds.setAttr(SWORD + "." + SWORD_CALIB_ATTR,
                 json.dumps({"A": _m_to_list(A), "E": _m_to_list(E)}), type="string")


def _load_calib():
    if not cmds.attributeQuery(SWORD_CALIB_ATTR, node=SWORD, exists=True):
        return None
    d = json.loads(cmds.getAttr(SWORD + "." + SWORD_CALIB_ATTR))
    return om.MMatrix(d["A"]), om.MMatrix(d["E"])


def _euler_matrix(node, attr):
    v = [math.radians(x) for x in cmds.getAttr(node + "." + attr)[0]]
    return om.MEulerRotation(v[0], v[1], v[2], om.MEulerRotation.kXYZ).asMatrix()


def _key_sword(start, end):
    """Key Sword1's rotate per frame from the source grip (hand-local)."""
    calib = _load_calib()
    if calib is None:
        cmds.warning("Sword not calibrated (run setup_sword()); Sword1 stays at bind pose.")
        return
    A, E = calib
    # Joint local rotation = rotateAxis * rotate * jointOrient  -> solve for rotate
    ra_inv = _euler_matrix(SWORD, "rotateAxis").inverse()
    jo_inv = _euler_matrix(SWORD, "jointOrient").inverse()
    order = cmds.getAttr(SWORD + ".rotateOrder")   # same enum order as MEulerRotation

    cmds.cutKey(SWORD, attribute=["rx", "ry", "rz"], clear=True)
    prev = None
    for f in range(start, end + 1):
        S = A * _local_rot(SOURCE_WEAPON, f) * E
        R = ra_inv * S * jo_inv
        e = om.MTransformationMatrix(R).rotation()   # MEulerRotation (XYZ)
        e = e.reorder(order)
        if prev is not None:
            e = e.closestSolution(prev)              # no 360-deg flips between frames
        prev = e
        for attr, val in (("rx", e.x), ("ry", e.y), ("rz", e.z)):
            cmds.setKeyframe(SWORD, attribute=attr, time=f, value=math.degrees(val))


# ---------------------------------------------------------------- export
def _export(path, start, end):
    cmds.select(TARGET_SKEL, replace=True)
    for cmd in (
        "FBXResetExport",
        "FBXExportIncludeChildren -v true",       # whole joint tree under Skeleton
        "FBXExportInputConnections -v false",     # do NOT pull in the source rig / HIK nodes
        "FBXExportSkins -v false",
        "FBXExportShapes -v false",
        "FBXExportConstraints -v false",
        "FBXExportSkeletonDefinitions -v false",
        "FBXExportCameras -v false",
        "FBXExportLights -v false",
        "FBXExportEmbeddedTextures -v false",
        "FBXExportInAscii -v false",
        "FBXExportUpAxis y",
        "FBXExportSplitAnimationIntoTakes -c",    # one take -> mAnimations[0]
        "FBXExportBakeComplexAnimation -v true",
        "FBXExportBakeComplexStart -v %d" % start,
        "FBXExportBakeComplexEnd -v %d" % end,
        "FBXExportBakeComplexStep -v 1",
        # Resample OFF: the complex bake already writes one key per scene frame (60 fps).
        # With resample ON the exporter re-sampled everything at 24 fps (Film),
        # ignoring the scene rate -> 50 keys instead of 122.
        "FBXExportBakeResampleAnimation -v false",
    ):
        mel.eval(cmd)
    mel.eval('FBXExport -f "%s" -s' % path.replace("\\", "/"))


# ---------------------------------------------------------------- public
def setup_sword():
    """
    Calibrate the sword grip ONCE (then save the scene). Two calls:

    1st call: removes anything still driving Sword1 (old orientConstraint, keys)
              so you can rotate it freely.
    Then:     with a clip loaded, go to a frame where the grip is clear and
              rotate Sword1 until the sword sits correctly in her hand.
    2nd call: stores the calibration on Sword1 (attribute 'swordCalib').
              From then on run() keys Sword1 for every clip.
    """
    _check_scene()
    driven = list(set(cmds.listConnections(SWORD, type="constraint") or []))
    keyed = cmds.keyframe(SWORD, attribute=["rx", "ry", "rz"], query=True, keyframeCount=True)
    if driven or keyed:
        rot = cmds.getAttr(SWORD + ".rotate")[0]   # keep what you see right now
        if driven:
            cmds.delete(driven)
        cmds.cutKey(SWORD, attribute=["rx", "ry", "rz"], clear=True)
        cmds.setAttr(SWORD + ".rotate", *rot)
        print("Sword1 is free now. Rotate it into her hand, then run setup_sword() again.")
        return

    N0 = _local_rot(SOURCE_WEAPON)
    S0 = _local_rot(SWORD)
    E = _world_rot(_parent(SOURCE_WEAPON)) * _world_rot(_parent(SWORD)).inverse()
    A = S0 * E.inverse() * N0.inverse()
    _save_calib(A, E)
    print("Sword grip calibrated (stored on %s.%s). Save the scene." % (SWORD, SWORD_CALIB_ATTR))


def run(only=None):
    """Retarget + export every clip in SRC_DIR (or just the names in `only`)."""
    _check_scene()
    models = _models_dir()
    SRC_DIR = os.path.join(models, SRC_SUBDIR)
    OUT_DIR = os.path.join(models, OUT_SUBDIR)
    print("source: %s\noutput: %s" % (SRC_DIR, OUT_DIR))
    if not os.path.isdir(OUT_DIR):
        os.makedirs(OUT_DIR)

    # Bake at the source's own rate (60 fps) so nothing is resampled.
    cmds.currentUnit(time=SCENE_TIME_UNIT)

    # Keys left on the violinist from a manual bake would override HumanIK.
    _clear_keys([j for j in _hierarchy(TARGET_SKEL) if not j.endswith("|" + SWORD)])

    files = sorted(f for f in os.listdir(SRC_DIR) if f.lower().endswith(".fbx"))
    done, failed = [], []
    for f in files:
        name = os.path.splitext(f)[0]
        if name in SKIP or (only and name not in only):
            continue
        try:
            _import_source(SRC_DIR + "/" + f)
            start, end = _key_range()
            cmds.playbackOptions(minTime=start, maxTime=end,
                                 animationStartTime=start, animationEndTime=end)
            _connect_hik()
            _key_sword(start, end)
            cmds.currentTime(start)

            travel = _root_travel(start, end)
            is_rm = travel > RM_THRESHOLD_CM
            out_name = name + RM_SUFFIX if is_rm else name
            _export(OUT_DIR + "/" + out_name + ".fbx", start, end)

            tag = "RM %.0f cm" % travel if is_rm else "in-place"
            done.append(out_name)
            print("[ok]   %-22s frames %d-%d  (%s)" % (out_name, start, end, tag))
        except Exception as e:  # keep going, report at the end
            failed.append((name, str(e)))
            cmds.warning("[fail] %s: %s" % (name, e))

    print("\n%d exported, %d failed -> %s" % (len(done), len(failed), OUT_DIR))
    for name, err in failed:
        print("   FAILED %s: %s" % (name, err))