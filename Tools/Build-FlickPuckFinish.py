"""Refine the existing shared puck parent/materials. Never import a mesh.

Run with Unreal's PythonScript commandlet. Reuses the already-cooked asset paths;
all nine classes and all six palette finishes inherit the same optical response.
"""
import json
from pathlib import Path
import unreal as u

root = Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
settings = json.loads((root / 'AssetDevelopment/Pucks/finish.json').read_text())['roles']
folder = '/Game/TestArena/Pucks/PrototypeStandard'
material = u.load_asset(folder + '/M_BlueStandardSurface')
assert isinstance(material, u.Material), 'Existing production puck parent missing'
edit = u.MaterialEditingLibrary
edit.delete_all_material_expressions(material)
material.set_editor_property('shading_model', u.MaterialShadingModel.MSM_DEFAULT_LIT)
material.set_editor_property('tangent_space_normal', True)


def parameter(name, value):
    kind = u.MaterialExpressionVectorParameter if isinstance(value, u.LinearColor) else u.MaterialExpressionScalarParameter
    node = edit.create_material_expression(material, kind)
    node.set_editor_property('parameter_name', name)
    node.set_editor_property('default_value', value)
    return node


def connect(source, destination, slot):
    assert edit.connect_material_expressions(source, '', destination, slot)


# Keep environment bounce secondary to the dedicated neutral studio pair.
# Direct studio illumination and the existing material/normal detail remain lit.
assert edit.connect_material_property(parameter('PuckEnvironmentResponse', .12), '', u.MaterialProperty.MP_AMBIENT_OCCLUSION)


base = parameter('BaseColor', u.LinearColor(.025, .035, .045, 1))
scale = parameter('FinishColorScale', 1.)
colour = edit.create_material_expression(material, u.MaterialExpressionMultiply)
connect(base, colour, 'A')
connect(scale, colour, 'B')

# Rest-space coordinates travel/rotate with the visual mesh, never with world
# space. Derivative filtering removes subpixel machining marks at game distance.
position = edit.create_material_expression(material, u.MaterialExpressionVertexInterpolator)
connect(edit.create_material_expression(material, u.MaterialExpressionPreSkinnedPosition), position, 'VS')
# Standard's supplied FBX assigns its center disc to Housing rather than Core.
# Coat only that upper, recessed center in rest space. Do not dull the housing
# sidewall, silver crown, or lower service plate, or change any mesh/section.
coating = edit.create_material_expression(material, u.MaterialExpressionCustom)
coating.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT1)
coating.set_editor_property('code', 'return Coat * step(0., P.z) * (1. - smoothstep(24.5, 25., length(P.xy)));')
inputs = []
for name in ['P', 'Coat']:
    entry = u.CustomInput()
    entry.set_editor_property('input_name', name)
    inputs.append(entry)
coating.set_editor_property('inputs', inputs)
connect(position, coating, 'P')
connect(parameter('FinishFaceCoating', 0.), coating, 'Coat')


def coated(original, face):
    node = edit.create_material_expression(material, u.MaterialExpressionLinearInterpolate)
    connect(original, node, 'A')
    connect(face, node, 'B')
    connect(coating, node, 'Alpha')
    return node


face_colour = edit.create_material_expression(material, u.MaterialExpressionMultiply)
connect(colour, face_colour, 'A')
connect(parameter('FaceCoatingColorScale', .5), face_colour, 'B')
assert edit.connect_material_property(coated(colour, face_colour), '', u.MaterialProperty.MP_BASE_COLOR)
for name, value, face, prop in [('Metallic', .82, settings['Core']['metallic'], u.MaterialProperty.MP_METALLIC),
                               ('Specular', .5, settings['Core']['specular'], u.MaterialProperty.MP_SPECULAR),
                               ('Anisotropy', .2, settings['Core']['anisotropy'], u.MaterialProperty.MP_ANISOTROPY)]:
    assert edit.connect_material_property(coated(parameter(name, value), parameter('FaceCoating' + name, face)), '', prop)
rough = parameter('Roughness', .27)
detail = parameter('FinishDetail', .6)
finish = edit.create_material_expression(material, u.MaterialExpressionCustom)
finish.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT4)
inputs = []
for name in ['P', 'Rough', 'Detail']:
    entry = u.CustomInput()
    entry.set_editor_property('input_name', name)
    inputs.append(entry)
finish.set_editor_property('inputs', inputs)
finish.set_editor_property('code', r'''
float radius = length(P.xy);
float radialFootprint = max(fwidth(radius), .001);
// Fine concentric tool marks, not scratches painted onto a diffuse silver face.
float phase = radius * 10.;
float filter = 1. - smoothstep(.12, .55, radialFootprint * 10.);
float grain = sin(phase) * filter;
float broad = sin(P.x*.39 + sin(P.y*.28)) * sin(P.y*.47);
float roughness = clamp(Rough + Detail*(grain*.014 + broad*.008), .12, .6);
float2 radial = P.xy / max(radius, .01);
float2 n = radial * cos(phase) * filter * Detail * .009;
return float4(n, sqrt(saturate(1.-dot(n,n))), roughness);
''')
for node, name in [(position, 'P'), (coated(rough, parameter('FaceCoatingRoughness', settings['Core']['roughness'])), 'Rough'), (detail, 'Detail')]:
    connect(node, finish, name)
