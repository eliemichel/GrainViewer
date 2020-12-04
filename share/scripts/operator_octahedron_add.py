import bpy
import bmesh
from bpy_extras.object_utils import AddObjectHelper
from math import sqrt

def normalize(v):
    x, y, z = v[0], v[1], v[2]
    d = 1 / sqrt(x*x+y*y+z*z)
    return [x * d, y * d, z * d]

def add_box(n, spherize):
    verts = []
    faces = []
    for i in range(n):
        u = i / (n - 1)
        for j in range(n):
            v = j / (n - 1)
            x = u + v - 1;
            y = u - v;
            z = 1 - abs(x) - abs(y)
            verts.append((x, y, z))
            if i > 0 and j > 0:
                faces.append([
                    (i - 1) * n + j - 1,
                    (i - 1) * n + j,
                    i * n + j,
                    i * n + j - 1,
                ])
    
    if spherize:
        verts = [normalize(v) for v in verts]

    return verts, faces


from bpy.props import (
    BoolProperty,
    IntProperty,
    EnumProperty,
    FloatVectorProperty,
)


class AddOctahedron(bpy.types.Operator):
    """Add a subdivided octahedron mesh"""
    bl_idname = "mesh.primitive_octahedron_add"
    bl_label = "Add Octahedron"
    bl_options = {'REGISTER', 'UNDO'}

    subdivisions: IntProperty(
        name="Subdivisions",
        description="Number of subdivisions",
        min=2, max=32,
        default=8,
    )
    spherize: BoolProperty(
        name="Spherize",
        description="Normalize point positions using L2 norm",
        default=False
    )

    # generic transform props
    align_items = (
        ('WORLD', "World", "Align the new object to the world"),
        ('VIEW', "View", "Align the new object to the view"),
        ('CURSOR', "3D Cursor", "Use the 3D cursor orientation for the new object")
    )
    align: EnumProperty(
        name="Align",
        items=align_items,
        default='WORLD',
        update=AddObjectHelper.align_update_callback,
    )
    location: FloatVectorProperty(
        name="Location",
        subtype='TRANSLATION',
    )
    rotation: FloatVectorProperty(
        name="Rotation",
        subtype='EULER',
    )

    def execute(self, context):

        verts_loc, faces = add_box(
            self.subdivisions,
            self.spherize,
        )

        mesh = bpy.data.meshes.new("Octahedron")

        bm = bmesh.new()

        for v_co in verts_loc:
            bm.verts.new(v_co)

        bm.verts.ensure_lookup_table()
        for f_idx in faces:
            bm.faces.new([bm.verts[i] for i in f_idx])

        bm.to_mesh(mesh)
        mesh.update()

        # add the mesh as an object into the scene with this utility module
        from bpy_extras import object_utils
        object_utils.object_data_add(context, mesh, operator=self)

        return {'FINISHED'}


def menu_func(self, context):
    self.layout.operator(AddOctahedron.bl_idname, icon='MESH_CUBE')


def register():
    bpy.utils.register_class(AddOctahedron)
    bpy.types.VIEW3D_MT_mesh_add.append(menu_func)


def unregister():
    bpy.utils.unregister_class(AddOctahedron)
    bpy.types.VIEW3D_MT_mesh_add.remove(menu_func)

register()
