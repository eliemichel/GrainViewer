import bpy
import struct

def get_particles(context, objname):
    degp = context.evaluated_depsgraph_get()
    object = bpy.data.objects[objname]
    return object.evaluated_get(degp).particle_systems[0].particles
    
    

def write_point_buffer(context, filepath):
    objname = context.object.name
    start = context.scene.frame_start
    end = context.scene.frame_end
    point_count
    print("Exporting point positions from '" + objname + "' between frames " + str(start) + " and " + str(end))
    print(" -- packed as 32 bit floats, 4x4 column major matrix (glm compatible) --")
    point_count = None
    buffer = None
    with open(filepath, 'wb') as f:
        for frame in range(start, end + 1):
            context.scene.frame_set(frame)
            particles = get_particles(context, objname)
            if buffer is None:
                point_count = len(particles)
                buffer = [0]*(3*point_count)
                # Write "Header"
                f.write(struct.pack("ff", point_count, end - start + 1))
            particles.foreach_get("location", buffer)
            f.write(struct.pack("f"*len(buffer), *buffer))

    return {'FINISHED'}


from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator


class ExportPointBuffer(Operator, ExportHelper):
    """Export the particles position at each frame as a raw binary buffer
    WARNING: This assumes that the number of particles is constant over time."""
    bl_idname = "export.point_buffer"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Export Point Buffer"

    filename_ext = ".bin"

    filter_glob: StringProperty(
        default="*.bin",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )

    def execute(self, context):
        return write_point_buffer(context, self.filepath)


# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(ExportCameraMatrixBuffer.bl_idname, text="Export Point Buffer")


def register():
    bpy.utils.register_class(ExportPointBuffer)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(ExportPointBuffer)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()
