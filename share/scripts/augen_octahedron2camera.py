import sys
import struct
from math import sqrt

def cross(a, b):
    return [
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]
    ]

def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]

def normalized(a):
    s = 1 / sqrt(dot(a, a))
    return [ a[0] * s, a[1] * s, a[2] * s ]

def mul(m, a):
    return [
        dot(m[0], a),
        dot(m[1], a),
        dot(m[2], a)
    ]

def opp(a):
    return [-a[0], -a[1], -a[2]]

def lookFrom(p):
    z = p
    x = normalized(cross([0,0,1], z))
    y = normalized(cross(z, x))
    invp = opp(mul([x, y, z], p))
    return [
        [x[0], x[1], x[2], invp[0]],
        [y[0], y[1], y[2], invp[1]],
        [z[0], z[1], z[2], invp[2]],
        [0, 0, 0, 1],
    ]

def write_view_matrix(inputFilename, outputFilepath):
    with open(outputFilepath, 'wb') as outFile:
        for i, line in enumerate(open(inputFilename, 'r')):
            coords = [float(x) for x in line.split()]
            if len(coords) != 3:
                print("Unable to parse line: %s " % line)
                exit(1)

            mat = lookFrom(coords)
            print(mat)
            column_major_data = tuple(mat[i][j] for j in range(4) for i in range(4))
            outFile.write(struct.pack("f"*16, *column_major_data))

if __name__ == "__main__":
    inputFilename = sys.argv[1] if len(sys.argv) > 1 else "octahedron.xyz"
    outputFilepath = sys.argv[2] if len(sys.argv) > 2 else "octahedron_camera.bin"
    write_view_matrix(inputFilename, outputFilepath)
