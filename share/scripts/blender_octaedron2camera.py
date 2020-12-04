#
# Blender script to place camera from a set of points
#
from mathutils import Vector, Matrix
import bpy

def lookFrom(p):
    y = Vector([0,0,1])
    z = p
    x = y.cross(z).normalized()
    y = z.cross(x).normalized()
    return Matrix.Translation(p) @ Matrix([x, y, z]).transposed().to_4x4()

camera = bpy.context.scene.camera

for i, line in enumerate(open('octahedron.xyz', 'r')):
    coords = [float(x) for x in line.split()]
    if len(coords) != 3:
        print("Unable to parse line: %s " % line)
        exit(1)

    p = Vector(coords)
    camera.matrix_world = lookFrom(p)
    camera.keyframe_insert(data_path="location", frame=i)
    camera.keyframe_insert(data_path="rotation_euler", frame=i)
