"""
FLICK / Neon arena puck — procedural, studio-lit Blender scene.
Reference: the cyan / hot-pink / yellow futuristic puck in the supplied image.

Physical puck dimensions (not including the presentation floor):
    DIAMETER  = 90 cm  (0.90 m)
    HEIGHT    = 20 cm  (0.20 m)

Compatible with Blender 3.6 / 4.x and designed for Cycles.
Run from Blender's Scripting workspace, or:
    blender -b --python FLICK_neon_puck.py

The script creates all geometry, shader nodes, camera, light rig, a reflective
floor and render settings; no external textures, add-ons or assets are needed.
Change RENDER_NOW to True if you want the script to render immediately.
"""

import bpy
import math
import random
from mathutils import Vector
from collections import defaultdict

# -----------------------------------------------------------------------------
# PARAMETERS — world geometry is in meters, helper coordinates below are in cm.
# -----------------------------------------------------------------------------
CM = 0.01
DIAMETER_CM = 90.0
HEIGHT_CM = 20.0
RADIUS_CM = DIAMETER_CM / 2
RENDER_NOW = False
SAVE_BLEND = True
OUTPUT_DIRECTORY = "//"
IMAGE_NAME = "FLICK_neon_puck_render.png"
BLEND_NAME = "FLICK_neon_puck.blend"
RENDER_RESOLUTION = 1536
CYCLES_SAMPLES = 192
RNG = random.Random(20261108)

CYAN_RGB    = (0.012, 0.82, 1.00, 1)
CYAN_SOFT   = (0.014, 0.36, 0.55, 1)
PINK_RGB    = (1.00, 0.008, 0.58, 1)
PINK_SOFT   = (0.41, 0.008, 0.25, 1)
YELLOW_RGB  = (1.00, 0.77, 0.018, 1)
PURPLE_RGB  = (0.37, 0.015, 0.48, 1)


def radians(degrees):
    return math.radians(degrees)


def xyz(radius_cm, theta_degrees, height_cm):
    a = radians(theta_degrees)
    return (CM * radius_cm * math.cos(a),
            CM * radius_cm * math.sin(a),
            CM * height_cm)


# -----------------------------------------------------------------------------
# CLEAR SCENE AND SET UP COLLECTIONS
# -----------------------------------------------------------------------------
if bpy.context.object is not None and bpy.context.object.mode != 'OBJECT':
    bpy.ops.object.mode_set(mode='OBJECT')
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
for data_group in (bpy.data.meshes, bpy.data.curves, bpy.data.materials,
                   bpy.data.cameras, bpy.data.lights):
    for block in list(data_group):
        if block.users == 0:
            data_group.remove(block)

scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
scene.unit_settings.length_unit = 'CENTIMETERS'
scene.render.engine = 'CYCLES'
scene.cycles.samples = CYCLES_SAMPLES
if hasattr(scene.cycles, 'use_denoising'):
    scene.cycles.use_denoising = True
scene.render.resolution_x = RENDER_RESOLUTION
scene.render.resolution_y = RENDER_RESOLUTION
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.render.image_settings.color_mode = 'RGBA'
scene.render.film_transparent = False
scene.render.filepath = OUTPUT_DIRECTORY + IMAGE_NAME

try:
    scene.view_settings.view_transform = 'AgX'
except Exception:
    scene.view_settings.view_transform = 'Filmic'
for candidate_look in ('AgX - Medium High Contrast', 'Medium High Contrast', 'Medium High Contrast'):
    try:
        scene.view_settings.look = candidate_look
        break
    except Exception:
        continue
scene.view_settings.exposure = -0.18
scene.view_settings.gamma = 1.0


def collection(name):
    c = bpy.data.collections.new(name)
    scene.collection.children.link(c)
    return c

COL_SHELL = collection('01 • Machined black shell')
COL_TOP = collection('02 • Top concentric assemblies')
COL_LIGHTS = collection('03 • Illuminated physical inserts')
COL_LED = collection('04 • Individual LED / equalizer mosaics')
COL_DETAILS = collection('05 • Screws, seam details and vents')
COL_STUDIO = collection('06 • Camera, floor and studio')


def in_collection(obj, target):
    for coll in list(obj.users_collection):
        coll.objects.unlink(obj)
    target.objects.link(obj)
    return obj


