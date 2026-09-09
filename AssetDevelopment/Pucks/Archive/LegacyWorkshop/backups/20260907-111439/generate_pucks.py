"""Run with Blender --background --python this_file. No external dependencies."""
import bpy, math, json, re
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT.parents[1]
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
# Model in meters; game defaults are centimeters. Origin is collider center.
rules = (PROJECT/'Source/FLICK/Core/FlickPieceArchetypeRules.cpp').read_text()
defaults = (PROJECT/'Source/FLICK/Game/FlickGameMode.h').read_text()
base_r = float(re.search(r'float PieceRadius = ([\d.]+)f', defaults)[1])/100
base_h = float(re.search(r'float PieceThickness = ([\d.]+)f', defaults)[1])/100
names = ['Standard','Toppler','Bouncer','Compact','Blocker','Slider','Grippy','Striker','Heavy']

def material(name, color, metallic, roughness, emission=0):
    m=bpy.data.materials.new(name); m.diffuse_color=(*color,1); m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=(*color,1)
    p.inputs['Metallic'].default_value=metallic; p.inputs['Roughness'].default_value=roughness
    if emission:
        p.inputs['Emission Color'].default_value=(*color,1)
        p.inputs['Emission Strength'].default_value=emission
    return m
silver=material('01_Satin_Titanium',(0.34,0.40,0.46),0.88,0.34)
dark=material('02_Graphite_Chassis',(0.018,0.028,0.037),0.7,0.32)
rubber=material('03_Grip_Polymer',(0.009,0.013,0.018),0.05,0.58)
inset=material('04_Ceramic_Insert',(0.009,0.018,0.024),0.18,0.4)
cyan=material('05_Team_Cyan',(0.002,0.48,0.8),0.1,0.3,1.5)
orange=material('05_Team_Orange',(1.0,0.18,0.003),0.1,0.3,1.5)
black=material('06_Recess',(0.004,0.008,0.011),0.2,0.42)

def finish(obj,name,mat,bevel=0):
    obj.name=name; obj.data.materials.append(mat)
    if bevel:
        m=obj.modifiers.new('Machined edge','BEVEL'); m.width=bevel; m.segments=3
    for p in obj.data.polygons: p.use_smooth=True
    n=obj.modifiers.new('Surface normals','WEIGHTED_NORMAL'); n.keep_sharp=True
    return obj

def cyl(name,r,h,z,mat):
    bpy.ops.mesh.primitive_cylinder_add(vertices=96,radius=r,depth=h,location=(0,0,z))
    return finish(bpy.context.object,name,mat,min(h*.18,r*.018))

def arc(name,ri,ro,z,h,start,end,mat,steps=32):
    verts=[]
    for zz in [z-h/2,z+h/2]:
        for rr in [ri,ro]:
            for j in range(steps+1):
                a=start+(end-start)*j/steps; verts.append((rr*math.cos(a),rr*math.sin(a),zz))
    n=steps+1; faces=[]
    for j in range(steps):
        faces += [(j,j+1,n+j+1,n+j),(2*n+j,3*n+j,3*n+j+1,2*n+j+1),
                  (j,2*n+j,2*n+j+1,j+1),(n+j,n+j+1,3*n+j+1,3*n+j)]
    faces += [(0,n,3*n,2*n),(steps,2*n+steps,3*n+steps,n+steps)]
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(verts,[],faces); mesh.update()
    obj=bpy.data.objects.new(name,mesh); scene.collection.objects.link(obj)
    return finish(obj,name,mat,min(h*.15,.0015))

def line(name,pts,z,width,mat):
    curve=bpy.data.curves.new(name,'CURVE'); curve.dimensions='3D'; curve.resolution_u=1
    curve.bevel_depth=width/2; curve.bevel_resolution=3
    sp=curve.splines.new('POLY'); sp.points.add(len(pts)-1)
    for p,xy in zip(sp.points,pts): p.co=(xy[0],xy[1],z,1)
    obj=bpy.data.objects.new(name,curve); scene.collection.objects.link(obj); obj.data.materials.append(mat)
    return obj

