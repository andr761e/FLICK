"""Unreal editor commandlet: import the shared Blender meshes and native materials."""
import json
from pathlib import Path
import unreal as u

project=Path(u.Paths.project_dir())
root='/Game/TestArena/Pucks'
u.EditorAssetLibrary.make_directory(root)
assets=u.AssetToolsHelpers.get_asset_tools()
environment_path=root+'/T_PuckEnvironment'
if not u.EditorAssetLibrary.does_asset_exist(environment_path):
    source=u.load_object(None,'/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap')
    environment=assets.duplicate_asset('T_PuckEnvironment',root,source) if source else None
    if not environment: raise RuntimeError('Could not create the test arena reflection environment')
    u.EditorAssetLibrary.save_loaded_asset(environment)
edit=u.MaterialEditingLibrary
# Python commandlets do not initialize the editor subsystem collection.
mesh_editor=u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem)

def param(mat,cls,name,value,prop):
    node=edit.create_material_expression(mat,cls)
    node.set_editor_property('parameter_name',name)
    node.set_editor_property('default_value',value)
    edit.connect_material_property(node,'',prop)
    return node

parent_path=root+'/M_PuckSurface'
parent=u.load_asset(parent_path)
if not parent:
    parent=assets.create_asset('M_PuckSurface',root,u.Material,u.MaterialFactoryNew())

