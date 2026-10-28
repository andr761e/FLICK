"""Shared authoring for the three FX shaders, soft atmosphere and solid facets.

Uses existing asset paths. No imported textures or runtime Python dependency.
"""
from pathlib import Path
import unreal as u


def build(path, atmosphere=False):
    edit = u.MaterialEditingLibrary
    material = u.load_asset(path)
    if not material:
        folder, name = path.rsplit('/', 1)
        u.EditorAssetLibrary.make_directory(folder)
        material = u.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, u.Material, u.MaterialFactoryNew())
    edit.delete_all_material_expressions(material)
    material.set_editor_property('blend_mode', u.BlendMode.BLEND_TRANSLUCENT if atmosphere else u.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property('two_sided', True)
    material.set_editor_property('translucency_pass', u.MaterialTranslucencyPass.MTP_BEFORE_DOF)

    def node(kind):
        return edit.create_material_expression(material, kind)

    def wire(source, target, slot, output=''):
        assert edit.connect_material_expressions(source, output, target, slot)

    def scalar(name, value):
        item = node(u.MaterialExpressionScalarParameter)
        item.set_editor_property('parameter_name', name)
        item.set_editor_property('default_value', value)
        return item

    vertex = node(u.MaterialExpressionVertexColor)
    uv = node(u.MaterialExpressionTextureCoordinate)
    shape = scalar('MaskShape', 8. if atmosphere else 0.)
    age = scalar('EffectAge', 0.)
    mask = node(u.MaterialExpressionCustom)
    mask.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT4)
    inputs = []
    for name in ['UV', 'Shape', 'FlowTime']:
        entry = u.CustomInput()
        entry.set_editor_property('input_name', name)
        inputs.append(entry)
    mask.set_editor_property('inputs', inputs)
    mask.set_editor_property('code', (Path(__file__).parent / 'Shaders/FlickCosmeticMasks.hlsl').read_text())
    for source, name in [(uv, 'UV'), (shape, 'Shape'), (age, 'FlowTime')]:
        wire(source, mask, name)
    shade = node(u.MaterialExpressionComponentMask)
    shade.set_editor_property('r', True)
    shade.set_editor_property('g', True)
    shade.set_editor_property('b', True)
    shade.set_editor_property('a', False)
    wire(mask, shade, '')
    coverage = node(u.MaterialExpressionComponentMask)
    for channel in ['r', 'g', 'b']:
        coverage.set_editor_property(channel, False)
    coverage.set_editor_property('a', True)
    wire(mask, coverage, '')
    power = node(u.MaterialExpressionPower)
    power.set_editor_property('const_exponent', 1.65 if not atmosphere else 1.)
    wire(vertex, power, 'Base')
    shaded = node(u.MaterialExpressionMultiply)
    wire(power, shaded, 'A')
    wire(shade, shaded, 'B')
    emission = node(u.MaterialExpressionMultiply)
    wire(shaded, emission, 'A')
    wire(scalar('GlowStrength', .8 if atmosphere else 5.), emission, 'B')
    inverse = node(u.MaterialExpressionEyeAdaptationInverse)
    wire(emission, inverse, edit.get_material_expression_input_names(inverse)[0])
    assert edit.connect_material_property(inverse, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    fade = node(u.MaterialExpressionMultiply)
    wire(vertex, fade, 'A', 'A')
    wire(coverage, fade, 'B')
    # Soft intersections with the puck/floor, while retaining normal depth tests.
    depth = node(u.MaterialExpressionDepthFade)
    depth.set_editor_property('fade_distance_default', 8.)
    wire(fade, depth, edit.get_material_expression_input_names(depth)[0])
    assert edit.connect_material_property(depth, '', u.MaterialProperty.MP_OPACITY)
    edit.recompile_material(material)
    assert u.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    u.log('FLICK premium cosmetic material ready: ' + path)


def build_facets():
    """Depth-writing, shaded crystal/foil faces. Glints remain separate ribbons."""
    edit = u.MaterialEditingLibrary
    folder, name = '/Game/Cosmetics/Knockouts', 'M_PuckFacet'
    material = u.load_asset(folder + '/' + name)
    if not material:
        material = u.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, u.Material, u.MaterialFactoryNew())
    edit.delete_all_material_expressions(material)
    material.set_editor_property('blend_mode', u.BlendMode.BLEND_MASKED)
    material.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property('two_sided', True)
    vertex = edit.create_material_expression(material, u.MaterialExpressionVertexColor)
    position = edit.create_material_expression(material, u.MaterialExpressionWorldPosition)
    view = edit.create_material_expression(material, u.MaterialExpressionCameraVectorWS)
    surface = edit.create_material_expression(material, u.MaterialExpressionCustom)
    surface.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT4)
    inputs = []
    for name in ['P', 'V', 'Colour', 'Fade']:
        item = u.CustomInput()
        item.set_editor_property('input_name', name)
        inputs.append(item)
    surface.set_editor_property('inputs', inputs)
    surface.set_editor_property('code', '''
float3 N = cross(ddx(P),ddy(P)); N *= rsqrt(max(dot(N,N),1.e-10));
V = normalize(V); N *= dot(N,V) < 0. ? -1. : 1.;
float3 L = normalize(float3(-.45,-.3,.84));
float diffuse = .28 + .66*saturate(dot(N,L));
float highlight = pow(saturate(dot(N,normalize(L+V))),38.);
float edge = pow(1.-saturate(dot(N,V)),4.);
float3 tint = lerp(Colour,float3(.45,.7,.9),.24);
float3 metal = tint*diffuse + float3(.7,.87,1.)*highlight*1.25 + tint*edge*.35;
float grain = frac(sin(dot(floor(P*4.),float3(12.9898,78.233,37.719)))*43758.5453);
return float4(metal,step(grain,saturate(Fade*3.)));
''')
    for source, output, slot in [(position, '', 'P'), (view, '', 'V'), (vertex, '', 'Colour'), (vertex, 'A', 'Fade')]:
        assert edit.connect_material_expressions(source, output, surface, slot)
    rgb = edit.create_material_expression(material, u.MaterialExpressionComponentMask)
    for channel in ['r', 'g', 'b', 'a']:
        rgb.set_editor_property(channel, channel != 'a')
    assert edit.connect_material_expressions(surface, '', rgb, '')
    gain = edit.create_material_expression(material, u.MaterialExpressionScalarParameter)
    gain.set_editor_property('parameter_name', 'GlowStrength')
    gain.set_editor_property('default_value', 1.35)
    emission = edit.create_material_expression(material, u.MaterialExpressionMultiply)
    assert edit.connect_material_expressions(rgb, '', emission, 'A')
    assert edit.connect_material_expressions(gain, '', emission, 'B')
    inverse = edit.create_material_expression(material, u.MaterialExpressionEyeAdaptationInverse)
    assert edit.connect_material_expressions(emission, '', inverse, edit.get_material_expression_input_names(inverse)[0])
    assert edit.connect_material_property(inverse, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    coverage = edit.create_material_expression(material, u.MaterialExpressionComponentMask)
    for channel in ['r', 'g', 'b', 'a']:
        coverage.set_editor_property(channel, channel == 'a')
    assert edit.connect_material_expressions(surface, '', coverage, '')
    assert edit.connect_material_property(coverage, '', u.MaterialProperty.MP_OPACITY_MASK)
    edit.recompile_material(material)
    assert u.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    u.log('FLICK shaded cosmetic facets ready: ' + folder + '/M_PuckFacet')
