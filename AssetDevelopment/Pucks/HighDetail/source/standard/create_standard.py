"""Reference puck, 0.90 m diameter x 0.20 m high.

BLENDER: Scripting > Open this file > Run Script (Alt-P).
Creates a NEW scene, then saves Standard.blend beside this script.
Regular Python writes an importable Standard.glb.
No third-party dependencies. Blender 4.x / 5.x compatible APIs.
"""
import math, json, struct, os
from pathlib import Path

TAU = math.tau
PARTS = []
MATERIALS = [
    ('Graphite anodized housing', (0.027, 0.040, 0.053), .88, .32, 0),
    ('Circular brushed silver', (.48, .55, .61), .97, .27, 0),
    ('Recessed dark titanium', (.019, .029, .039), .82, .39, 0),
    ('Cyan light diffuser', (.005, .60, .83), .15, .22, 5),
    ('Black gasket and sockets', (.004, .007, .010), .20, .48, 0),
    ('Machined edge highlights', (.21, .28, .33), .95, .25, 0),
    ('Cyan center emblem', (.003, .50, .68), .20, .30, 2),
]

def add(name, verts, normals, faces, material, group):
    PARTS.append(dict(name=name, verts=verts, normals=normals,
                      faces=faces, material=material, group=group))

def lathe(name, profile, material=0, start=0, end=TAU, group='Housing', steps=None,
          origin=(0,0,0), axis=None):
    """Closed revolved cross section with explicit split corner normals."""
    area=sum(profile[i][0]*profile[(i+1)%len(profile)][1]-
             profile[(i+1)%len(profile)][0]*profile[i][1] for i in range(len(profile)))
    if area < 0: profile=list(reversed(profile))
    steps = steps or max(8, round(384*(end-start)/TAU))
    v, n, f = [], [], []
    def transform(p, vector=False):
        if axis is None:
            q=p
        else:
            # Local z points along a horizontal radial axis.
            a=axis; c=math.cos(a); s=math.sin(a)
            q=(-s*p[0]+c*p[2], c*p[0]+s*p[2], p[1])
        return q if vector else tuple(q[i]+origin[i] for i in range(3))
    for j in range(len(profile)):
        r0,z0=profile[j]; r1,z1=profile[(j+1)%len(profile)]
        dr,dz=r1-r0,z1-z0; length=math.hypot(dr,dz)
        if length < 1e-10: continue
        base=len(v)
        for k in range(steps+1):
            a=start+(end-start)*k/steps; c,s=math.cos(a),math.sin(a)
            for r,z in [(r0,z0),(r1,z1)]:
                v.append(transform((r*c,r*s,z)))
                n.append(transform((dz*c/length,dz*s/length,-dr/length),True))
        for k in range(steps):
            i=base+2*k
            f.extend([(i,i+2,i+3),(i,i+3,i+1)])
    if end-start < TAU-1e-5:
        # Profiles here are convex, so centroid triangle fans close the ends.
        for a,sign in [(start,-1),(end,1)]:
            c,s=math.cos(a),math.sin(a); normal=transform((-sign*s,sign*c,0),True)
            base=len(v)
            rc=sum(p[0] for p in profile)/len(profile)
            zc=sum(p[1] for p in profile)/len(profile)
            v.append(transform((rc*c,rc*s,zc))); n.append(normal)
            for r,z in profile:
                v.append(transform((r*c,r*s,z))); n.append(normal)
            for j in range(len(profile)):
                q=(base,base+1+j,base+1+(j+1)%len(profile))
                f.append(q if sign<0 else q[::-1])
    add(name,v,n,f,material,group)

def ring(name,ri,ro,z0,z1,material=0,bevel=.001,**kw):
    b=min(bevel,(ro-ri)*.3,(z1-z0)*.3)
    lathe(name,[(ri+b,z0),(ro-b,z0),(ro,z0+b),(ro,z1-b),
                (ro-b,z1),(ri+b,z1),(ri,z1-b),(ri,z0+b)],material,**kw)

