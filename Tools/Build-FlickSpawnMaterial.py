"""Build the texture-free spawn-only HDR shader using Unreal's editor Python API.

No trail, knockout or arena materials are modified. Every connection is asserted.
"""
import unreal as u

path = "/Game/Cosmetics/Spawns/M_PuckSpawn_SoftV1"
material = u.load_asset(path)
if not material:
    u.EditorAssetLibrary.make_directory("/Game/Cosmetics/Spawns")
    material = u.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_PuckSpawn_SoftV1", "/Game/Cosmetics/Spawns", u.Material, u.MaterialFactoryNew())
    material.set_editor_property("blend_mode", u.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property("shading_model", u.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    edit = u.MaterialEditingLibrary
    vertex = edit.create_material_expression(material, u.MaterialExpressionVertexColor)
    glow = edit.create_material_expression(material, u.MaterialExpressionScalarParameter)
    glow.set_editor_property("parameter_name", "GlowStrength")
    glow.set_editor_property("default_value", 4.0)
    emission = edit.create_material_expression(material, u.MaterialExpressionMultiply)
    assert edit.connect_material_expressions(vertex, "", emission, "A")
    assert edit.connect_material_expressions(glow, "", emission, "B")
    assert edit.connect_material_property(emission, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
    uv = edit.create_material_expression(material, u.MaterialExpressionTextureCoordinate)
    shape = edit.create_material_expression(material, u.MaterialExpressionScalarParameter)
    shape.set_editor_property("parameter_name", "MaskShape")
    shape.set_editor_property("default_value", 0.0)
    time = edit.create_material_expression(material, u.MaterialExpressionTime)
    mask = edit.create_material_expression(material, u.MaterialExpressionCustom)
    mask.set_editor_property("output_type", u.CustomMaterialOutputType.CMOT_FLOAT1)
    inputs = []
    for name in ["UV", "Shape", "FlowTime"]:
        item = u.CustomInput()
        item.set_editor_property("input_name", name)
        inputs.append(item)
    mask.set_editor_property("inputs", inputs)
    mask.set_editor_property("code", """
float2 p = UV * 2.0 - 1.0;
if (Shape > 6.5) {
    float h = saturate(1.0-UV.y);
    float sway = sin(h*8.0 + FlowTime*4.0 + UV.x*2.0)*.22*h;
    float width = max(.02,pow(1.0-h,.72)*.9);
    float body = pow(saturate(1.0-abs(p.x+sway)/width),1.2);
    float detail = .7+.3*sin(h*31.0-p.x*8.0-FlowTime*6.0);
    return body*smoothstep(0.0,.06,h)*pow(1.0-h,.3)*detail;
}
if (Shape > 5.5) {
    float warp = sin(UV.x*27.0 - FlowTime*4.0 + p.y*5.0)*0.2;
    float noise = .55 + .22*sin(UV.x*47.0 + p.y*13.0 - FlowTime*6.0)
        + .15*sin(UV.x*83.0 - p.y*19.0 + FlowTime*4.0);
    return pow(saturate(1.0-abs(p.y+warp)),1.5) * smoothstep(.12,.75,noise);
}
if (Shape < .5) return pow(saturate(1.0-abs(p.y)),1.6);
if (Shape < 1.5) return pow(saturate(1.0-dot(p,p)),2.0);
if (Shape < 2.5) return smoothstep(1.0,.72,abs(p.x)+abs(p.y));
if (Shape < 3.5) return smoothstep(.98,.8,max(abs(p.x),abs(p.y)));
if (Shape < 4.5) {
    p *= 1.3; p.y = -p.y + .15;
    float q = dot(p,p)-1.0;
    return 1.0-smoothstep(-.05,.06,q*q*q-p.x*p.x*p.y*p.y*p.y);
}
float cross = max(pow(saturate(1.0-abs(p.x)*8.0),2.0)*(1.0-abs(p.y)),
                  pow(saturate(1.0-abs(p.y)*8.0),2.0)*(1.0-abs(p.x)));
return saturate(cross + pow(saturate(1.0-dot(p,p)*5.0),3.0));
""")
    for source, name in [(uv, "UV"), (shape, "Shape"), (time, "FlowTime")]:
        assert edit.connect_material_expressions(source, "", mask, name)
    fade = edit.create_material_expression(material, u.MaterialExpressionMultiply)
    assert edit.connect_material_expressions(vertex, "A", fade, "A")
    assert edit.connect_material_expressions(mask, "", fade, "B")
    assert edit.connect_material_property(fade, "", u.MaterialProperty.MP_OPACITY)
# Exposure-independent emission keeps arrivals readable under the player's lighting
# settings without raising exposure, adding lights, or washing out the whole arena.
material.set_editor_property("translucency_pass", u.MaterialTranslucencyPass.MTP_BEFORE_DOF)
edit = u.MaterialEditingLibrary
nodes = list(edit.get_material_expressions(material))
mask = next(n for n in nodes if isinstance(n, u.MaterialExpressionCustom))
code = mask.get_editor_property("code")
if "Shape > 6.5" not in code:
    code = code.replace("float2 p = UV * 2.0 - 1.0;", """float2 p = UV * 2.0 - 1.0;
if (Shape > 6.5) {
    float h = saturate(1.0-UV.y);
    float sway = sin(h*8.0 + FlowTime*4.0 + UV.x*2.0)*.22*h;
    float width = max(.02,pow(1.0-h,.72)*.9);
    float body = pow(saturate(1.0-abs(p.x+sway)/width),1.2);
    float detail = .7+.3*sin(h*31.0-p.x*8.0-FlowTime*6.0);
    return body*smoothstep(0.0,.06,h)*pow(1.0-h,.3)*detail;
}
""")
    mask.set_editor_property("code", code)
vertex = next(n for n in nodes if isinstance(n, u.MaterialExpressionVertexColor))
glow = next(n for n in nodes if isinstance(n, u.MaterialExpressionScalarParameter)
            and str(n.get_editor_property("parameter_name")) == "GlowStrength")
glow.set_editor_property("default_value", 4.0)
emission = next(n for n in nodes if isinstance(n, u.MaterialExpressionMultiply)
                and glow in edit.get_inputs_for_material_expression(material, n))
saturation = next((n for n in nodes if isinstance(n, u.MaterialExpressionPower)), None)
if not saturation:
    saturation = edit.create_material_expression(material, u.MaterialExpressionPower)
    saturation.set_editor_property("const_exponent", 1.8)
assert edit.connect_material_expressions(vertex, "", saturation, "Base")
assert edit.connect_material_expressions(saturation, "", emission, "A")
inverse = next((n for n in nodes if isinstance(n, u.MaterialExpressionEyeAdaptationInverse)), None)
if not inverse:
    inverse = edit.create_material_expression(material, u.MaterialExpressionEyeAdaptationInverse)
input_names = edit.get_material_expression_input_names(inverse)
assert edit.connect_material_expressions(emission, "", inverse, input_names[0])
assert edit.connect_material_property(inverse, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
edit.recompile_material(material)
assert u.EditorAssetLibrary.save_loaded_asset(material)
u.log("FLICK spawn material ready: " + path)
