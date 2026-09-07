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

graphite=mat('01_Arena_Graphite',(0.055,0.075,0.10),.68,.30)
floor_mat=mat('02_Arena_Surface',(0.10,0.14,0.19),.32,.55)
rubber=mat('03_Rim_Polymer',(0.016,0.024,0.035),.04,.60)
titanium=mat('04_Brushed_Titanium',(.42,.50,.58),.92,.21)
recess=mat('05_Deep_Recess',(.002,.006,.012),.28,.55)
cyan=mat('06_Team_Cyan',(0,.20,.48),.05,.32,2.0)
orange=mat('07_Team_Orange',(.75,.075,.003),.05,.32,2.0)
line_mat=mat('08_Floor_Lines',(.42,.51,.60),.48,.42)
switch_mat=mat('09_Switch_Accent',(.02,.48,.72),.12,.34,1.25)

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

def curve_line(name,points,z,width,material,cyclic=False):
    curve=bpy.data.curves.new(name,'CURVE'); curve.dimensions='3D'; curve.resolution_u=2
    curve.bevel_depth=width/2; curve.bevel_resolution=3
    spline=curve.splines.new('POLY'); spline.points.add(len(points)-1)
    for p,point in zip(spline.points,points): p.co=(*point,z,1)
    spline.use_cyclic_u=cyclic
    obj=bpy.data.objects.new(name,curve); scene.collection.objects.link(obj)
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
# Keep the structural deck below the finish layer. Coplanar top faces cause
# severe depth-buffer flicker once the combined mesh is imported into Unreal.
parts=[]
deck_surface_clearance=.060
parts += [cylinder(
    'Structural deck',arena_radius,arena_thickness-deck_surface_clearance,
    -arena_thickness/2-deck_surface_clearance/2,graphite,192,.025)]
parts += [cylinder('Playing surface',arena_radius*.990,.055,-.0275,floor_mat,192,.012)]
# The authoritative outer dividers reach 629.75 cm and their sockets reach
# 633.75 cm. Start the visible rim beyond both footprints instead of letting
# its trim intersect the mechanisms.
parts += [ring('Outer chassis',arena_radius*.987,arena_radius, -.19,.30,rubber)]
parts += [ring('Titanium rim seat',arena_radius*.981,arena_radius*.987,-.02,.105,titanium)]
parts += [ring('Inner rim shadow',arena_radius*.977,arena_radius*.981,.006,.025,recess)]
parts += [ring('Center circle recess',arena_radius*.135,arena_radius*.137,.013,.012,line_mat)]
parts += [cylinder('Center spot',.035,.014,.007,line_mat,48,.003)]
curve_line('Center seam',[(0,-arena_radius*.88),(0,arena_radius*.88)],.014,.018,line_mat)
parts.append(bpy.data.objects['Center seam'])
# Subtle radial construction seams and inset field records.
for index in range(12):
    angle=2*math.pi*index/12
    start=Vector((math.cos(angle),math.sin(angle)))*arena_radius*.16
    end=Vector((math.cos(angle),math.sin(angle)))*arena_radius*.91
    parts.append(curve_line('Radial deck seam',[(start.x,start.y),(end.x,end.y)],.008,.008,recess))
for fraction in (.42,.68,.90): parts.append(ring('Field machining ring',arena_radius*fraction-.006,arena_radius*fraction,.009,.006,recess))
# Segmented rim armor and team-side light channels.
rim_segments=44
for index in range(rim_segments):
    center=2*math.pi*index/rim_segments; half=math.pi/rim_segments*.82
    parts.append(arc('Rim armor segment',arena_radius*.984,arena_radius*.997,.045,.075,center-half,center+half,titanium,12))
    team=cyan if math.sin(center)>=0 else orange
    parts.append(arc('Team rim lens',arena_radius*.978,arena_radius*.982,.052,.026,center-half*.88,center+half*.88,team,12))
    # Dark clips visually separate the repeated manufactured modules.
    radial=Vector((math.cos(center),math.sin(center)))
    parts.append(cube('Rim module clip',(radial.x*arena_radius*.987,radial.y*arena_radius*.987,.087),
        (.15,.075,.055),graphite,center,bevel=.008))
# Four directional chevrons are recessed floor inlays.
for angle in (0,math.pi/2,math.pi,3*math.pi/2):
    radial=Vector((math.cos(angle),math.sin(angle))); tangent=Vector((-radial.y,radial.x))
    tip=radial*arena_radius*.77
    parts.append(curve_line('Direction chevron',[(tip.x-tangent.x*.24,tip.y-tangent.y*.24),
        (tip.x,tip.y),(tip.x+tangent.x*.24,tip.y+tangent.y*.24)],.016,.035,line_mat))
static_asset=join_asset('SM_TestArena_Static',parts)

