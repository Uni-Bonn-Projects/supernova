struct CSGInterval {
  float near;
  float far;
  vec3 normalNear;
  vec3 normalFar;
};

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