# -----------------------------------------------------------------------------
# PHYSICALLY BASED MATERIALS
# -----------------------------------------------------------------------------
def material(name, color, metallic=0.0, roughness=0.35, emission=None,
             strength=0.0, texture=False, coat=0.0):
    m = bpy.data.materials.new(name)
    m.diffuse_color = color
    m.use_nodes = True
    nodes = m.node_tree.nodes
    bsdf = nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = color
    bsdf.inputs['Metallic'].default_value = metallic
    bsdf.inputs['Roughness'].default_value = roughness
    if coat:
        key = 'Coat Weight' if 'Coat Weight' in bsdf.inputs else 'Clearcoat'
        if key in bsdf.inputs:
            bsdf.inputs[key].default_value = coat
    if emission is not None:
        emission_key = 'Emission Color' if 'Emission Color' in bsdf.inputs else 'Emission'
        if emission_key in bsdf.inputs:
            bsdf.inputs[emission_key].default_value = emission
        if 'Emission Strength' in bsdf.inputs:
            bsdf.inputs['Emission Strength'].default_value = strength
    if texture:
        tex = nodes.new('ShaderNodeTexNoise')
        tex.label = 'Subtle brushed / anodized micro-texture'
        tex.inputs['Scale'].default_value = 255
        tex.inputs['Detail'].default_value = 2
        tex.inputs['Roughness'].default_value = 0.7
        bump = nodes.new('ShaderNodeBump')
        bump.inputs['Strength'].default_value = 0.085
        bump.inputs['Distance'].default_value = 0.00005
        m.node_tree.links.new(tex.outputs['Fac'], bump.inputs['Height'])
        m.node_tree.links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])
    return m

MAT_HULL = material('01 | satin almost-black titanium',
                    (0.009, 0.016, 0.029, 1), metallic=.78,
                    roughness=.27, texture=True, coat=.22)
MAT_DARK = material('02 | black graphite ceramic',
                    (0.005, 0.010, 0.019, 1), metallic=.42,
                    roughness=.32, texture=True, coat=.18)
MAT_DISC = material('03 | slightly textured deep black top',
                    (0.006, 0.013, 0.023, 1), metallic=.18,
                    roughness=.40, texture=True, coat=.11)
MAT_GUNMETAL = material('04 | machine-grooved gunmetal',
                       (0.035, .052, .073, 1), metallic=.81,
                       roughness=.23, texture=True, coat=.25)
MAT_POLISH = material('05 | glossy polished edge titanium',
                     (.071, .101, .135, 1), metallic=.93,
                     roughness=.16, coat=.32)
MAT_SEAM = material('06 | unlit seam / graphite cutout',
                    (.0015, .003, .0065, 1), metallic=.25,
                    roughness=.47)
MAT_BLUE_DARK = material('07 | dim cool lens glass',
                         (.005, .045, .074, 1), metallic=.28,
                         roughness=.23, emission=CYAN_SOFT, strength=.35)

MAT_CYAN = material('08 | CYAN • glassy bright emissive core',
                    (.005, .38, .54, 1), metallic=.17, roughness=.22,
                    emission=CYAN_RGB, strength=7.0, coat=.35)
MAT_CYAN_HI = material('09 | CYAN • intense central ring',
                       (.005, .42, .65, 1), metallic=.08, roughness=.18,
                       emission=CYAN_RGB, strength=9.4, coat=.36)
MAT_CYAN_LOW = material('10 | CYAN • muted pixel',
                        (.005, .13, .17, 1), metallic=.08, roughness=.24,
                        emission=CYAN_RGB, strength=1.3)
MAT_CYAN_MID = material('11 | CYAN • medium pixel',
                        (.005, .23, .33, 1), metallic=.08, roughness=.22,
                        emission=CYAN_RGB, strength=3.6)
MAT_PINK = material('12 | MAGENTA • hot-pink neon',
                    (.52, .006, .23, 1), metallic=.09, roughness=.20,
                    emission=PINK_RGB, strength=8.3, coat=.36)
MAT_PINK_LO = material('13 | MAGENTA • muted pixel',
                       (.17, .008, .10, 1), metallic=.10, roughness=.26,
                       emission=PINK_RGB, strength=1.4)
MAT_PINK_MID = material('14 | MAGENTA • medium pixel',
                        (.35, .008, .15, 1), metallic=.10, roughness=.24,
                        emission=PINK_RGB, strength=4.0)
