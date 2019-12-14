import bpy
from bpy_extras.object_utils import object_data_add
import struct

def read_point_cloud(context, filepath):
    print("running read_some_data...")
    f = open(filepath, 'rb')
    
    point_count = int(struct.unpack("f", f.read(4))[0])
    f.read(4) # anim
    print("Loading {} points...".format(point_count))
    points = [(0.,0.,0.)]*point_count
    for i in range(point_count):
        points[i] = struct.unpack("fff", f.read(3*4))
    f.close()

    mesh = bpy.data.meshes.new(name="Point Cloud")
    mesh.from_pydata(points, [], [])
    object_data_add(context, mesh)

    return {'FINISHED'}

from bpy_extras.io_utils import ImportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ImportPointBuffer(Operator, ImportHelper):
    """Import simple point cloud from XYZ ascii file"""
    bl_idname = "import.bin_point_cloud"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Import Point Cloud"

    # ImportHelper mixin class uses this
    filename_ext = ".bin"

    filter_glob: StringProperty(
        default="*.bin",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    def execute(self, context):
        return read_point_cloud(context, self.filepath)


def menu_func_import(self, context):
    self.layout.operator(ImportPointBuffer.bl_idname, text="Import Point Cloud")


def register():
    bpy.utils.register_class(ImportPointBuffer)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)


def unregister():
    bpy.utils.unregister_class(ImportPointBuffer)
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)


if __name__ == "__main__":
    register()
