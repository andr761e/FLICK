"""Slider puck — 89.9 cm diameter, 18.3 cm visible height.
Based on the approved first Standard puck script: original materials retained.
Open in Blender's Text Editor and Run Script. Saves Slider.blend with the supplied script's writable output folder selection.
Standalone; no other scripts, textures, or libraries required inside Blender.
Regular Python writes Slider.glb. Geometry is modeled in meters with Z up.
"""
ASSET = 'Slider'
DIAMETER_CM = 89.9
VISIBLE_HEIGHT_CM = 18.3
PHYSICS_HEIGHT_CM = 20
CREATE_COLLISION_PROXY = True  # Separate hidden collection; no rigid-body simulation.

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

def build_standard_geometry():
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

# Family-specific geometry. All construction coordinates use the Standard's
# 0.90 x 0.20 meter envelope; apply_dimensions sets each final size exactly.

def triangulate(poly):
    poly=list(poly)
    area=sum(a[0]*b[1]-b[0]*a[1] for a,b in zip(poly,poly[1:]+poly[:1]))
    if area<0: poly.reverse()
    def cross(a,b,c): return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    ids=list(range(len(poly))); triangles=[]
    while len(ids)>3:
        for j in range(len(ids)):
            a,b,c=ids[j-1],ids[j],ids[(j+1)%len(ids)]
            if cross(poly[a],poly[b],poly[c])<=1e-13: continue
            inside=False
            for k in ids:
                if k in (a,b,c): continue
                if min(cross(poly[a],poly[b],poly[k]),cross(poly[b],poly[c],poly[k]),cross(poly[c],poly[a],poly[k]))>=-1e-13:
                    inside=True; break
            if inside: continue
            triangles.append((a,b,c)); ids.pop(j); break
        else: raise ValueError('Cannot triangulate polygon')
    triangles.append(tuple(ids)); return poly,triangles

def clip_u(poly,bound,keep_greater):
    result=[]
    for a,b in zip(poly,poly[1:]+poly[:1]):
        ia=a[0]>=bound if keep_greater else a[0]<=bound
        ib=b[0]>=bound if keep_greater else b[0]<=bound
        if ia: result.append(a)
        if ia != ib:
            t=(bound-a[0])/(b[0]-a[0]); result.append((bound,a[1]+t*(b[1]-a[1])))
    return result

def extrude_polygon(name,poly,bottom,top,material=6,group='Emblem',side_angle=None):
    poly,triangles=triangulate(poly)
    verts=[]; normals=[]; faces=[]
    def position(p,depth):
        if side_angle is None: return (p[0],p[1],depth)
        a=p[0]+side_angle; return (depth*math.cos(a),depth*math.sin(a),p[1])
    def surface_normal(p,sign):
        if side_angle is None:return (0,0,sign)
        a=p[0]+side_angle;return (sign*math.cos(a),sign*math.sin(a),0)
    for tri in triangles:
        pts=[poly[i] for i in tri]
        patches=[pts]
        if side_angle is not None:
            lo=min(p[0] for p in pts); hi=max(p[0] for p in pts)
            count=max(1,math.ceil((hi-lo)/math.radians(1)))
            patches=[]
            for j in range(count):
                q=clip_u(pts,lo+(hi-lo)*j/count,True)
                q=clip_u(q,lo+(hi-lo)*(j+1)/count,False)
                if len(q)>=3: patches.append(q)
        for patch in patches:
            for depth,sign in [(bottom,-1),(top,1)]:
                start=len(verts)
                verts.extend(position(p,depth) for p in patch)
                normals.extend(surface_normal(p,sign) for p in patch)
                for k in range(1,len(patch)-1):
                    f=(start,start+k,start+k+1)
                    faces.append(f if sign>0 else f[::-1])
    for a,b in zip(poly,poly[1:]+poly[:1]):
        count=1 if side_angle is None else max(1,math.ceil(abs(b[0]-a[0])/math.radians(1)))
        for j in range(count):
            p=tuple(a[k]+(b[k]-a[k])*j/count for k in range(2))
            q=tuple(a[k]+(b[k]-a[k])*(j+1)/count for k in range(2))
            du,dv=q[0]-p[0],q[1]-p[1]
            if abs(du)+abs(dv)<1e-12:continue
            if side_angle is None:normal=(dv,-du,0)
            else:
                theta=(p[0]+q[0])/2+side_angle
                normal=(-math.sin(theta)*dv,math.cos(theta)*dv,-du*(bottom+top)/2)
            length=math.sqrt(sum(t*t for t in normal)); normal=tuple(t/length for t in normal)
            start=len(verts)
            verts.extend([position(p,bottom),position(q,bottom),position(q,top),position(p,top)])
            normals.extend([normal]*4);faces.extend([(start,start+1,start+2),(start,start+2,start+3)])
    add(name,verts,normals,faces,material,group)

