#include "common.glsl"
#include "math.glsl"
#include "uniforms.glsl"
#line 5

struct CSGInterval {
  float near;
  float far;
  vec3 normalNear;
  vec3 normalFar;
};

/// Get data directly from ssbo
void getTriangleData(uint meshIndex, uint triIndex, out vec3 v0, out vec3 v1, out vec3 v2, out vec3 n0, out vec3 n1, out vec3 n2) {
  if (meshIndex == 0u) { // top_dome
    uvec4 tri = oldman.top_dome.indices[triIndex];
    v0 = oldman.top_dome.vertices[tri.x].xyz;
    v1 = oldman.top_dome.vertices[tri.y].xyz;
    v2 = oldman.top_dome.vertices[tri.z].xyz;
    n0 = oldman.top_dome.normals[tri.x].xyz;
    n1 = oldman.top_dome.normals[tri.y].xyz;
    n2 = oldman.top_dome.normals[tri.z].xyz;
  } else if (meshIndex == 1u) { // bottom_body_cut
    uvec4 tri = oldman.bottom_body_cut.indices[triIndex];
    v0 = oldman.bottom_body_cut.vertices[tri.x].xyz;
    v1 = oldman.bottom_body_cut.vertices[tri.y].xyz;
    v2 = oldman.bottom_body_cut.vertices[tri.z].xyz;
    n0 = oldman.bottom_body_cut.normals[tri.x].xyz;
    n1 = oldman.bottom_body_cut.normals[tri.y].xyz;
    n2 = oldman.bottom_body_cut.normals[tri.z].xyz;
  } else if (meshIndex == 2u) { // bottom_dome
    uvec4 tri = oldman.bottom_dome.indices[triIndex];
    v0 = oldman.bottom_dome.vertices[tri.x].xyz;
    v1 = oldman.bottom_dome.vertices[tri.y].xyz;
    v2 = oldman.bottom_dome.vertices[tri.z].xyz;
    n0 = oldman.bottom_dome.normals[tri.x].xyz;
    n1 = oldman.bottom_dome.normals[tri.y].xyz;
    n2 = oldman.bottom_dome.normals[tri.z].xyz;
  } else { // sections
    uvec4 tri = oldman.sections[14 - meshIndex].indices[triIndex];
    v0 = oldman.sections[14 - meshIndex].vertices[tri.x].xyz;
    v1 = oldman.sections[14 - meshIndex].vertices[tri.y].xyz;
    v2 = oldman.sections[14 - meshIndex].vertices[tri.z].xyz;
    n0 = oldman.sections[14 - meshIndex].normals[tri.x].xyz;
    n1 = oldman.sections[14 - meshIndex].normals[tri.y].xyz;
    n2 = oldman.sections[14 - meshIndex].normals[tri.z].xyz;
  }
}

uint getTriangleCount(uint meshIndex) {
  if (meshIndex == 0u) return oldman.top_dome.triangleCount;
  else if (meshIndex == 1u) return oldman.bottom_body_cut.triangleCount;
  else if (meshIndex == 2u) return oldman.bottom_dome.triangleCount;
  else return oldman.sections[14 - meshIndex].triangleCount;
}

CSGInterval calcCSGInterval(vec3 rayOrigin, vec3 rayDir, uint meshIndex) {
  CSGInterval result = CSGInterval(Inf, -Inf, vec3(0.0), vec3(0.0));

  // for each triangle in current mesh
  uint triangleCount = getTriangleCount(meshIndex);
  for (uint i = 0u; i < triangleCount; i++) {
    vec3 v0, v1, v2, n0, n1, n2;
    getTriangleData(meshIndex, i, v0, v1, v2, n0, n1, n2);

    vec3 curResult = intersectTriangle(rayOrigin, rayDir, v0, v1, v2);

    if (curResult.z > 0.0 && curResult.z < Inf) { // valid result
      // calc real geometry normal
      vec3 edge1 = v1 - v0;
      vec3 edge2 = v2 - v0;
      vec3 realNormal = normalize(cross(edge1, edge2));

      // calc normal given by the texture
      vec3 barycentrics = vec3(1.0 - curResult.x - curResult.y, curResult.xy);
      vec3 normal = normalize(mat3(n0, n1, n2) * barycentrics);

      bool isEntry = dot(realNormal, rayDir) < 0.0;
      bool isNearer = curResult.z < result.near;
      bool isFurther = curResult.z > result.far;

      if (isNearer && isEntry) {
        result.near = curResult.z;
        result.normalNear = normal;
      }

      if (isFurther) {
        result.far = curResult.z;
        result.normalFar = normal;
      }
    }
  }

  // if we started inside the mesh
  if (result.near == Inf && result.far > 0.0) {
    result.near = 0.0;
    result.normalNear = -rayDir;
  }

  return result;
}

RaytraceResult renderCSGInterval(
  vec3 rayOrigin,
  vec3 rayDir,
  RaytraceResult result,
  CSGInterval x
) {
  if (x.near < result.distance) {
    result.hitPos = rayOrigin + x.near * rayDir;
    result.normal = x.normalNear;
    result.objectColor = starshipColor;
    result.distance = x.near;
    result.glowing = false;
  }

  return result;
}

/// every point which is in a and b
CSGInterval CSGAnd(CSGInterval a, CSGInterval b) {
  CSGInterval result;
  if (a.near > b.far || b.near > a.far) {
    // No overlap
    result.near = Inf;
    result.far = -Inf;
  } else {
    // overlap

    if (a.near > b.near) {
      result.near = a.near;
      result.normalNear = a.normalNear;
    } else {
      result.near = b.near;
      result.normalNear = b.normalNear;
    }

    if (a.far < b.far) {
      result.far = a.far;
      result.normalFar = a.normalFar;
    } else {
      result.far = b.far;
      result.normalFar = b.normalFar;
    }
  }
  return result;
}

/// every point which is in a or b
CSGInterval CSGOr(CSGInterval a, CSGInterval b) {
  // no overlap -> return nearest
  if (a.near > b.far || b.near > a.far) {
    if (a.near <= b.near) return a;
    else return b;
  }

  CSGInterval result;

  // near
  if (a.near <= b.near) {
    result.near = a.near;
    result.normalNear = a.normalNear;
  } else {
    result.near = b.near;
    result.normalNear = b.normalNear;
  }

  // far
  if (a.far >= b.far) {
    result.far = a.far;
    result.normalFar = a.normalFar;
  } else {
    result.far = b.far;
    result.normalFar = b.normalFar;
  }
  return result;
}

/// every point which is in a but not in b
CSGInterval CSGSub(CSGInterval a, CSGInterval b) {
  if (a.near > b.far || b.near > a.far) {
    // No overlap -> nothing happens to a
    return a;
  }

  CSGInterval result;

  // B completely contains A: A is entirely subtracted
  if (b.near <= a.near && b.far >= a.far) {
    result.near = Inf;
    result.far = -Inf;
    return result;
  }

  // b's normals are inverted because we come from the other side as intended for b
  if (b.near <= a.near) {
    result.near = b.far;
    result.normalNear = -b.normalFar;

    result.far = a.far;
    result.normalFar = a.normalFar;
  } else if (b.far >= a.far) {
    result.near = a.near;
    result.normalNear = a.normalNear;

    result.far = b.near;
    result.normalFar = -b.normalNear;
  } else {
    // b splits a into two parts
    // return the near part

    result.near = a.near;
    result.normalNear = a.normalNear;

    result.far = b.near;
    result.normalFar = -b.normalNear;
  }

  return result;
}
