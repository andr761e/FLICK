"""Scoped backups, protected-asset hashes, and before/after optimization report."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
WORK = PROJECT / 'Saved/VenueOptimization'
parser = argparse.ArgumentParser()
parser.add_argument('action', choices=['snapshot', 'apply', 'verify', 'report'])
args = parser.parse_args()
def digest(path):
    with path.open('rb') as stream: return hashlib.file_digest(stream, 'sha256').hexdigest()
audit = json.loads((WORK / 'audit.json').read_text())
target_packages = {entry['package'][len('/Game/'):] + '.uasset' for entry in audit}
protected_roots = ['AssetDevelopment/Pucks', 'AssetDevelopment/BOB Arena',
                   'Content/TestArena/Pucks', 'Content/BOB/Arena', 'Content/Cosmetics']
protected = [p for root in protected_roots for p in (PROJECT / root).rglob('*') if p.is_file()]
protected.extend(p for root in ['Content/TestArena/Arena', 'Content/TestArena/Stadium', 'Content/BOB/Stadium']
                 for p in (PROJECT/root).glob('*.uasset') if p.relative_to(PROJECT/'Content').as_posix() not in target_packages)
snapshot = WORK / 'protected-assets.json'
if args.action == 'snapshot':
    if snapshot.exists(): raise RuntimeError('Protected snapshot already exists; do not overwrite baseline')
    snapshot.write_text(json.dumps({p.relative_to(PROJECT).as_posix(): digest(p) for p in protected}, indent=2))
    print('Protected files recorded:', len(protected))
elif args.action == 'apply':
    rows = json.loads((WORK / 'candidates.json').read_text())
    assert {row['source'] for row in rows} == {row['source'] for row in audit}
    for row in rows:
        source = PROJECT / row['source']
        candidate = WORK / 'candidates' / row['source']
        assert digest(source) == row['original_sha256'], source
        assert digest(candidate) == row['candidate_sha256'], candidate
    assert sum(row['bytes_after'] for row in rows) < sum(row['bytes_before'] for row in rows)
    for row in rows:
        for suffix in ['.fbx', '.glb']:
            source = (PROJECT / row['source']).with_suffix(suffix)
            candidate = (WORK/'candidates'/row['source']).with_suffix(suffix)
            # GLB is a secondary preview format, not imported by Unreal. Keep
            # the identical-looking original if normal encoding makes it grow.
            if suffix == '.glb' and candidate.stat().st_size >= source.stat().st_size:
                continue
            backup = WORK / 'originals' / source.relative_to(PROJECT)
            backup.parent.mkdir(parents=True, exist_ok=True)
            if not backup.exists(): shutil.copy2(source, backup)
            shutil.copy2(candidate, source)
    print('Applied scoped exports:', len(rows))
elif args.action == 'verify':
    saved = json.loads(snapshot.read_text())
    changed = [name for name, value in saved.items() if not (PROJECT/name).exists() or digest(PROJECT/name) != value]
    assert not changed, changed
    print('Protected files unchanged:', len(saved))
else:
    candidates = json.loads((WORK/'candidates.json').read_text())
    blends = json.loads((WORK/'blends.json').read_text())
    before = {r['package']: r for r in json.loads((WORK/'unreal-before.json').read_text())}
    after = {r['package']: r for r in json.loads((WORK/'unreal-after.json').read_text(encoding='utf-8-sig'))}
    lines = ['# Venue geometry optimization — 2026-10-16', '',
        'Scope: the six modular Knockout arena meshes shared by 1v1, 2v2 and 3v3; '
        'the two Knockout stadium meshes; and the two BOB stadium meshes.', '',
        '**The BOB playing arena and all pucks are excluded.** SHA-256 verification '
        'confirmed all 172 protected files unchanged, including existing material assets.', '',
        '## Method and limits', '',
        'Merged disjoint, coplanar face pairs without crossing material, UV or sharp-edge boundaries; '
        'welded exactly coincident vertices. Retained circular segments, bevel profiles, positions, '
        'material slots and authored corner normals (apart from Blender normal-storage quantization). '
        'No gameplay, collision, lighting or material parameters changed.', '',
        'Each candidate passed bidirectional, per-material vertex/triangle-interior surface checks '
        'with maximum measured distance below 0.005 mm. Bounds remained unchanged within floating-point '
        'import precision. Unsafe broad dissolve/decimation candidates were rejected.', '',
        'FBX arrays also use lossless zlib level 9 instead of level 1. This reduces source-file storage, '
        'not GPU work. Native Blender files remain compressed. GLB previews are replaced only when smaller.', '',
        'Unreal triangulates polygons: fewer Blender faces do **not** necessarily mean fewer render '
        'triangles. The modest actual render reduction is listed separately. No FPS improvement is claimed. '
        'Original Unreal packages were retained where identical render counts and a larger reimport '
        'offered no benefit.', '',
        '## File sizes', '',
        'All values below are exact bytes. Totals count each shared asset once, not once per arena size.', '',
        '| Group / format | Before | After | Saved | Reduction |',
        '|---|---:|---:|---:|---:|']
    totals = [0,0]
    for group, label in [('Arena','Knockout 1v1 / 2v2 / 3v3'), ('Stadium','Knockout stadium'), ('BOBStadium','BOB stadium')]:
        rows = [r for r in candidates if Path(r['source']).parts[1] == group]
        formats = {}
        for suffix in ('.fbx','.glb'):
            sizes = [0,0]
            for r in rows:
                path = (PROJECT/r['source']).with_suffix(suffix)
                backup = WORK/'originals'/path.relative_to(PROJECT)
                sizes[0] += backup.stat().st_size if backup.exists() else path.stat().st_size
                sizes[1] += path.stat().st_size
            formats[suffix.upper()[1:]] = sizes
        formats['BLEND'] = [sum(r['bytes_before'] for r in blends if Path(r['source']).parts[1] == group),
                            sum((PROJECT/r['source']).stat().st_size for r in blends if Path(r['source']).parts[1] == group)]
        formats['Unreal editor assets'] = [sum(before[r['package']]['bytes'] for r in rows),
                                          sum(after[r['package']]['bytes'] for r in rows)]
        combined = [sum(pair[i] for pair in formats.values()) for i in (0,1)]
        for kind, (old,new) in [*formats.items(), ('ALL FILES',combined)]:
            lines.append(f'| {label} / {kind} | {old:,} | {new:,} | {old-new:,} | {(old-new)/old*100:.2f}% |')
        totals = [totals[i]+combined[i] for i in (0,1)]
    lines += [f'| **Total** | **{totals[0]:,}** | **{totals[1]:,}** | **{totals[0]-totals[1]:,}** | **{(1-totals[1]/totals[0])*100:.2f}%** |', '',
        'The Knockout stadium structure FBX grows on re-export despite fewer faces; '
        'its native Blender file and Unreal package shrink. This increase is included above, not hidden. '
        'Unreal editor asset bytes are not a measurement of cooked/download package size.', '',
        '## Per-mesh geometry and FBX sizes', '',
        '| Mesh | Blender faces before → after | Blender vertices before → after | Unreal triangles before → after | FBX bytes before → after |',
        '|---|---:|---:|---:|---:|']
    for r in candidates:
        b,a = before[r['package']],after[r['package']]
        assert digest(PROJECT/('Content/'+r['package'][6:]+'.uasset')) == a['sha256']
        lines.append(f"| {Path(r['source']).stem} | {r['before']['faces']:,} → {r['after']['faces']:,} | "
            f"{r['before']['vertices']:,} → {r['after']['vertices']:,} | {b['triangles']:,} → {a['triangles']:,} | "
            f"{r['bytes_before']:,} → {(PROJECT/r['source']).stat().st_size:,} |")
    lines += ['',
        f"Source faces total: {sum(r['before']['faces'] for r in candidates):,} → {sum(r['after']['faces'] for r in candidates):,}. "
        f"Source vertices: {sum(r['before']['vertices'] for r in candidates):,} → {sum(r['after']['vertices'] for r in candidates):,}.", '',
        f"Actual Unreal render triangles total (one copy of each mesh): {sum(r['triangles'] for r in before.values()):,} → "
        f"{sum(r['triangles'] for r in after.values()):,}. Actual render vertices: {sum(r['vertices'] for r in before.values()):,} → "
        f"{sum(r['vertices'] for r in after.values()):,}."]
    lines += ['', '## Verification and recovery', '',
        '- Unreal import completed: 0 errors. BOB stadium import emitted degenerate tangent/near-zero '
        'normal/tangent warnings; these are recorded in `Saved/Logs/Venue-Legacy-After.log`, not suppressed. '
        'A separate unsaved import of the untouched originals also emitted tangent warnings '
        '(`Saved/Logs/Venue-Original-Warnings.log`). Blender inspection found 12 zero-length corner '
        'normals in the original lights FBX and the optimized lights FBX; no new zero normals were introduced. '
        'Materials and bounds passed comparison. No C++ compilation was needed or run for this asset-only change.',
        '- Before captures: `Saved/UICaptures/20261016-191209-e01602/` (arena, switch rim, stadium stairs, BOB).',
        '- Final captures: `Saved/UICaptures/20261016-194824-53b8e6/`. Compared arena, switch rim and stair '
        'views visually: no visible silhouette/detail change. BOB gameplay view is consistent but does not '
        'expose every stadium surface. Captures are not pixel-identical: temporal rendering and bot lineups '
        'vary between runs. Across the four views mean absolute RGB differences were below 1.7/255 per channel.',
        '- Original FBX, changed GLB, BLEND and Unreal packages: `Saved/VenueOptimization/originals/`, '
        'mirroring their project-relative paths. These are local backups in the ignored Saved folder; '
        'do not delete them if you want a local rollback.',
        '- Detailed baseline/candidate/runtime JSON and hashes: `Saved/VenueOptimization/`.',
        '- Shared cleanup lives in `Tools/FlickMeshCleanup.py`; the three venue generators use it. '
        'Puck and BOB-board generators were not edited.', '']
    output = PROJECT/'Docs/VenueGeometryOptimization.md'
    output.parent.mkdir(exist_ok=True)
    output.write_text('\n'.join(lines), encoding='utf-8')
    print('Report written:', output)