def rect_points(x0,y0,x1,y1,b=.002):
    b=min(b,(x1-x0)/3,(y1-y0)/3)
    return [(x0+b,y0),(x1-b,y0),(x1,y0+b),(x1,y1-b),(x1-b,y1),(x0+b,y1),(x0,y1-b),(x0,y0+b)]

def icon_poly(name,poly,z=.1895):
    extrude_polygon(name,poly,.1888,z,6,'Emblem')

def icon_rect(name,x0,y0,x1,y1,b=.001):
    icon_poly(name,rect_points(x0,y0,x1,y1,b))

def stroke(name,points,width):
    left=[];right=[]
    for i,p in enumerate(points):
        a=points[max(i-1,0)];b=points[min(i+1,len(points)-1)]
        dx,dy=b[0]-a[0],b[1]-a[1];length=math.hypot(dx,dy)
        nx,ny=-dy/length*width/2,dx/length*width/2
        left.append((p[0]+nx,p[1]+ny));right.append((p[0]-nx,p[1]-ny))
    icon_poly(name,left+right[::-1])

def emblem():
    if ASSET=='Toppler':
        icon_poly('Toppler upward chevron',[(-.120,.015),(-.065,.015),(0,.081),(.065,.015),(.120,.015),(0,.145)])
        icon_poly('Toppler lower arrowhead',[(-.117,-.083),(.117,-.083),(0,.025)])
        icon_rect('Toppler lower dash',-.034,-.148,.034,-.118)
    elif ASSET=='Bouncer':
        ring('Bouncer ball',.00001,.047,.1888,.1895,6,.0001,origin=(0,.057,0),group='Emblem',steps=96)
        for sign in [-1,1]:
            pts=[]
            for i in range(33):
                t=i/32
                x=(1-t)**2*.170+2*(1-t)*t*.057+t*t*.025
                y=(1-t)**2*.015+2*(1-t)*t*.005+t*t*(-.119)
                pts.append((sign*x,y))
            stroke('Bouncer rebound arc '+str(sign),pts,.022)
    elif ASSET=='Compact':
        ring('Compact thick O',.047,.090,.1888,.1895,6,.00015,group='Emblem')
    elif ASSET=='Blocker':
        # Point-up nested hexagons, with an actual dark gap.
        poly=[(.125*math.cos(math.pi/2+i*TAU/6),.125*math.sin(math.pi/2+i*TAU/6)) for i in range(6)]
        inner=[(x*.78,y*.78) for x,y in poly]
        for i in range(6):
            j=(i+1)%6
            icon_poly('Blocker shield border '+str(i),[poly[i],poly[j],inner[j],inner[i]])
        icon_poly('Blocker shield core',[(x*.60,y*.60) for x,y in poly])
    elif ASSET=='Slider':
        for i in range(3):
            cx=(i-1)*.083
            icon_poly('Slider chevron '+str(i+1),[(cx-.059,-.079),(cx-.014,-.079),(cx+.062,0),(cx-.014,.079),(cx-.059,.079),(cx+.015,0)])
    elif ASSET=='Grippy':
        icon_poly('Grippy mountain grip emblem',[(-.150,-.090),(-.064,.035),(-.055,.017),(0,.126),(.055,.017),(.064,.035),(.150,-.090),(.083,-.052),(.084,-.071),(0,.045),(-.084,-.071),(-.083,-.052)])
    elif ASSET=='Striker':
        ring('Striker target ring',.086,.107,.1888,.1895,6,.0001,group='Emblem')
        ring('Striker center dot',.00001,.025,.1888,.1895,6,.0001,group='Emblem',steps=96)
        for i in range(4):
            a=i*TAU/4;c,s=math.cos(a),math.sin(a)
            pts=[(.075,-.009),(.165,-.009),(.165,.009),(.075,.009)]
            icon_poly('Striker crosshair '+str(i),[(x*c-y*s,x*s+y*c) for x,y in pts],.1896)
    elif ASSET=='Heavy':
        icon_rect('Heavy dumbbell bar',-.059,-.016,.059,.016,.001)
        for s in [-1,1]:
            cx=s*.096;icon_rect('Heavy large weight '+str(s),cx-.024,-.077,cx+.024,.077,.004)
            cx=s*.147;icon_rect('Heavy outer weight '+str(s),cx-.013,-.038,cx+.013,.038,.003)

