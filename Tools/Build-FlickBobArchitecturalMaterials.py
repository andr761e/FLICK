"""Author BOB's material-only ivory/walnut/foundry presentation.

Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>. This intentionally
does NOT call either BOB importer: it saves only the two existing material
parents and their eighteen existing instances. No mesh, collision, physical
material, transform, marking, puck, texture or gameplay asset is edited.

FinishType: 0 = plain lacquer, 1 = sealed maple, 2 = walnut, 3 = satin stone,
4 = brushed metal. All detail is world-space, derivative-filtered and shallow;
there is no displacement or world-position offset.
"""

import unreal as u


edit = u.MaterialEditingLibrary
ARENA = "/Game/BOB/Arena"
STADIUM = "/Game/BOB/Stadium"

# Base color, metallic, roughness, specular, emission, roughness variation,
# finish type, detail, normal strength. Values are linear, not sRGB swatches.
ARENA_SPECS = {
    "01_BOB_Surface": ((.68, .50, .29), .00, .26, .48, .00, .060, 1, .90, .65),
    "02_BOB_Wood": ((.120, .044, .019), .00, .28, .48, .00, .065, 2, .95, .75),
    "03_BOB_InnerWall": ((.24, .12, .046), .00, .30, .48, .00, .045, 2, .70, .55),
    "04_BOB_Red": ((.13, .002, .004), .00, .35, .48, .00, .004, 0, .15, .00),
    "05_BOB_PocketRed": ((.48, .004, .012), .06, .27, .48, .22, .004, 0, .10, .00),
    "06_BOB_PocketVoid": ((.003, .002, .003), .00, .70, .28, .00, .002, 0, .10, .00),
    "07_BOB_Silver": ((.28, .17, .075), .88, .25, .52, .00, .016, 4, .65, .35),
    "08_BOB_DarkMetal": ((.022, .025, .029), .72, .29, .50, .00, .014, 4, .40, .30),
    "09_BOB_PocketBottom": ((.016, .0015, .003), .00, .60, .32, .00, .003, 0, .10, .00),
}
STADIUM_SPECS = {
    "01_Foundry_Black": ((.020, .023, .028), .42, .35, .48, .00, .018, 4, .45, .35),
    "02_Blue_Steel": ((.031, .039, .047), .74, .30, .50, .00, .016, 4, .55, .35),
    "03_Gunmetal": ((.13, .14, .145), .83, .26, .50, .00, .014, 4, .55, .35),
    "04_Smoked_Concrete": ((.055, .051, .046), .04, .45, .48, .00, .060, 3, .90, .80),
    "05_Aged_Brass": ((.48, .31, .13), .86, .26, .50, .00, .016, 4, .65, .35),
    "06_Warm_Pocket_Light": ((1.0, .53, .20), .00, .23, .50, 4.0, .000, 0, .00, .00),
    "07_Cyan_Score_Light": ((.035, .68, 1.0), .00, .23, .50, 4.5, .000, 0, .00, .00),
    "08_Orange_Score_Light": ((1.0, .27, .035), .00, .23, .50, 4.5, .000, 0, .00, .00),
    "09_Light_Composite": ((.040, .043, .046), .08, .44, .48, .00, .045, 3, .95, .85),
}
GROUPS = (
    (ARENA, "M_BobArena_Premium", ARENA_SPECS),
    (STADIUM, "M_BobStadiumSurface", STADIUM_SPECS),
)
# A standalone geometry importer reapplies only its own group's finish. The
# normal authoring commandlet (no supplied filter) updates both groups.
requested_directories = globals().get("bob_material_directories", (ARENA, STADIUM))
if not requested_directories or any(directory not in (ARENA, STADIUM) for directory in requested_directories):
    raise RuntimeError("BOB material authoring accepts only the arena/stadium material groups")
GROUPS = tuple(group for group in GROUPS if group[0] in requested_directories)

