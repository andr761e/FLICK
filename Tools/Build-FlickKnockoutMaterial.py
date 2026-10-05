"""Rebuild the knockout-only, texture-free shader. Run through Unreal editor Python.

No arena, spawn or trail materials are changed. All graph connections are checked.
"""
import unreal as u

path = "/Game/Cosmetics/Knockouts/M_PuckKnockout_SoftV1"
edit = u.MaterialEditingLibrary
material = u.load_asset(path)
if not material:
    u.EditorAssetLibrary.make_directory("/Game/Cosmetics/Knockouts")
    material = u.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_PuckKnockout_SoftV1", "/Game/Cosmetics/Knockouts", u.Material, u.MaterialFactoryNew())
edit.delete_all_material_expressions(material)
material.set_editor_property("blend_mode", u.BlendMode.BLEND_ADDITIVE)
material.set_editor_property("shading_model", u.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property("two_sided", True)
material.set_editor_property("translucency_pass", u.MaterialTranslucencyPass.MTP_BEFORE_DOF)
vertex = edit.create_material_expression(material, u.MaterialExpressionVertexColor)
power = edit.create_material_expression(material, u.MaterialExpressionPower)
power.set_editor_property("const_exponent", 1.8)
glow = edit.create_material_expression(material, u.MaterialExpressionScalarParameter)
glow.set_editor_property("parameter_name", "GlowStrength")
glow.set_editor_property("default_value", 20.0)
emission = edit.create_material_expression(material, u.MaterialExpressionMultiply)
inverse = edit.create_material_expression(material, u.MaterialExpressionEyeAdaptationInverse)
assert edit.connect_material_expressions(vertex, "", power, "Base")
assert edit.connect_material_expressions(power, "", emission, "A")
assert edit.connect_material_expressions(glow, "", emission, "B")
assert edit.connect_material_expressions(emission, "", inverse, edit.get_material_expression_input_names(inverse)[0])
assert edit.connect_material_property(inverse, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
uv = edit.create_material_expression(material, u.MaterialExpressionTextureCoordinate)
shape = edit.create_material_expression(material, u.MaterialExpressionScalarParameter)
shape.set_editor_property("parameter_name", "MaskShape")
age = edit.create_material_expression(material, u.MaterialExpressionScalarParameter)
age.set_editor_property("parameter_name", "EffectAge")
mask = edit.create_material_expression(material, u.MaterialExpressionCustom)
mask.set_editor_property("output_type", u.CustomMaterialOutputType.CMOT_FLOAT1)
inputs = []
for name in ["UV", "Shape", "FlowTime"]:
    item = u.CustomInput()
    item.set_editor_property("input_name", name)
    inputs.append(item)
mask.set_editor_property("inputs", inputs)
mask.set_editor_property("code", """
float2 p = UV*2.0-1.0;
if (Shape > 8.5) {
    float r = length(p);
    float rim = exp(-pow((r-.7)*15.0,2.0));
    float glint = exp(-dot(p-float2(-.24,-.32),p-float2(-.24,-.32))*60.0);
    return saturate(rim*.65+glint+pow(saturate(1.0-r*r),3.0)*.16);
}
if (Shape > 7.5) {
    float warp = sin(p.x*8.0+FlowTime*2.0)*sin(p.y*9.0-FlowTime*3.0)*.14;
    float body = pow(saturate(1.0-dot(p,p)+warp),2.0);
    float detail = .65+.22*sin(p.x*17.0+p.y*11.0-FlowTime*4.0)
        +.12*sin(p.x*27.0-p.y*19.0+FlowTime*5.0);
    return body*saturate(detail);
}
if (Shape > 6.5) {
    float h = saturate(1.0-UV.y);
    float sway = sin(h*8.0+FlowTime*6.0+UV.x*2.0)*.22*h;
    float width = max(.02,pow(1.0-h,.72)*.9);
    float body = pow(saturate(1.0-abs(p.x+sway)/width),1.2);
    return body*smoothstep(0.0,.06,h)*pow(1.0-h,.3)
        *(.7+.3*sin(h*31.0-p.x*8.0-FlowTime*6.0));
}
if (Shape > 5.5) {
    float warp = sin(UV.x*27.0-FlowTime*4.0+p.y*5.0)*.2;
    float detail = .65+.2*sin(UV.x*47.0+p.y*13.0-FlowTime*6.0)
        +.14*sin(UV.x*83.0-p.y*19.0+FlowTime*4.0);
    return pow(saturate(1.0-abs(p.y+warp)),1.5)*smoothstep(.1,.7,detail);
}
if (Shape < .5) return pow(saturate(1.0-abs(p.y)),1.6);
if (Shape < 1.5) return pow(saturate(1.0-dot(p,p)),2.0);
if (Shape < 2.5) return 1.0-smoothstep(.72,1.0,abs(p.x)+abs(p.y));
if (Shape < 3.5) return 1.0-smoothstep(.8,.98,max(abs(p.x),abs(p.y)));
if (Shape < 4.5) {
    p *= 1.3; p.y = -p.y+.15;
    float q = dot(p,p)-1.0;
    return 1.0-smoothstep(-.05,.06,q*q*q-p.x*p.x*p.y*p.y*p.y);
}
float cross = max(pow(saturate(1.0-abs(p.x)*8.0),2.0)*(1.0-abs(p.y)),
                  pow(saturate(1.0-abs(p.y)*8.0),2.0)*(1.0-abs(p.x)));
return saturate(cross+pow(saturate(1.0-dot(p,p)*5.0),3.0));
""")
for source, name in [(uv, "UV"), (shape, "Shape"), (age, "FlowTime")]:
    assert edit.connect_material_expressions(source, "", mask, name)
fade = edit.create_material_expression(material, u.MaterialExpressionMultiply)
assert edit.connect_material_expressions(vertex, "A", fade, "A")
assert edit.connect_material_expressions(mask, "", fade, "B")
assert edit.connect_material_property(fade, "", u.MaterialProperty.MP_OPACITY)
edit.recompile_material(material)
assert u.EditorAssetLibrary.save_loaded_asset(material)
u.log("FLICK knockout material ready: " + path)