def side_panel(name,points,c,material=0,r=.450,trim=True):
    pts=[(math.radians(x),z) for x,z in points]
    if trim:
        extrude_polygon(name+' edge bevel',pts,r-.004,r-.001,5,'Side armor',c)
        cx=sum(p[0] for p in pts)/len(pts); cy=sum(p[1] for p in pts)/len(pts)
        pts=[(cx+(x-cx)*.988,cy+(y-cy)*.965) for x,y in pts]
    extrude_polygon(name,pts,r-.003,r,material,'Side armor',c)

def slot_on_clamp(i,t):
    # Narrow tangential blue inlay across the clamp, recessed in a black rim.
    ring('Clamp slot gasket '+str(i),.386,.396,.1963,.1976,4,.0004,start=t-.074,end=t+.074,group='Clamps')
    ring('Clamp cyan slit '+str(i),.388,.392,.1973,.1978,3,.0002,start=t-.068,end=t+.068,group='Cyan lights')

def apply_dimensions():
    sx=DIAMETER_CM/90.0;sz=VISIBLE_HEIGHT_CM/20.0
    for part in PARTS:
        part['verts']=[(x*sx,y*sx,z*sz) for x,y,z in part['verts']]
        transformed=[]
        for x,y,z in part['normals']:
            n=(x/sx,y/sx,z/sz);length=math.sqrt(sum(t*t for t in n))
            transformed.append(tuple(t/length for t in n))
        part['normals']=transformed

