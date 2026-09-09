"""Generate the modular Switchyard arena workshop with no Blender add-ons."""
import bpy, json, math, re
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
PROJECT=ROOT.parents[1]
SOURCE=(PROJECT/'Source/FLICK/Arena/FlickTestArena.h').read_text()
ARENA_SOURCE=(PROJECT/'Source/FLICK/Game/FlickGameMode.h').read_text()

def cpp_float(source,name,default):
    match=re.search(r'float\s+'+name+r'\s*=\s*([\d.]+)f',source)
    return float(match.group(1)) if match else default

# Centimeters in C++; meters in Blender.
arena_radius=cpp_float(ARENA_SOURCE,'ArenaRadius',650.0)/100
arena_thickness=cpp_float(ARENA_SOURCE,'ArenaThickness',50.0)/100
divider_length=cpp_float(SOURCE,'DividerLength',146.0)/100
divider_thickness=cpp_float(SOURCE,'DividerThickness',18.0)/100
divider_height=cpp_float(SOURCE,'DividerHeight',56.0)/100
switch_radius=cpp_float(SOURCE,'ControlZoneRadius',40.0)/100
dot_radius=cpp_float(SOURCE,'SwitchActivationDotRadius',8.0)/100
switch_distance=cpp_float(SOURCE,'SwitchDistanceFromDivider',145.0)/100
outer_radius_fraction=cpp_float(SOURCE,'OuterDividerRadiusFraction',.935)
possible_count=20
active_count=8

bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
for datablocks in (bpy.data.materials,):
    for block in list(datablocks): datablocks.remove(block)
scene=bpy.context.scene
scene.unit_settings.system='METRIC'; scene.unit_settings.length_unit='METERS'

def mat(name,color,metallic,roughness,emission=0):
    value=bpy.data.materials.new(name); value.diffuse_color=(*color,1); value.use_nodes=True
    shader=value.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Base Color'].default_value=(*color,1)
    shader.inputs['Metallic'].default_value=metallic
    shader.inputs['Roughness'].default_value=roughness
    if emission:
        shader.inputs['Emission Color'].default_value=(*color,1)
        shader.inputs['Emission Strength'].default_value=emission
    return value

# The hierarchy deliberately mirrors the high-detail pucks: reflective charcoal,
# bright machined metal, and sharp embedded team lighting. The floor stays in a
# lighter blue-grey family so black puck sidewalls remain readable in gameplay.
graphite=mat('01_Arena_Graphite',(0.18,0.225,0.29),.55,.28)
floor_mat=mat('02_Arena_Surface',(0.62,0.70,0.79),.12,.46)
rubber=mat('03_Rim_Polymer',(0.055,0.078,0.11),.22,.36)
titanium=mat('04_Brushed_Titanium',(.72,.79,.87),.94,.20)
recess=mat('05_Deep_Recess',(.022,.034,.050),.34,.34)
cyan=mat('06_Team_Cyan',(0,.32,.76),.06,.25,2.75)
orange=mat('07_Team_Orange',(.95,.16,.008),.06,.25,2.75)
line_mat=mat('08_Floor_Lines',(.79,.85,.92),.48,.27)
switch_mat=mat('09_Switch_Accent',(.025,.48,.72),.12,.32,.72)
inner_field=mat('10_Inner_Field',(.53,.61,.70),.16,.42)
center_inset=mat('11_Center_Inset',(.70,.77,.84),.28,.30)
accent_metal=mat('12_Accent_Metal',(.43,.51,.61),.90,.23)
dark_marking=mat('13_Dark_Marking',(.075,.105,.15),.40,.32)

def finish(obj,name,material,bevel=0.0):
    obj.name=name; obj.data.materials.append(material)
    if bevel:
        mod=obj.modifiers.new('Edge chamfer','BEVEL'); mod.width=bevel; mod.segments=3
    for polygon in obj.data.polygons: polygon.use_smooth=True
    normal=obj.modifiers.new('Weighted normals','WEIGHTED_NORMAL'); normal.keep_sharp=True
    return obj

