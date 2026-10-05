"""Reproducible trail-only shader. Run with UnrealEditor-Cmd -ExecutePythonScript.

No textures, Niagara systems, or external assets. Vertex RGB encodes the palette;
vertex alpha encodes lifetime. A small analytic mask feathers ribbons and sprites.
Existing graphs are reused on reruns and the vertex-colour connection is checked.
"""
import unreal as u

path = "/Game/Cosmetics/Trails/M_PuckTrail_SoftV1"
material = u.load_asset(path)
if not material:
    u.EditorAssetLibrary.make_directory("/Game/Cosmetics/Trails")
    material = u.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_PuckTrail_SoftV1", "/Game/Cosmetics/Trails", u.Material, u.MaterialFactoryNew())
    material.set_editor_property("blend_mode", u.BlendMode.BLEND_ADDITIVE)
    material.set_editor_property("shading_model", u.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)
    editing = u.MaterialEditingLibrary
    vertex = editing.create_material_expression(material, u.MaterialExpressionVertexColor)
    energy = editing.create_material_expression(material, u.MaterialExpressionScalarParameter)
    energy.set_editor_property("parameter_name", "GlowStrength")
    energy.set_editor_property("default_value", 5.0)
    multiply = editing.create_material_expression(material, u.MaterialExpressionMultiply)
    # VertexColor's RGB output is unnamed in UE's material scripting API.
    assert editing.connect_material_expressions(vertex, "", multiply, "A")
    assert editing.connect_material_expressions(energy, "", multiply, "B")
    assert editing.connect_material_property(multiply, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
    uv = editing.create_material_expression(material, u.MaterialExpressionTextureCoordinate)
    shape = editing.create_material_expression(material, u.MaterialExpressionScalarParameter)
    shape.set_editor_property("parameter_name", "MaskShape")
    shape.set_editor_property("default_value", 0.0)
    mask = editing.create_material_expression(material, u.MaterialExpressionCustom)
    mask.set_editor_property("output_type", u.CustomMaterialOutputType.CMOT_FLOAT1)
    uv_input, shape_input, flow_input = u.CustomInput(), u.CustomInput(), u.CustomInput()
    uv_input.set_editor_property("input_name", "UV")
    shape_input.set_editor_property("input_name", "Shape")
    flow_input.set_editor_property("input_name", "FlowTime")
    mask.set_editor_property("inputs", [uv_input, shape_input, flow_input])
    mask.set_editor_property("code", """
float2 p = UV * 2.0 - 1.0;
if (Shape > 5.5) {
    float warp = sin(UV.x*25.0 - FlowTime*3.0 + sin(UV.y*6.0 + FlowTime)) * 0.22;
    float noise = 0.5 + 0.22*sin(UV.x*41.0 + p.y*9.0 - FlowTime*5.0)
        + 0.16*sin(UV.x*73.0 - p.y*14.0 + FlowTime*3.0)
        + 0.12*sin(UV.x*113.0 + p.y*23.0 - FlowTime*7.0);
    float feather = pow(saturate(1.0 - abs(p.y + warp)), 1.35);
    return feather * (Shape > 6.5 ? smoothstep(0.18, 0.7, noise) : 0.35 + 0.65*noise);
}
if (Shape < 0.5) return pow(saturate(1.0 - abs(p.y)), 1.35);
if (Shape < 1.5) return pow(saturate(1.0 - dot(p,p)), 2.2);
if (Shape < 2.5) return smoothstep(1.0, 0.72, abs(p.x) + abs(p.y));
if (Shape < 3.5) return smoothstep(0.96, 0.76, max(abs(p.x), abs(p.y)));
if (Shape < 4.5) {
    p *= 1.3; p.y = -p.y + 0.15;
    float q = dot(p,p) - 1.0;
    return 1.0 - smoothstep(-0.05, 0.06, q*q*q - p.x*p.x*p.y*p.y*p.y);
}
float cross = max(pow(saturate(1.0 - abs(p.x)*9.0), 2.0)*(1.0-abs(p.y)),
                  pow(saturate(1.0 - abs(p.y)*9.0), 2.0)*(1.0-abs(p.x)));
return saturate(cross + pow(saturate(1.0-dot(p,p)*5.0), 3.0));
""")
    assert editing.connect_material_expressions(uv, "", mask, "UV")
    assert editing.connect_material_expressions(shape, "", mask, "Shape")
    alpha = editing.create_material_expression(material, u.MaterialExpressionMultiply)
    assert editing.connect_material_expressions(vertex, "A", alpha, "A")
    assert editing.connect_material_expressions(mask, "", alpha, "B")
    assert editing.connect_material_property(alpha, "", u.MaterialProperty.MP_OPACITY)

# Repair the original unnamed-output wiring without replacing/deleting a graph.
editing = u.MaterialEditingLibrary
vertex = next(node for node in editing.get_material_expressions(material)
              if isinstance(node, u.MaterialExpressionVertexColor))
emissive = editing.get_material_property_input_node(material, u.MaterialProperty.MP_EMISSIVE_COLOR)
assert editing.connect_material_expressions(vertex, "", emissive, "A")
mask = next(node for node in editing.get_material_expressions(material)
            if isinstance(node, u.MaterialExpressionCustom))
inputs = list(mask.get_editor_property("inputs"))
if len(inputs) == 2:
    flow_input = u.CustomInput()
    flow_input.set_editor_property("input_name", "FlowTime")
    inputs.append(flow_input)
    mask.set_editor_property("inputs", inputs)
time = next((node for node in editing.get_material_expressions(material)
             if isinstance(node, u.MaterialExpressionTime)), None)
if not time:
    time = editing.create_material_expression(material, u.MaterialExpressionTime)
assert editing.connect_material_expressions(time, "", mask, "FlowTime")
code = mask.get_editor_property("code")
if "Shape > 7.5" not in code:
    code = code.replace("float2 p = UV * 2.0 - 1.0;", """float2 p = UV * 2.0 - 1.0;
if (Shape > 7.5) {
    float h = saturate(1.0 - UV.y);
    float sway = sin(h*10.0 + FlowTime*6.0 + UV.x*3.0)*0.18*h;
    float width = max(0.025, pow(1.0-h, 0.72)*0.86);
    float body = pow(saturate(1.0-abs(p.x+sway)/width), 1.35);
    return body * smoothstep(0.0, 0.13, h) * pow(1.0-h, 0.35);
}
""")
    mask.set_editor_property("code", code)
if "Shape > 5.5" not in code:
    code = code.replace("float2 p = UV * 2.0 - 1.0;", """float2 p = UV * 2.0 - 1.0;
if (Shape > 5.5) {
    float warp = sin(UV.x*25.0 - FlowTime*3.0 + sin(UV.y*6.0 + FlowTime)) * 0.22;
    float noise = 0.5 + 0.22*sin(UV.x*41.0 + p.y*9.0 - FlowTime*5.0)
        + 0.16*sin(UV.x*73.0 - p.y*14.0 + FlowTime*3.0)
        + 0.12*sin(UV.x*113.0 + p.y*23.0 - FlowTime*7.0);
    float feather = pow(saturate(1.0 - abs(p.y + warp)), 1.35);
    return feather * (Shape > 6.5 ? smoothstep(0.18, 0.7, noise) : 0.35 + 0.65*noise);
}
""")
    mask.set_editor_property("code", code)
editing.recompile_material(material)
u.EditorAssetLibrary.save_loaded_asset(material)

u.log("FLICK trail material ready: " + path)
