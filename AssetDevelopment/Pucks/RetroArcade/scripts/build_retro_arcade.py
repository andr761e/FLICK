"""Author all nine Retro Arcade cosmetic meshes from the approved solid-panel design.

Uses the supplied FLICK_neon_puck.py design vocabulary; rebuilt proportions,
segmentation and LED layout follow the user's reference. Run in Blender:
  blender -b --python .../build_neon.py -- [--render] [--portraits]
Only this set's authoring/export files are written. No existing skins are edited.
"""
import bpy, math, json, sys, argparse
from pathlib import Path
from mathutils import Vector
ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(); p.add_argument('--render', action='store_true'); p.add_argument('--portraits', action='store_true'); p.add_argument('--archetype', default='Standard', choices=['Standard','Heavy','Striker','Grippy','Slider','Blocker','Compact','Bouncer','Toppler'])
args = p.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
NAME=args.archetype
SCALES={'Standard':(1.,1.),'Heavy':(1.08,1.18),'Striker':(.92,.82),'Grippy':(1.,1.),'Slider':(1.,1.),'Blocker':(1.18,.92),'Compact':(.8,.9),'Bouncer':(.96,.9),'Toppler':(1.,1.35)}
RS,HS=SCALES[NAME]
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system='METRIC'; scene.unit_settings.scale_length=1
MATS={}; SPEC={}
def mat(name, color, metal, rough, emission=(0,0,0), power=0, grain=False):
    m=bpy.data.materials.new(name); m.use_nodes=True; m.diffuse_color=(*color,1)
    n=m.node_tree.nodes; l=m.node_tree.links; s=n.get('Principled BSDF')
    s.inputs['Base Color'].default_value=(*color,1); s.inputs['Metallic'].default_value=metal; s.inputs['Roughness'].default_value=rough
    s.inputs['Emission Color'].default_value=(*emission,1); s.inputs['Emission Strength'].default_value=power
    # Actual radial lens profile, not a constant flat emission on the top face.
    def mathnode(op,a,b=None):
        q=n.new('ShaderNodeMath');q.operation=op
        for i,v in enumerate([a,b] if b is not None else [a]):
            if isinstance(v,(float,int)):q.inputs[i].default_value=v
            else:l.new(v,q.inputs[i])
        return q.outputs[0]
    def mix(a,b,f):return mathnode('ADD',mathnode('MULTIPLY',a,mathnode('SUBTRACT',1,f)),mathnode('MULTIPLY',b,f))
    energy=power
    if name in ['RetroArcade_Cyan','RetroArcade_Magenta','RetroArcade_Yellow']:
        tc=n.new('ShaderNodeTexCoord');sep=n.new('ShaderNodeSeparateXYZ');coords=n.new('ShaderNodeVectorMath');coords.operation='MULTIPLY';l.new(tc.outputs['Object'],coords.inputs[0]);coords.inputs[1].default_value=(1/RS,1/RS,1/HS);l.new(coords.outputs[0],sep.inputs[0])
        xy=n.new('ShaderNodeVectorMath');xy.operation='MULTIPLY';l.new(coords.outputs[0],xy.inputs[0]);xy.inputs[1].default_value=(1,1,0)
        ln=n.new('ShaderNodeVectorMath');ln.operation='LENGTH';l.new(xy.outputs[0],ln.inputs[0]);r=ln.outputs['Value'];z=sep.outputs['Z']
        def distance(v,center,width):return mathnode('DIVIDE',mathnode('ABSOLUTE',mathnode('SUBTRACT',v,center)),width)
        t=mix(.5,distance(r,.10075,.01825),mathnode('LESS_THAN',r,.15))
        pinkmask=mathnode('MULTIPLY',mathnode('GREATER_THAN',r,.15),mathnode('LESS_THAN',r,.28))
        t=mix(t,distance(r,.2635,.0072),pinkmask)
        t=mix(t,distance(r,.4155,.0205),mathnode('GREATER_THAN',r,.38))
        side=mix(distance(z,-.0075,.008),distance(z,-.0635,.0075),mathnode('LESS_THAN',z,-.05))
        t=mix(side,t,mathnode('GREATER_THAN',z,.06))
        glow=mathnode('ADD',.18,mathnode('MULTIPLY',1.60,mathnode('EXPONENT',mathnode('MULTIPLY',-3,mathnode('MULTIPLY',t,t)))))
        energy=mathnode('MULTIPLY',glow,power)
        # Keep direct lens radiance below clipping, while glossy/diffuse rays
        # retain enough energy to reflect the neon onto the floor and casing.
        path=n.new('ShaderNodeLightPath')
        visible_power=3.0 if name=='RetroArcade_Magenta' else 2.5
        energy=mathnode('MULTIPLY',energy,mix(3,visible_power/power,path.outputs['Is Camera Ray']))
    if not isinstance(energy,(float,int)):l.new(energy,s.inputs['Emission Strength'])
    # Per-cell LED brightness survives export; no extra draw calls or animation.
    vc=n.new('ShaderNodeVertexColor');vc.layer_name='ArcadeTint'
    tint=n.new('ShaderNodeMixRGB');tint.blend_type='MULTIPLY';tint.inputs[0].default_value=1;tint.inputs[1].default_value=(*emission,1)
    l.new(vc.outputs['Color'],tint.inputs[2]);l.new(tint.outputs[0],s.inputs['Emission Color'])
    s.inputs['Coat Weight'].default_value=.15 if metal>.5 else .05
    s.inputs['Specular IOR Level'].default_value=.08 if name=='RetroArcade_Face' else .35
    if grain:
        t=n.new('ShaderNodeTexNoise'); t.inputs['Scale'].default_value=650
        b=n.new('ShaderNodeBump'); b.inputs['Strength'].default_value=.1; b.inputs['Distance'].default_value=.000025
        l.new(t.outputs['Fac'],b.inputs['Height']); l.new(b.outputs['Normal'],s.inputs['Normal'])
    MATS[name]=m; SPEC[name]=dict(base=color,metallic=metal,roughness=rough,emissive=emission,power=power,grain=grain)
    return m
