import bpy
from bpy_extras.object_utils import object_data_add

def read_xyz(context, filepath):
    print("running read_some_data...")
    f = open(filepath, 'r', encoding='utf-8')
    points = []
    for line in f:
        coords = [float(c) for c in line.split()[0:3]]
        points.append(coords)
    f.close()

    mesh = bpy.data.meshes.new(name="Point Cloud")
    mesh.from_pydata(points, [], [])
    object_data_add(context, mesh)

    return {'FINISHED'}


from bpy_extras.io_utils import ImportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ImportXYZ(Operator, ImportHelper):
    """Import simple point cloud from XYZ ascii file"""
    bl_idname = "import.xyz_point_cloud"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Import XYZ Point Cloud"

    # ImportHelper mixin class uses this
    filename_ext = ".xyz"

    filter_glob: StringProperty(
        default="*.xyz",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    def execute(self, context):
        return read_xyz(context, self.filepath)


def menu_func_import(self, context):
    self.layout.operator(ImportXYZ.bl_idname, text="Import XYZ Point Cloud")


def register():
    bpy.utils.register_class(ImportXYZ)
    bpy.types.TOPBAR_MT_file_import.append(menu_func_import)


def unregister():
    bpy.utils.unregister_class(ImportXYZ)
    bpy.types.TOPBAR_MT_file_import.remove(menu_func_import)


if __name__ == "__main__":
    register()