def build_geometry():
    build_standard_geometry()
    # Keep the successful Standard crown, channels, lights and metal shaders.
    # Replace its identifying mark and armor with the individual reference parts.
    remove_prefixes=('Side armor panel','Side panel upper highlight','Side screw',
                     'Diagonal side joint','Top clamp screw')
    PARTS[:]=[p for p in PARTS if p['name']!='Center cyan O emblem' and not p['name'].startswith(remove_prefixes)]
    if ASSET in ('Blocker','Slider'):
        PARTS[:]=[p for p in PARTS if p['name']!='Top cyan window 4.2']
    if ASSET in ('Striker','Heavy'):
        PARTS[:]=[p for p in PARTS if not p['name'].startswith('Top light bridge')]
    if ASSET=='Blocker':
        # Broad silver crown matches the larger defensive puck.
        for p in PARTS:
            if p['group']=='Top crown' or p['name']=='Recessed center face' or p['name']=='Top cyan perimeter halo':
                new=[]
                for x,y,z in p['verts']:
                    r=math.hypot(x,y)
                    rr=r*.94 if r<.27 else .2538+(r-.27)*1.20
                    factor=rr/r if r>1e-10 else 1
                    new.append((x*factor,y*factor,z))
                p['verts']=new
    for i in range(4):
        c=i*TAU/4; t=c+math.pi/4
        slot_on_clamp(i,t)
        if ASSET in ('Toppler','Grippy','Heavy'):
            width={'Toppler':7.0,'Grippy':7.8,'Heavy':9.4}[ASSET]
            side_panel('Reinforced vertical brace '+str(i),[(-width,.025),(width,.025),(width,.150),(width-2,.175),(-width+2,.175),(-width,.150)],t,
                       material=5 if ASSET=='Heavy' else 0)
            # The reinforced cap bridges onto the top surface.
            lathe('Brace shoulder cap '+str(i),[(.426,.143),(.449,.146),(.444,.174),(.423,.193),(.414,.193),(.427,.169)],
                  5,t-math.radians(width-1),t+math.radians(width-1),group='Clamps')
            for z in ([.049,.135] if ASSET!='Heavy' else [.052]):
                screw('Brace bolt '+str(i)+' '+str(z),.4485,z,t,True)
            if ASSET in ('Grippy','Heavy'):
                for da in [-4.8,4.8]:screw('Brace corner bolt '+str(i)+' '+str(da),.4485,.042,t+math.radians(da),True)
            if ASSET=='Toppler':
                points=[(-35,.040),(35,.040),(35,.124),(29,.121),(25,.101),(-25,.101),(-29,.121),(-35,.124)]
                side_panel('Toppler lower raised armor '+str(i),points,c)
                side_panel('Toppler upper brow '+str(i),[(-35,.127),(-28,.123),(-24,.105),(24,.105),(28,.123),(35,.127),(35,.140),(-35,.140)],c,r=.449)
            elif ASSET=='Grippy':
                side_panel('Grippy bottom grip panel '+str(i),[(-34,.039),(34,.039),(34,.100),(27,.103),(25,.112),(-25,.112),(-27,.103),(-34,.100)],c)
                side_panel('Grippy upper reinforced panel '+str(i),[(-34,.104),(-27,.107),(-25,.116),(25,.116),(27,.107),(34,.104),(34,.140),(-34,.140)],c)
                for da in [-29,29]:screw('Grippy upper side bolt '+str(i)+str(da),.4485,.129,c+math.radians(da),True)
            else:
                side_panel('Heavy layered upper band '+str(i),[(-34,.091),(34,.091),(34,.139),(-34,.139)],c)
                side_panel('Heavy deep lower plate '+str(i),[(-34,.041),(-13,.041),(-9,.027),(9,.027),(13,.041),(34,.041),(34,.087),(-34,.087)],c)
                screw('Heavy center anchor '+str(i),.4485,.116,c,True)
        elif ASSET=='Bouncer':
            side_panel('Bouncer impact panel '+str(i),[(-34,.041),(34,.041),(38,.063),(33,.085),(36,.104),(36,.137),(-36,.137),(-36,.104),(-33,.085),(-38,.063)],c)
            side_panel('Bouncer interlocking bumper '+str(i),[(-7,.040),(7,.040),(4,.064),(9,.086),(6,.107),(6,.136),(-6,.136),(-6,.107),(-9,.086),(-4,.064)],t)
            for da in [-30,30]:screw('Bouncer bumper stud '+str(i)+str(da),.4485,.055,c+math.radians(da),True)
        elif ASSET=='Compact':
            side_panel('Compact tapered center armor '+str(i),[(-18,.039),(18,.039),(28,.138),(-28,.138)],c)
            side_panel('Compact diagonal side wedge '+str(i),[(-23,.039),(23,.039),(14,.138),(-14,.138)],t)
        elif ASSET=='Blocker':
            side_panel('Blocker wide shield panel '+str(i),[(-34,.040),(-7,.040),(-4,.058),(4,.058),(7,.040),(34,.040),(36,.062),(33,.078),(36,.099),(36,.138),(-36,.138),(-36,.099),(-33,.078),(-36,.062)],c)
            side_panel('Blocker side join '+str(i),[(-7,.039),(7,.039),(7,.138),(-7,.138)],t)
            side_panel('Blocker indicator socket '+str(i),[(-5,.040),(5,.040),(5,.055),(-5,.055)],c,4,.4498,False)
            side_panel('Blocker cyan status dash '+str(i),[(-3.8,.046),(3.8,.046),(3.8,.050),(-3.8,.050)],c,3,.450,False)
        elif ASSET=='Slider':
            side_panel('Slider streamlined side plate '+str(i),[(-35,.039),(-6,.039),(-4,.048),(4,.048),(6,.039),(35,.039),(31,.070),(36,.137),(-36,.137),(-31,.070)],c)
            side_panel('Slider swept corner armor '+str(i),[(-8,.039),(8,.039),(5,.071),(10,.137),(-10,.137),(-5,.071)],t)
        elif ASSET=='Striker':
            side_panel('Striker upper impact brow '+str(i),[(-31,.100),(-26,.109),(-22,.084),(22,.084),(26,.109),(31,.100),(35,.138),(-35,.138)],c)
            side_panel('Striker tapered lower impact plate '+str(i),[(-15,.027),(15,.027),(24,.080),(-24,.080)],c)
            side_panel('Striker angular side impact plate '+str(i),[(-16,.032),(16,.032),(18,.106),(10,.112),(9,.138),(-9,.138),(-10,.112),(-18,.106)],t)
    if ASSET in ('Blocker','Slider'):
        for i,(a,b) in enumerate([(-24,-8),(-7.5,7.5),(8,24)]):
            ring(ASSET+' front dark rim insert '+str(i),.359,.422,.181,.191,0,.001,
                 start=-math.pi/2+math.radians(a),end=-math.pi/2+math.radians(b),group='Top perimeter')
    if ASSET=='Slider':
        # Three small blue speed indicators in the front shoulder.
        for i in range(3):
            c=-math.pi/2+math.radians((i-1)*3.5)
            lathe('Slider front telemetry '+str(i),[(.430,.174),(.437,.168),(.440,.171),(.433,.178)],3,
                  c-math.radians(1.5),c+math.radians(1.5),group='Cyan lights')
    if ASSET in ('Striker','Heavy'):
        # Reinforced metallic pads replacing the Standard's thin window bridges.
        for i in range(4):
            for s in [-1,1]:
                a=i*TAU/4+s*math.radians(30)
                lathe('Armored outer rim pad '+str(i)+str(s),[(.366,.181),(.430,.175),(.429,.184),(.419,.193),(.369,.195)],5,
                      a-.065,a+.065,group='Top perimeter')
    # Preserve bottommost z=0 and outer radius=.45; these establish true dimensions.
    emblem()
    apply_dimensions()


