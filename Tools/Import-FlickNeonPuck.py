"""Import only Neon Standard geometry, materials and its five UI portraits.
Run with Unreal's PythonScript commandlet after build_neon.py.
"""
import json
from pathlib import Path
import unreal as u
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
source=root/'AssetDevelopment/Pucks/Neon'
spec=json.loads((source/'manifests/standard.json').read_text())
dest='/Game/Cosmetics/Pucks/Neon'
u.EditorAssetLibrary.make_directory(dest)
assets=u.AssetToolsHelpers.get_asset_tools(); edit=u.MaterialEditingLibrary
parent=u.load_asset(dest+'/M_NeonSurface') or assets.create_asset('M_NeonSurface',dest,u.Material,u.MaterialFactoryNew())
edit.delete_all_material_expressions(parent)
parent.set_editor_property('shading_model',u.MaterialShadingModel.MSM_DEFAULT_LIT)
def param(name,value):
    t=u.MaterialExpressionVectorParameter if isinstance(value,u.LinearColor) else u.MaterialExpressionScalarParameter
    n=edit.create_material_expression(parent,t);n.set_editor_property('parameter_name',name);n.set_editor_property('default_value',value);return n
def prop(node,target):assert edit.connect_material_property(node,'',target)
base=param('BaseColor',u.LinearColor(.02,.03,.04,1))
prop(base,u.MaterialProperty.MP_BASE_COLOR)
prop(param('Metallic',.85),u.MaterialProperty.MP_METALLIC)
prop(param('Roughness',.27),u.MaterialProperty.MP_ROUGHNESS)
prop(param('Specular',.35),u.MaterialProperty.MP_SPECULAR)
prop(param('PuckEnvironmentResponse',.16),u.MaterialProperty.MP_AMBIENT_OCCLUSION)
# Soft edge-to-core luminance gives the cyan/yellow glass inserts a rounded
# lens response. Authored centimetre coordinates keep it stable in BOB, replay
# transforms and the large locker showcase; it is not camera/exposure driven.
position=edit.create_material_expression(parent,u.MaterialExpressionVertexInterpolator)
assert edit.connect_material_expressions(edit.create_material_expression(parent,u.MaterialExpressionPreSkinnedPosition),'',position,'VS')
lens=edit.create_material_expression(parent,u.MaterialExpressionCustom)
lens.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
lens.set_editor_property('code', 'float r=length(P.xy); float t=.5; if(P.z>6.) { if(r<15.) t=abs(r-10.075)/1.825; else if(r<28.) t=abs(r-26.35)/.72; else if(r>38.) t=abs(r-41.55)/2.05; } else if(P.z<-5.) t=abs(P.z+6.35)/.75; else t=abs(P.z+.75)/.8; return lerp(1.,.18+1.60*exp(-t*t*3.),Lens);')
inputs=[]
for key in ['P','Lens']:
    ci=u.CustomInput();ci.set_editor_property('input_name',key);inputs.append(ci)
lens.set_editor_property('inputs',inputs)
assert edit.connect_material_expressions(position,'',lens,'P')
assert edit.connect_material_expressions(param('LensProfile',0.),'',lens,'Lens')
strength=edit.create_material_expression(parent,u.MaterialExpressionMultiply)
assert edit.connect_material_expressions(lens,'',strength,'A')
assert edit.connect_material_expressions(param('Emission',0.),'',strength,'B')
emit=edit.create_material_expression(parent,u.MaterialExpressionMultiply)
tint=edit.create_material_expression(parent,u.MaterialExpressionMultiply)
assert edit.connect_material_expressions(param('LEDColor',u.LinearColor(0,0,0,1)),'',tint,'A')
assert edit.connect_material_expressions(edit.create_material_expression(parent,u.MaterialExpressionVertexColor),'',tint,'B')
assert edit.connect_material_expressions(tint,'',emit,'A')
assert edit.connect_material_expressions(strength,'',emit,'B')
# Local reflected LED colour in the neighboring metal, independent of arena
# sliders. This approximates the near-field bounce at centimetre distances
# that screen-space reflections cannot reliably resolve on a moving puck.
bleed=edit.create_material_expression(parent,u.MaterialExpressionCustom)
bleed.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
bleed.set_editor_property('code', 'float r=length(P.xy); float face=step(6.,P.z)*Face; float deck=step(6.,P.z)*Deck; float pink=exp(-abs(r-26.35)*1.15)*(face*.42+deck*.23); float cyan=exp(-abs(r-11.9)*1.4)*face*.10+exp(-abs(r-39.3)*1.1)*deck*.06; return float3(1.,.001,.85)*pink+float3(.001,.7,1.)*cyan;')
ci=[]
for key in ['P','Face','Deck']:
    entry=u.CustomInput();entry.set_editor_property('input_name',key);ci.append(entry)