# Fail before writing anything if this checkout lacks the expected imported
# materials. Missing assets are an import/setup error, not permission to import
# geometry or to silently create parallel dependencies.
loaded = {}
for directory, parent_name, specs in GROUPS:
    parent_path = directory + "/" + parent_name
    parent = u.load_asset(parent_path)
    if not isinstance(parent, u.Material):
        raise RuntimeError("Expected existing BOB material parent: " + parent_path)
    loaded[parent_path] = parent
    for slot in specs:
        instance_path = directory + "/MI_" + slot
        instance = u.load_asset(instance_path)
        if not isinstance(instance, u.MaterialInstanceConstant):
            raise RuntimeError("Expected existing BOB material instance: " + instance_path)
        loaded[instance_path] = instance


NOISE = r"""
struct BobFinish {
 float hash(float2 p) { return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
 float noise(float2 p) {
  float2 i=floor(p), t=frac(p); t=t*t*(3.-2.*t);
  return lerp(lerp(hash(i),hash(i+float2(1,0)),t.x),
              lerp(hash(i+float2(0,1)),hash(i+1.),t.x),t.y);
 }
};
BobFinish f;
float3 p=P/max(GrainScale,.1);
float3 n=normalize(N);
// Stable world projection. Frame rails use grain along their long dimension;
// side faces use height for the across-grain direction, not stretched UVs.
float2 q=abs(n.z)>.65 ? p.xy : (abs(n.x)>abs(n.y) ? p.yz : p.xz);
if (Type>1.5 && Type<2.5 && abs(n.z)>.65)
 q=abs(p.x)>abs(p.y) ? p.yx : p.xy;
float footprint=max(length(ddx(q)),length(ddy(q)));
"""

PATTERN = NOISE + r"""
float shade=0., wear=0.;
if (Type>.5 && Type<2.5) {
 // Fine, long wood fibres. A low-amplitude warp avoids perfect machine lines
 // without turning an ivory playing bed into cloudy marble/camouflage.
 float warp=(f.noise(float2(q.x*.005,q.y*.025))-.5)*1.5;
 float fibres=(f.noise(float2(q.x*.015,q.y*.24+warp))-.5)
  *exp2(-footprint*footprint*.06);
 float grainCoord=q.y*4.5+warp*2.;
 float fine=sin(grainCoord)*exp2(-2.*abs(fwidth(grainCoord)));
 float longGrain=f.noise(float2(q.x*.003,q.y*.065))-.5;
 float woodContrast=Type<1.5 ? .24 : .85;
 shade=woodContrast*(fibres*.80+longGrain*.20)+fine*(Type<1.5 ? .010 : .026);
 wear=fibres*.65+longGrain*.35;
} else if (Type>2.5 && Type<3.5) {
 // Sealed fine-grain charcoal stone: mostly roughness variation, not strongly
 // painted cloud patterns. Suppress the finest octave as it becomes subpixel.
 float coarse=f.noise(q*.075)-.5;
 float fine=(f.noise(q*.26)-.5)*exp2(-footprint*footprint*.08);
 shade=coarse*.14+fine*.10;
 wear=coarse*.7+fine*.3;
} else if (Type>3.5) {
 float brushCoord=q.y*5.5;
 float brush=sin(brushCoord)*exp2(-2.*abs(fwidth(brushCoord)));
 shade=(f.noise(q*.024)-.5)*.025+brush*.005;
 wear=(f.noise(q*.06)-.5)*.65+brush*.15;
}
float3 color=Base.rgb*(1.+shade*Detail);
float roughness=clamp(Roughness+wear*Variation*Detail,.16,.95);
return float4(color,roughness);
"""