MAT_YELLOW = material('15 | YELLOW • warm neon',
                      (.58, .41, .006, 1), metallic=.10, roughness=.20,
                      emission=YELLOW_RGB, strength=7.4, coat=.33)
MAT_PURPLE = material('16 | PURPLE • dim equalizer',
                      (.17, .010, .23, 1), metallic=.10, roughness=.24,
                      emission=PURPLE_RGB, strength=1.65)
MAT_FLOOR = material('17 | reflective blue-grey studio floor',
                     (.23, .32, .46, 1), metallic=.22,
                     roughness=.285, coat=.18)


# -----------------------------------------------------------------------------
# MODELING HELPERS
# -----------------------------------------------------------------------------
def assign(obj, mat, coll):
    obj.data.materials.append(mat)
    in_collection(obj, coll)
    return obj


def soften(obj, width_cm=.10, segments=3):
    if width_cm <= 0:
        return obj
    mod = obj.modifiers.new('Fine machined bevels catch reflections', 'BEVEL')
    mod.width = CM * width_cm
    mod.segments = segments
    if hasattr(mod, 'affect'):
        mod.affect = 'EDGES'
    # Use weighted normals to make large hard-metal faces read as smooth.
    try:
        normal = obj.modifiers.new('Weighted face normals', 'WEIGHTED_NORMAL')
        normal.keep_sharp = True
    except Exception:
        pass
    return obj


def cylinder(name, radius_cm, z_bottom_cm, z_top_cm, mat, coll,
             bevel_cm=.12, segments=256):
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=segments, radius=radius_cm*CM,
        depth=(z_top_cm-z_bottom_cm)*CM,
        location=(0, 0, (z_top_cm+z_bottom_cm)*CM/2))
    obj = bpy.context.object
    obj.name = name
    assign(obj, mat, coll)
    for poly in obj.data.polygons:
        poly.use_smooth = (len(poly.vertices) == 4)
    soften(obj, bevel_cm)
    return obj


def arc_solid(name, r_inner_cm, r_outer_cm, a0_degrees, a1_degrees,
              bottom_cm, top_cm, mat, coll, bevel_cm=.07,
              steps_per_degree=1.3):
    """A genuinely extruded curved panel with radial seams, not a flat decal."""
    width = a1_degrees - a0_degrees
    if width <= 0:
        raise ValueError('Arc angles must ascend')
    n = max(3, int(math.ceil(width * steps_per_degree)))
    full_ring = abs(width - 360.0) < .0001
    N = n if full_ring else n + 1
    verts = []
    for z in (bottom_cm, top_cm):
        for r in (r_inner_cm, r_outer_cm):
            for j in range(N):
                a = a0_degrees + width * j / n
                verts.append(xyz(r, a, z))
    faces = []
    for j in range(n):
        j_next = (j+1)%N if full_ring else j+1
        # Bottom and top, normals controlled by face orientation.
        faces.append((j, j_next, N+j_next, N+j))
        faces.append((2*N+j, 3*N+j, 3*N+j_next, 2*N+j_next))
        faces.append((j, 2*N+j, 2*N+j_next, j_next))
        faces.append((N+j, N+j_next, 3*N+j_next, 3*N+j))
    if not full_ring:
        faces.extend(((0, N, 3*N, 2*N),
                      (n, 2*N+n, 3*N+n, N+n)))
    mesh = bpy.data.meshes.new(name + ' | mesh')
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    coll.objects.link(obj)
    mesh.materials.append(mat)
    soften(obj, bevel_cm, 2)
    return obj


def annulus(name, inner, outer, bottom, top, mat, coll, bevel=.07):
    return arc_solid(name, inner, outer, 0, 360,
                     bottom, top, mat, coll, bevel, .76)


def torus(name, major_radius_cm, minor_radius_cm, z_cm, mat, coll,
          major_segments=256, minor_segments=12):
    bpy.ops.mesh.primitive_torus_add(
        major_segments=major_segments, minor_segments=minor_segments,
        location=(0, 0, z_cm*CM),
        major_radius=major_radius_cm*CM,
        minor_radius=minor_radius_cm*CM)
    obj = bpy.context.object
    obj.name = name
    assign(obj, mat, coll)
    for p in obj.data.polygons:
        p.use_smooth = True
    return obj