def export_glb(path):
    """Write glTF 2.0 directly; dimensions in meters; Y-up conversion."""
    doc=dict(asset={'version':'2.0','generator':'Reference Puck Builder'},
             scene=0,scenes=[{'nodes':[0]}],nodes=[{'name':ASSET+' | '+str(DIAMETER_CM)+' cm diameter','children':[], 'extras':{'diameter_cm':DIAMETER_CM,'visible_height_cm':VISIBLE_HEIGHT_CM,'physics_height_cm':PHYSICS_HEIGHT_CM}}],
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
    scene=bpy.data.scenes.new(ASSET+' | reference reconstruction')
    if bpy.context.window: bpy.context.window.scene=scene
    scene.unit_settings.system='METRIC'; scene.unit_settings.length_unit='CENTIMETERS'
    scene.unit_settings.scale_length=1.0
    root=bpy.data.collections.new(ASSET+' | visual mesh'); scene.collection.children.link(root)
    groups={}
    for group in sorted({p['group'] for p in PARTS}):
        groups[group]=bpy.data.collections.new(group); root.children.link(groups[group])
    anchor=bpy.data.objects.new(ASSET+' ROOT | floor-centered origin',None); root.objects.link(anchor)
    anchor['diameter_m']=DIAMETER_CM/100; anchor['visible_height_m']=VISIBLE_HEIGHT_CM/100
    anchor['physics_height_m']=PHYSICS_HEIGHT_CM/100
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
    if CREATE_COLLISION_PROXY:
        collision_collection=bpy.data.collections.new('COLLISION | physics height | hidden')
        scene.collection.children.link(collision_collection)
        count=64; radius=DIAMETER_CM/200; mid=VISIBLE_HEIGHT_CM/200; half=PHYSICS_HEIGHT_CM/200
        cv=[(radius*math.cos(TAU*i/count),radius*math.sin(TAU*i/count),z) for z in (mid-half,mid+half) for i in range(count)]
        cf=[tuple(reversed(range(count))),tuple(range(count,2*count))]
        cf.extend((i,(i+1)%count,(i+1)%count+count,i+count) for i in range(count))
        cm=bpy.data.meshes.new('Physics cylinder');cm.from_pydata(cv,[],cf);cm.update()
        proxy=bpy.data.objects.new('UCX_'+ASSET+'_00',cm);collision_collection.objects.link(proxy)
        proxy.parent=anchor;proxy.display_type='WIRE';proxy.hide_render=True
        proxy['physics_height_cm']=PHYSICS_HEIGHT_CM
        proxy['note']='Optional centered collision proxy; not included in the visual GLB.'
        collision_collection.hide_render=True;collision_collection.hide_viewport=True
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
    cam.location=(0,-1.50,1.35); aim(cam,(0,0,VISIBLE_HEIGHT_CM/200)); cam_data.type='ORTHO'; cam_data.ortho_scale=DIAMETER_CM/100*1.22; scene.camera=cam
    scene.world=bpy.data.worlds.new('Soft gray studio'); scene.world.use_nodes=True
    scene.world.node_tree.nodes.get('Background').inputs[0].default_value=(.30,.35,.40,1)
    scene.world.node_tree.nodes.get('Background').inputs[1].default_value=.45
    scene.render.engine='CYCLES'; scene.cycles.samples=96; scene.cycles.use_denoising=True
    scene.render.resolution_x=1536; scene.render.resolution_y=1536; scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG'; scene.render.filepath=str(output_dir/(ASSET+'_render.png'))
    scene.view_settings.view_transform='AgX'
    setup_glow_compositor(scene)
    if bpy.context.screen:
        for area in bpy.context.screen.areas:
            if area.type=='VIEW_3D':
                area.spaces.active.region_3d.view_perspective='CAMERA'
                area.spaces.active.shading.type='MATERIAL'
    save_path = output_dir / (ASSET+'.blend')
    print(f'Saving {ASSET} puck to: {save_path}')
    bpy.ops.wm.save_as_mainfile(filepath=str(save_path))
    print('Created '+ASSET+'.blend. Press F12 to render the reference view.')

def output_directory():
    """Choose a user-writable output folder instead of the filesystem root."""
    try:
        import bpy
    except ImportError:
        return Path(__file__).resolve().parent

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
        folder = Path(__file__).resolve().parent.parent / "models"
        folder.mkdir(parents=True, exist_ok=True)
        if folder.is_dir() and os.access(folder, os.W_OK):
            return folder
    except NameError:
        pass

    # Reliable Windows/macOS/Linux fallback.
    home = Path.home()
    preferred = home / "Documents" / "Blue_Team_Puck_Output"
    try:
        preferred.mkdir(parents=True, exist_ok=True)
        return preferred
    except OSError:
        return home

if __name__=='__main__':
    build_geometry(); output=output_directory()
    try: import bpy
    except ImportError:
        export_glb(output/(ASSET+'.glb'))
        print(f'Created {ASSET}.glb: {len(PARTS)} editable mesh parts.')
    else:
        blender_scene(output)
        export_glb(output/(ASSET+'.glb'))
        print('Created '+str(output/(ASSET+'.glb'))+' as well.')