NORMAL = NOISE + r"""
float height=0.;
if (Type>.5 && Type<2.5) {
 float warp=(f.noise(float2(q.x*.005,q.y*.025))-.5)*1.5;
 float grain=f.noise(float2(q.x*.015,q.y*.24+warp));
 height=grain*.08*exp2(-footprint*footprint*.06);
} else if (Type>2.5 && Type<3.5) {
 height=f.noise(q*.075)*.24
       +f.noise(q*.26)*.065*exp2(-footprint*footprint*.08);
} else if (Type>3.5) {
 height=f.noise(q*.06)*.0015;
}
// Surface-gradient normal perturbation only. Its millimetre-scale height is
// virtual: it never moves a vertex or changes puck contact/friction.
float3 dx=ddx(P),dy=ddy(P),a=cross(dy,n),b=cross(n,dx);
float determinant=dot(dx,a);
float3 gradient=(a*ddx(height)+b*ddy(height))*sign(determinant)
 /max(abs(determinant),.0001);
return normalize(n-gradient*Detail*Strength);
"""


def connect(node, target, slot, output=""):
    if not edit.connect_material_expressions(node, output, target, slot):
        raise RuntimeError("Cannot connect BOB material expression: " + slot)


def output(node, prop, pin=""):
    if not edit.connect_material_property(node, pin, prop):
        raise RuntimeError("Cannot connect BOB material property: " + str(prop))


def author_parent(material):
    u.log("FLICK_BOB_MATERIAL_GRAPH_BEGIN " + material.get_path_name())
    # BOB's meshes/materials are loaded by CDO constructors during initial
    # startup. Their expressions are rooted, so UE's deletion API asserts in a
    # commandlet. Update/reconnect in place instead: preserve existing parameter
    # nodes and identify our helper nodes by stable descriptions on every run.
    # The original importer's few unused math/noise nodes remain editor-only;
    # disconnected nodes have no shader cost, and reruns do not accumulate more.
    existing = list(edit.get_material_expressions(material))
    helper_nodes = {str(node.get_editor_property("desc")): node for node in existing}
    parameter_nodes = {
        str(node.get_editor_property("parameter_name")): node for node in existing
        if isinstance(node, (u.MaterialExpressionScalarParameter, u.MaterialExpressionVectorParameter))
    }

    def expression(kind, key):
        description = "FLICK.BOBFinish." + key
        node = helper_nodes.get(description)
        if node is not None and not isinstance(node, kind):
            raise RuntimeError("Unexpected existing BOB finish expression class: " + description)
        if node is None:
            node = edit.create_material_expression(material, kind)
            node.set_editor_property("desc", description)
            helper_nodes[description] = node
        return node

    material.set_editor_property("shading_model", u.MaterialShadingModel.MSM_DEFAULT_LIT)
    material.set_editor_property("tangent_space_normal", False)
    material.set_editor_property("used_with_instanced_static_meshes", True)

    def parameter(name, value):
        kind = u.MaterialExpressionVectorParameter if isinstance(value, u.LinearColor) else u.MaterialExpressionScalarParameter
        node = parameter_nodes.get(name)
        if node is not None and not isinstance(node, kind):
            raise RuntimeError("Unexpected existing BOB parameter class: " + name)
        if node is None:
            node = expression(kind, "Parameter." + name)
            parameter_nodes[name] = node
        node.set_editor_property("parameter_name", name)
        node.set_editor_property("default_value", value)
        return node

    def custom(code, output_type, sources, key):
        node = expression(u.MaterialExpressionCustom, key)
        node.set_editor_property("code", code)
        node.set_editor_property("output_type", output_type)
        inputs = []
        for name in sources:
            entry = u.CustomInput()
            entry.set_editor_property("input_name", name)
            inputs.append(entry)
        node.set_editor_property("inputs", inputs)
        for name, source in sources.items():
            connect(source, node, name)
        return node

    base = parameter("BaseColor", u.LinearColor(.3, .25, .18, 1))
    roughness = parameter("Roughness", .3)
    position = expression(u.MaterialExpressionWorldPosition, "WorldPosition")
    vertex_normal = expression(u.MaterialExpressionVertexNormalWS, "VertexNormal")
    detail = parameter("FinishDetail", .7)
    sources = {
        "P": position, "N": vertex_normal, "Base": base,
        "Type": parameter("FinishType", 0.), "Detail": detail,
        "GrainScale": parameter("GrainScale", 1.),
        "Roughness": roughness, "Variation": parameter("RoughnessVariation", .02),
    }
    pattern = custom(PATTERN, u.CustomMaterialOutputType.CMOT_FLOAT4, sources, "Pattern")
    mask = expression(u.MaterialExpressionComponentMask, "BaseColorMask")
    for channel in ("r", "g", "b"):
        mask.set_editor_property(channel, True)
    mask.set_editor_property("a", False)
    connect(pattern, mask, "")
    output(mask, u.MaterialProperty.MP_BASE_COLOR)
    alpha = expression(u.MaterialExpressionComponentMask, "RoughnessMask")
    alpha.set_editor_property("r", False)
    alpha.set_editor_property("g", False)
    alpha.set_editor_property("b", False)
    alpha.set_editor_property("a", True)
    connect(pattern, alpha, "")
    output(alpha, u.MaterialProperty.MP_ROUGHNESS)
    for name, default, prop in (
        ("Metallic", .0, u.MaterialProperty.MP_METALLIC),
        ("Specular", .48, u.MaterialProperty.MP_SPECULAR),
    ):
        output(parameter(name, default), prop)
    normal_sources = {name: sources[name] for name in ("P", "N", "Type", "Detail", "GrainScale")}
    normal_sources["Strength"] = parameter("NormalStrength", .5)
    output(custom(NORMAL, u.CustomMaterialOutputType.CMOT_FLOAT3, normal_sources, "Normal"), u.MaterialProperty.MP_NORMAL)

    emissive = expression(u.MaterialExpressionMultiply, "VisibleEmission")
    connect(base, emissive, "A")
    connect(parameter("Emission", 0.), emissive, "B")
    bounded_emissive = expression(u.MaterialExpressionMultiply, "IndirectEmission")
    connect(emissive, bounded_emissive, "A")
    connect(parameter("IndirectEmissionScale", .25), bounded_emissive, "B")
    switch = expression(u.MaterialExpressionRayTracingQualitySwitch, "EmissionPaths")
    connect(emissive, switch, "Normal")
    connect(bounded_emissive, switch, "RayTraced")
    output(switch, u.MaterialProperty.MP_EMISSIVE_COLOR)
    edit.recompile_material(material)
    if not u.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError("Cannot save BOB material parent: " + material.get_path_name())
    u.log("FLICK_BOB_MATERIAL_GRAPH_READY " + material.get_path_name()
          + " expressions=" + str(edit.get_num_material_expressions(material)))


