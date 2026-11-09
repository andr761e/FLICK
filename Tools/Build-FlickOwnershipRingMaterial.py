"""Opaque, unlit ownership markers: solid colour without additive washout."""
import unreal as u
directory = '/Game/Cosmetics/Pucks/BOB'
name = 'M_PuckOwnershipRing'
u.EditorAssetLibrary.make_directory(directory)
m = u.load_asset(directory+'/'+name) or u.AssetToolsHelpers.get_asset_tools().create_asset(name,directory,u.Material,u.MaterialFactoryNew())
e = u.MaterialEditingLibrary
e.delete_all_material_expressions(m)
m.set_editor_property('blend_mode',u.BlendMode.BLEND_OPAQUE)
m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
c = e.create_material_expression(m,u.MaterialExpressionVectorParameter)
c.set_editor_property('parameter_name','Color')
c.set_editor_property('default_value',u.LinearColor(1,.18,.015,1))
assert e.connect_material_property(c,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
e.recompile_material(m)
assert u.EditorAssetLibrary.save_loaded_asset(m)
u.log('FLICK_SOLID_OWNERSHIP_RING_READY')