def uv_sphere(name, pos_cm, radius_cm, mat, coll):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=6,
                                        radius=CM*radius_cm,
                                        location=tuple(CM*t for t in pos_cm))
    obj = bpy.context.object
    obj.name = name
    assign(obj, mat, coll)
    for poly in obj.data.polygons:
        poly.use_smooth = True
    return obj


class LEDBatches:
    """Thousands of real LED squares, only one mesh object per LED material."""
    def __init__(self):
        self.data = defaultdict(lambda: [[], []])

    def quad(self, mat, pts):
        verts, faces = self.data[mat]
        offset = len(verts)
        verts.extend(pts)
        faces.append((offset, offset+1, offset+2, offset+3))

    def top(self, mat, r0, r1, a0, a1, z):
        self.quad(mat, (xyz(r0,a0,z), xyz(r1,a0,z),
                        xyz(r1,a1,z), xyz(r0,a1,z)))

    def side(self, mat, a0, a1, z0, z1, radius=44.94):
        self.quad(mat, (xyz(radius,a0,z0), xyz(radius,a1,z0),
                        xyz(radius,a1,z1), xyz(radius,a0,z1)))

    def finish(self):
        for mat, (verts, faces) in self.data.items():
            mesh = bpy.data.meshes.new('Curved/planar LED cell mesh')
            mesh.from_pydata(verts, [], faces)
            mesh.update()
            obj = bpy.data.objects.new('Thousands of tiny LEDs | ' + mat.name, mesh)
            COL_LED.objects.link(obj)
            mesh.materials.append(mat)


leds = LEDBatches()

# -----------------------------------------------------------------------------
# A. LAYERED CHASSIS — exactly radius <=45cm and z within 0..20cm
# -----------------------------------------------------------------------------
cylinder('Interior solid load-bearing chassis', 42.90, .40, 17.95,
         MAT_HULL, COL_SHELL, .35)
cylinder('Rubber-black flat contact base | z=0.00', 42.0, 0.00, 1.05,
         MAT_SEAM, COL_SHELL, .18)
cylinder('Bottom bevelled shell lip', 43.65, .75, 4.00,
         MAT_DARK, COL_SHELL, .39)
cylinder('Side fascia upper cylinder', 44.32, 3.60, 17.58,
         MAT_HULL, COL_SHELL, .28)
annulus('Lower perimeter machined-edge band', 42.85, 44.73,
        1.85, 3.30, MAT_POLISH, COL_SHELL, .17)
annulus('Lower black line between light and casing', 43.55, 44.83,
        4.48, 4.87, MAT_SEAM, COL_SHELL, .045)
annulus('Upper dark uninterrupted shoulder', 42.6, 44.76,
        16.84, 18.47, MAT_DARK, COL_SHELL, .14)
annulus('OUTERMOST physical 90cm diameter rim', 43.83, RADIUS_CM,
        17.82, 18.32, MAT_POLISH, COL_SHELL, .10)
annulus('Fine dark top highlight separator', 43.20, 44.82,
        18.42, 18.73, MAT_SEAM, COL_SHELL, .04)

# Side shell is intentionally not mirror smooth: slightly offset raised
# black-metal panels leave visible radial joins.
FRONT_ANGLE = -53.0  # orientation toward camera
for j in range(16):
    a0 = FRONT_ANGLE - 180 + j*22.5 + .6
    a1 = a0 + 21.25
    arc_solid('Side fascia panel %02d' % j,
              44.33, 44.61, a0, a1, 4.85, 16.00,
              MAT_DARK if j % 4 == 0 else MAT_HULL,
              COL_SHELL, .10)
    # Tiny chrome corner mounts on side edges.
    if j % 2 == 0:
        arc_solid('Side panel fine bright upper seam %02d' % j,
                  44.50, 44.67, a0, a1,
                  15.90, 16.10, MAT_POLISH, COL_DETAILS, .03)

# Lower cyan / yellow reflective base stripes, seen wrapping around the side.
for idx,(start,end,mat) in enumerate([
    (-157,-115,MAT_CYAN_MID),(-113,-78,MAT_CYAN),
    (-76,-36,MAT_CYAN),(-33,-15,MAT_CYAN_MID),
    (-13,12,MAT_YELLOW),(15,51,MAT_CYAN),
    (54,94,MAT_CYAN),(97,146,MAT_CYAN_MID),
    (150,177,MAT_PINK_MID),
]):
    arc_solid('Glowing waistline / lower side accent %02d' % idx,
              44.64, 44.90, FRONT_ANGLE+start, FRONT_ANGLE+end,
              3.33, 4.05, mat, COL_LIGHTS, .08)

