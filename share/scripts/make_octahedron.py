import sys
from random import random
from math import pi, sqrt, sin, cos, acos

"""
Generate point cloud, that you can then use with blender_octaedron2camera.py
to generate camera positions to render impostor textures.
Use then with GlPointCloudMultiViewImpostors
"""

def make_octahedron(n, output_filename):
    r = 1
    sqrt2 = sqrt(2.0)

    octahedron = []
    for eps in [-1, 1]:
        for i in range(n):
            u = float(i) / (n - 1)
            for j in range(n):
                v = float(j) / (n - 1)
                x = (u + v - 1.0)
                y = (u - v)
                z = eps * (1 - (abs(x) + abs(y)))
                octahedron.append((x, y, z))

    with open(output_filename, 'w') as f:
        for (x, y, z) in octahedron:
            d = 1.0 / sqrt(x*x + y*y + z*z)
            f.write("%f %f %f\n" % (x*d, y*d, z*d))

if __name__ == "__main__":
    n = int(sys.argv[1]) if len(sys.argv) >= 2 else 8
    output_filename = sys.argv[2] if len(sys.argv) >= 3 else "octahedron.xyz"
    make_octahedron(n, output_filename)
