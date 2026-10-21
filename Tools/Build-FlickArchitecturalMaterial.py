"""Author the texture-free architectural finish; never import/change a mesh.

Only the Knockout presentation opts into this parent. Pucks, switch lenses,
BOB and authored render-layer offsets remain independent.
"""
import unreal as u

path = "/Game/TestArena/Arena"
name = "M_ArchitecturalMetal"
edit = u.MaterialEditingLibrary
material = u.load_asset(path + "/" + name)
if not material:
    material = u.AssetToolsHelpers.get_asset_tools().create_asset(name, path, u.Material, u.MaterialFactoryNew())
edit.delete_all_material_expressions(material)
material.set_editor_property("used_with_instanced_static_meshes", True)
material.set_editor_property("shading_model", u.MaterialShadingModel.MSM_DEFAULT_LIT)
material.set_editor_property("tangent_space_normal", False)


def param(name, value):
    kind = u.MaterialExpressionVectorParameter if isinstance(value, u.LinearColor) else u.MaterialExpressionScalarParameter
    node = edit.create_material_expression(material, kind)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def connect(node, target, slot, output=""):
    assert edit.connect_material_expressions(node, output, target, slot)


base = param("BaseColor", u.LinearColor(.12, .15, .18, 1))
rough = param("Roughness", .32)
detail = param("FinishDetail", .7)
deck = param("DeckFinish", 0.)
collar = param("RimCollarFinish", 0.)
radius = param("ArenaRadius", 650.)
arena_scale = param("ArenaScale", 1.)
surface_z = param("ArenaSurfaceZ", 250.)
lens_count = param("RimLensCount", 28.)
vertex_normal = edit.create_material_expression(material, u.MaterialExpressionVertexNormalWS)
position = edit.create_material_expression(material, u.MaterialExpressionWorldPosition)
pattern = edit.create_material_expression(material, u.MaterialExpressionCustom)
pattern.set_editor_property("output_type", u.CustomMaterialOutputType.CMOT_FLOAT4)
inputs = []
for key in ("P", "N", "Detail", "Deck", "Roughness", "Base", "Collar", "Radius", "ArenaScale", "SurfaceZ", "LensCount", "Band"):
    entry = u.CustomInput()
    entry.set_editor_property("input_name", key)
    inputs.append(entry)
pattern.set_editor_property("inputs", inputs)
pattern.set_editor_property("code", r"""
struct Finish {
 float hash(float2 p) { return frac(sin(dot(p, float2(127.1,311.7))) * 43758.5453); }
 float noise(float2 p) {
  float2 i=floor(p), f=frac(p); f=f*f*(3.-2.*f);
  return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),
              lerp(hash(i+float2(0,1)),hash(i+1.),f.x),f.y);
 }
};
Finish f;
// Irregular, overlapping handling marks rather than one identical scratch in
// every square cell. Variation mainly changes roughness, not painted clouds.
float broad=f.noise(P.xy*.018);
float2 warp=float2(f.noise(P.xy*.008),f.noise(P.xy*.008+19.))*5.;
float scuff=f.noise(float2(P.x*.11+P.y*.035,P.y*.045)+warp);
float wear=smoothstep(.32,.72,scuff)*smoothstep(.23,.65,broad);
float brushCoord=(P.y+P.x*.13)*3.8;
// Fade sub-pixel brushing instead of producing glitter/flickering at distance.
float brush=sin(brushCoord)*exp2(-3.*abs(fwidth(brushCoord)));
float variation=(broad-.5)*.018+(scuff-.5)*.018+brush*.004+Deck*(wear-.3)*.035;
// Sparse anti-aliased hairline scratches; localized, not a tiled cloudy finish.
float2 cell=floor(P.xy/70.);
float2 local=(frac(P.xy/70.)-float2(.25+.5*f.hash(cell),.25+.5*f.hash(cell+13.)))*70.;
float scratchAngle=6.2831853*f.hash(cell+23.);
float2 tangent=float2(cos(scratchAngle),sin(scratchAngle));
float across=abs(dot(local,float2(-tangent.y,tangent.x)));
float along=abs(dot(local,tangent));
float scratch=saturate((.04-across)/max(fwidth(across),.04)+.5)
 *(1.-smoothstep(4.,12.,along))*step(.78,f.hash(cell+41.));
float3 color=Base.rgb*(1.+Detail*((broad-.5)*.035+Deck*(wear-.3)*.05+scratch*.05));
float roughness=clamp(Roughness+Detail*(variation+scratch*.025),.12,.85);
// Existing flat collar faces receive a polished crown and dark lens gaskets.
// This is material shading only: neither vertices nor the collision surface move.
float scale=max(Radius/650.,.1), r=length(P.xy);
float top=Collar*smoothstep(.35,.8,N.z)*smoothstep(-1.8,-.8,(P.z-SurfaceZ)/max(ArenaScale,.1));
float rim=top*smoothstep(Radius-27.*scale,Radius-25.*scale,r);
float edge=abs((r-(Radius-12.*scale))/(12.*scale));
color*=1.+rim*(.14-.24*pow(saturate(edge),5.));
roughness=lerp(roughness,.17+variation*.3,rim*.8);
float angle=atan2(P.y,P.x)/6.2831853;
float arc=abs(frac(angle*LensCount)-.5)*(6.2831853*(Radius-16.*scale)/LensCount);
float2 lens=float2(max(arc-36.*scale,0.),r-(Radius-16.*scale));
float distance=length(lens);
float aa=max(fwidth(distance),.08*scale);
float gasket=rim*(1.-smoothstep(4.0*scale-aa,4.0*scale+aa,distance));
color=lerp(color,float3(.014,.025,.032),gasket*.88);
roughness=lerp(roughness,.2,gasket);
// Amber slots retain an actual dark surround on the same cylindrical wall.
float z=(P.z-SurfaceZ)/max(ArenaScale,.1);
float segment=abs(frac(angle*28.+.5)-.5);
float ends=1.-smoothstep(.305-max(fwidth(segment),.002),.305+max(fwidth(segment),.002),segment);
float recess=Band*ends*(1.-smoothstep(4.1,4.1+max(fwidth(z),.08),abs(z+23.)));
color=lerp(color,float3(.008,.010,.014),recess*.8);
return float4(color,roughness);
""")
side_band = param("RimSideBand", 0.)
for node, slot in ((position, "P"), (vertex_normal, "N"), (detail, "Detail"), (deck, "Deck"), (rough, "Roughness"), (base, "Base"),
                   (collar, "Collar"), (radius, "Radius"), (arena_scale, "ArenaScale"), (surface_z, "SurfaceZ"),
                   (lens_count, "LensCount"), (side_band, "Band")):
    connect(node, pattern, slot)