hull=mat('RetroArcade_Housing',(.007,.011,.019),.85,.23,grain=True)
black=mat('RetroArcade_Gasket',(.003,.005,.009),.28,.42)
face=mat('RetroArcade_Face',(.009,.012,.022),.55,.34,grain=True)
metal=mat('RetroArcade_ControlDeck',(.10,.125,.16),.92,.26,grain=True)
edge=mat('RetroArcade_PolishedEdges',(.30,.36,.44),.95,.19)
cyan=mat('RetroArcade_Cyan',(.001,.12,.16),.08,.22,(.001,.70,1),8.5)
pink=mat('RetroArcade_Magenta',(.18,.002,.13),.06,.23,(1,.001,.85),9.0)
yellow=mat('RetroArcade_Yellow',(.36,.29,.001),.08,.25,(1,.85,.001),7.5)
cyanlo=mat('RetroArcade_CyanPixels',(.002,.09,.12),.05,.27,(.002,.64,.79),3.3)
pinklo=mat('RetroArcade_MagentaPixels',(.09,.001,.07),.05,.27,(.65,.002,.49),3.3)
# Coordinates in centimetres, geometry in metres. Front points down Blender -Y.
F=-90
objects=[]
def xyz(r,a,z):
    a=math.radians(a);return (r*.01*math.cos(a),r*.01*math.sin(a),z*.01)
def mesh(name,vs,fs,material,smooth=False):
    m=bpy.data.meshes.new(name);m.from_pydata(vs,[],fs);m.update();o=bpy.data.objects.new(name,m);scene.collection.objects.link(o)
    m.materials.append(material)
    for poly in m.polygons:poly.use_smooth=smooth
    objects.append(o);return o
def bevel(o,w=.08):
    if not w:return o
    mod=o.modifiers.new('Machined rounded edges','BEVEL');mod.width=w*.01;mod.segments=2;mod.limit_method='ANGLE';mod.angle_limit=.5
    mod=o.modifiers.new('Weighted planar reflections','WEIGHTED_NORMAL');mod.keep_sharp=True
    return o
def arc(name,ri,ro,a0,a1,z0,z1,material,b=.07):
    full=a1-a0>=359.99;n=max(2,round((a1-a0)*.65));N=n if full else n+1
    vs=[xyz(r,a0+(a1-a0)*j/n,z) for z in (z0,z1) for r in (ri,ro) for j in range(N)]
    fs=[]
    for j in range(n):
        k=(j+1)%N
        fs.extend([(j,k,N+k,N+j),(2*N+j,3*N+j,3*N+k,2*N+k),(j,2*N+j,2*N+k,k),(N+j,N+k,3*N+k,3*N+j)])
    if not full:fs.extend([(0,N,3*N,2*N),(n,2*N+n,3*N+n,N+n)])
    o=mesh(name,vs,fs,material)
    # Curved radial walls are smooth, broad top annuli remain planar.
    for i,poly in enumerate(o.data.polygons):poly.use_smooth=i<n*4 and i%4 in (2,3)
    return bevel(o,b)
