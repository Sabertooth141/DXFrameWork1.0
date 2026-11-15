# Renames animation clips to the project convention (clip name == file stem, PascalCase,
# no underscores, lowercase .fbx, one flat animations folder per character).
# Safe to re-run: rows whose source is gone and target exists are skipped.
# Tracked files are moved with `git mv` inside the assets submodule (keeps history);
# untracked files are moved with a plain rename.
# Usage (from anywhere):  powershell -ExecutionPolicy Bypass -File tools\rename_clips.ps1 [-WhatIf]

param([switch]$WhatIf)

$ErrorActionPreference = 'Stop'
$assets = (Resolve-Path (Join-Path $PSScriptRoot '..\assets')).Path

# paths relative to assets/
$dragon = 'models/dragon/animations'
$scifi  = 'models/scifiDragon/Animations'
$player = 'models/10e/animations'

$map = @(
	# --- Usurper dragon (already flat) ---
	@("$dragon/attackFlame.fbx", "$dragon/AttkFlame.fbx"),
	@("$dragon/attackHand.fbx",  "$dragon/AttkHand.fbx"),
	@("$dragon/attackMouth.fbx", "$dragon/AttkMouth.fbx"),
	@("$dragon/attackWing.fbx",  "$dragon/AttkWing.fbx"),
	@("$dragon/getHit.fbx",      "$dragon/GetHit.fbx"),
	@("$dragon/idle01.fbx",      "$dragon/Idle1.fbx"),
	@("$dragon/idle02.fbx",      "$dragon/Idle2.fbx"),
	@("$dragon/takeOff.fbx",     "$dragon/TakeOff.fbx"),

	# --- Sci-Fi Dragon 4Wings (flatten) ---
	@("$scifi/attk/Attack1.fbx",                 "$scifi/Attk1.fbx"),
	@("$scifi/attk/Attack2.fbx",                 "$scifi/Attk2.fbx"),
	@("$scifi/attk/Attack3.fbx",                 "$scifi/Attk3.fbx"),
	@("$scifi/attk/Attack4.fbx",                 "$scifi/Attk4.fbx"),
	@("$scifi/attk/Attack5.fbx",                 "$scifi/Attk5.fbx"),
	@("$scifi/attk/Fly_Attack1.fbx",             "$scifi/FlyAttk1.fbx"),
	@("$scifi/attk/Fly_Attack2.fbx",             "$scifi/FlyAttk2.fbx"),
	@("$scifi/death/Death1.fbx",                 "$scifi/Death1.fbx"),
	@("$scifi/death/Death2.fbx",                 "$scifi/Death2.fbx"),
	@("$scifi/fly/Fly_Accelerated_Start.fbx",    "$scifi/FlyAccelStart.fbx"),
	@("$scifi/fly/Fly_Accelerated_Loop.fbx",     "$scifi/FlyAccelLoop.fbx"),
	@("$scifi/fly/Fly_Accelerated_End.fbx",      "$scifi/FlyAccelEnd.fbx"),
	@("$scifi/fly/Fly_B.fbx",                    "$scifi/FlyB.fbx"),
	@("$scifi/fly/Fly_F.fbx",                    "$scifi/FlyF.fbx"),
	@("$scifi/fly/Fly_L.fbx",                    "$scifi/FlyL.fbx"),
	@("$scifi/fly/Fly_R.fbx",                    "$scifi/FlyR.fbx"),
	@("$scifi/fly/Fly_Rolling_L.fbx",            "$scifi/FlyRollL.fbx"),
	@("$scifi/fly/Fly_Rolling_R.fbx",            "$scifi/FlyRollR.fbx"),
	@("$scifi/fly/Fly_Turn180.fbx",              "$scifi/FlyTurn180.fbx"),
	@("$scifi/fly/Fly_Turn90L.fbx",              "$scifi/FlyTurn90L.fbx"),
	@("$scifi/fly/Fly_Turn90R.fbx",              "$scifi/FlyTurn90R.fbx"),
	@("$scifi/fly/TransitionFlyStart.fbx",       "$scifi/TakeOff.fbx"),
	@("$scifi/fly/TransitionFlyEnd.fbx",         "$scifi/Land.fbx"),
	@("$scifi/hit/Fly_GetHit1.fbx",              "$scifi/FlyGetHit1.fbx"),
	@("$scifi/hit/Fly_GetHit2.fbx",              "$scifi/FlyGetHit2.fbx"),
	@("$scifi/hit/GetHit_B.fbx",                 "$scifi/GetHitB.fbx"),
	@("$scifi/hit/GetHit_F.fbx",                 "$scifi/GetHitF.fbx"),
	@("$scifi/hit/GetHit_F2.fbx",                "$scifi/GetHitF2.fbx"),
	@("$scifi/hit/GetHit_L.fbx",                 "$scifi/GetHitL.fbx"),
	@("$scifi/hit/GetHit_R.fbx",                 "$scifi/GetHitR.fbx"),
	@("$scifi/idle/Idle1.fbx",                   "$scifi/Idle1.fbx"),
	@("$scifi/idle/Idle2.fbx",                   "$scifi/Idle2.fbx"),
	@("$scifi/idle/Idle3.fbx",                   "$scifi/Idle3.fbx"),
	@("$scifi/roar/Roar.fbx",                    "$scifi/Roar.fbx"),
	@("$scifi/turn/Turn180.fbx",                 "$scifi/Turn180.fbx"),
	@("$scifi/turn/Turn90L.fbx",                 "$scifi/Turn90L.fbx"),
	@("$scifi/turn/Turn90R.fbx",                 "$scifi/Turn90R.fbx"),
	@("$scifi/walk/Walk_B.fbx",                  "$scifi/WalkB.fbx"),
	@("$scifi/walk/Walk_F.fbx",                  "$scifi/WalkF.fbx"),
	@("$scifi/walk/Walk_L.fbx",                  "$scifi/WalkL.fbx"),
	@("$scifi/walk/Walk_R.fbx",                  "$scifi/WalkR.fbx"),
	# root motion: unused, original names, not loaded
	@("$scifi/walk/Walk_B_RM.fbx",               "$scifi/rootmotion/Walk_B_RM.fbx"),
	@("$scifi/walk/Walk_F_RM.fbx",               "$scifi/rootmotion/Walk_F_RM.fbx"),
	@("$scifi/walk/Walk_L_RM.fbx",               "$scifi/rootmotion/Walk_L_RM.fbx"),
	@("$scifi/walk/Walk_R_RM.fbx",               "$scifi/rootmotion/Walk_R_RM.fbx"),

	# --- Player (10e) (flatten, lowercase extension) ---
	@("$player/UAL2_Standard.fbx",               "$player/UAL2Standard.fbx"),
	@("$player/attk/RunAttk_1.FBX",              "$player/RunAttk1.fbx"),
	@("$player/attk/combo1/ComboAttk1_1.FBX",    "$player/Combo1Attk1.fbx"),
	@("$player/attk/combo1/ComboAttk1_2.FBX",    "$player/Combo1Attk2.fbx"),
	@("$player/attk/combo1/ComboAttk1_3.FBX",    "$player/Combo1Attk3.fbx"),
	@("$player/attk/combo1/ComboAttk1_4.FBX",    "$player/Combo1Attk4.fbx"),
	@("$player/attk/combo2/ComboAttk2_1.FBX",    "$player/Combo2Attk1.fbx"),
	@("$player/attk/combo2/ComboAttk2_2.FBX",    "$player/Combo2Attk2.fbx"),
	@("$player/attk/combo2/ComboAttk2_3.FBX",    "$player/Combo2Attk3.fbx"),
	@("$player/attk/combo2/ComboAttk2_4.FBX",    "$player/Combo2Attk4.fbx"),
	@("$player/attk/combo3/ComboAttk3_1.FBX",    "$player/Combo3Attk1.fbx"),
	@("$player/attk/combo3/ComboAttk3_2.FBX",    "$player/Combo3Attk2.fbx"),
	@("$player/attk/combo3/ComboAttk3_3.FBX",    "$player/Combo3Attk3.fbx"),
	@("$player/attk/combo3/ComboAttk3_4.FBX",    "$player/Combo3Attk4.fbx"),
	@("$player/death/Death.FBX",                 "$player/Death.fbx"),
	@("$player/getup/GetUp_B.FBX",               "$player/GetUpB.fbx"),
	@("$player/hit/HitLarge_F.FBX",              "$player/HitLargeF.fbx"),
	@("$player/idle/Idle.FBX",                   "$player/Idle.fbx"),
	@("$player/idle/IdleCombat.FBX",             "$player/IdleCombat.fbx"),
	@("$player/roll/Roll_B.FBX",                 "$player/RollB.fbx"),
	@("$player/roll/Roll_F.FBX",                 "$player/RollF.fbx"),
	@("$player/run/RunFast_LeanL.FBX",           "$player/RunFastLeanL.fbx"),
	@("$player/run/RunFast_LeanR.FBX",           "$player/RunFastLeanR.fbx"),
	@("$player/run/RunFast_Loop.FBX",            "$player/RunFastLoop.fbx"),
	@("$player/run/RunFast_Start.FBX",           "$player/RunFastStart.fbx"),
	@("$player/run/RunFast_Stop.FBX",            "$player/RunFastStop.fbx"),
	@("$player/run/Run_Loop.FBX",                "$player/RunLoop.fbx"),
	@("$player/run/Run_Start.FBX",               "$player/RunStart.fbx"),
	@("$player/run/Run_Stop.FBX",                "$player/RunStop.fbx"),
	@("$player/turn/Turn_180L.FBX",              "$player/Turn180L.fbx"),
	@("$player/turn/Turn_180R.FBX",              "$player/Turn180R.fbx"),
	@("$player/turn/Turn_90L.FBX",               "$player/Turn90L.fbx"),
	@("$player/turn/Turn_90R.FBX",               "$player/Turn90R.fbx"),
	@("$player/walk/Walk_Loop.FBX",              "$player/WalkLoop.fbx"),
	@("$player/walk/Walk_Start.FBX",             "$player/WalkStart.fbx"),
	@("$player/walk/Walk_Stop.FBX",              "$player/WalkStop.fbx")
)