mask = edit.create_material_expression(material, u.MaterialExpressionComponentMask)
mask.set_editor_property("r", True)
mask.set_editor_property("g", True)
mask.set_editor_property("b", True)
connect(pattern, mask, "")
assert edit.connect_material_property(mask, "", u.MaterialProperty.MP_BASE_COLOR)
alpha = edit.create_material_expression(material, u.MaterialExpressionComponentMask)
alpha.set_editor_property("r", False)
alpha.set_editor_property("g", False)
alpha.set_editor_property("b", False)
alpha.set_editor_property("a", True)
connect(pattern, alpha, "")
assert edit.connect_material_property(alpha, "", u.MaterialProperty.MP_ROUGHNESS)
for key, value, prop in (("Metallic", .8, u.MaterialProperty.MP_METALLIC),
                         ("Specular", .5, u.MaterialProperty.MP_SPECULAR),
                         ("Anisotropy", .2, u.MaterialProperty.MP_ANISOTROPY)):
    assert edit.connect_material_property(param(key, value), "", prop)

# A real, small surface gradient perturbs reflected light. World-space normals
# keep the machining direction continuous across separate imported deck plates.
normal = edit.create_material_expression(material, u.MaterialExpressionCustom)
normal.set_editor_property("output_type", u.CustomMaterialOutputType.CMOT_FLOAT3)
inputs = []
for key in ("P", "N", "Detail", "Collar", "Radius", "ArenaScale", "SurfaceZ"):
    entry = u.CustomInput()
    entry.set_editor_property("input_name", key)
    inputs.append(entry)
normal.set_editor_property("inputs", inputs)
normal.set_editor_property("code", r"""
struct Finish {
 float hash(float2 p) { return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
 float noise(float2 p) {
  float2 i=floor(p), f=frac(p); f=f*f*(3.-2.*f);
  return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),
              lerp(hash(i+float2(0,1)),hash(i+1.),f.x),f.y);
 }
};
Finish f;
float2 p=P.xy;
float fineCoord=max(length(ddx(p)),length(ddy(p)))*.32;
float height=f.noise(p*.10)*.012
 +f.noise(float2(p.x*.045,p.y*.32))*.003*exp2(-fineCoord*fineCoord);
float3 n=normalize(N), dx=ddx(P), dy=ddy(P);
float3 a=cross(dy,n), b=cross(n,dx);
float det=dot(dx,a);
float3 gradient=(a*ddx(height)+b*ddy(height))*sign(det)/max(abs(det),.0001);
float scale=max(Radius/650.,.1), r=length(p);
float top=Collar*smoothstep(.35,.8,n.z)*smoothstep(-1.8,-.8,(P.z-SurfaceZ)/max(ArenaScale,.1));
float rim=top*smoothstep(Radius-27.*scale,Radius-25.*scale,r);
// A restrained convex normal response catches broad stadium/softbox highlights
// on the collar. The silhouette, authored bevels and gameplay edge stay exact.
float slope=clamp((r-(Radius-12.*scale))/(12.*scale),-1.,1.)*.16;
float3 crown=float3(p/max(r,.1)*slope,0.)*rim;
// The polished rim uses only the smooth crown: tiny bumped highlights on the
// luminous edge shimmer when the camera moves. Keep microtexture on the deck.
return normalize(n-gradient*Detail*(1.-rim)+crown);
""")
connect(position, normal, "P")
connect(vertex_normal, normal, "N")
connect(detail, normal, "Detail")
for node, slot in ((collar, "Collar"), (radius, "Radius"), (arena_scale, "ArenaScale"), (surface_z, "SurfaceZ")):
    connect(node, normal, slot)