# Rebuild the generated parent on every import so checked-in material values and
# shader structure cannot drift from an older binary asset. Fine anisotropic UV
# noise modulates roughness rather than colour, producing moving brushed-metal
# highlights while preserving the clean, unweathered finish.
edit.delete_all_material_expressions(parent)
base_color=param(parent,u.MaterialExpressionVectorParameter,'BaseColor',u.LinearColor(.02,.03,.04,1),u.MaterialProperty.MP_BASE_COLOR)
param(parent,u.MaterialExpressionScalarParameter,'Metallic',.5,u.MaterialProperty.MP_METALLIC)
roughness=edit.create_material_expression(parent,u.MaterialExpressionScalarParameter,-700,80)
roughness.set_editor_property('parameter_name','Roughness'); roughness.set_editor_property('default_value',.3)
texcoord=edit.create_material_expression(parent,u.MaterialExpressionTextureCoordinate,-700,220)
texcoord.set_editor_property('u_tiling',48.0); texcoord.set_editor_property('v_tiling',6.0)
noise=edit.create_material_expression(parent,u.MaterialExpressionNoise,-480,220)
noise.set_editor_property('scale',1.0); noise.set_editor_property('quality',1); noise.set_editor_property('levels',1)
noise.set_editor_property('output_min',-.028); noise.set_editor_property('output_max',.028)
edit.connect_material_expressions(texcoord,'',noise,'Position')
roughness_add=edit.create_material_expression(parent,u.MaterialExpressionAdd,-220,100)
edit.connect_material_expressions(roughness,'',roughness_add,'A')
edit.connect_material_expressions(noise,'',roughness_add,'B')
edit.connect_material_property(roughness_add,'',u.MaterialProperty.MP_ROUGHNESS)
param(parent,u.MaterialExpressionScalarParameter,'Specular',.55,u.MaterialProperty.MP_SPECULAR)
color=edit.create_material_expression(parent,u.MaterialExpressionVectorParameter,-500,420)
color.set_editor_property('parameter_name','TeamColor'); color.set_editor_property('default_value',u.LinearColor(0,.5,1,1))
strength=edit.create_material_expression(parent,u.MaterialExpressionScalarParameter,-500,500)
strength.set_editor_property('parameter_name','Emission'); strength.set_editor_property('default_value',0)
mul=edit.create_material_expression(parent,u.MaterialExpressionMultiply,-220,440)
edit.connect_material_expressions(color,'',mul,'A'); edit.connect_material_expressions(strength,'',mul,'B')
# A restrained, non-blooming reflection fill keeps polished metal readable in
# the dark gameplay environment. It is strongest on titanium and nearly absent
# on graphite, so this restores the intended value hierarchy without flooding
# the arena with more lights or flattening the dynamic specular highlights.
surface_lift=edit.create_material_expression(parent,u.MaterialExpressionScalarParameter,-500,600)
surface_lift.set_editor_property('parameter_name','SurfaceLift'); surface_lift.set_editor_property('default_value',0)
surface_mul=edit.create_material_expression(parent,u.MaterialExpressionMultiply,-220,600)
edit.connect_material_expressions(base_color,'',surface_mul,'A'); edit.connect_material_expressions(surface_lift,'',surface_mul,'B')
emissive_add=edit.create_material_expression(parent,u.MaterialExpressionAdd,20,480)
edit.connect_material_expressions(mul,'',emissive_add,'A'); edit.connect_material_expressions(surface_mul,'',emissive_add,'B')
edit.connect_material_property(emissive_add,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
edit.recompile_material(parent)
u.EditorAssetLibrary.save_loaded_asset(parent)

specs={
    # Titanium stays physically metallic but receives a restrained reflection
    # floor. Graphite remains reflective charcoal, visibly below the silver.
    # Tuple: base colour, metallic, roughness, team emission, surface lift.
    '01_Satin_Titanium':((.72,.78,.86),.92,.23,0,.42),
    '02_Graphite_Chassis':((.045,.058,.075),.38,.30,0,.055),
    '03_Grip_Polymer':((.030,.040,.055),.14,.38,0,.018),
    '04_Ceramic_Insert':((.040,.062,.090),.22,.25,0,.035),
    '05_Team_Cyan':((0,.025,.05),0,.24,10.0,0),
    '06_Recess':((.010,.015,.024),.12,.36,0,.004)
}
materials={}
for key,(rgb,metal,rough,emission,surface_lift) in specs.items():
    name='MI_'+key
    mat=u.load_asset(root+'/'+name) or assets.create_asset(name,root,u.MaterialInstanceConstant,u.MaterialInstanceConstantFactoryNew())
    edit.set_material_instance_parent(mat,parent)
    edit.set_material_instance_vector_parameter_value(mat,'BaseColor',u.LinearColor(*rgb,1))
    for parameter,value in [('Metallic',metal),('Roughness',rough),('Emission',emission),('SurfaceLift',surface_lift)]:
        edit.set_material_instance_scalar_parameter_value(mat,parameter,value)
    edit.set_material_instance_vector_parameter_value(mat,'TeamColor',u.LinearColor(0,.5,1,1))
    u.EditorAssetLibrary.save_loaded_asset(mat)
    materials[key]=mat

manifest=json.loads((project/'AssetDevelopment/Pucks/dimensions.json').read_text())
report=[]
for row in manifest:
    name=row['name']
    task=u.AssetImportTask()
    task.filename=str(project/'AssetDevelopment/Pucks/exports/Unreal'/(name+'.fbx'))
    task.destination_path=root; task.destination_name='SM_Puck_'+name
    task.automated=True; task.replace_existing=True; task.save=True
    options=u.FbxImportUI(); options.import_mesh=True; options.import_materials=False; options.import_textures=False
    options.import_as_skeletal=False; options.automated_import_should_detect_type=False
    options.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
    data=options.static_mesh_import_data
    data.combine_meshes=True; data.auto_generate_collision=False
    data.generate_lightmap_u_vs=False
    data.normal_import_method=u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS
    task.options=options; task.factory=u.FbxFactory()
    if '-FlickRebuildPuckTangents' not in u.SystemLibrary.get_command_line():
        assets.import_asset_tasks([task])
    mesh=u.load_asset(root+'/SM_Puck_'+name)
    if not isinstance(mesh,u.StaticMesh): raise RuntimeError('Mesh import failed: '+name)
    settings=mesh_editor.get_lod_build_settings(mesh,0)
    settings.recompute_tangents=False
    settings.use_mikk_t_space=False
    settings.remove_degenerates=True
    mesh_editor.set_lod_build_settings(mesh,0,settings)
    slots=mesh.get_editor_property('static_materials')
    mapped=[]
    for index,slot in enumerate(slots):
        slot_name=str(slot.get_editor_property('material_slot_name'))
        matches=[key for key in materials if key in slot_name]
        if len(matches)!=1: raise RuntimeError('Unknown material slot '+slot_name)
        mesh.set_material(index,materials[matches[0]]); mapped.append(matches[0])
    bounds=mesh.get_bounds()
    diameter=max(bounds.box_extent.x,bounds.box_extent.y)*2
    height=bounds.box_extent.z*2
    if abs(diameter-row['mesh_dimensions_cm'][0])>1 or abs(height-row['mesh_dimensions_cm'][2])>1:
        raise RuntimeError('Import unit mismatch: '+name+' '+str((diameter,height)))
    if bounds.origin.length()>1: raise RuntimeError('Off-center pivot: '+name)
    u.EditorAssetLibrary.save_loaded_asset(mesh)
    report.append(dict(name=name,diameter_cm=diameter,height_cm=height,materials=mapped))
(project/'Saved/PuckImportReport.json').write_text(json.dumps(report,indent=2))
u.log('FLICK_PUCK_IMPORT_COMPLETE: nine shared meshes and six native material instances')
