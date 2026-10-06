"""Build Standard-only recolors using the existing Blue material instances."""
import json
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
source = '/Game/TestArena/Pucks/PrototypeStandard'
destination = '/Game/Cosmetics/Pucks'
roles = {
    'Housing': 'MI_01_Graphite_anodized_housing',
    'Crown': 'MI_02_Circular_brushed_silver',
    'Diffuser': 'MI_04_Cyan_light_diffuser',
    'Edges': 'MI_06_Machined_edge_highlights',
    'Emblem': 'MI_07_Cyan_center_emblem',
}
editing = u.MaterialEditingLibrary
assets = u.AssetToolsHelpers.get_asset_tools()
for name in ['Emerald', 'Amethyst', 'Crimson', 'Amber']:
    definition = json.loads((root / 'AssetDevelopment/Pucks' / name / 'set.json').read_text())
    assert (root / 'AssetDevelopment/Pucks' / name / definition['shared_geometry']).resolve().is_file()
    folder = destination + '/' + name
    u.EditorAssetLibrary.make_directory(folder)
    for role, base_color in definition['base_colors'].items():
        original = u.load_asset(source + '/' + roles[role])
        assert isinstance(original, u.MaterialInstanceConstant), role
        asset_name = 'MI_Standard_' + role
        instance = u.load_asset(folder + '/' + asset_name)
        if not instance:
            instance = assets.create_asset(asset_name, folder, u.MaterialInstanceConstant,
                                           u.MaterialInstanceConstantFactoryNew())
        # Inherit the exact existing finish. Override vector colors, nothing else.
        editing.set_material_instance_parent(instance, original)
        vectors = {'BaseColor': base_color}
        if role in ['Diffuser', 'Emblem']:
            vectors['TeamColor'] = definition['accent_color']
        for parameter, value in vectors.items():
            editing.set_material_instance_vector_parameter_value(instance, parameter, u.LinearColor(*value, 1))
            stored = editing.get_material_instance_vector_parameter_value(instance, parameter)
            assert max(abs(a-b) for a,b in zip([stored.r,stored.g,stored.b],value)) < .0001
        for parameter in ['Metallic', 'Roughness', 'Anisotropy', 'Emission', 'SurfaceLift']:
            assert abs(editing.get_material_instance_scalar_parameter_value(instance, parameter) -
                       editing.get_material_instance_scalar_parameter_value(original, parameter)) < .0001, (name,role,parameter)
        editing.update_material_instance(instance)
        assert u.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
    u.log('FLICK Standard palette ready: ' + name)
u.log('FLICK_PUCK_PALETTES_COMPLETE')