def screw(name,r,z,a,side=False):
    origin=(r*math.cos(a),r*math.sin(a),z)
    kw=dict(origin=origin,axis=a if side else None,group='Fasteners',steps=48)
    ring(name+' countersink',.00001,.0060,0,.0006,4,.0001,**kw)
    ring(name+' titanium rim',.0036,.0048,.0005,.0014,5,.0002,**kw)
    ring(name+' socket wall',.0020,.00365,.00035,.0010,0,.0001,**kw)
    kw['steps']=6
    ring(name+' hex recess',.00001,.0020,.0001,.0004,4,.00005,**kw)

def build_geometry():
    PARTS.clear()
    lathe('Continuous structural core',[(.00001,.008),(.425,.008),(.443,.024),
          (.443,.145),(.430,.169),(.00001,.169)],0)
    lathe('Rounded lower bumper',[(.00001,0),(.433,0),(.444,.004),(.449,.010),
          (.450,.018),(.449,.021),(.436,.023),(.00001,.023)],0)
    ring('Bottom edge metal reveal',.446,.449,.010,.012,5,.0005)
    ring('Lower continuous light channel',.442,.448,.021,.037,4,.0005,group='Light channels')
    # Four main quadrants, with reinforced diagonal joints.
    for i in range(4):
        c=i*math.pi/2
        a=c-math.radians(36); b=c+math.radians(36)
        lathe(f'Side armor panel {i+1}',[(.433,.038),(.443,.038),(.449,.042),
              (.450,.047),(.450,.125),(.448,.134),(.442,.138),(.433,.138)],0,
              a,b,group='Side armor')
        ring(f'Side panel upper highlight {i+1}',.449,.450,.126,.128,5,.0004,
             start=a+.003,end=b-.003,group='Side armor')
        # Lower blue strip, split by two narrow divider joints.
        for j,(da,db) in enumerate([(-36,-23.3),(-22.7,22.7),(23.3,36)]):
            ring(f'Lower cyan strip {i+1}.{j+1}',.443,.449,.024,.034,3,.001,
                 start=c+math.radians(da),end=c+math.radians(db),group='Cyan lights')
        lathe(f'Upper shoulder armor {i+1}',[(.422,.139),(.441,.139),(.449,.146),
              (.446,.158),(.433,.178),(.420,.183)],0,a,b,group='Top perimeter')
        lathe(f'Upper shoulder silver bevel {i+1}',[(.432,.174),(.436,.173),
              (.442,.166),(.441,.164),(.435,.169)],5,a,b,group='Top perimeter')
        # Top window follows the sloping perimeter.
        lathe(f'Top light black socket {i+1}',[(.346,.170),(.420,.170),(.424,.180),
              (.414,.184),(.360,.184),(.346,.180)],4,a,b,group='Light channels')
        for j,(da,db) in enumerate([(-35.5,-25.7),(-24.3,24.3),(25.7,35.5)]):
            lathe(f'Top cyan window {i+1}.{j+1}',[(.365,.180),(.411,.180),
                  (.416,.185),(.411,.190),(.369,.194),(.365,.191)],3,
                  c+math.radians(da),c+math.radians(db),group='Cyan lights')
        for delta in [-25,25]:
            t=c+math.radians(delta)
            lathe(f'Top light bridge {i+1} {delta}',[(.357,.184),(.423,.178),
                  (.425,.188),(.415,.194),(.359,.198),(.355,.194)],0,
                  t-.014,t+.014,group='Top perimeter')
        # The wide diagonal clamps connect the top rim and side armor.
        t=c+math.pi/4; sa=t-math.radians(7.6); sb=t+math.radians(7.6)
        lathe(f'Diagonal side joint {i+1}',[(.434,.020),(.447,.020),(.450,.026),
              (.450,.139),(.442,.146),(.430,.146)],0,sa,sb,group='Clamps')
        lathe(f'Clamp gasket {i+1}',[(.344,.171),(.435,.162),(.442,.180),
              (.425,.193),(.354,.200),(.344,.194)],4,sa,sb,group='Clamps')
        lathe(f'Bolted top clamp {i+1}',[(.352,.182),(.427,.174),(.430,.182),
              (.418,.194),(.360,.200),(.352,.195)],0,sa+.006,sb-.006,group='Clamps')
        # A raised bevel traces the outer tip, retaining the black inset face.
        lathe(f'Clamp machined outer edge {i+1}',[(.413,.192),(.416,.192),
              (.426,.181),(.424,.181)],5,sa+.012,sb-.012,group='Clamps')
        screw(f'Top clamp screw {i+1}',.389,.197,t)
        for da in [-29,0,29]:
            screw(f'Side screw {i+1} {da}',.4485,.111 if da else .098,
                  c+math.radians(da),True)
    # Single broad brushed metal crown and a physically recessed top disc.
    lathe('Broad brushed steel crown',[(.264,.179),(.344,.179),(.357,.187),
          (.354,.193),(.347,.197),(.272,.200),(.266,.198),(.263,.193)],1,
          group='Top crown')
    ring('Crown inner polished chamfer',.263,.267,.193,.197,5,.001,group='Top crown')
    ring('Recessed circular black gasket',.251,.264,.177,.193,4,.001,group='Top crown')
    ring('Top cyan perimeter halo',.244,.253,.188,.192,3,.001,group='Cyan lights')
    ring('Recessed center face',.00001,.244,.172,.189,2,.001,group='Center')
    ring('Center cyan O emblem',.077,.112,.1888,.1895,6,.00015,group='Cyan lights')
    ring('Underside service plate',.00001,.353,.0002,.003,2,.001,group='Underside')
    for i in range(8): screw(f'Underside bolt {i+1}',.328,.0003,TAU*i/8)