def ring(name,ri,ro,z0,z1,material,b=.06):return arc(name,ri,ro,0,360,z0,z1,material,b)

def profile_arc(name,profile,a0,a1,material,b=.04):
    """Revolve a closed radial section; the crown has real relief, not a normal decal."""
    full=a1-a0>=359.99;n=max(2,round((a1-a0)*.65));N=n if full else n+1
    vs=[xyz(r,a0+(a1-a0)*j/n,z) for r,z in profile for j in range(N)]
    fs=[]
    for k in range(len(profile)):
        q=(k+1)%len(profile)
        for j in range(n):fs.append((k*N+j,k*N+(j+1)%N,q*N+(j+1)%N,q*N+j))
    if not full:
        fs.extend([tuple(k*N for k in reversed(range(len(profile)))),tuple(k*N+n for k in range(len(profile)))])
    o=mesh(name,vs,fs,material,True)
    return bevel(o,b)

def deck_z(r):return 18.78+(r-29.6)*.105

def deck_insert(name,ri,ro,a0,a1,m,offset=.02):
    return profile_arc(name,[(ri,deck_z(ri)-.08),(ro,deck_z(ro)-.08),
        (ro,deck_z(ro)+offset),(ri,deck_z(ri)+offset)],a0,a1,m,.025)
def disc(name,r,z0,z1,material,b=.1):
    bpy.ops.mesh.primitive_cylinder_add(vertices=12 if r<1 else 192,radius=r*.01,depth=(z1-z0)*.01,location=(0,0,(z0+z1)*.005))
    o=bpy.context.object;o.name=name;o.data.materials.append(material)
    for poly in o.data.polygons:poly.use_smooth=len(poly.vertices)==4
    objects.append(o);return bevel(o,b)
def torus(name,r,w,z,material):
    bpy.ops.mesh.primitive_torus_add(major_segments=20 if r<1 else 192,minor_segments=12 if w>.4 else 8,location=(0,0,z*.01),major_radius=r*.01,minor_radius=w*.01)
    o=bpy.context.object;o.name=name;o.data.materials.append(material)
    for poly in o.data.polygons:poly.use_smooth=True
    objects.append(o);return o
def screw(r,a,z):
    # Low-poly flush fastener with an actual recessed slot.
    o=disc('Flush titanium screw',.19,z,z+.07,edge,.02);o.location.x,o.location.y=xyz(r,a,0)[:2]
    x,y,_=xyz(r,a,z+.08);s=.0013
    mesh('Screw slot',[(x-s,y-.0003,(z+.081)*.01),(x+s,y-.0003,(z+.081)*.01),(x+s,y+.0003,(z+.081)*.01),(x-s,y+.0003,(z+.081)*.01)],[(0,1,2,3)],black)
# Layered chassis; no single bright silver crown from the original finish.
disc('Contact base',42.8,0,1.4,black,.25)
disc('Load-bearing visual core',44,1.2,16.5,hull,.45)
ring('Bottom polished bevel',42.9,44.7,1.0,2.25,edge,.13)
ring('Lower black reveal',43.8,44.95,2.2,2.85,black,.08)
ring('Upper shoulder',42.8,44.9,15.8,18.3,hull,.25)
ring('Outer bright machined lip',44.05,45,16.0,18.0,edge,.14)
ring('Crown dark channel',38.8,44.4,17.75,18.6,black,.14)
profile_arc('Sculpted titanium matrix crown',[(27.3,17.8),(38.9,18.3),(38.9,19.35),
    (38.3,19.72),(29.6,18.78),(28.2,18.72),(27.3,18.05)],0,360,metal,.10)
disc('Deep recessed black circular face',26.5,16.4,17.25,face,.16)
profile_arc('Recessed halo socket wall',[(25.6,17.1),(27.6,17.1),(27.6,18.25),
    (27.1,18.50),(25.6,18.0)],0,360,black,.08)
