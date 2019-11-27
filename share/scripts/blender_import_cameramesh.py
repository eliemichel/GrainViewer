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

def import_camera_mesh(context, filename):
    camera = context.scene.camera

    for i, line in enumerate(open(filename, 'r')):
        coords = [float(x) for x in line.split()]
        if len(coords) != 3:
            print("Unable to parse line: %s " % line)
            exit(1)

        p = Vector(coords)
        camera.matrix_world = lookFrom(p)
        camera.keyframe_insert(data_path="location", frame=i)
        camera.keyframe_insert(data_path="rotation_euler", frame=i)

    return {'FINISHED'}


from bpy_extras.io_utils import ImportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ImportCameraMesh(Operator, ImportHelper):
    """Import XYZ file as camera positions for impostor baking"""
    bl_idname = "import.camera_mesh"
    bl_label = "Import Camera Mesh"

    filename_ext = ".xyz"

    filter_glob: StringProperty(
        default="*.xyz",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    def execute(self, context):
        return import_camera_mesh(context, self.filepath)


def menu_func_import(self, context):
    self.layout.operator(ImportCameraMesh.bl_idname, text="Import Camera Mesh")


def register():
    bpy.utils.register_class(ImportCameraMesh)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)


def unregister():
    bpy.utils.unregister_class(ImportCameraMesh)
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)


if __name__ == "__main__":
    register()
