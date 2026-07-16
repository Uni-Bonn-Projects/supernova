#include "common.glsl"
#include "math.glsl"
#line 5

struct CSGInterval {
  float near;
  float far;
  vec3 normalNear;
  vec3 normalFar;
};

CSGInterval calcCSGInterval(
  vec3 rayOrigin,
  vec3 rayDir,
  SNMesh mesh
) {
  CSGInterval result = CSGInterval(
      Inf,
      -1.0,
      vec3(0.0),
      vec3(0.0)
    );

  // for each triangle in current mesh
  for (uint i = 0u; i < mesh.triangleCount; i++) {
    // Fetch vertex positions
    vec3 v0 = mesh.vertices[mesh.indices[i].x].xyz;
    vec3 v1 = mesh.vertices[mesh.indices[i].y].xyz;
    vec3 v2 = mesh.vertices[mesh.indices[i].z].xyz;

    vec3 curResult = intersectTriangle(rayOrigin, rayDir, v0, v1, v2);

    bool isNearer = curResult.z < result.near;
    bool isFurther = curResult.z > result.far;
    if (isNearer || isFurther) {
      vec3 barycentrics = vec3(1.0 - curResult.x - curResult.y, curResult.xy);

      // Fetch vertex normals
      vec3 n0 = mesh.normals[mesh.indices[i].x].xyz;
      vec3 n1 = mesh.normals[mesh.indices[i].y].xyz;
      vec3 n2 = mesh.normals[mesh.indices[i].z].xyz;

      vec3 normal = normalize(mat3(n0, n1, n2) * barycentrics);

      if (isNearer) {
        result.near = curResult.z;
        result.normalNear = normal;
      }
      if (isFurther) {
        result.far = curResult.z;
        result.normalFar = normal;
      }
    }
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
  CSGInterval result;
  if (a.near > b.far || b.near > a.far) {
    // No overlap
    result.near = Inf;
    result.far = -Inf;
  } else {
    // overlap

    if (a.near <= b.near) {
      result.near = a.near;
      result.normalNear = a.normalNear;
    } else {
      result.near = b.near;
      result.normalNear = b.normalNear;
    }

    if (a.far >= b.far) {
      result.far = a.far;
      result.normalFar = a.normalFar;
    } else {
      result.far = b.far;
      result.normalFar = b.normalFar;
    }
  }
  return result;
}
