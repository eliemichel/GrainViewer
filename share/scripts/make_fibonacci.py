import sys
from random import random
from math import pi, sqrt, sin, cos, acos

"""
Generate point cloud, that you can then use with blender_import_cameramesh.py
to generate camera positions to render impostor textures.
"""

from numpy import arange, pi, sin, cos, arccos

def make_fibonacci(n, output_filename):
    PHI = (1 + 5**0.5)/2
    i = arange(0, n)
    theta = 2 *pi * i / PHI
    phi = arccos(1 - 2*(i+0.5)/n)
    xs, ys, zs = cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi);

    with open(output_filename, 'w') as f:
        for (x, y, z) in zip(xs, ys, zs):
            f.write("%f %f %f\n" % (x, y, z))

if __name__ == "__main__":
    n = int(sys.argv[1]) if len(sys.argv) >= 2 else 128
    output_filename = sys.argv[2] if len(sys.argv) >= 3 else "fibonacci.xyz"
    make_fibonacci(n, output_filename)