assert edit.connect_material_property(normal, "", u.MaterialProperty.MP_NORMAL)

# Preserve the existing material's exact render-order convention, not new WPO.
offset = edit.create_material_expression(material, u.MaterialExpressionMultiply)
connect(param("LayerDirection", u.LinearColor(0, 0, 1, 0)), offset, "A")
connect(param("RenderLayerOffset", 0.), offset, "B")
assert edit.connect_material_property(offset, "", u.MaterialProperty.MP_WORLD_POSITION_OFFSET)

emission = edit.create_material_expression(material, u.MaterialExpressionCustom)
emission.set_editor_property("output_type", u.CustomMaterialOutputType.CMOT_FLOAT3)
inputs = []
for key in ("P", "N", "Base", "Emission", "SurfaceLift", "Band", "SurfaceZ", "ArenaScale", "Collar", "Radius", "LensCount", "LensStrength"):
    entry = u.CustomInput()
    entry.set_editor_property("input_name", key)
    inputs.append(entry)
emission.set_editor_property("inputs", inputs)
emission.set_editor_property("code", r"""
float z=(P.z-SurfaceZ)/max(ArenaScale,.1);
float angle=atan2(P.y,P.x)/6.2831853;
float segment=abs(frac(angle*28.+.5)-.5);
float aa=max(fwidth(segment),.002);
float ends=1.-smoothstep(.29-aa,.29+aa,segment);
float stripe=1.-smoothstep(3.1,3.1+max(fwidth(z),.08),abs(z+23.));
float core=1.-smoothstep(1.35,1.35+max(fwidth(z),.08),abs(z+23.));
float3 amber=Band*ends*stripe*(float3(1.,.38,.075)*8.+float3(1.,.73,.40)*core*6.);
// Broaden the existing cyan insert optically across its host collar, rather
// than adding raised lights or changing its 3.2-cm primitive geometry. Centers
// and lengths match BuildOneVsOneRim exactly; all edges are derivative filtered.
float scale=max(Radius/650.,.1), r=length(P.xy);
float top=Collar*smoothstep(.35,.8,N.z)*smoothstep(-1.8,-.8,z);
float arc=abs(frac(angle*LensCount)-.5)*(6.2831853*(Radius-16.*scale)/LensCount);
float2 lens=float2(max(arc-36.*scale,0.),r-(Radius-16.*scale));
float distance=length(lens), lensAA=max(fwidth(distance),.08*scale);
// Pixel-area coverage keeps the integrated light stable as an edge becomes
// sub-pixel; no separate near-coplanar emitter is needed above the collar.
float cyanMask=top*saturate((3.*scale-distance)/lensAA+.5);
float cyanCore=saturate((1.25*scale-distance)/lensAA+.5);
float3 cyan=cyanMask*LensStrength*(float3(.015,.43,.62)*.45+float3(.48,.88,1.)*cyanCore*.55);
return Base.rgb*(Emission+SurfaceLift)+amber+cyan;
""")
for node, slot in ((position, "P"), (vertex_normal, "N"), (base, "Base"), (param("Emission", 0.), "Emission"),
                   (param("SurfaceLift", 0.), "SurfaceLift"), (side_band, "Band"),
                   (surface_z, "SurfaceZ"), (arena_scale, "ArenaScale"), (collar, "Collar"), (radius, "Radius"),
                   (lens_count, "LensCount"), (param("RimLensStrength", 30.), "LensStrength")):
    connect(node, emission, slot)
# Tiny HDR strips are unreliable stochastic GI/reflection emitters. Keep their
# full visible radiance, but a bounded traced contribution: the explicit local
# lights supply stable floor illumination. UE also uses this switch in Lumen's
# surface-cache capture. Other materials (especially pucks) are unaffected.
indirect_emission = edit.create_material_expression(material, u.MaterialExpressionMultiply)
connect(emission, indirect_emission, "A")
connect(param("IndirectEmissionScale", .15), indirect_emission, "B")
emission_paths = edit.create_material_expression(material, u.MaterialExpressionRayTracingQualitySwitch)
connect(emission, emission_paths, "Normal")
connect(indirect_emission, emission_paths, "RayTraced")
assert edit.connect_material_property(emission_paths, "", u.MaterialProperty.MP_EMISSIVE_COLOR)
edit.recompile_material(material)
assert u.EditorAssetLibrary.save_loaded_asset(material)
u.log("FLICK_ARCHITECTURAL_MATERIAL_READY " + material.get_path_name())
