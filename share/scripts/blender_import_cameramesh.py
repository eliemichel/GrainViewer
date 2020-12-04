# This Python code is part of the supplemental material for the paper
# "Real time multi-scale sand rendering" submitted at I3D 2020.
#
# Released under the 3-Clause BSD License:
# 
# Copyright (c) 2019 - 2020 Élie Michel and Tamy Boubekeur.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from this
#    software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

#
# Blender script to place camera from a set of points, looking toward the origin.
# To bake impostors, use an orthographic camera.
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