# Backed center front badge and smooth solid magenta strip.
arc_solid('Front center dark angular panel', 44.60, 44.81,
          FRONT_ANGLE-19, FRONT_ANGLE+19,
          5.12, 12.40, MAT_DARK, COL_DETAILS, .10)
arc_solid('Front magenta glow strip | black bezel', 44.81, 44.89,
          FRONT_ANGLE-11.9, FRONT_ANGLE+11.9,
          8.45, 10.07, MAT_SEAM, COL_LIGHTS, .07)
arc_solid('Front magenta glow strip | luminous lens', 44.89, 44.975,
          FRONT_ANGLE-10.6, FRONT_ANGLE+10.6,
          8.63, 9.78, MAT_PINK, COL_LIGHTS, .05)
arc_solid('Small central dark dot in neon strip', 44.975, 44.992,
          FRONT_ANGLE-.8, FRONT_ANGLE+.8,
          9.02, 9.28, MAT_DARK, COL_DETAILS, .01)
for side in (-1,1):
    arc_solid('Small pink outer triangle substitute / inset',44.82,44.94,
              FRONT_ANGLE+side*21-.85, FRONT_ANGLE+side*21+.85,
              5.30, 6.15, MAT_PINK_MID, COL_LIGHTS, .04)

# -----------------------------------------------------------------------------
# B. UPPER HARD-SURFACE DETAILS, CONCENTRIC GUNMETAL CHANNELS
# -----------------------------------------------------------------------------
annulus('Outer engineered raised crown', 36.90, 44.66,
        18.47, 19.00, MAT_HULL, COL_TOP, .11)
annulus('Dark metal channel for bright outer tiles', 39.80, 44.08,
        18.88, 19.20, MAT_SEAM, COL_TOP, .11)
annulus('Inner ring with vents and LED matrices', 28.48, 39.46,
        18.90, 19.26, MAT_GUNMETAL, COL_TOP, .10)
annulus('Radial dark slit separating magenta and matrices', 27.43, 29.26,
        19.09, 19.38, MAT_DARK, COL_TOP, .09)
cylinder('Central almost-black circular play surface', 27.0,
         19.05, 19.25, MAT_DISC, COL_TOP, .07)

# Radial top panel segmentation lines: different widths give realistic casing.
for k in range(16):
    theta = FRONT_ANGLE + (k + .5)*22.5
    arc_solid('Flush top radial subdivision / %02d' % k,
              29.2, 39.3, theta-.23, theta+.23,
              19.258, 19.345, MAT_SEAM, COL_DETAILS, .014)

# Circular bright/dark trims within the etched control surface.
for i,(radius,minor,mat,z) in enumerate([
    (28.65,.105,MAT_POLISH,19.36),
    (30.04,.085,MAT_SEAM,19.40),
    (37.95,.105,MAT_BLUE_DARK,19.43),
    (38.74,.085,MAT_POLISH,19.38),
    (40.02,.095,MAT_SEAM,19.24),
    (43.94,.105,MAT_POLISH,18.92),
]):
    torus('Concentric precision machined separator %02d' % i,
          radius, minor, z, mat, COL_DETAILS, 320, 10)

# -----------------------------------------------------------------------------
# C. LARGE CYAN AND YELLOW OUTER TILES
# -----------------------------------------------------------------------------
# Angles here are in degrees relative to the front of the camera.
# Cyan dominates; alternating yellow tiles and magenta details reproduce the
# design language and asymmetric color placement of the reference.
TILES = [
    (-179,-150,'C'),(-147,-129,'Y'),(-126,-89,'C'),
    (-86,-65,'C'),(-62,-44,'Y'),(-41,5,'C'),
    (8,28,'Y'),(31,56,'C'),(59,80,'Y'),
    (83,123,'C'),(126,145,'Y'),(148,179,'C'),
]
for k,(aa,bb,key) in enumerate(TILES):
    a0,a1 = aa+FRONT_ANGLE, bb+FRONT_ANGLE
    arc_solid('Dark recessed frame underneath outer tile %02d' % k,
              40.17, 43.96, a0-.28, a1+.28,
              19.07, 19.41, MAT_HULL, COL_TOP, .14)
    bright = MAT_CYAN if key == 'C' else MAT_YELLOW
    arc_solid('Cyan/yellow glass light diffuser %02d' % k,
              40.56, 43.56, a0, a1,
              19.41, 19.68, bright, COL_LIGHTS, .14)
    # Visually distinct inner lower tracing strips.
    a2,a3 = a0+2.5,a1-2.5
    if a3 > a2:
        arc_solid('Crown lower thin neon filament %02d' % k,
                  38.82, 39.10, a2,a3,
                  19.36, 19.47,
                  MAT_YELLOW if (key == 'Y' and k%2==0) else MAT_CYAN_MID,
                  COL_LIGHTS, .025)