def box(name,loc,scale,mat,angle=0):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc); o=bpy.context.object
    o.dimensions=scale; bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    o.rotation_euler.z=angle; return finish(o,name,mat,min(scale)*.18)

def emblem_polygon(points,scale,z):
    """Flat filled inlay, rather than a raised wire outline."""
    curve=bpy.data.curves.new('Emblem inlay','CURVE')
    curve.dimensions='2D'; curve.fill_mode='BOTH'; curve.extrude=.0004
    sp=curve.splines.new('POLY'); sp.points.add(len(points)-1)
    for p,(x,y) in zip(sp.points,points): p.co=(x*scale,y*scale,0,1)
    sp.use_cyclic_u=True
    obj=bpy.data.objects.new('Class emblem inlay',curve)
    scene.collection.objects.link(obj); obj.location.z=z; obj.data.materials.append(cyan)
    return obj

label_mat=material('Studio lettering',(.6,.75,.85),0,.7,.25)
manifest=[]; groups=[]; orange_groups=[]; labels=[]
blue_collection=bpy.data.collections.new('BLUE TEAM'); scene.collection.children.link(blue_collection)
orange_collection=bpy.data.collections.new('ORANGE TEAM'); scene.collection.children.link(orange_collection)
(ROOT/'exports'/'Orange').mkdir(parents=True,exist_ok=True)
for index,name in enumerate(names):
    body=re.search(r'const FFlickPieceArchetypeRules '+name+r'Rules = \[\].*?\}\(\);',rules,re.S)
    def multiplier(key):
        m=re.search(key+r' = ([\d.]+)f',body[0]) if body else None
        return float(m[1]) if m else 1
    r=base_r*multiplier('RadiusMultiplier'); h=base_h*multiplier('ThicknessMultiplier')
    before=set(bpy.data.objects)
    cyl(name+'_Body',r*.985,h*.82,-h*.04,dark)
    cyl(name+'_Bottom',r*.94,h*.09,-h*.455,rubber)
    arc('Lower titanium rail',r*.94,r*.985,-h*.34,h*.055,0,2*math.pi,silver,96)
    arc('Lower team light',r*.982,r*.993,-h*.26,h*.035,0,2*math.pi,cyan,96)
    arc('Shoulder seat',r*.60,r*.98,h*.34,h*.14,0,2*math.pi,black,96)
    arc('Machined shoulder',r*.64,r*.86,h*.415,h*.10,0,2*math.pi,silver,96)
    cyl('Center insert',r*.625,h*.065,h*.412,inset)
    arc('Inner light seal',r*.612,r*.632,h*.46,h*.018,0,2*math.pi,cyan,96)
    arc('Inner dark groove',r*.645,r*.67,h*.469,h*.009,0,2*math.pi,black,96)
    segments=6 if name in ['Grippy','Heavy','Bouncer'] else 8
    for j in range(segments):
        a=2*math.pi*j/segments; span=2*math.pi/segments
        arc('Segment socket',r*.86,r*.997,h*.33,h*.13,a+.035,a+span-.035,dark)
        arc('Cyan shoulder insert',r*.875,r*.974,h*.407,h*.025,a+.085,a+span-.085,cyan)
        # Side panels and recessed fasteners establish physical construction.
        arc('Side armor',r*.979,r*.998,-h*.015,h*.40,a+.045,a+span-.045,rubber if name=='Grippy' else dark)
        mid=a+span/2
        bolt=cyl('Recessed fastener',r*.013,h*.015,h*.477,black)
        bolt.location.x=r*.82*math.cos(mid); bolt.location.y=r*.82*math.sin(mid)
        box('Fastener slot',(bolt.location.x,bolt.location.y,h*.486),(r*.016,r*.003,h*.004),silver,mid)
        if name in ['Heavy','Grippy','Toppler']:
            arc('Reinforced grip lug',r*.93,r,0,h*.82,a-.085,a+.085,silver if name=='Heavy' else rubber,8)
    z=h*.45; s=r*.31; w=r*.045
    def stroke(pts): return line('Class emblem',[(x*s,y*s) for x,y in pts],z,w,cyan)
    def fill(pts): return emblem_polygon(pts,s,z)
    if name in ['Standard','Compact','Striker']:
        rr=s*(.60 if name=='Compact' else .92)
        arc('Class ring',rr-w/2,rr+w/2,z,h*.012,0,2*math.pi,cyan,64)
        if name=='Striker': cyl('Class dot',s*.16,h*.014,z,cyan)
        if name=='Striker':
            for j in range(4):
                a=j*math.pi/2
                line('Sight tick',[(s*v*math.cos(a),s*v*math.sin(a)) for v in [.65,1.3]],z,w,cyan)
    elif name=='Toppler':
        fill([(-.95,.18),(0,1.03),(.95,.18),(.58,.18),(0,.70),(-.58,.18)])
        fill([(-.94,-.42),(0,.38),(.94,-.42)])
        fill([(-.23,-.65),(.23,-.65),(.23,-.88),(-.23,-.88)])
    elif name=='Slider':
        for x in [-.85,-.05,.75]:
            fill([(x-.42,-.62),(x+.20,0),(x-.42,.62),(x-.06,.62),(x+.56,0),(x-.06,-.62)])
    elif name=='Blocker':
        stroke([(-.78,.6),(0,.92),(.78,.6),(.68,-.45),(0,-.95),(-.68,-.45),(-.78,.6)])
        fill([(-.48,.38),(0,.59),(.48,.38),(.42,-.25),(0,-.59),(-.42,-.25)])
    elif name=='Grippy': fill([(-1,-.65),(-.25,.5),(0,.25),(.25,.85),(1,-.65),(.48,-.29),(.55,-.51),(0,-.02),(-.55,-.51),(-.48,-.29)])
    elif name=='Heavy':
        fill([(-.60,-.13),(.60,-.13),(.60,.13),(-.60,.13)])
        for x in [-.82,.82]: fill([(x-.20,-.64),(x+.20,-.64),(x+.20,.64),(x-.20,.64)])
        for x in [-1.16,1.16]: fill([(x-.09,-.34),(x+.09,-.34),(x+.09,.34),(x-.09,.34)])
    else:
        # Updated reference: a flat bounce symbol with a ball and two curved paths.
        ball=cyl('Bounce symbol ball',s*.26,h*.012,z,cyan); ball.location.y=s*.48
        for side in [-1,1]:
            pts=[]
            for k in range(25):
                t=k/24
                pts.append((side*(.15+.92*t),-.68+.85*t-.30*t*t))
            stroke(pts)
    objects=sorted(set(bpy.data.objects)-before,key=lambda o:o.name)
    # Apply modifiers and merge by puck for predictable import and material slots.
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects: o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.object.convert(target='MESH'); bpy.ops.object.join()
    puck=bpy.context.object; puck.name='SM_Puck_'+name
    scene.cursor.location=(0,0,0); bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    puck.data.name='Shared_Puck_'+name
    for collection in list(puck.users_collection): collection.objects.unlink(puck)
    blue_collection.objects.link(puck)
    bpy.ops.export_scene.gltf(filepath=str(ROOT/'exports'/f'{name}.glb'),use_selection=True,export_format='GLB')
    bpy.ops.export_scene.fbx(filepath=str(ROOT/'exports'/f'{name}.fbx'),use_selection=True,object_types={'MESH'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z')
    # One mesh datablock per class; only the team material differs between objects.
    other=puck.copy(); other.data=puck.data; other.name='SM_Puck_'+name+'_Orange'
    orange_collection.objects.link(other)
    for slot in other.material_slots:
        original=slot.material; slot.link='OBJECT'
        slot.material=orange if original==cyan else original
    bpy.ops.object.select_all(action='DESELECT'); other.select_set(True)
    bpy.context.view_layer.objects.active=other
    bpy.ops.export_scene.gltf(filepath=str(ROOT/'exports'/'Orange'/f'{name}.glb'),use_selection=True,export_format='GLB')
    bpy.ops.export_scene.fbx(filepath=str(ROOT/'exports'/'Orange'/f'{name}.fbx'),use_selection=True,object_types={'MESH'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z')
    assert other.data is puck.data
    actual=[round(v*100,3) for v in puck.dimensions]
    assert max(actual[:2]) <= r*200+.5, (name,actual)
    assert actual[2] <= h*100+.5, (name,actual)
    manifest.append(dict(name=name,radius_cm=r*100,thickness_cm=h*100,mesh_dimensions_cm=actual,triangles=sum(len(p.vertices)-2 for p in puck.data.polygons)))
    x=(index%5-2)*1.27 if index<5 else (index-5-1.5)*1.46
    y=1.05 if index<5 else -.75
    puck.location=(x,y,h/2+.015); groups.append(puck)
    other.location=(x+8,y,h/2+.015); orange_groups.append(other)
    bpy.ops.object.text_add(location=(x,y-r-.17,.008)); t=bpy.context.object
    t.name='Label_'+name; t.data.body=name.upper(); t.data.align_x='CENTER'; t.data.size=.105; t.data.space_character=1.3; t.data.materials.append(label_mat)
    labels.append(t)
    other_label=t.copy(); other_label.data=t.data; scene.collection.objects.link(other_label); other_label.location.x+=8

(ROOT/'dimensions.json').write_text(json.dumps(manifest,indent=2))
floor=material('Studio floor',(.035,.045,.060),.05,.65)
box('Studio plinth',(0,0,-.06),(200,200,.1),floor)
def area(name,loc,power,size,color):
    bpy.ops.object.light_add(type='AREA',location=loc); o=bpy.context.object; o.name=name
    o.data.energy=power; o.data.shape='DISK'; o.data.size=size; o.data.color=color
    o.rotation_euler=(Vector((0,0,0))-o.location).to_track_quat('-Z','Y').to_euler()
area('Large softbox',(-3,-1,7),750,5,(.85,.93,1))
area('Silver edge light',(2,4,5),950,4,(.7,.88,1))
area('Front fill',(1,-5,3),200,4,(1,.91,.83))
for light in [o for o in scene.objects if o.type=='LIGHT']:
    duplicate=light.copy(); duplicate.data=light.data; scene.collection.objects.link(duplicate); duplicate.location.x+=8
bpy.ops.object.camera_add(location=(0,-8.0,7.0)); cam=bpy.context.object
cam.rotation_euler=(Vector((0,.2,0))-cam.location).to_track_quat('-Z','Y').to_euler()
cam.data.type='ORTHO'; cam.data.ortho_scale=7.45; scene.camera=cam
scene.render.engine='CYCLES'; scene.cycles.samples=32; scene.cycles.use_denoising=True
scene.render.resolution_x=1800; scene.render.resolution_y=1100; scene.render.resolution_percentage=100
scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.18
scene.view_settings.exposure=-.4
scene.render.image_settings.file_format='PNG'; scene.render.filepath=str(ROOT/'renders'/'blue_team_sheet.png')
# Open the workshop with both teams in view and material colors visible.
for screen in bpy.data.screens:
    for viewport in screen.areas:
        if viewport.type=='VIEW_3D':
            viewport.spaces.active.shading.type='MATERIAL'
            viewport.spaces.active.region_3d.view_location=(4,0,0)
            viewport.spaces.active.region_3d.view_distance=17
            viewport.spaces.active.region_3d.view_rotation=cam.rotation_euler.to_quaternion()
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'PuckWorkshop.blend'))
bpy.ops.render.render(write_still=True)
cam.location.x+=8
scene.render.filepath=str(ROOT/'renders'/'orange_team_sheet.png')
bpy.ops.render.render(write_still=True)
print('FLICK_PUCK_GENERATION_COMPLETE',len(manifest))