def export_glb(path):
    """Write glTF 2.0 directly; dimensions in meters; Y-up conversion."""
    doc=dict(asset={'version':'2.0','generator':'Reference Puck Builder'},
             scene=0,scenes=[{'nodes':[0]}],nodes=[{'name':'Cyan Puck - diameter 90 cm, height 20 cm','children':[]}],
             meshes=[],materials=[],buffers=[],bufferViews=[],accessors=[],
             extensionsUsed=['KHR_materials_emissive_strength'])
    binary=bytearray()
    for name,col,metal,rough,em in MATERIALS:
        mat={'name':name,'pbrMetallicRoughness':{'baseColorFactor':[*col,1],
             'metallicFactor':metal,'roughnessFactor':rough}}
        if em:
            mat['emissiveFactor']=list(col)
            mat['extensions']={'KHR_materials_emissive_strength':{'emissiveStrength':em}}
        doc['materials'].append(mat)
    def accessor(data,typ,component,target):
        while len(binary)%4: binary.append(0)
        off=len(binary)
        flat=[q for p in data for q in p] if typ=='VEC3' else data
        binary.extend(struct.pack('<'+('f' if component==5126 else 'I')*len(flat),*flat))
        vi=len(doc['bufferViews']); doc['bufferViews'].append({'buffer':0,'byteOffset':off,'byteLength':len(binary)-off,'target':target})
        ac={'bufferView':vi,'componentType':component,'count':len(data),'type':typ}
        if typ=='VEC3':
            ac['min']=[min(p[k] for p in data) for k in range(3)]
            ac['max']=[max(p[k] for p in data) for k in range(3)]
        ai=len(doc['accessors']); doc['accessors'].append(ac); return ai
    for part in PARTS:
        pos=[(x,z,-y) for x,y,z in part['verts']]
        nor=[(x,z,-y) for x,y,z in part['normals']]
        pa=accessor(pos,'VEC3',5126,34962); na=accessor(nor,'VEC3',5126,34962)
        ia=accessor([q for f in part['faces'] for q in f],'SCALAR',5125,34963)
        mi=len(doc['meshes'])
        doc['meshes'].append({'name':part['name'],'primitives':[{'attributes':{'POSITION':pa,'NORMAL':na},'indices':ia,'material':part['material']}]})
        ni=len(doc['nodes']); doc['nodes'].append({'name':part['name'],'mesh':mi})
        doc['nodes'][0]['children'].append(ni)
    doc['buffers']=[{'byteLength':len(binary)}]
    js=json.dumps(doc,separators=(',',':')).encode(); js+=b' '*((-len(js))%4)
    binary+=b'\0'*((-len(binary))%4)
    Path(path).write_bytes(struct.pack('<III',0x46546c67,2,12+8+len(js)+8+len(binary))+
        struct.pack('<II',len(js),0x4e4f534a)+js+struct.pack('<II',len(binary),0x004e4942)+binary)