def cylinder(name,radius,depth,z,material,vertices=128,bevel=.01):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius,depth=depth,location=(0,0,z))
    return finish(bpy.context.object,name,material,min(bevel,depth*.2))

def cube(name,location,dimensions,material,angle=0,bevel=.01):
    bpy.ops.mesh.primitive_cube_add(size=1,location=location); obj=bpy.context.object
    obj.dimensions=dimensions; bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    obj.rotation_euler.z=angle
    return finish(obj,name,material,min(bevel,min(dimensions)*.22))

def ring(name,inner,outer,z,height,material,segments=192):
    vertices=[]; faces=[]
    for zz in (z-height/2,z+height/2):
        for radius in (inner,outer):
            for i in range(segments):
                angle=2*math.pi*i/segments
                vertices.append((radius*math.cos(angle),radius*math.sin(angle),zz))
    n=segments
    for i in range(n):
        j=(i+1)%n
        faces.extend(((i,j,n+j,n+i),(2*n+i,3*n+i,3*n+j,2*n+j),
                      (i,2*n+i,2*n+j,j),(n+i,n+j,3*n+j,3*n+i)))
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(vertices,[],faces); mesh.update()
    obj=bpy.data.objects.new(name,mesh); scene.collection.objects.link(obj)
    return finish(obj,name,material,min(height*.18,.012))

def arc(name,inner,outer,z,height,start,end,material,steps=24):
    vertices=[]
    for zz in (z-height/2,z+height/2):
        for radius in (inner,outer):
            for i in range(steps+1):
                a=start+(end-start)*i/steps
                vertices.append((radius*math.cos(a),radius*math.sin(a),zz))
    n=steps+1; faces=[]
    for i in range(steps):
        faces.extend(((i,i+1,n+i+1,n+i),(2*n+i,3*n+i,3*n+i+1,2*n+i+1),
                      (i,2*n+i,2*n+i+1,i+1),(n+i,n+i+1,3*n+i+1,3*n+i)))
    faces.extend(((0,n,3*n,2*n),(steps,2*n+steps,3*n+steps,n+steps)))
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(vertices,[],faces); mesh.update()
    obj=bpy.data.objects.new(name,mesh); scene.collection.objects.link(obj)
    return finish(obj,name,material,min(height*.15,.01))

def curve_line(name,points,z,width,material,cyclic=False,height=.006):
    curve=bpy.data.curves.new(name,'CURVE'); curve.dimensions='3D'; curve.resolution_u=2
    curve.bevel_depth=width/2; curve.bevel_resolution=3
    spline=curve.splines.new('POLY'); spline.points.add(len(points)-1)
    for p,point in zip(spline.points,points): p.co=(*point,0,1)
    spline.use_cyclic_u=cyclic
    obj=bpy.data.objects.new(name,curve); scene.collection.objects.link(obj)
    # Flatten the circular curve bevel into a shallow embedded inlay. A round
    # cross-section made the old markings look like wires sitting on the floor.
    obj.location.z=z; obj.scale.z=height/max(width,1e-4)
    obj.data.materials.append(material); return obj

def join_asset(name,objects,origin=(0,0,0)):
    bpy.ops.object.select_all(action='DESELECT')
    for obj in objects: obj.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.object.convert(target='MESH'); bpy.ops.object.join()
    result=bpy.context.object; result.name=name; result.data.name=name+'_Mesh'
    scene.cursor.location=origin; bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    return result