for directory, parent_name, specs in GROUPS:
    parent = loaded[directory + "/" + parent_name]
    author_parent(parent)
    for slot, spec in specs.items():
        rgb, metallic, roughness, specular, emission, variation, finish, detail, normal = spec
        instance = loaded[directory + "/MI_" + slot]
        edit.set_material_instance_parent(instance, parent)
        edit.set_material_instance_vector_parameter_value(instance, "BaseColor", u.LinearColor(*rgb, 1.0))
        for name, value in (
            ("Metallic", metallic), ("Roughness", roughness), ("Specular", specular),
            ("Emission", emission), ("RoughnessVariation", variation),
            ("FinishType", finish), ("FinishDetail", detail), ("NormalStrength", normal),
            ("GrainScale", 1.), ("IndirectEmissionScale", .25),
        ):
            edit.set_material_instance_scalar_parameter_value(instance, name, value)
        if not u.EditorAssetLibrary.save_loaded_asset(instance):
            raise RuntimeError("Cannot save BOB material instance: " + instance.get_path_name())

u.log("FLICK_BOB_ARCHITECTURAL_MATERIALS_READY: "
      + str(len(GROUPS)) + " parents, " + str(sum(len(group[2]) for group in GROUPS))
      + " existing instances; no mesh changes")