def setup_glow_compositor(scene):
    """Set up a simple emissive glow compositor in Blender 4.x and 5.x."""
    import bpy

    # Blender 5.x uses this flag instead of scene.use_nodes to control
    # whether compositing is applied to the final render.
    if hasattr(scene.render, "use_compositing"):
        scene.render.use_compositing = True

    def configure_glare(glow):
        """Handle both old property-style and newer socket-style Glare APIs."""
        def set_value(attr_name, socket_names, value, string_fallback=None):
            # Blender 4.x / older API style.
            if hasattr(glow, attr_name):
                try:
                    setattr(glow, attr_name, value)
                    return True
                except (TypeError, ValueError, AttributeError):
                    pass

            # Blender 5.x moved a number of compositor options to inputs.
            for socket_name in socket_names:
                sock = glow.inputs.get(socket_name)
                if sock is None or not hasattr(sock, "default_value"):
                    continue

                for candidate in (value, string_fallback):
                    if candidate is None:
                        continue
                    try:
                        sock.default_value = candidate
                        return True
                    except (TypeError, ValueError):
                        pass
            return False

        set_value("glare_type", ("Glare Type", "Type"), "FOG_GLOW", "Fog Glow")
        set_value("quality", ("Quality",), "HIGH", "High")
        set_value("threshold", ("Threshold",), 1.4)

    if bpy.app.version < (5, 0, 0):
        # Blender 4.x compositor API.
        scene.use_nodes = True
        tree = scene.node_tree
        nodes = tree.nodes
        links = tree.links
        nodes.clear()

        rl = nodes.new("CompositorNodeRLayers")
        rl.scene = scene

        glow = nodes.new("CompositorNodeGlare")
        configure_glare(glow)

        comp = nodes.new("CompositorNodeComposite")
        links.new(rl.outputs["Image"], glow.inputs["Image"])
        links.new(glow.outputs["Image"], comp.inputs["Image"])
        return

    # Blender 5.x: the compositor is a separate CompositorNodeTree data-block.
    tree = bpy.data.node_groups.new(
        name="Cyan Puck | Glow Compositor",
        type="CompositorNodeTree",
    )

    # Blender 5.3+ exposes compositor node groups through the effects stack.
    # This compatibility property remains available throughout Blender 5.x.
    if hasattr(tree, "allow_usage_in_scene_compositor_effect"):
        tree.allow_usage_in_scene_compositor_effect = True

    scene.compositing_node_group = tree

    # Blender 5.x replaced the Composite node with a Group Output.
    tree.interface.new_socket(
        name="Image",
        in_out="OUTPUT",
        socket_type="NodeSocketColor",
    )

    nodes = tree.nodes
    links = tree.links

    rl = nodes.new("CompositorNodeRLayers")
    rl.scene = scene

    glow = nodes.new("CompositorNodeGlare")
    configure_glare(glow)

    output = nodes.new("NodeGroupOutput")

    # Use named sockets when available; fall back to the first image socket.
    glow_input = glow.inputs.get("Image") or glow.inputs[0]
    glow_output = glow.outputs.get("Image") or glow.outputs[0]
    output_input = output.inputs.get("Image") or output.inputs[0]

    links.new(rl.outputs["Image"], glow_input)
    links.new(glow_output, output_input)