bleed.set_editor_property('inputs',ci)
for node,slot in [(position,'P'),(param('FaceBleed',0.),'Face'),(param('DeckBleed',0.),'Deck')]:assert edit.connect_material_expressions(node,'',bleed,slot)
addition=edit.create_material_expression(parent,u.MaterialExpressionAdd)
assert edit.connect_material_expressions(emit,'',addition,'A')
assert edit.connect_material_expressions(bleed,'',addition,'B')
# A restrained analytic reflection environment for this polished finish. The
# game's anti-glare key is intentionally weak in specular and cannot reveal
# the crown's machining on its own. Broad reflection cards respond to the real
# reflection vector/curved normals, like a tiny studio cubemap, without extra
# per-piece lights, captures or textures. Face and LED roles receive none.
cards=edit.create_material_expression(parent,u.MaterialExpressionCustom)
cards.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
cards.set_editor_property('code','float3 r=normalize(R); float p=lerp(24.,6.,Rough); float a=pow(saturate(abs(r.y)*.8+abs(r.z)*.6),p); float b=pow(saturate(abs(r.x)*.75+abs(r.z)*.66),p)*.40; float strip=exp(-pow((abs(r.z)-.12)/.16,2.))*pow(abs(r.y),3.)*.35; return Gain*(float3(.78,.88,1.)*a+float3(.48,.65,.88)*b+float3(.72,.82,1.)*strip);')
ci=[]
for key in ['R','Rough','Gain']:
    entry=u.CustomInput();entry.set_editor_property('input_name',key);ci.append(entry)
cards.set_editor_property('inputs',ci)
for node,slot in [(edit.create_material_expression(parent,u.MaterialExpressionReflectionVectorWS),'R'),(param('Roughness',.27),'Rough'),(param('StudioReflection',0.),'Gain')]:
    assert edit.connect_material_expressions(node,'',cards,slot)
reflected=edit.create_material_expression(parent,u.MaterialExpressionAdd)
assert edit.connect_material_expressions(addition,'',reflected,'A')
assert edit.connect_material_expressions(cards,'',reflected,'B')
prop(reflected,u.MaterialProperty.MP_EMISSIVE_COLOR)
# Derivative-filtered fine grain. The geometry's UVs are planar metres; the
# normal stays flat as machining marks become subpixel at gameplay distance.
uv=edit.create_material_expression(parent,u.MaterialExpressionTextureCoordinate)
custom=edit.create_material_expression(parent,u.MaterialExpressionCustom)
custom.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
custom.set_editor_property('code','float2 q=UV*Scale; float w=max(length(ddx(q)),length(ddy(q))); float fade=1.-smoothstep(.25,1.5,w); float a=(sin(q.x*6.283)*cos(q.y*5.3)+sin(q.x*2.71+q.y*3.17)*.4)*Amount*fade; return normalize(float3(a,a*.45,1.));')
inputs=[]
for key in ['UV','Amount','Scale']:
    v=u.CustomInput();v.set_editor_property('input_name',key);inputs.append(v)