torus('Continuous neon magenta halo',26.35,.72,18.30,pink)
torus('Inner halo tracing edge',25.75,.12,18.12,pinklo)
def glyph_polygon(name,points):
    # Solid cyan inlays are geometry, so no decal/texture dependency or z fighting.
    if sum(points[i][0]*points[(i+1)%len(points)][1]-points[(i+1)%len(points)][0]*points[i][1] for i in range(len(points)))<0:points=list(reversed(points))
    n=len(points);vs=[(x*.01,y*.01,z*.01) for z in [17.38,17.65] for x,y in points]
    fs=[tuple(reversed(range(n))),tuple(range(n,2*n))]
    fs.extend((i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n))
    return bevel(mesh(name,vs,fs,cyanlo),.07)

def glyph_stroke(name,points,width):
    left=[];right=[]
    for i,(x,y) in enumerate(points):
        before=points[max(0,i-1)];after=points[min(len(points)-1,i+1)]
        dx,dy=after[0]-before[0],after[1]-before[1];length=math.hypot(dx,dy)
        nx,ny=-dy/length*width/2,dx/length*width/2
        left.append((x+nx,y+ny));right.append((x-nx,y-ny))
    return glyph_polygon(name,left+list(reversed(right)))

def glyph_dot(x,y,r):
    o=disc('Cyan emblem dot',r,17.38,17.65,cyanlo,.08);o.location.x=x*.01;o.location.y=y*.01

if NAME=='Standard':
    ring('Center target socket',8.0,12.1,17.25,17.4,black,.07)
    ring('Central cyan target',8.25,11.9,17.4,17.68,cyan,.13)
elif NAME=='Compact':
    ring('Compact precision ring',3.8,6.5,17.38,17.65,cyanlo,.10)
elif NAME=='Heavy':
    glyph_polygon('Barbell shaft',[(-8,-1.3),(8,-1.3),(8,1.3),(-8,1.3)])
    for side in [-1,1]:
        x=side*9.4
        glyph_polygon('Heavy weight block',[(x-1.9,-6),(x+1.9,-6),(x+1.9,6),(x-1.9,6)])
        x=side*13.4
        glyph_polygon('Barbell outer collar',[(x-1,-3.7),(x+1,-3.7),(x+1,3.7),(x-1,3.7)])
elif NAME=='Striker':
    ring('Striker crosshair ring',6.5,8.,17.38,17.65,cyanlo,.07)
    glyph_dot(0,0,1.7)
    for a in [0,90,180,270]:
        glyph_stroke('Crosshair arm',[(r*math.cos(math.radians(a)),r*math.sin(math.radians(a))) for r in [7.8,13.5]],1.7)
elif NAME=='Slider':
    for x in [-10,0,10]:
        glyph_polygon('Slider triple chevron',[(x-5,-7),(x+3,0),(x-5,7),(x-1,7),(x+7,0),(x-1,-7)])
elif NAME=='Toppler':
    for y in [5,-3]:
        glyph_polygon('Toppler stacked chevron',[(-10,y-2),(0,y+6),(10,y-2),(5,y-2),(0,y+2),(-5,y-2)])
    glyph_polygon('Toppler base dash',[(-3,-12),(3,-12),(3,-10),(-3,-10)])
elif NAME=='Blocker':
    for r in [8.1,12.8]:
        # Closed annular mesh, not a capped stroke: no seam at the last corner.
        n=6
        vs=[xyz(radius,30+60*i,z) for z in [17.38,17.65] for radius in [r-.825,r+.825] for i in range(n)]
        fs=[]
        for j in range(n):
            k=(j+1)%n
            fs.extend([(j,k,n+k,n+j),(2*n+j,3*n+j,3*n+k,2*n+k),
                (j,2*n+j,2*n+k,k),(n+j,n+k,3*n+k,3*n+j)])
        bevel(mesh('Blocker closed double hexagon',vs,fs,cyanlo),.07)
elif NAME=='Grippy':
    glyph_polygon('Grippy angular grip emblem',[(-15,-6),(-7,5),(-4,2),(0,10),(8,1),(10,4),(16,-6),(4,1),(1,-2),(-2,3),(-5,-1)])
