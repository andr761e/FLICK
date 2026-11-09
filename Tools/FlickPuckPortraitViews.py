"""Shared offline portrait framing for every finish, including Retro Arcade."""
VIEWS = {
    'Original': dict(camera=(1.15,-1.9,1.75), target=(0,0,.05), ortho=1.48, size=(512,384), transparent=True),
    'Locker': dict(camera=(0,-.95,2.6), target=(0,0,.06), ortho=1.5, size=(768,768), transparent=False),
    'Presentation': dict(camera=(1.05,-1.9,1.85), target=(0,0,.065), ortho=1.55, size=(1024,768), transparent=False),
    'Hero': dict(camera=(.9,-1.9,1.45), target=(0,0,.08), ortho=1.62, size=(1024,768), transparent=False),
    'ProfileCutout': dict(camera=(.65,-2,1.18), target=(0,0,.07), ortho=1.42, size=(1024,768), transparent=True),
}

def apply_view(scene,camera,name):
    from mathutils import Vector
    spec=VIEWS[name]
    camera.location=spec['camera']
    camera.rotation_euler=(Vector(spec['target'])-camera.location).to_track_quat('-Z','Y').to_euler()
    camera.data.type='ORTHO';camera.data.ortho_scale=spec['ortho']
    scene.render.resolution_x,scene.render.resolution_y=spec['size']
    scene.render.resolution_percentage=100
    scene.render.film_transparent=spec['transparent']
    return spec