def blender_scene(output_dir):
    import bpy
    from mathutils import Vector
    scene=bpy.data.scenes.new('Cyan Puck | reference reconstruction')
    if bpy.context.window: bpy.context.window.scene=scene
    scene.unit_settings.system='METRIC'; scene.unit_settings.length_unit='CENTIMETERS'
    scene.unit_settings.scale_length=1.0
    root=bpy.data.collections.new('PUCK | 90 cm x 20 cm'); scene.collection.children.link(root)
    groups={}
    for group in sorted({p['group'] for p in PARTS}):
        groups[group]=bpy.data.collections.new(group); root.children.link(groups[group])
    anchor=bpy.data.objects.new('PUCK ROOT | floor-centered origin',None); root.objects.link(anchor)
    anchor['diameter_m']=.90; anchor['height_m']=.20
    anchor['reference_note']='Visible features reconstructed from one reference. Hidden underside inferred.'
    mats=[]
    for idx,(name,col,metal,rough,em) in enumerate(MATERIALS):
        mat=bpy.data.materials.new(name); mat.diffuse_color=(*col,1); mat.use_nodes=True
        nodes=mat.node_tree.nodes; links=mat.node_tree.links; p=nodes.get('Principled BSDF')
        p.inputs['Base Color'].default_value=(*col,1)
        p.inputs['Metallic'].default_value=metal; p.inputs['Roughness'].default_value=rough
        if em:
            p.inputs['Emission Color'].default_value=(*col,1)
            p.inputs['Emission Strength'].default_value=em
        elif idx in [0,1,2,5]:
            tex=nodes.new('ShaderNodeTexCoord'); sep=nodes.new('ShaderNodeSeparateXYZ')
            links.new(tex.outputs['Object'],sep.inputs[0])
            if idx in [1,5]:
                # Concentric machining grain, referenced to the puck center.
                vm=nodes.new('ShaderNodeVectorMath'); vm.operation='MULTIPLY'
                vm.inputs[1].default_value=(1,1,0); links.new(tex.outputs['Object'],vm.inputs[0])
                length=nodes.new('ShaderNodeVectorMath'); length.operation='LENGTH'; links.new(vm.outputs[0],length.inputs[0])
                comb=nodes.new('ShaderNodeCombineXYZ'); links.new(length.outputs['Value'],comb.inputs['X'])
                links.new(sep.outputs['Z'],comb.inputs['Y'])
                noise=nodes.new('ShaderNodeTexNoise'); noise.inputs['Scale'].default_value=7500
                noise.inputs['Detail'].default_value=2; links.new(comb.outputs[0],noise.inputs[0])
                if 'Anisotropic IOR Level' in p.inputs: p.inputs['Anisotropic IOR Level'].default_value=.65
                tangent=nodes.new('ShaderNodeTangent'); tangent.direction_type='RADIAL'; tangent.axis='Z'
                links.new(tangent.outputs[0],p.inputs['Tangent'])
            else:
                noise=nodes.new('ShaderNodeTexNoise'); noise.inputs['Scale'].default_value=1100
                noise.inputs['Detail'].default_value=2; links.new(tex.outputs['Object'],noise.inputs[0])
            ramp=nodes.new('ShaderNodeValToRGB')
            ramp.color_ramp.elements[0].color=(*[v*.65 for v in col],1)
            ramp.color_ramp.elements[1].color=(*[min(v*1.22,1) for v in col],1)
            links.new(noise.outputs['Fac'],ramp.inputs[0]); links.new(ramp.outputs[0],p.inputs['Base Color'])
            bump=nodes.new('ShaderNodeBump'); bump.inputs['Strength'].default_value=.14
            bump.inputs['Distance'].default_value=.00008
            links.new(noise.outputs['Fac'],bump.inputs['Height']); links.new(bump.outputs[0],p.inputs['Normal'])
        mats.append(mat)
    for part in PARTS:
        mesh=bpy.data.meshes.new(part['name']); mesh.from_pydata(part['verts'],[],part['faces']); mesh.update()
        for poly in mesh.polygons: poly.use_smooth=True
        mesh.normals_split_custom_set_from_vertices(part['normals'])
        obj=bpy.data.objects.new(part['name'],mesh); groups[part['group']].objects.link(obj)
        obj.data.materials.append(mats[part['material']]); obj.parent=anchor
    studio=bpy.data.collections.new('STUDIO | excluded from model export'); scene.collection.children.link(studio)
    floor_mesh=bpy.data.meshes.new('Studio floor'); floor_mesh.from_pydata([(-200,-200,-.002),(200,-200,-.002),(200,200,-.002),(-200,200,-.002)],[],[(0,1,2,3)])
    floor=bpy.data.objects.new('Studio floor',floor_mesh); studio.objects.link(floor)
    fm=bpy.data.materials.new('Neutral gray backdrop'); fm.diffuse_color=(.22,.255,.29,1); fm.use_nodes=True
    fm.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(.22,.255,.29,1)
    fm.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.7; floor.data.materials.append(fm)
    def aim(obj,point): obj.rotation_euler=(Vector(point)-obj.location).to_track_quat('-Z','Y').to_euler()
    for name,loc,power,size,color in [
        ('Key softbox',(-.7,-.5,1.4),110,1.1,(.84,.93,1)),
        ('Right strip',(.8,.1,.85),90,.7,(.90,.96,1)),
        ('Back rim',(-.1,.8,1.1),140,.8,(1,1,1)),
        ('Front fill',(0,-1,.65),30,1,(.76,.88,1))]:
        light=bpy.data.lights.new(name,'AREA'); light.energy=power; light.shape='DISK'; light.size=size; light.color=color
        ob=bpy.data.objects.new(name,light); studio.objects.link(ob); ob.location=loc; aim(ob,(0,0,.10))
    cam_data=bpy.data.cameras.new('Reference view'); cam=bpy.data.objects.new('Reference view',cam_data); studio.objects.link(cam)
    cam.location=(0,-1.50,1.35); aim(cam,(0,0,.085)); cam_data.type='ORTHO'; cam_data.ortho_scale=1.045; scene.camera=cam
    scene.world=bpy.data.worlds.new('Soft gray studio'); scene.world.use_nodes=True
    scene.world.node_tree.nodes.get('Background').inputs[0].default_value=(.30,.35,.40,1)
    scene.world.node_tree.nodes.get('Background').inputs[1].default_value=.45
    scene.render.engine='CYCLES'; scene.cycles.samples=96; scene.cycles.use_denoising=True
    scene.render.resolution_x=1536; scene.render.resolution_y=1536; scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG'; scene.render.filepath=str(output_dir/'Cyan_Puck_render.png')
    scene.view_settings.view_transform='AgX'
    setup_glow_compositor(scene)
    if bpy.context.screen:
        for area in bpy.context.screen.areas:
            if area.type=='VIEW_3D':
                area.spaces.active.region_3d.view_perspective='CAMERA'
                area.spaces.active.shading.type='MATERIAL'
    save_path = output_dir / 'Standard.blend'
    print(f'Saving Cyan puck to: {save_path}')
    bpy.ops.wm.save_as_mainfile(filepath=str(save_path))
    print('Created Standard.blend. Press F12 to render the reference view.')

