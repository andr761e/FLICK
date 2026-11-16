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
# Opt-in shader relief: zero preserves the existing concourse/locker finish.
relief = param("DeckRelief", 0.)
collar = param("RimCollarFinish", 0.)
radius = param("ArenaRadius", 650.)
arena_scale = param("ArenaScale", 1.)
surface_z = param("ArenaSurfaceZ", 250.)
lens_count = param("RimLensCount", 28.)
vertex_normal = edit.create_material_expression(material, u.MaterialExpressionVertexNormalWS)
position = edit.create_material_expression(material, u.MaterialExpressionWorldPosition)
# Painted guides and concourse joints share their host surface's depth. Thin
# boxes above that surface produced subpixel silhouettes, intersections and
# independent Lumen occluders. Filter their actual pixel coverage instead.
markings = edit.create_material_expression(material, u.MaterialExpressionCustom)
markings.set_editor_property("output_type", u.CustomMaterialOutputType.CMOT_FLOAT2)
inputs = []
for key in ("P", "N", "Guides", "Seams", "Width", "Radius", "SurfaceZ"):
    entry = u.CustomInput()
    entry.set_editor_property("input_name", key)
    inputs.append(entry)
markings.set_editor_property("inputs", inputs)
markings.set_editor_property("code", r"""
struct FloorInk {
 float coverage(float distance) {
  return saturate(.5-distance/max(fwidth(distance),.001));
 }
 float stroke(float coordinate,float halfWidth) {
  // Integrate the interval over the pixel, including strokes narrower than
  // one pixel. Differentiate BEFORE abs(): fwidth(abs(x)) collapses when a
  // quad straddles a thin line, exactly where stable coverage matters most.
  float pixel=max(fwidth(coordinate),.001);
  return saturate((min(halfWidth,coordinate+pixel*.5)
                 -max(-halfWidth,coordinate-pixel*.5))/pixel);
 }
};
FloorInk ink;
if(Guides<.5 && Seams<.5) return float2(0.,0.);
float scale=max(Radius/650.,.1), r=length(P.xy);
float up=saturate((N.z-.8)*5.);
float cross=max(ink.stroke(P.x,Width*.5*scale),ink.stroke(P.y,Width*.5*scale))
 *ink.coverage(max(abs(P.x),abs(P.y))-Radius*.94);
float inner=ink.stroke(r-Radius*.151,.65*scale);
float rings=max(max(ink.stroke(r-Radius*.36,Width*.5*scale),ink.stroke(r-Radius*.52,Width*.5*scale)),
                max(ink.stroke(r-Radius*.705,Width*.5*scale),ink.stroke(r-Radius*.855,Width*.5*scale)));
// Match the existing 3/192-segment break immediately after each cardinal axis.
// Rotate its centre rather than differentiating a wrapped angle at that axis.
float2 rotated=float2(.998795456*P.x+.049067674*P.y,
                     -.049067674*P.x+.998795456*P.y);
float gap=min(abs(rotated.x),abs(rotated.y))-r*.049067674;
float guide=max(cross,max(inner,rings*ink.coverage(-gap)));
guide*=Guides*up;

float angle=atan2(P.y,P.x);
float radialStep=3.141592654/12.;
float nearest=angle-radialStep*floor(angle/radialStep+.5);
float radial=ink.stroke(r*sin(nearest),.6*scale)
 *ink.coverage(670.*scale-r)*ink.coverage(r-1512.*scale);
float joints=max(ink.stroke(r-935.9*scale,.9*scale),
                 max(ink.stroke(r-1225.9*scale,.9*scale),ink.stroke(r-1475.9*scale,.9*scale)));
float seam=max(radial,joints);
// Retain the 48 timing ticks that share the authored seam material slot.
float tickStep=3.141592654/24.;
float tick=floor(angle/tickStep+.5), localAngle=angle-tick*tickStep;
float halfLength=frac(tick*.25)<.01?16.:7.;
float tickCoverage=ink.stroke(r*cos(localAngle)-1055.*scale,1.25*scale)
 *ink.stroke(r*sin(localAngle),halfLength*scale);
seam=max(seam,tickCoverage);
float floorZ=SurfaceZ-50.*scale;
seam*=Seams*up*(1.-smoothstep(.5*scale,1.5*scale,abs(P.z-floorZ)));
return float2(guide,seam);
""")
for node, slot in ((position, "P"), (vertex_normal, "N"),
                   (param("DeckGuides", 0.), "Guides"), (param("ConcourseSeams", 0.), "Seams"),
                   (param("GuideWidth", 1.05), "Width"), (radius, "Radius"), (surface_z, "SurfaceZ")):
    connect(node, markings, slot)


