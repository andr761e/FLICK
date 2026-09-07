"""Run against PuckWorkshop.blend in background Blender."""
import bpy, json, struct
from pathlib import Path

root=Path(__file__).resolve().parents[1]
dimensions=json.loads((root/'dimensions.json').read_text())
results=[]
for entry in dimensions:
    name=entry['name']
    blue=bpy.data.objects['SM_Puck_'+name]
    orange=bpy.data.objects['SM_Puck_'+name+'_Orange']
    assert blue.data is orange.data, name+' does not share its mesh'
    differences=[]
    for a,b in zip(blue.material_slots,orange.material_slots):
        if a.material is not b.material:
            assert a.material.name=='05_Team_Cyan' and b.material.name=='05_Team_Orange'
            differences.append(a.material.name)
    assert len(differences)==1, (name,differences)
    radius=entry['radius_cm']/100
    # Visual Z proportions are intentionally independent from collider thickness.
    # Diameter remains constrained to the gameplay radius.
    half_height=entry.get('visual_height_cm',entry['thickness_cm'])/200
    for v in blue.data.vertices:
        assert v.co.xy.length <= radius+.005, (name,'radius')
        assert abs(v.co.z)<=half_height+.005, (name,'height')
    exported=[]
    for team,folder,material in [('Blue',root/'exports','05_Team_Cyan'),('Orange',root/'exports'/'Orange','05_Team_Orange')]:
        path=folder/(name+'.glb'); data=path.read_bytes()
        magic,version,length=struct.unpack_from('<III',data)
        assert magic==0x46546c67 and version==2 and length==len(data)
        size,kind=struct.unpack_from('<II',data,12)
        doc=json.loads(data[20:20+size])
        assert len(doc['meshes'])==1
        assert material in [m['name'] for m in doc['materials']]
        assert (folder/(name+'.fbx')).stat().st_size>1000
        exported.append(team)
    results.append(dict(name=name,shared_geometry=True,team_material_only=True,bounds_passed=True,exports=exported))
assert len({bpy.data.objects['SM_Puck_'+e['name']].data.as_pointer() for e in dimensions})==9
(root/'validation.json').write_text(json.dumps(results,indent=2))
print('FLICK_WORKSHOP_VALIDATED: 18 pucks, 9 shared meshes, 18 GLBs, 18 FBXs; team materials and bounds passed')