def export_asset(obj):
    exports=ROOT/'exports'; exports.mkdir(parents=True,exist_ok=True)
    saved_location=obj.location.copy(); saved_rotation=obj.rotation_euler.copy()
    obj.location=(0,0,0); obj.rotation_euler=(0,0,0)
    bpy.ops.object.select_all(action='DESELECT'); obj.select_set(True); bpy.context.view_layer.objects.active=obj
    bpy.ops.export_scene.fbx(filepath=str(exports/(obj.name+'.fbx')),use_selection=True,
        object_types={'MESH'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE')
    bpy.ops.export_scene.gltf(filepath=str(exports/(obj.name+'.glb')),use_selection=True,export_format='GLB')
    obj.location=saved_location; obj.rotation_euler=saved_rotation

# Static arena: all artwork remains inside/under the authoritative 650 cm edge.
# The layout deliberately echoes the simple 1v1 arena, but each graphic is a
# shallow physical inlay rather than a coplanar decal. That prevents flicker and
# gives grazing light something to describe at the normal gameplay camera.
parts=[]
deck_surface_clearance=.060
parts += [cylinder(
    # Stop the structural cylinder underneath the outer-chassis ring. Giving
    # both meshes the exact 650 cm outer wall made their side faces coplanar,
    # which produced severe depth-buffer flicker around the arena edge.
    'Structural deck',arena_radius*.982,arena_thickness-deck_surface_clearance,
    -arena_thickness/2-deck_surface_clearance/2,graphite,192,.025)]
parts += [cylinder('Lower armored deck',arena_radius*.997,.18,-arena_thickness+.09,rubber,192,.025)]
parts += [ring('Lower titanium reveal',arena_radius*.958,arena_radius*.997,-arena_thickness+.205,.055,accent_metal,192)]
parts += [cylinder('Playing surface',arena_radius*.960,.055,-.0275,floor_mat,192,.012)]
# The broad inner tactical field and outer lane reproduce the large-value zones
# of the original 1v1 arena while remaining only fractions of a centimeter high.
parts += [cylinder('Inner tactical field',arena_radius*.705,.004,-.001,inner_field,192,.001)]
parts += [ring('Outer tactical lane',arena_radius*.708,arena_radius*.956,-.001,.004,floor_mat,192)]
# The authoritative outer dividers reach 629.75 cm and their sockets reach
# 633.75 cm. Start the visible rim beyond both footprints instead of letting
# its trim intersect the mechanisms.
parts += [ring('Outer chassis',arena_radius*.968,arena_radius,-.20,.31,rubber,192)]
# The complete upper rim is a solid annular collar whose top is flush with the
# authoritative Z=0 play surface. Earlier raised clamps reached 16.5 cm above
# that collider, so a puck correctly leaving the arena appeared to phase through
# decorative geometry. These shallow layers retain the premium segmentation
# without presenting any non-colliding wall above the gameplay surface.
parts += [ring('Titanium rim seat',arena_radius*.960,arena_radius*.997,-.020,.040,titanium,192)]
parts += [ring('Inner rim shadow',arena_radius*.960,arena_radius*.967,-.006,.012,recess,192)]
parts += [ring('Top rail chamfer',arena_radius*.969,arena_radius*.996,-.008,.016,accent_metal,192)]

# Layered center target: a pale inset, dark separation groove and fine metal lip.
parts += [cylinder('Center field plate',arena_radius*.142,.006,-.002,center_inset,128,.001)]
parts += [ring('Center plate shadow',arena_radius*.142,arena_radius*.151,-.0015,.005,recess,128)]
parts += [ring('Center plate lip',arena_radius*.151,arena_radius*.154,-.001,.004,line_mat,128)]
parts += [cylinder('Center spot recess',.052,.005,-.0015,dark_marking,64,.001)]
parts += [cylinder('Center spot',.025,.004,-.001,line_mat,48,.001)]

# Strong orthogonal axes plus 24 fine radial construction lines match the visual
# grammar of the normal 1v1 arena without overwhelming the switch information.
for name,points in (
    ('Primary center line',[(0,-arena_radius*.945),(0,arena_radius*.945)]),
    ('Primary cross line',[(-arena_radius*.945,0),(arena_radius*.945,0)])):
    parts.append(curve_line(name,points,-.002,.014,line_mat))
for index in range(24):
    angle=2*math.pi*index/24
    start=Vector((math.cos(angle),math.sin(angle)))*arena_radius*.165
    end=Vector((math.cos(angle),math.sin(angle)))*arena_radius*.925
    width=.010 if index%3==0 else .006
    parts.append(curve_line('Radial field spoke',[(start.x,start.y),(end.x,end.y)],-.002,width,dark_marking))

# Segmented circular records read like the dashed rings in the original Unreal
# construction, but each dash has a tiny bevel and proper metallic response.
for ring_index,fraction in enumerate((.36,.52,.705,.855)):
    segment_count=48
    radius=arena_radius*fraction
    for index in range(segment_count):
        center=2*math.pi*index/segment_count
        half=math.pi/segment_count*(.50 if ring_index<2 else .62)
        parts.append(arc('Segmented field ring',radius-.010,radius+.010,-.003,.008,
                         center-half,center+half,dark_marking,5))

# Four large cardinal arrows are the brightest non-emissive floor markings.
for angle in (0,math.pi/2,math.pi,3*math.pi/2):
    radial=Vector((math.cos(angle),math.sin(angle))); tangent=Vector((-radial.y,radial.x))
    tip=radial*arena_radius*.805
    base=radial*arena_radius*.755
    parts.append(curve_line('Direction arrow left',[(base.x-tangent.x*.25,base.y-tangent.y*.25),(tip.x,tip.y)],-.004,.040,line_mat,height=.010))
    parts.append(curve_line('Direction arrow right',[(tip.x,tip.y),(base.x+tangent.x*.25,base.y+tangent.y*.25)],-.004,.040,line_mat,height=.010))

# Segmented machined rim modules and broad team light rails.
rim_segments=40
for index in range(rim_segments):
    center=2*math.pi*index/rim_segments; half=math.pi/rim_segments*.80
    parts.append(arc('Rim armor segment',arena_radius*.972,arena_radius*.994,-.006,.012,center-half,center+half,titanium,12))
    team=cyan if math.sin(center)>=0 else orange
    parts.append(arc('Team rim lens',arena_radius*.961,arena_radius*.969,-.005,.010,center-half*.91,center+half*.91,team,12))
    parts.append(arc('Rim graphite insert',arena_radius*.978,arena_radius*.988,-.004,.008,center-half*.60,center+half*.60,graphite,8))
    # Flush dark separators keep the repeated rail manufactured and readable.
    radial=Vector((math.cos(center),math.sin(center)))
    parts.append(cube('Rim module clamp',(radial.x*arena_radius*.984,radial.y*arena_radius*.984,-.006),
        (.16,.085,.012),graphite,center,bevel=.003))
static_asset=join_asset('SM_TestArena_Static',parts)

# Modular divider: exact nominal exterior dimensions, bottom-center pivot.
parts=[]
parts.append(cube('Divider lower plinth',(0,0,divider_height*.07),(divider_length*.97,divider_thickness*.84,divider_height*.14),recess,bevel=.018))
parts.append(cube('Divider structural core',(0,0,divider_height*.48),(divider_length*.94,divider_thickness*.68,divider_height*.80),graphite,bevel=.028))
for side in (-1,1):
    parts.append(cube('Divider face frame',(0,side*divider_thickness*.405,divider_height*.49),
        (divider_length*.82,divider_thickness*.14,divider_height*.69),titanium,bevel=.012))
    parts.append(cube('Divider face inset',(0,side*divider_thickness*.478,divider_height*.49),
        (divider_length*.72,divider_thickness*.025,divider_height*.52),inner_field,bevel=.006))
    parts.append(cube('Divider lower light',(0,side*divider_thickness*.494,divider_height*.20),
        (divider_length*.58,divider_thickness*.012,divider_height*.035),switch_mat,bevel=.004))
parts.append(cube('Divider silver crown',(0,0,divider_height*.91),(divider_length*.94,divider_thickness*.78,divider_height*.12),titanium,bevel=.012))
parts.append(cube('Divider illuminated cap',(0,0,divider_height*.982),(divider_length*.84,divider_thickness*.52,divider_height*.036),switch_mat,bevel=.006))
for side in (-1,1):
    parts.append(cube('Divider end brace',(side*divider_length*.47,0,divider_height*.48),
        (divider_length*.06,divider_thickness,divider_height*.88),rubber,bevel=.014))
    parts.append(cube('Divider end metal',(side*divider_length*.468,0,divider_height*.50),
        (divider_length*.018,divider_thickness*.72,divider_height*.56),accent_metal,bevel=.006))
divider_asset=join_asset('SM_TestArena_Divider',parts,origin=(0,0,0))

socket_asset=join_asset('SM_TestArena_DividerSocket',[
    cube('Socket machined frame',(0,0,-.008),(divider_length,divider_thickness+.08,.016),accent_metal,bevel=.008),
    cube('Socket deep channel',(0,0,-.007),(divider_length*.94,divider_thickness*.72,.014),recess,bevel=.006),
    cube('Socket status rail',(0,0,-.005),(divider_length*.82,divider_thickness*.22,.010),switch_mat,bevel=.004)
],origin=(0,0,0))

switch_asset=join_asset('SM_TestArena_SwitchHousing',[
    # Blender's cylinder primitive takes a radius, so the complete housing stays
    # inside the same 40 cm gameplay footprint as ControlZoneRadius.
    cylinder('Switch shadow well',switch_radius,.012,-.006,recess,128,.004),
    ring('Switch outer metal bevel',switch_radius*.82,switch_radius,-.011,.022,titanium,128),
    ring('Switch graphite separator',switch_radius*.75,switch_radius*.82,-.009,.018,graphite,128),
    ring('Switch illuminated lens',switch_radius*.68,switch_radius*.75,-.007,.014,switch_mat,128),
    cylinder('Switch inner field',switch_radius*.675,.020,-.010,inner_field,128,.005)
],origin=(0,0,0))
dot_asset=join_asset('SM_TestArena_SwitchDot',[
    cylinder('Activation dot gasket',dot_radius*1.28,.015,-.0075,recess,96,.004),
    ring('Activation dot metal lip',dot_radius*.92,dot_radius*1.12,-.009,.018,titanium,96),
    cylinder('Activation dot',dot_radius,.030,-.015,switch_mat,96,.006)
],origin=(0,0,0))
trace_asset=join_asset('SM_TestArena_SignalTrace',[
    cube('Trace metal bed',(0,0,-.004),(1.0,.050,.008),accent_metal,bevel=.003),
    cube('Trace channel',(0,0,-.005),(1.0,.032,.010),recess,bevel=.003),
    cube('Trace lens',(0,0,-.005),(1.0,.014,.010),switch_mat,bevel=.003)
],origin=(-.5,0,0))

assets=[static_asset,divider_asset,socket_asset,switch_asset,dot_asset,trace_asset]
for asset in assets: export_asset(asset)

# Preview collection uses linked duplicates; source export objects remain reusable.
preview=bpy.data.collections.new('SEEDED LAYOUT PREVIEW'); scene.collection.children.link(preview)
for asset in assets:
    for collection in list(asset.users_collection): collection.objects.unlink(asset)
    source_collection=bpy.data.collections.get('EXPORT ASSETS')
    if source_collection is None:
        source_collection=bpy.data.collections.new('EXPORT ASSETS'); scene.collection.children.link(source_collection)
    source_collection.objects.link(asset); asset.hide_render=True

locations=[]
active_indices=[0,2,5,7,10,12,15,17]
for index in range(possible_count):
    outer=index%2==0; radial_angle=math.radians(index*(360/possible_count))
    radial=Vector((math.cos(radial_angle),math.sin(radial_angle)))
    center=radial*arena_radius*(outer_radius_fraction if outer else .82)
    angle=radial_angle+math.pi/2+(0 if outer else math.radians(11 if index%4==1 else -11))
    length=divider_length*(.92 if outer else 1.08)
    socket=socket_asset.copy(); socket.data=socket_asset.data.copy(); socket.hide_render=False
    preview.objects.link(socket); socket.location=(center.x,center.y,0); socket.rotation_euler.z=angle; socket.scale.x=length/divider_length
    entry={'index':index,'outer':outer,'center_cm':[round(center.x*100,3),round(center.y*100,3)],
           'angle_degrees':round(math.degrees(angle),3),'length_cm':round(length*100,3),'active':index in active_indices}
    if index in active_indices:
        direction=1 if index%4<2 else -1
        switch_angle=radial_angle+math.radians(direction*3)
        switch_distance_from_center=max(arena_radius*.35,center.length-switch_distance)
        switch_center=Vector((math.cos(switch_angle),math.sin(switch_angle)))*switch_distance_from_center
        for source,z in ((switch_asset,0),(dot_asset,0)):
            copy=source.copy(); copy.data=source.data.copy(); copy.hide_render=False; preview.objects.link(copy)
            copy.location=(switch_center.x,switch_center.y,z)
        delta=center-switch_center; trace=trace_asset.copy(); trace.data=trace_asset.data.copy(); trace.hide_render=False
        preview.objects.link(trace); trace.location=(switch_center.x,switch_center.y,0); trace.rotation_euler.z=math.atan2(delta.y,delta.x); trace.scale.x=max(.01,delta.length)
        divider=divider_asset.copy(); divider.data=divider_asset.data.copy(); divider.hide_render=False
        preview.objects.link(divider); divider.location=(center.x,center.y,.025); divider.rotation_euler.z=angle; divider.scale.x=length/divider_length
        entry['switch_center_cm']=[round(switch_center.x*100,3),round(switch_center.y*100,3)]
    locations.append(entry)

static_preview=static_asset.copy(); static_preview.data=static_asset.data.copy(); static_preview.hide_render=False; preview.objects.link(static_preview)

# Studio environment and render.
studio=mat('Studio floor',(.008,.014,.024),.15,.72)
cylinder('Studio ground',arena_radius*3.0,.08,-arena_thickness-.09,studio,192,.02)
def area(name,location,energy,size,color):
    bpy.ops.object.light_add(type='AREA',location=location); light=bpy.context.object; light.name=name
    light.data.energy=energy; light.data.shape='DISK'; light.data.size=size; light.data.color=color
    light.rotation_euler=(Vector((0,0,0))-light.location).to_track_quat('-Z','Y').to_euler()
area('Neutral key',(-6,-4,9),1350,5.5,(.82,.90,1))
area('Blue edge',(-7,1,5.5),700,3.5,(.25,.68,1))
area('Orange edge',(7,2,5.5),700,3.5,(1,.38,.10))
area('Top softbox',(0,-3,11),1200,6.5,(.94,.97,1))
bpy.ops.object.camera_add(location=(0,-18.0,15.5)); camera=bpy.context.object
camera.rotation_euler=(Vector((0,0,-.15))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.lens=54; scene.camera=camera
scene.render.engine='BLENDER_EEVEE'; scene.render.resolution_x=1920; scene.render.resolution_y=1200
scene.render.resolution_percentage=100; scene.render.image_settings.file_format='PNG'
scene.render.film_transparent=False; scene.world.color=(.002,.005,.012)
try:
    scene.view_settings.look='AgX - Medium High Contrast'
except TypeError:
    pass
scene.render.filepath=str(ROOT/'renders'/'test_arena_preview.png'); (ROOT/'renders').mkdir(exist_ok=True)
manifest={'arena_radius_cm':arena_radius*100,'arena_thickness_cm':arena_thickness*100,
          'divider_nominal_cm':[divider_length*100,divider_thickness*100,divider_height*100],
          'switch_radius_cm':switch_radius*100,'activation_dot_radius_cm':dot_radius*100,
          'rim_inner_radius_cm':arena_radius*.977*100,
          'outer_divider_radius_fraction':outer_radius_fraction,
          'possible_locations':locations,'exports':[asset.name for asset in assets]}
(ROOT/'dimensions.json').write_text(json.dumps(manifest,indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'TestArenaWorkshop.blend'))
bpy.ops.render.render(write_still=True)
print('FLICK_TEST_ARENA_GENERATION_COMPLETE',len(assets),'assets',len(locations),'locations')