def marked_property(surface, property, paint, seams=None):
    """Blend the same filtered coverage through every shading channel."""
    node = edit.create_material_expression(material, u.MaterialExpressionCustom)
    node.set_editor_property("output_type", u.CustomMaterialOutputType.CMOT_FLOAT3
                             if property in (u.MaterialProperty.MP_BASE_COLOR,
                                             u.MaterialProperty.MP_NORMAL,
                                             u.MaterialProperty.MP_EMISSIVE_COLOR)
                             else u.CustomMaterialOutputType.CMOT_FLOAT1)
    entries = []
    for key in ("Surface", "Ink", "Paint", "Seam"):
        entry = u.CustomInput()
        entry.set_editor_property("input_name", key)
        entries.append(entry)
    node.set_editor_property("inputs", entries)
    vector = property in (u.MaterialProperty.MP_BASE_COLOR, u.MaterialProperty.MP_NORMAL,
                          u.MaterialProperty.MP_EMISSIVE_COLOR)
    node.set_editor_property("code", "return lerp(lerp(Surface,Paint.rgb,Ink.x),Seam.rgb,Ink.y);"
                             if vector else "return lerp(lerp(Surface,Paint,Ink.x),Seam,Ink.y);")
    connect(surface, node, "Surface")
    connect(markings, node, "Ink")
    connect(paint, node, "Paint")
    connect(seams if seams is not None else paint, node, "Seam")
    assert edit.connect_material_property(node, "", property)


pattern = edit.create_material_expression(material, u.MaterialExpressionCustom)
pattern.set_editor_property("output_type", u.CustomMaterialOutputType.CMOT_FLOAT4)
inputs = []
for key in ("P", "N", "Detail", "Deck", "Relief", "Roughness", "Base", "Collar", "Radius", "ArenaScale", "SurfaceZ", "LensCount", "Band"):
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
// Fine satin wear varies reflection width, not painted albedo. Opt-in deck only.
roughness=clamp(roughness+Deck*Relief*(f.noise(P.xy*.24)-.5)*.28,.12,.85);
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
for node, slot in ((position, "P"), (vertex_normal, "N"), (detail, "Detail"), (deck, "Deck"), (relief, "Relief"), (rough, "Roughness"), (base, "Base"),
                   (collar, "Collar"), (radius, "Radius"), (arena_scale, "ArenaScale"), (surface_z, "SurfaceZ"),
                   (lens_count, "LensCount"), (side_band, "Band")):
    connect(node, pattern, slot)
mask = edit.create_material_expression(material, u.MaterialExpressionComponentMask)
mask.set_editor_property("r", True)
mask.set_editor_property("g", True)
mask.set_editor_property("b", True)
connect(pattern, mask, "")
marked_property(mask, u.MaterialProperty.MP_BASE_COLOR,
                param("GuideColor", u.LinearColor(.48, .57, .62, 1)),
                param("SeamColor", u.LinearColor(.003, .006, .009, 1)))
alpha = edit.create_material_expression(material, u.MaterialExpressionComponentMask)
alpha.set_editor_property("r", False)
alpha.set_editor_property("g", False)
alpha.set_editor_property("b", False)
alpha.set_editor_property("a", True)
connect(pattern, alpha, "")
marked_property(alpha, u.MaterialProperty.MP_ROUGHNESS, param("MarkingRoughness", .8), param("SeamRoughness", .75))
for key, value, prop in (("Metallic", .8, u.MaterialProperty.MP_METALLIC),
                         ("Specular", .5, u.MaterialProperty.MP_SPECULAR),
                         ("Anisotropy", .2, u.MaterialProperty.MP_ANISOTROPY)):
    marked_property(param(key, value), prop,
                    param("Marking" + key, {"Metallic": .05, "Specular": .25, "Anisotropy": 0.}[key]),
                    param("Seam" + key, {"Metallic": .2, "Specular": .1, "Anisotropy": 0.}[key]))