elif NAME=='Bouncer':
    glyph_dot(0,7,2.7)
    for side in [-1,1]:
        pts=[]
        for i in range(16):
            t=i/15;pts.append((side*(2*(1-t)**2+7*2*t*(1-t)+15*t*t),-8*(1-t)**2+1*2*t*(1-t)+1*t*t))
        glyph_stroke('Bouncer curved rebound wing',pts,2.4)
# Large outer diffusers in the reference: four broad cyan arcs, yellow end caps,
# with only four substantial clamp blocks rather than a uniform picket fence.
tiles=[(-180,-160,cyan),(-159,-145,yellow),(-133,-65,cyan),(-60,-38,yellow),(-34,16,cyan),(17,27,yellow),(35,59,yellow),(64,130,cyan),(141,155,yellow),(157,180,cyan)]
for i,(a,b,m) in enumerate(tiles):
    arc('Recessed outer lens bezel',39.1,44.08,F+a-.35,F+b+.35,18.2,19.1,hull,.15)
    profile_arc('Convex glass cyan / yellow diffuser',[(39.5,18.55),(43.6,18.55),
        (43.6,19.10),(43.2,19.58),(42.1,19.91),(40.9,19.91),(39.8,19.58),(39.5,19.1)],F+a,F+b,m,.10)
# Raised clamp plates with slot lights at rear corners, and cyan fasteners in front.
for i,a in enumerate([-140,-34,34,140]):
    arc('Radial armored clamp',37.8,44.2,F+a-5,F+a+5,19.0,20.10,hull,.12)
    if abs(a)>100:arc('Clamp inset magenta slot',39.25,39.75,F+a-3.1,F+a+3.1,20.10,20.15,pink,.035)
    else:
        x,y,_=xyz(39.3,F+a,0)
        o=torus('Cyan countersunk bolt',.36,.095,20.12,cyanlo);o.location.x=x;o.location.y=y
    for da in [-3.6,3.6]:screw(42.5,F+a+da,20.10)
# Tiny precision grooves encircle the LED display.
for r,z,m in [(27.8,18.46,edge),(29.5,18.79,black),(37.9,19.65,black),(38.55,19.56,edge)]:
    torus('Machined concentric line',r,.065,z,m)
# Approved solid-panel crown: broad shapes stay legible at gameplay distance.
batches={}
def quad(m,pts):
    vs,fs=batches.setdefault(m,([],[]));j=len(vs);vs.extend(pts);fs.append((j,j+1,j+2,j+3))
for bay in range(12):
    mid=F+bay*30
    m=pink if bay in [1,5,7,11] else cyan
    deck_insert('Dark display recess',29.95,37.65,mid-14.1,mid+14.1,black)
    deck_insert('Single broad glass crown panel',30.3,37.1,mid-12.9,mid+12.9,m,.20)
    if bay in [2,4,8,10]:deck_insert('Yellow crown accent',30.2,36.9,mid-4.1,mid+4.1,yellow,.35)