# exact-case existence check (the filesystem is case-insensitive)
function Test-ExactPath([string]$rel)
{
	$full = Join-Path $assets $rel
	if (-not (Test-Path -LiteralPath $full)) { return $false }
	$leaf = Split-Path $rel -Leaf
	$names = Get-ChildItem -LiteralPath (Split-Path $full -Parent) -Force | ForEach-Object Name
	return $names -ccontains $leaf
}

function Test-Tracked([string]$rel)
{
	$out = git -C $assets ls-files -- $rel
	return [bool]$out
}

function Move-One([string]$src, [string]$dst, [bool]$tracked)
{
	$dstDir = Split-Path (Join-Path $assets $dst) -Parent
	if (-not (Test-Path -LiteralPath $dstDir)) { New-Item -ItemType Directory -Path $dstDir | Out-Null }
	if ($tracked)
	{
		git -C $assets mv -- $src $dst
		if ($LASTEXITCODE -ne 0) { throw "git mv failed: $src -> $dst" }
	}
	else
	{
		Move-Item -LiteralPath (Join-Path $assets $src) -Destination (Join-Path $assets $dst)
	}
}

$moved = 0; $skipped = 0; $errors = 0
foreach ($row in $map)
{
	$src = $row[0]; $dst = $row[1]
	$srcExists = Test-ExactPath $src
	$dstExists = Test-ExactPath $dst

	if (-not $srcExists -and $dstExists) { $skipped++; continue }          # already renamed
	if (-not $srcExists) { Write-Warning "missing source: $src"; $errors++; continue }
	$caseOnly = ($src -ieq $dst)
	if ($dstExists -and -not $caseOnly) { Write-Warning "target exists, not overwriting: $dst"; $errors++; continue }

	if ($WhatIf) { Write-Host "would move  $src -> $dst"; continue }

	$tracked = Test-Tracked $src
	if ($caseOnly)
	{
		# two-step rename so a case-insensitive filesystem/git sees the change
		$tmp = "$dst.renametmp"
		Move-One $src $tmp $tracked
		Move-One $tmp $dst $tracked
	}
	else
	{
		Move-One $src $dst $tracked
	}
	Write-Host "moved  $src -> $dst$(if (-not $tracked) { '  (untracked)' })"
	$moved++
}

# remove subfolders left empty by flattening
if (-not $WhatIf)
{
	foreach ($root in @($scifi, $player))
	{
		Get-ChildItem -LiteralPath (Join-Path $assets $root) -Directory -Recurse |
			Sort-Object { $_.FullName.Length } -Descending |
			Where-Object { -not (Get-ChildItem -LiteralPath $_.FullName -Force) } |
			ForEach-Object { Remove-Item -LiteralPath $_.FullName; Write-Host "removed empty dir  $($_.FullName.Substring($assets.Length + 1))" }
	}
}

Write-Host "done: $moved moved, $skipped already renamed, $errors problems"
if ($errors -gt 0) { exit 1 }