# Tiny black bridges, screws and inset magenta caps around the wide outer rim.
for i in range(12):
    a = FRONT_ANGLE + i*30 + 9
    arc_solid('Machined exterior radial black bridge %02d' % i,
              43.52, 44.72, a-2.2, a+2.2,
              19.05, 19.73, MAT_DARK, COL_DETAILS, .10)
    if i in (0,2,5,8,10):
        arc_solid('Fuchsia micro-strip on dark bridge %02d' % i,
                  43.81,44.21,a-1.1,a+1.1,
                  19.73,19.80,MAT_PINK,COL_LIGHTS,.022)

# -----------------------------------------------------------------------------
# D. TOP EQUALIZER: thousands of cyan and pink rectangular luminous pixels
# -----------------------------------------------------------------------------
# Dark backing vents define individual bays; the rectangles are genuinely
# placed on the circular surface, not drawn on as a flat image texture.
for bay in range(12):
    center = FRONT_ANGLE + bay*30 + 15
    a0, a1 = center-12.05, center+12.05
    arc_solid('Recessed perforated equalizer bay %02d' % bay,
              30.73, 37.70, a0, a1, 19.26, 19.45,
              MAT_SEAM, COL_DETAILS, .045)

    # 7 radial rows x ~24 angular columns / bay with variation and gaps.
    num_cols = 26
    for row in range(7):
        r0 = 31.20 + row*.81
        r1 = r0 + .44
        for col in range(num_cols):
            a = a0 + .72 + col*.89
            if a + .54 > a1-.4:
                continue
            p = RNG.random()
            phase = 2.1*math.sin(col*.32 + row*.8 + bay*.6)
            phase += 1.3*math.cos(col*.18-bay*.7)
            # Equalizer waves determine how many lights are actually lit.
            if p > .70 + .10*math.sin(phase) and col % 4 != 0:
                continue
            pink_bay = bay in (2,3,7,8)
            if pink_bay and (row in (0,1,2) or (col//5)%3 == 1):
                mat = MAT_PINK if p < .17 else MAT_PINK_MID if p < .47 else MAT_PINK_LO
            else:
                mat = MAT_CYAN if p < .17 else MAT_CYAN_MID if p < .58 else MAT_CYAN_LOW
            leds.top(mat, r0, r1, a, a+.54, 19.48)

# Neon segment traces and micro-glowing circuit marks along the mid control ring.
for k in range(48):
    angle = FRONT_ANGLE + k*7.5
    if k % 7 != 0:
        leds.top(MAT_CYAN_MID if k%3 else MAT_PINK_MID,
                 38.20, 38.54, angle+.5, angle+3.5, 19.48)
    if k%4==0:
        arc_solid('Inset short hairline bracket / %02d' % k,
                  29.55,30.05,angle,angle+2.4,
                  19.37,19.47,MAT_POLISH,COL_DETAILS,.02)

# Small cyan-ringed screw heads around the forward edge.
for k in range(12):
    angle = FRONT_ANGLE + k*30
    a = radians(angle)
    pos = (39.0*math.cos(a), 39.0*math.sin(a), 19.55)
    uv_sphere('Blue glowing exposed fastener %02d' % k,
              pos, .42, MAT_CYAN_MID, COL_DETAILS)
    uv_sphere('Dark fastener recessed center %02d' % k,
              (pos[0],pos[1],19.78), .18, MAT_DARK, COL_DETAILS)

# -----------------------------------------------------------------------------
# E. PRIMARY FUCHSIA TRACK / CYAN ROUND CENTER BUTTON
# -----------------------------------------------------------------------------
# The photograph's characteristic uninterrupted neon-pink ring.
torus('Inner magenta ring / unlit dark undercut', 27.07, .85, 19.05,
      MAT_SEAM, COL_TOP, 320, 20)
torus('MAIN uninterrupted magenta edge neon', 26.75, .59, 19.40,
      MAT_PINK, COL_LIGHTS, 384, 24)  # Maximum z = exactly 19.99cm.
torus('Thin inner secondary magenta halo', 25.95, .18, 19.37,
      MAT_PINK_MID, COL_LIGHTS, 384, 12)

# Cyan target bullseye. Wide, carefully rounded flat annular lens.
annulus('Central electric turquoise halo / backing groove',
        8.32, 12.36, 19.25, 19.42, MAT_BLUE_DARK, COL_TOP, .16)
annulus('MAIN thick central cyan luminous annulus',
        8.56, 12.13, 19.42, 19.76, MAT_CYAN_HI, COL_LIGHTS, .32)
torus('Fine glassy reflection around central cyan perimeter',
      11.91, .14, 19.74, MAT_CYAN, COL_LIGHTS, 256, 12)
cylinder('Inset black circle within bullseye', 8.52,
         19.25, 19.48, MAT_DISC, COL_TOP, .07)

# -----------------------------------------------------------------------------
# F. SIDES: CYAN / MAGENTA LED EQUALIZER MOSAICS AND VERTICAL SEAMS
# -----------------------------------------------------------------------------
# Each pixel is a curved-wall cell facing radially outward, so the blocks
# follow the cylindrical side silhouette correctly as the camera moves.
for side in (-1,1):
    lo,hi = ((-119,-22) if side < 0 else (22,119))
    n_cols = int((hi-lo) / 1.72)
    for col in range(n_cols):
        rel = lo + col*1.72
        wave = .50+.25*math.sin(col*.41 + side*2.5)
        wave += .15*math.sin(col*.93 + side*0.8)
        wave += .14*math.cos(col*.18)
        active_rows = max(2, min(9, int(2 + 7*wave)))
        for row in range(9):
            if row >= active_rows and RNG.random() > .22:
                continue
            # Sparse pixels: a faint wave has gaps and uneven heights.
            if RNG.random() < (.10 if row < 4 else .18):
                continue
            a0 = FRONT_ANGLE + rel + .18
            a1 = a0 + 1.14
            z0 = 5.06 + row*.91
            z1 = z0 + .64
            outboard = abs(rel) > 88
            central = abs(rel) < 44
            p = RNG.random()
            if outboard and (col+row)%4 != 0:
                mat = MAT_PINK if p < .15 else MAT_PINK_MID if p < .52 else MAT_PINK_LO
            elif not central and (col+2*row)%9 in (1,2):
                mat = MAT_PURPLE if p<.35 else MAT_PINK_MID
            else:
                mat = MAT_CYAN if p<.11 else MAT_CYAN_MID if p<.58 else MAT_CYAN_LOW
            leds.side(mat, a0,a1,z0,z1)

# Thin exposed vertical joints between side armor plates; narrow and sparse.
for k in range(16):
    a = FRONT_ANGLE - 180 + k*22.5
    arc_solid('Cylindrical side armor seam / %02d' % k,
              44.62,44.74,a-.20,a+.20,
              4.90,15.85,MAT_SEAM,COL_DETAILS,.014)
    if k % 3 == 0:
        arc_solid('Faint pink screw light at panel foot / %02d' % k,
                  44.70,44.90,a-.83,a+.83,
                  5.05,5.43,MAT_PINK_MID,COL_LIGHTS,.025)

leds.finish()

# -----------------------------------------------------------------------------
# G. STUDIO CYCLORAMA, CAMERA AND CONTROLLED MULTI-COLOR REFLECTIONS
# -----------------------------------------------------------------------------
bpy.ops.mesh.primitive_plane_add(size=200, location=(0,0,-.006))
floor = bpy.context.object
floor.name = 'Seamless blue-grey mirror/satin product photography floor'
assign(floor, MAT_FLOOR, COL_STUDIO)

world = bpy.data.worlds.new('Steel-grey photographic studio ambience') if not bpy.data.worlds else bpy.data.worlds[0]
scene.world = world
world.use_nodes = True
bg = world.node_tree.nodes.get('Background')
bg.inputs['Color'].default_value = (.24,.34,.47,1)
bg.inputs['Strength'].default_value = .55


def area_light(name, location, energy, color=(1,1,1), size=1.0,
               size_y=None, target=(0,0,.12)):
    light_data = bpy.data.lights.new(name, 'AREA')
    light = bpy.data.objects.new(name, light_data)
    COL_STUDIO.objects.link(light)
    light.location = location
    direction = Vector(target) - light.location
    light.rotation_euler = direction.to_track_quat('-Z', 'Y').to_euler()
    light_data.energy = energy
    light_data.color = color
    if size_y is None:
        light_data.shape = 'DISK'
        light_data.size = size
    else:
        light_data.shape = 'RECTANGLE'
        light_data.size = size
        light_data.size_y = size_y
    return light

# Large softboxes trace realistic curved silver highlights. Colored softboxes
# supplement the neon geometry with colored spill on side panels and the floor.
area_light('01 Huge white left-hand softbox', (-1.45,-.95,2.25), 520,
           (0.82,0.93,1.00), 1.60, 1.05)
area_light('02 Long overhead white light strip', (.50,.80,2.45), 760,
           (0.86,0.94,1.00), 1.85, .75)
area_light('03 Cool blue left reflection', (-1.45,.32,.90), 315,
           (.08,.73,1.00), 1.10)
area_light('04 Magenta right rim reflection', (1.52,.30,1.12), 275,
           (1.00,.12,.72), 1.05)
area_light('05 Gentle warm back edge fill', (.20,1.70,1.30), 215,
           (1.00,.76,.37), 1.12)
area_light('06 Front card for glossy black highlights', (.00,-1.55,1.15), 95,
           (.88,.96,1.00), 1.35, .35)

cam_data = bpy.data.cameras.new('Three-quarter product camera / ~29 degree elevation')
camera = bpy.data.objects.new('Hero camera 90cm puck', cam_data)
COL_STUDIO.objects.link(camera)
camera.location = (1.35,-1.82,1.29)
look_at = Vector((0,0,.090))
camera.rotation_euler = (look_at - camera.location).to_track_quat('-Z','Y').to_euler()
cam_data.type = 'PERSP'
cam_data.lens = 59.0
cam_data.sensor_width = 36.0
cam_data.clip_start = .025
cam_data.clip_end = 60
scene.camera = camera

# Low-intensity fog glow in addition to the physically emissive shaders.
# Blender 4.x and 3.x have a classic glare node; in newer APIs, failure to
# initialize this effect is harmless and DOES NOT affect object geometry.
scene.use_nodes = True
compositor = scene.node_tree
compositor.nodes.clear()
rl = compositor.nodes.new('CompositorNodeRLayers')
rl.location = (-340,0)
composite = compositor.nodes.new('CompositorNodeComposite')
composite.location = (220,0)
try:
    glow = compositor.nodes.new('CompositorNodeGlare')
    glow.glare_type = 'FOG_GLOW'
    glow.quality = 'HIGH'
    glow.threshold = 1.30
    glow.mix = -0.82
    glow.size = 8
    glow.location = (-65,0)
    compositor.links.new(rl.outputs['Image'],glow.inputs['Image'])
    compositor.links.new(glow.outputs['Image'],composite.inputs['Image'])
except Exception:
    compositor.links.new(rl.outputs['Image'],composite.inputs['Image'])

# Helpful custom properties visible in Blender's Object Properties panel.
master = bpy.data.objects.new('PUCK | dimension specification • no extra geometry', None)
COL_SHELL.objects.link(master)
master['diameter_cm'] = DIAMETER_CM
master['height_cm'] = HEIGHT_CM
master['physical_radius_m'] = .45
master['puck_origin'] = 'Center of bottom plane, z = 0'
master['note'] = 'Objects named as studio/floor are outside the actual puck dimensions.'

# Convenient camera view for when the .blend opens.
for screen in bpy.data.screens:
    for area in screen.areas:
        if area.type == 'VIEW_3D':
            area.spaces.active.region_3d.view_perspective = 'CAMERA'

if SAVE_BLEND:
    bpy.ops.wm.save_as_mainfile(filepath=OUTPUT_DIRECTORY+BLEND_NAME)
if RENDER_NOW:
    bpy.ops.render.render(write_still=True)

print('FLICK puck created. Target diameter: 0.90m | target height: 0.20m.')
print('Render image:', bpy.path.abspath(OUTPUT_DIRECTORY + IMAGE_NAME))
print('Render now:', RENDER_NOW, '| Samples:', CYCLES_SAMPLES)
