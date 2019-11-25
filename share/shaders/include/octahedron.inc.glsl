//////////////////////////////////////////////////////
// Octahedric projection/unprojection functions

/**
 * Return the index of the closest octaedral map cell corresponding to the given direction
 */
int directionToViewIndex(vec3 direction_gs, int nb = 8) {
    float l1 = abs(direction_gs.x) + abs(direction_gs.y) + abs(direction_gs.z);
    vec3 l1UnitDirection = direction_gs / l1;
    float x = l1UnitDirection.x;
    float y = l1UnitDirection.y;
    float u = (x + y + 1) * 0.5;
    float v = (x - y + 1) * 0.5;
    int layer = int(round(u * (nb - 1))) * nb + int(round(v * (nb - 1)));
    if (l1UnitDirection.z > 0) {
        layer += nb*nb;
    }
    return layer;
}

/**
 * Returns indices of the fourth directions enclosing the target direction in the octahedron cell
 */
void directionToViewIndexAll(in vec3 direction_gs, out int idx1, out int idx2, out int idx3, out int idx4, out float du, out float dv, in int nb = 8) {
    float l1 = abs(direction_gs.x) + abs(direction_gs.y) + abs(direction_gs.z);
    vec3 l1UnitDirection = direction_gs / l1;
    float x = l1UnitDirection.x;
    float y = l1UnitDirection.y;
    float u = (x + y + 1) * 0.5;
    float v = (x - y + 1) * 0.5;
    idx1 = int(floor(u * (nb - 1))) * nb + int(floor(v * (nb - 1)));
    idx2 = int(ceil(u * (nb - 1))) * nb + int(floor(v * (nb - 1)));
    idx3 = int(floor(u * (nb - 1))) * nb + int(ceil(v * (nb - 1)));
    idx4 = int(ceil(u * (nb - 1))) * nb + int(ceil(v * (nb - 1)));
    if (l1UnitDirection.z > 0) {
        idx1 += nb*nb;
        idx2 += nb*nb;
        idx3 += nb*nb;
        idx4 += nb*nb;
    }
    du = fract(u * (nb - 1));
    dv = fract(v * (nb - 1));
}

vec3 viewIndexToDirection(int viewIndex, int nb = 8) {
    float eps = -1.0;
    if (viewIndex >= nb * nb) {
        eps = 1.0;
        viewIndex -= nb * nb;
    }
    int i = viewIndex / nb;
    int j = viewIndex % nb;
    float u = float(i) / (nb - 1);
    float v = float(j) / (nb - 1);
    float x = (u + v - 1.0);
    float y = (u - v);
    float z = eps * (1.0 - (abs(x) + abs(y)));
    return normalize(vec3(x, y, z));
}