# Side windows: the reference has two distinct scales of pixels. Cyan blocks
# contain tiny cells; the outer fuchsia fields use large alternating squares.
# Authored bitmaps deliberately avoid the old equalizer/bar-chart silhouette.
cyan_mosaic=[
    '001100000011',
    '001100000011',
    '110011001100',
    '110011001100',
    '001100110011',
    '001100110011',
    '110011000000',
    '110011000000',
]
for bay in range(16):
    mid=F+bay*22.5
    arc('Segmented black side fascia',44.02,44.63,mid-10.9,mid+10.9,4.0,15.85,hull,.12)
    if bay%4!=0:arc('Fine upper side highlight',44.62,44.76,mid-10.7,mid+10.7,15.65,15.83,edge,.02)
    if bay in [0,8]:continue
    if bay in [3,4,5,11,12,13]:
        for col in range(5):
            a=mid-10.0+col*4.0
            for row in range(4):
                if (col+row+bay)%2:continue
                z=4.8+row*2.65
                quad(pinklo,[xyz(44.78,a,z),xyz(44.78,a+3.15,z),
                    xyz(44.78,a+3.15,z+2.15),xyz(44.78,a,z+2.15)])
    else:
        for col in range(24):
            a=mid-10.1+col*.85
            for row in range(16):
                if cyan_mosaic[7-row//2][(col//2+bay*2)%12]!='1':continue
                z=4.7+row*.635
                quad(cyanlo,[xyz(44.78,a,z),xyz(44.78,a+.64,z),
                    xyz(44.78,a+.64,z+.48),xyz(44.78,a,z+.48)])
for m,(vs,fs) in batches.items():
    o=mesh('LED matrix '+m.name,vs,fs,m)
    colors=o.data.color_attributes.new(name='ArcadeTint',type='FLOAT_COLOR',domain='CORNER')
    for i,poly in enumerate(o.data.polygons):
        # Deterministic soft variation in little clusters; not frame-based noise.
        value=.78+.22*((i//8*7)%9)/8
        for loop in poly.loop_indices:colors.data[loop].color=(value,value,value,1)

# Continuous cyan base interrupted by a yellow center and magenta detail.
for a,b,m in [(-180,-22,cyan),(-21,-12,pink),(-11,11,yellow),(12,21,pink),(22,180,cyan)]:
    arc('Lower glass neon waistline',44.65,44.96,F+a,F+b,2.9,4.4,m,.12)
# Front shield is a true trapezoid on the curved fascia, with stepped side caps.
def shield(a,zbottom,ztop,width_top,width_bottom,ri,ro,m):
    # Subdivide the trapezoid along the cylinder. A flat chord would bury the
    # centre of a wide plate inside the housing and leave only its corners.
    n=20;N=n+1
    vs=[xyz(r,F+a+(-width+2*width*j/n),z) for r in [ri,ro] for width,z in [(width_bottom,zbottom),(width_top,ztop)] for j in range(N)]
    fs=[]
    for j in range(n):
        k=j+1
        fs.extend([(j,N+j,N+k,k),(2*N+j,2*N+k,3*N+k,3*N+j),(j,k,2*N+k,2*N+j),(N+j,3*N+j,3*N+k,N+k)])
    fs.extend([(0,2*N,3*N,N),(n,N+n,3*N+n,2*N+n)])
    o=mesh('Curved trapezoid front armor',vs,fs,m)
    for poly in o.data.polygons:poly.use_smooth=True
    return bevel(o,.09)

for a in [0,180]:
    shield(a,4.2,14.7,26,19,44.5,45.0,black)
    shield(a,4.8,14.0,24.7,18.1,44.8,45.04,hull)
    arc('Front magenta display bezel',44.92,45.08,F+a-9.4,F+a+9.4,8.1,10.4,black,.09)
    arc('Front magenta luminous bar',45.06,45.13,F+a-8.5,F+a+8.5,8.45,10.05,pink,.12)
    arc('Inset dark service dot',45.13,45.14,F+a-.7,F+a+.7,8.9,9.3,black,.04)
    for side in [-1,1]:
        shield(a+side*21.5,4.5,7.0,2.4,3,44.66,44.96,metal)
        arc('Pink corner status',44.98,45.06,F+a+side*21.5-1,F+a+side*21.5+1,5.1,5.7,pink,.04)
# Flush service-plate fasteners: cylinders follow the radial side normal.
for da,z in [(-19,13.4),(19,13.4),(-15,5.25),(15,5.25)]:
    for rear in [0,180]:
        angle=F+da+rear
        o=disc('Side plate recessed fastener',.20,-.04,.04,edge,.02)
        o.location=xyz(45.09,angle,z)
        o.rotation_euler=Vector((math.cos(math.radians(angle)),math.sin(math.radians(angle)),0)).to_track_quat('Z','Y').to_euler()
# Type-specific construction from the set reference. All details stay inside
# the same class-scaled visual envelope; none are collision or simulation pieces.
if NAME in ['Heavy','Toppler','Grippy']:
    for a in [-145,-35,35,145]:
        if NAME=='Heavy':
            shield(a,2.7,18.4,5.8,5.8,44.45,45.08,metal)
            shield(a,3.2,17.8,4.6,4.6,44.95,45.12,hull)
        elif NAME=='Toppler':
            shield(a,2.7,18.5,4.8,4.8,44.50,45.12,hull)
            shield(a,3.4,17.8,3.5,3.5,45.10,45.13,metal)
        else:
            shield(a,3.4,17.5,5.0,5.0,44.6,45.12,black)
            for z in [5,7.5,10,12.5,15]:
                arc('Grippy ribbed rubber grip',44.9,45.13,F+a-4,F+a+4,z,z+.45,hull,.05)
        for z in [4.5,16.5]:
            arc('Brace magenta status notch',45.12,45.14,F+a-1.8,F+a+1.8,z,z+.38,pinklo,.035)
elif NAME=='Blocker':
    ring('Blocker reinforced low guard',44.6,45.,4.6,5.15,edge,.08)
    for a in [-120,-60,60,120]:
        arc('Blocker defensive crown guard',38.7,44.3,F+a-3,F+a+3,19.2,20.0,hull,.12)
        arc('Blocker fuchsia guard marker',39.2,43.8,F+a-1.4,F+a+1.4,20.01,20.10,pink,.05)
elif NAME=='Bouncer':
    torus('Bouncer rounded rebound bumper',44.5,.45,4.9,hull)
    torus('Bouncer cyan bumper seam',44.6,.16,5.6,cyanlo)
elif NAME=='Slider':
    for z in [5.0,5.7]:ring('Slider polished glide seam',44.65,44.96,z,z+.18,edge,.025)
elif NAME=='Striker':
    for a in [-80,80]:
        arc('Striker low-profile crown clamp',38.4,44.0,F+a-2.8,F+a+2.8,19.0,20.05,hull,.10)

# Consolidate bevelled mesh into one renderer, weld duplicate material slots.
bpy.ops.object.select_all(action='DESELECT')
for o in objects:
    o.select_set(True)
    if not o.data.color_attributes.get('ArcadeTint'):
        colors=o.data.color_attributes.new(name='ArcadeTint',type='FLOAT_COLOR',domain='CORNER')
        for value in colors.data:value.color=(1,1,1,1)
bpy.context.view_layer.objects.active=objects[0]
bpy.ops.object.convert(target='MESH');bpy.ops.object.join();puck=bpy.context.object;puck.name=f'SM_Puck_{NAME}_RetroArcade'
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
# Keep the original gameplay center origin; the authored art is 20 cm high.
for v in puck.data.vertices:v.co.z-=.10
# Normalize the visual envelope to existing Standard; collision is never exported.
zs=[v.co.z for v in puck.data.vertices];zmin,zmax=min(zs),max(zs)
for v in puck.data.vertices:v.co.z=(v.co.z-(zmin+zmax)/2)*(.20/(zmax-zmin))
for v in puck.data.vertices:v.co.x*=RS;v.co.y*=RS;v.co.z*=HS
puck.data.update()
# UV0 carries grain coordinates. UV1/2 carry canonical art-space position so
# all differently sized types share the same ten materials and lens response.
# Primitives contribute their own UVMap; discard it to keep channel indices exact.
while puck.data.uv_layers:puck.data.uv_layers.remove(puck.data.uv_layers[0])
uv=puck.data.uv_layers.new(name='SurfaceUV')
position_xy=puck.data.uv_layers.new(name='ArcadePositionXY')
position_z=puck.data.uv_layers.new(name='ArcadePositionZ')
for poly in puck.data.polygons:
    axes=[0,1] if abs(poly.normal.z)>.5 else ([0,2] if abs(poly.normal.y)>.5 else [1,2])
    for loop in poly.loop_indices:
        co=puck.data.vertices[puck.data.loops[loop].vertex_index].co
        canonical=(co.x/RS,co.y/RS,co.z/HS)
        uv.data[loop].uv=tuple(canonical[a]*2 for a in axes)
        position_xy.data[loop].uv=(canonical[0]*100,canonical[1]*100)
        position_z.data[loop].uv=(canonical[2]*100,1 if NAME=='Standard' else 0)
for d in ['exports','renders','manifests']: (ROOT/d).mkdir(exist_ok=True)
sys.path.insert(0,str(Path(__file__).resolve().parent))
from mesh_cleanup import clean_export_mesh
clean_export_mesh(puck)
bpy.ops.export_scene.fbx(filepath=str(ROOT/f'exports/{NAME}.fbx'),use_selection=True,object_types={'MESH'},add_leaf_bones=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE',use_tspace=False)
manifest=dict(archetype=NAME,canonical_scale=[1/RS,1/RS,1/HS],asset=puck.name,dimensions_cm=[round(v*100,3) for v in puck.dimensions],triangles=sum(len(p.vertices)-2 for p in puck.data.polygons),materials=SPEC)
(ROOT/f'manifests/{NAME.lower()}.json').write_text(json.dumps(manifest,indent=2))
# Product studio also supports source portraits, with the exact game mesh.
scene.render.engine='CYCLES';scene.cycles.samples=80;scene.cycles.use_denoising=True
try:
    prefs=bpy.context.preferences.addons['cycles'].preferences;prefs.compute_device_type='OPTIX';prefs.get_devices()
    for device in prefs.devices:device.use=device.type=='OPTIX'
    if any(d.use for d in prefs.devices):scene.cycles.device='GPU'
except Exception:pass
scene.world.color=(.20,.27,.35);scene.world.use_nodes=True
scene.world.node_tree.nodes['Background'].inputs['Color'].default_value=(.27,.34,.44,1)
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.45
floor_mat=mat('StudioFloor',(.13,.18,.25),.52,.55,grain=True)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.10*HS-.005));floor=bpy.context.object;floor.name='Studio floor, not exported';floor.data.materials.append(floor_mat)
def area(name,pos,power,color,size,size_y):
    d=bpy.data.lights.new(name,'AREA');d.energy=power;d.color=color;d.shape='RECTANGLE';d.size=size;d.size_y=size_y
    o=bpy.data.objects.new(name,d);scene.collection.objects.link(o);o.location=pos;o.rotation_euler=(Vector((0,0,0))-o.location).to_track_quat('-Z','Y').to_euler()
area('Left broad softbox',(-1.5,-1.3,2.4),210,(.82,.92,1),1.7,1.2)
area('Overhead strip',(1.2,.7,2.1),250,(.88,.94,1),1.6,.6)
area('Front metal reflection card',(.1,-2,.9),60,(.9,.95,1),1.6,.25)
# Keep broad product softboxes off the reflective floor. Its separate, quiet
# fill reveals the blue-grey stage without a giant white softbox reflection.
receivers=bpy.data.collections.new('Puck studio receivers');scene.collection.children.link(receivers);receivers.objects.link(puck)
for fixture in scene.objects:
    if fixture.type=='LIGHT':fixture.light_linking.receiver_collection=receivers
floor_receivers=bpy.data.collections.new('Floor studio receivers');scene.collection.children.link(floor_receivers);floor_receivers.objects.link(floor)
area('Quiet floor fill',(0,0,4),350,(1,1,1),6,6)
fixture=bpy.data.objects['Quiet floor fill'];fixture.light_linking.receiver_collection=floor_receivers;fixture.data.specular_factor=.25

d=bpy.data.cameras.new('Product lens');camera=bpy.data.objects.new('Product camera',d);scene.collection.objects.link(camera);scene.camera=camera
camera.location=(0,-1.65,1.02);camera.rotation_euler=(Vector((0,0,-.04))-camera.location).to_track_quat('-Z','Y').to_euler();d.type='ORTHO';d.ortho_scale=1.28;d.clip_start=.01
scene.view_settings.view_transform='Standard';scene.view_settings.look='None';scene.view_settings.exposure=-1.5
scene.render.image_settings.file_format='PNG';scene.render.resolution_x=1200;scene.render.resolution_y=1200;scene.render.resolution_percentage=100
# restrained compositor halo; emission remains physically reflected on the floor.
group=bpy.data.node_groups.new('NeonStudioGlow','CompositorNodeTree')
scene.compositing_node_group=group
n=group.nodes
rl=n.new('CompositorNodeRLayers');g=n.new('CompositorNodeGlare');g.inputs['Type'].default_value='Fog Glow';g.inputs['Quality'].default_value='High'
if 'Threshold' in g.inputs:g.inputs['Threshold'].default_value=.12
if 'Strength' in g.inputs:g.inputs['Strength'].default_value=1.15
group.interface.new_socket(name='Image',in_out='OUTPUT',socket_type='NodeSocketColor')
c=n.new('NodeGroupOutput');group.links.new(rl.outputs['Image'],g.inputs['Image']);group.links.new(g.outputs['Image'],c.inputs['Image'])

bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/f'RetroArcade_{NAME}.blend'))
if args.render:scene.render.filepath=str(ROOT/f'renders/{NAME.lower()}-studio.png');bpy.ops.render.render(write_still=True)
if args.portraits:
    sys.path.insert(0,str(ROOT.parents[2]/'Tools'))
    from FlickRetroArcadePortraits import render_portraits
    render_portraits(scene,puck,ROOT.parents[2],NAME)
print('RETRO_ARCADE_EXPORT',NAME,manifest['triangles'],manifest['dimensions_cm'])
