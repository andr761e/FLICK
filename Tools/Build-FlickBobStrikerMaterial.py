"""Build the texture-free, surface-integrated BOB striker face in Unreal.

Only this material is authored; existing puck meshes/palettes are untouched.
The parent lives in the already-cooked puck cosmetics directory.
"""
import unreal as u

directory = "/Game/Cosmetics/Pucks/BOB"
name = "M_BobStrikerFace"
u.EditorAssetLibrary.make_directory(directory)
material = u.load_asset(directory + "/" + name)
if not material:
    material = u.AssetToolsHelpers.get_asset_tools().create_asset(
        name, directory, u.Material, u.MaterialFactoryNew())
edit = u.MaterialEditingLibrary
edit.delete_all_material_expressions(material)
material.set_editor_property("blend_mode", u.BlendMode.BLEND_OPAQUE)
material.set_editor_property("shading_model", u.MaterialShadingModel.MSM_DEFAULT_LIT)


def parameter(kind, name, value):
    node = edit.create_material_expression(material, kind)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


# Rest-space coordinates keep the mark attached to the puck under movement,
# rotation, arrival animation and replay. No UV-layout or texture dependency.
rest_position = edit.create_material_expression(material, u.MaterialExpressionPreSkinnedPosition)
position = edit.create_material_expression(material, u.MaterialExpressionVertexInterpolator)
assert edit.connect_material_expressions(rest_position, "", position, "VS")
radius = parameter(u.MaterialExpressionScalarParameter, "FaceRadius", 28.0)
pearl = parameter(u.MaterialExpressionVectorParameter, "BaseColor", u.LinearColor(.90, .92, .88, 1))
ink = parameter(u.MaterialExpressionVectorParameter, "InkColor", u.LinearColor(.012, .022, .032, 1))
accent = parameter(u.MaterialExpressionVectorParameter, "AccentColor", u.LinearColor(.02, .65, 1, 1))
housing = parameter(u.MaterialExpressionVectorParameter, "HousingColor", u.LinearColor(.035, .047, .060, 1))
pattern = edit.create_material_expression(material, u.MaterialExpressionCustom)
pattern.set_editor_property("output_type", u.CustomMaterialOutputType.CMOT_FLOAT3)
inputs = []
for name in ("Position", "Radius", "Pearl", "Ink", "Accent", "Housing"):
    entry = u.CustomInput()
    entry.set_editor_property("input_name", name)
    inputs.append(entry)
pattern.set_editor_property("inputs", inputs)
pattern.set_editor_property("code", """
float2 p = Position.xy / max(Radius, 1.0);
float2 a = abs(p);
float r = length(p);
float aa = max(length(fwidth(p)), .0015);
// Outlined diamond with a small central sight, not a raised cross or label.
float diamond = 1.0 - smoothstep(.035, .035 + aa, abs(a.x + a.y - .49));
float dotMark = 1.0 - smoothstep(.075, .075 + aa, r);
// Four fine index bars and a perimeter hairline frame the face.
float tickX = (1.0 - smoothstep(.018, .018 + aa, a.y))
    * smoothstep(.70 - aa, .70, a.x) * (1.0 - smoothstep(.84, .84 + aa, a.x));
float tickY = (1.0 - smoothstep(.018, .018 + aa, a.x))
    * smoothstep(.70 - aa, .70, a.y) * (1.0 - smoothstep(.84, .84 + aa, a.y));
float rim = 1.0 - smoothstep(.012, .012 + aa, abs(r - .94));
float mark = saturate(max(diamond, max(dotMark, max(rim, max(tickX, tickY)))));
float3 colour = lerp(Pearl.rgb, Ink.rgb, mark);
// A restrained coloured sight dot preserves the equipped skin identity.
float centre = 1.0 - smoothstep(.037, .037 + aa, r);
colour = lerp(colour, Accent.rgb, centre);
// Project one continuous medallion across the source mesh's face, gasket and
// emblem sections. Everything beyond the face remains graphite, including clips.
float face = 1.0 - smoothstep(1.0, 1.0 + aa, r);
return lerp(Housing.rgb, colour, face);
""")
for source, slot in ((position, "Position"), (radius, "Radius"),
                     (pearl, "Pearl"), (ink, "Ink"), (accent, "Accent"), (housing, "Housing")):
    assert edit.connect_material_expressions(source, "", pattern, slot)
assert edit.connect_material_property(pattern, "", u.MaterialProperty.MP_BASE_COLOR)
for name, value, output in (("Metallic", .1, u.MaterialProperty.MP_METALLIC),
                            ("Roughness", .48, u.MaterialProperty.MP_ROUGHNESS),
                            ("Specular", .4, u.MaterialProperty.MP_SPECULAR)):
    node = parameter(u.MaterialExpressionScalarParameter, name, value)
    assert edit.connect_material_property(node, "", output)
edit.recompile_material(material)
assert u.EditorAssetLibrary.save_loaded_asset(material)
u.log("FLICK_BOB_STRIKER_MATERIAL_READY " + material.get_path_name())