custom.set_editor_property('inputs',inputs)
assert edit.connect_material_expressions(uv,'',custom,'UV')
assert edit.connect_material_expressions(param('Grain',.05),'',custom,'Amount')
assert edit.connect_material_expressions(param('GrainScale',700.),'',custom,'Scale')
prop(custom,u.MaterialProperty.MP_NORMAL)
edit.recompile_material(parent);u.EditorAssetLibrary.save_loaded_asset(parent)
materials={}
for name,row in spec['materials'].items():
    mi=u.load_asset(dest+'/MI_'+name) or assets.create_asset('MI_'+name,dest,u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew())
    edit.set_material_instance_parent(mi,parent)
    for key,value in [('BaseColor',row['base']),('LEDColor',row['emissive'])]:edit.set_material_instance_vector_parameter_value(mi,key,u.LinearColor(*[c*(.55 if name=='Neon_ControlDeck' and key=='BaseColor' else 1.) for c in value],1))
    for key,value in [('Metallic',row['metallic']),('Roughness',row['roughness']),('Emission',row['power']*(.4 if name in ['Neon_Cyan','Neon_Magenta','Neon_Yellow'] else .8)),('Specular',.08 if row['power'] else .14 if name=='Neon_Face' else .35),('Grain',.045 if row['grain'] else 0.),('LensProfile',1. if name in ['Neon_Cyan','Neon_Magenta','Neon_Yellow'] else 0.),('FaceBleed',1. if name=='Neon_Face' else 0.),('DeckBleed',1. if name=='Neon_ControlDeck' else 0.)]:
        edit.set_material_instance_scalar_parameter_value(mi,key,value)
    # The classic puck's aggressively suppressed environment response flattened
    # this finish's titanium. Let its shaped metal reflect the existing arena;
    # keep the dark face restrained and never change the shared light rig.
    edit.set_material_instance_scalar_parameter_value(mi,'PuckEnvironmentResponse',.65 if name in ['Neon_Housing','Neon_ControlDeck','Neon_PolishedEdges'] else .20)
    edit.set_material_instance_scalar_parameter_value(mi,'GrainScale',450. if name=='Neon_Face' else 700.)
    edit.set_material_instance_scalar_parameter_value(mi,'StudioReflection',{'Neon_PolishedEdges':.60,'Neon_ControlDeck':.28,'Neon_Housing':.10}.get(name,0.))
    u.EditorAssetLibrary.save_loaded_asset(mi);materials[name]=mi
options=u.FbxImportUI();options.import_mesh=True;options.import_materials=False;options.import_textures=False;options.import_as_skeletal=False;options.automated_import_should_detect_type=False;options.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
options.static_mesh_import_data.vertex_color_import_option=u.VertexColorImportOption.REPLACE
options.static_mesh_import_data.normal_generation_method=u.FBXNormalGenerationMethod.BUILT_IN
options.static_mesh_import_data.combine_meshes=True;options.static_mesh_import_data.auto_generate_collision=False;options.static_mesh_import_data.generate_lightmap_u_vs=False;options.static_mesh_import_data.normal_import_method=u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS
job=u.AssetImportTask();job.filename=str(source/'exports/Standard.fbx');job.destination_path=dest;job.destination_name=spec['asset'];job.automated=True;job.replace_existing=True;job.replace_existing_settings=True;job.save=True;job.options=options;job.factory=u.FbxFactory()
assets.import_asset_tasks([job]);mesh=u.load_asset(dest+'/'+spec['asset']);assert isinstance(mesh,u.StaticMesh)
for i,slot in enumerate(mesh.get_editor_property('static_materials')):
    name=str(slot.material_slot_name)
    assert name in materials,name
    mesh.set_material(i,materials[name])
# Never derive physics from the cosmetic mesh. One renderer, ten material batches.
sub=u.get_editor_subsystem(u.StaticMeshEditorSubsystem)
if sub: sub.remove_collisions(mesh)
# In a commandlet the editor subsystem is absent; auto_generate_collision=False
# above still guarantees a newly imported visual asset has no simple collision.
bounds=mesh.get_bounds();assert abs(bounds.box_extent.x*2-90.3)<1 and abs(bounds.box_extent.z*2-20)<.15,str(bounds)
u.EditorAssetLibrary.save_loaded_asset(mesh)
for view in ['','Locker','Presentation','Hero','ProfileCutout']:
    png=root/'AssetDevelopment/UI/PuckThumbnails'/view/'Standard_6.png'
    if not png.exists():continue
    folder='/Game/UI/PuckThumbnails'+('/'+view if view else '')
    j=u.AssetImportTask();j.filename=str(png);j.destination_path=folder;j.destination_name='T_Puck_Standard_6';j.automated=True;j.replace_existing=True;j.save=True
    assets.import_asset_tasks([j]);tex=u.load_asset(folder+'/T_Puck_Standard_6')
    tex.set_editor_property('lod_group',u.TextureGroup.TEXTUREGROUP_UI);tex.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_BC7);tex.set_editor_property('srgb',True);tex.set_editor_property('max_texture_size',1024);tex.set_editor_property('never_stream',False)
    u.EditorAssetLibrary.save_loaded_asset(tex)
report=dict(mesh=mesh.get_path_name(),triangles=mesh.get_num_triangles(0),vertices=mesh.get_num_vertices(0),materials=len(mesh.get_editor_property('static_materials')),bounds=str(bounds))
(root/'Saved/NeonImportReport.json').write_text(json.dumps(report,indent=2));u.log('NEON_STANDARD_IMPORT_COMPLETE '+str(report))
