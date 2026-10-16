"""Inspect/reimport only the ten visual venue meshes, preserving material assets.

Unreal -run=pythonscript -script=Tools/Optimize-FlickVenueAssets.py
Add -FlickVenueApply only after Blender candidates have been accepted/applied.
Never invokes the broad asset updater (which also imports pucks and BOB's board).
"""
import hashlib
import json
import shutil
from pathlib import Path
import unreal as u

PROJECT = Path(u.Paths.project_dir())
WORK = PROJECT / 'Saved/VenueOptimization'
apply = '-FlickVenueApply' in u.SystemLibrary.get_command_line()
audit = json.loads((WORK / 'audit.json').read_text())
baseline = {row['package']: row for row in json.loads((WORK / 'unreal-before.json').read_text())} if apply else {}
editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem)
assets = u.AssetToolsHelpers.get_asset_tools()
if apply:
    # Existing venues use the legacy importer. Mixing Interchange reimport data
    # into those packages adds pipeline assets without reducing render cost.
    u.SystemLibrary.execute_console_command(None, 'Interchange.FeatureFlags.Import.FBX 0')
rows = []
for entry in audit:
    package = entry['package']
    mesh = u.load_asset(package)
    if not isinstance(mesh, u.StaticMesh):
        raise RuntimeError('Missing venue mesh: ' + package)
    slots = mesh.get_editor_property('static_materials')
    settings = editor.get_lod_build_settings(mesh, 0)
    material_signature = [[str(s.material_slot_name), s.material_interface.get_path_name() if s.material_interface else ''] for s in slots]
    if apply:
        original = PROJECT / 'Content' / (package[len('/Game/'):] + '.uasset')
        backup = WORK / 'originals' / original.relative_to(PROJECT)
        backup.parent.mkdir(parents=True, exist_ok=True)
        if not backup.exists():
            shutil.copy2(original, backup)
        task = u.AssetImportTask()
        task.filename = str(PROJECT / entry['source'])
        task.destination_path, task.destination_name = package.rsplit('/', 1)
        task.automated = True
        task.replace_existing = True
        task.replace_existing_settings = False
        task.save = False
        options = u.FbxImportUI()
        options.import_mesh = True
        options.import_materials = False
        options.import_textures = False
        options.import_as_skeletal = False
        options.automated_import_should_detect_type = False
        options.mesh_type_to_import = u.FBXImportType.FBXIT_STATIC_MESH
        options.static_mesh_import_data.combine_meshes = True
        options.static_mesh_import_data.auto_generate_collision = False
        options.static_mesh_import_data.generate_lightmap_u_vs = settings.generate_lightmap_u_vs
        options.static_mesh_import_data.normal_import_method = u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
        task.options = options
        task.factory = u.FbxFactory()
        assets.import_asset_tasks([task])
        mesh = u.load_asset(package)
        new_slots = mesh.get_editor_property('static_materials')
        if [str(s.material_slot_name) for s in slots] != [str(s.material_slot_name) for s in new_slots]:
            raise RuntimeError('Material slot order changed: ' + package)
        mesh.set_editor_property('static_materials', slots)
        editor.set_lod_build_settings(mesh, 0, settings)
        u.EditorAssetLibrary.save_loaded_asset(mesh)
    path = PROJECT / 'Content' / (package[len('/Game/'):] + '.uasset')
    bounds = mesh.get_bounds()
    row = dict(package=package, vertices=editor.get_number_verts(mesh, 0),
               triangles=mesh.get_num_triangles(0), bytes=path.stat().st_size,
               sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
               materials=material_signature,
               bounds_origin=[bounds.origin.x, bounds.origin.y, bounds.origin.z],
               bounds_extent=[bounds.box_extent.x, bounds.box_extent.y, bounds.box_extent.z])
    if apply:
        previous = baseline[package]
        assert row['materials'] == previous['materials'], package
        for field in ('bounds_origin', 'bounds_extent'):
            assert max(abs(a-b) for a,b in zip(row[field], previous[field])) < .001, (package, field)
        if (row['vertices'] == previous['vertices'] and row['triangles'] == previous['triangles']
                and row['bytes'] >= previous['bytes']):
            # Re-serializing identical render geometry can add editor metadata.
            # Keep the existing runtime package when it has no size/count win.
            shutil.copy2(backup, path)
            row = dict(previous, retained_original_runtime_package=True)
    rows.append(row)
    u.log('VENUE_UNREAL ' + json.dumps(row))
(WORK / ('unreal-after.json' if apply else 'unreal-before.json')).write_text(json.dumps(rows, indent=2))
u.log('VENUE_UNREAL_COMPLETE')