for mask, channels, prop in [('Normal', (True, True, True, False), u.MaterialProperty.MP_NORMAL),
                              ('Roughness', (False, False, False, True), u.MaterialProperty.MP_ROUGHNESS)]:
    node = edit.create_material_expression(material, u.MaterialExpressionComponentMask)
    for channel, enabled in zip(['r', 'g', 'b', 'a'], channels):
        node.set_editor_property(channel, enabled)
    connect(finish, node, '')
    assert edit.connect_material_property(node, '', prop)

# Only LEDs emit. SurfaceLift remains a compatible parameter, default zero;
# neutral metal no longer supplies a constant fake white reflection.
emission = edit.create_material_expression(material, u.MaterialExpressionCustom)
emission.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
emission.set_editor_property('code', 'return pow(max(Team.rgb, 0.), HuePurity) * Emission + Base.rgb * SurfaceLift;')
inputs = []
for name in ['Team', 'Emission', 'Base', 'SurfaceLift', 'HuePurity']:
    item = u.CustomInput()
    item.set_editor_property('input_name', name)
    inputs.append(item)
emission.set_editor_property('inputs', inputs)
for node, name in [(parameter('TeamColor', u.LinearColor(0, .55, 1, 1)), 'Team'),
                   (parameter('Emission', 0.), 'Emission'), (base, 'Base'),
                   (parameter('SurfaceLift', 0.), 'SurfaceLift'),
                   (parameter('LEDHuePurity', 1.65), 'HuePurity')]:
    connect(node, emission, name)
assert edit.connect_material_property(emission, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
edit.recompile_material(material)
assert u.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)

assets = {
    'Housing': 'MI_01_Graphite_anodized_housing', 'Crown': 'MI_02_Circular_brushed_silver',
    'Core': 'MI_03_Recessed_dark_titanium', 'Diffuser': 'MI_04_Cyan_light_diffuser',
    'Gasket': 'MI_05_Black_gasket_And_sockets', 'Edges': 'MI_06_Machined_edge_highlights',
    'Emblem': 'MI_07_Cyan_center_emblem'}
report = []
for role, name in assets.items():
    instance = u.load_asset(folder + '/' + name)
    assert isinstance(instance, u.MaterialInstanceConstant), name
    spec = settings[role]
    # The dark face is a satin-coated insert, not the polished silver crown.
    # A separate dielectric F0 prevents a grazing softbox from bleaching it.
    for key, value in [('Metallic', spec['metallic']), ('Roughness', spec['roughness']),
                       ('Specular', spec.get('specular', .5)),
                       ('Anisotropy', spec['anisotropy']), ('FinishColorScale', spec['color_scale']),
                       ('FinishDetail', spec['detail']), ('FinishFaceCoating', spec.get('coat_center', 0.)), ('SurfaceLift', 0.)]:
        edit.set_material_instance_scalar_parameter_value(instance, key, value)
    # Runtime overrides LED energy on selection/hit. This is the authored idle.
    edit.set_material_instance_scalar_parameter_value(instance, 'Emission', 3.2 if role in ['Diffuser', 'Emblem'] else 0.)
    edit.update_material_instance(instance)
    assert u.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
    report.append({'asset': instance.get_path_name(), 'role': role, **spec})
for name, role in [('MI_Orange_Light_Diffuser', 'Diffuser'), ('MI_Orange_Center_Emblem', 'Emblem')]:
    instance = u.load_asset('/Game/TestArena/Pucks/HighDetail/' + name)
    assert isinstance(instance, u.MaterialInstanceConstant), name
    spec = settings[role]
    for key, value in [('Emission', 3.2), ('SurfaceLift', 0.), ('FinishDetail', 0.),
                       ('Metallic', spec['metallic']), ('Roughness', spec['roughness']),
                       ('Specular', spec.get('specular', .5)), ('FinishFaceCoating', 0.)]:
        edit.set_material_instance_scalar_parameter_value(instance, key, value)
    assert u.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
(root / 'Saved/PuckFinishReport.json').write_text(json.dumps(report, indent=2))
# Audit the bindings too: historical mesh paths can carry older per-slot assets.
bindings = []
for name in ['Standard', 'Heavy', 'Striker', 'Grippy', 'Slider', 'Blocker', 'Compact', 'Bouncer', 'Toppler']:
    path = (folder + '/SM_Puck_Standard_Blue_Prototype' if name == 'Standard' else
            '/Game/TestArena/Pucks/HighDetail/SM_Puck_' + name + '_HighDetail')
    mesh = u.load_asset(path)
    assert isinstance(mesh, u.StaticMesh), path
    for slot in mesh.get_editor_property('static_materials'):
        instance = slot.get_editor_property('material_interface')
        bindings.append({'class': name, 'slot': str(slot.get_editor_property('material_slot_name')),
                         'material': instance.get_path_name(),
                         **{key: edit.get_material_instance_scalar_parameter_value(instance, key)
                            for key in ['Metallic', 'Roughness', 'Specular']}})
(root / 'Saved/PuckFinishBindings.json').write_text(json.dumps(bindings, indent=2))
u.log('FLICK_PUCK_FINISH_COMPLETE')