def output_directory():
    """Choose a user-writable output folder instead of the drive root."""
    import bpy

    # If the current .blend file has already been saved somewhere, use that folder.
    if bpy.data.filepath:
        folder = Path(bpy.data.filepath).resolve().parent
        if os.access(folder, os.W_OK):
            return folder

    # If the text editor script has a real writable location, use it.
    text_block = getattr(getattr(bpy.context, 'space_data', None), 'text', None)
    if text_block and text_block.filepath:
        folder = Path(bpy.path.abspath(text_block.filepath)).resolve().parent
        if folder.is_dir() and os.access(folder, os.W_OK):
            return folder

    # __file__ can sometimes resolve to C:\ when a script is opened in Blender.
    # Only use it if the directory is actually writable.
    try:
        folder = Path(__file__).resolve().parent
        if folder.is_dir() and os.access(folder, os.W_OK):
            return folder
    except NameError:
        pass

    # Reliable Windows/macOS/Linux fallback.
    home = Path.home()
    preferred = home / "Documents" / "Cyan_Puck_Output"
    try:
        preferred.mkdir(parents=True, exist_ok=True)
        return preferred
    except OSError:
        return home

if __name__=='__main__':
    build_geometry(); output=output_directory()
    try: import bpy
    except ImportError:
        export_glb(output/'Standard.glb')
        print(f"Created {output/'Standard.glb'}: {len(PARTS)} editable mesh parts.")
    else:
        blender_scene(output)
        export_glb(output/'Standard.glb')
        print(f"Created {output/'Standard.glb'} as well.")