# A real, small surface gradient perturbs reflected light. World-space normals
# keep the machining direction continuous across separate imported deck plates.
normal = edit.create_material_expression(material, u.MaterialExpressionCustom)
normal.set_editor_property("output_type", u.CustomMaterialOutputType.CMOT_FLOAT3)
inputs = []
for key in ("P", "N", "Detail", "Deck", "Relief", "Collar", "Radius", "ArenaScale", "SurfaceZ"):
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
// A few-centimetre-scale finish breaks up the broad reflected fixtures.
// Screen-footprint filtering suppresses detail before it can become sparkle.
// This height exists ONLY in shading derivatives: no WPO or physics displacement.
float footprint=max(length(ddx(p)),length(ddy(p)));
// Rotate and warp the machining coordinates: an axis-aligned value-noise
// gradient otherwise reads as tiny rectangular tiles under a long light strip.
float2 machining=float2(dot(p,float2(.8,.6)),dot(p,float2(-.6,.8)));
machining+=float2(f.noise(p*.037),f.noise(p*.037+31.))*18.;
height+=Deck*Relief*.6*(f.noise(machining*.24)*exp2(-pow(footprint*.40,2.))
 +f.noise(machining*.62)*.10*exp2(-pow(footprint*.80,2.)));
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
connect(deck, normal, "Deck")
connect(relief, normal, "Relief")
for node, slot in ((collar, "Collar"), (radius, "Radius"), (arena_scale, "ArenaScale"), (surface_z, "SurfaceZ")):
    connect(node, normal, slot)
marked_property(normal, u.MaterialProperty.MP_NORMAL, vertex_normal)

# Preserve the existing material's exact render-order convention, not new WPO.
offset = edit.create_material_expression(material, u.MaterialExpressionMultiply)
connect(param("LayerDirection", u.LinearColor(0, 0, 1, 0)), offset, "A")
connect(param("RenderLayerOffset", 0.), offset, "B")
# The authored graphite slot also contains benches/walls. Bury ONLY its thin
# floor details; the filtered joints/ticks now live on the porcelain floor.
# Use undisplaced world position so selection cannot depend on its own WPO.
unshifted_position = edit.create_material_expression(material, u.MaterialExpressionWorldPosition)
unshifted_position.set_editor_property("world_position_shader_offset", u.WorldPositionIncludedOffsets.WPT_EXCLUDE_ALL_SHADER_OFFSETS)
floor_offset = edit.create_material_expression(material, u.MaterialExpressionCustom)
floor_offset.set_editor_property("output_type", u.CustomMaterialOutputType.CMOT_FLOAT3)
entries = []
for key in ("Offset", "P", "HideSeams", "Radius", "SurfaceZ"):
    entry = u.CustomInput()
    entry.set_editor_property("input_name", key)
    entries.append(entry)
floor_offset.set_editor_property("inputs", entries)
floor_offset.set_editor_property("code", r"""
float scale=max(Radius/650.,.1);
float z=(P.z-SurfaceZ)/scale+50.;
float floorDetail=step(.25,z)*step(z,2.1)*step(length(P.xy),1514.*scale);
return Offset-float3(0.,0.,4.*scale*HideSeams*floorDetail);
""")
for node, slot in ((offset, "Offset"), (unshifted_position, "P"),
                   (param("ConcourseSeamGeometry", 0.), "HideSeams"),
                   (radius, "Radius"), (surface_z, "SurfaceZ")):
    connect(node, floor_offset, slot)
assert edit.connect_material_property(floor_offset, "", u.MaterialProperty.MP_WORLD_POSITION_OFFSET)

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
float3 amber=Band*ends*stripe*(float3(1.,.38,.075)*5.+float3(1.,.73,.40)*core*3.5);
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
marked_property(emission_paths, u.MaterialProperty.MP_EMISSIVE_COLOR,
                param("GuideLift", u.LinearColor(.48*.035, .57*.035, .62*.035, 0)),
                param("SeamLift", u.LinearColor(0, 0, 0, 0)))
edit.recompile_material(material)
assert u.EditorAssetLibrary.save_loaded_asset(material)
u.log("FLICK_ARCHITECTURAL_MATERIAL_READY " + material.get_path_name())