# Modular divider: exact nominal exterior dimensions, bottom-center pivot.
parts=[]
parts.append(cube('Divider core',(0,0,divider_height*.47),(divider_length*.965,divider_thickness*.78,divider_height*.88),graphite,bevel=.025))
parts.append(cube('Divider front armor',(0,-divider_thickness*.43,divider_height*.48),(divider_length*.80,divider_thickness*.12,divider_height*.72),titanium,bevel=.012))
parts.append(cube('Divider back armor',(0,divider_thickness*.43,divider_height*.48),(divider_length*.80,divider_thickness*.12,divider_height*.72),titanium,bevel=.012))
parts.append(cube('Divider cap',(0,0,divider_height*.965),(divider_length*.92,divider_thickness*.70,divider_height*.07),switch_mat,bevel=.009))
for side in (-1,1):
    parts.append(cube('Divider end brace',(side*divider_length*.47,0,divider_height*.48),(divider_length*.06,divider_thickness,divider_height*.88),rubber,bevel=.012))
    parts.append(cylinder('Divider fastener',.018,.012,divider_height*.66,recess,32,.003))
    parts[-1].location.x=side*divider_length*.43; parts[-1].rotation_euler.x=math.pi/2
divider_asset=join_asset('SM_TestArena_Divider',parts,origin=(0,0,0))

socket_asset=join_asset('SM_TestArena_DividerSocket',[
    cube('Socket recess',(0,0,.008),(divider_length,divider_thickness+.08,.016),recess,bevel=.008),
    cube('Socket inner rail',(0,0,.019),(divider_length*.90,divider_thickness*.52,.010),switch_mat,bevel=.004)
],origin=(0,0,0))

switch_asset=join_asset('SM_TestArena_SwitchHousing',[
    # Blender's cylinder primitive takes a radius, so the complete housing stays
    # inside the same 40 cm gameplay footprint as ControlZoneRadius.
    cylinder('Switch outer',switch_radius,.012,.006,recess,96,.004),
    ring('Switch bezel',switch_radius*.76,switch_radius,.016,.020,titanium,96),
    ring('Switch lens',switch_radius*.67,switch_radius*.74,.028,.012,switch_mat,96),
    cylinder('Switch well',switch_radius*.66,.018,.009,graphite,96,.004)
],origin=(0,0,0))
dot_asset=join_asset('SM_TestArena_SwitchDot',[
    cylinder('Activation dot gasket',dot_radius*1.28,.015,.0075,recess,64,.004),
    cylinder('Activation dot',dot_radius,.026,.018,switch_mat,64,.005)
],origin=(0,0,0))
trace_asset=join_asset('SM_TestArena_SignalTrace',[
    cube('Trace channel',(0,0,.004),(1.0,.040,.008),recess,bevel=.003),
    cube('Trace lens',(0,0,.010),(1.0,.018,.010),switch_mat,bevel=.003)
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
    preview.objects.link(socket); socket.location=(center.x,center.y,.002); socket.rotation_euler.z=angle; socket.scale.x=length/divider_length
    entry={'index':index,'outer':outer,'center_cm':[round(center.x*100,3),round(center.y*100,3)],
           'angle_degrees':round(math.degrees(angle),3),'length_cm':round(length*100,3),'active':index in active_indices}
    if index in active_indices:
        direction=1 if index%4<2 else -1
        switch_angle=radial_angle+math.radians(direction*3)
        switch_distance_from_center=max(arena_radius*.35,center.length-switch_distance)
        switch_center=Vector((math.cos(switch_angle),math.sin(switch_angle)))*switch_distance_from_center
        for source,z in ((switch_asset,.012),(dot_asset,.042)):
            copy=source.copy(); copy.data=source.data.copy(); copy.hide_render=False; preview.objects.link(copy)
            copy.location=(switch_center.x,switch_center.y,z)
        delta=center-switch_center; trace=trace_asset.copy(); trace.data=trace_asset.data.copy(); trace.hide_render=False
        preview.objects.link(trace); trace.location=(switch_center.x,switch_center.y,.015); trace.rotation_euler.z=math.atan2(delta.y,delta.x); trace.scale.x=max(.01,delta.length)
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
area('Blue key',(-7,-2,8),1250,5,(.45,.78,1))
area('Orange rim',(7,2,6),1050,4,(1,.48,.18))
area('Top softbox',(0,-4,10),1550,6,(.9,.94,1))
bpy.ops.object.camera_add(location=(0,-19.5,17.0)); camera=bpy.context.object
camera.rotation_euler=(Vector((0,0,-.15))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.lens=52; scene.camera=camera
scene.render.engine='BLENDER_EEVEE'; scene.render.resolution_x=1800; scene.render.resolution_y=1100
scene.render.resolution_percentage=100; scene.render.image_settings.file_format='PNG'
scene.render.film_transparent=False; scene.world.color=(.002,.005,.012)
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
