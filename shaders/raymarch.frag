#version 330 core

in vec3 viewDir;
out vec3 fragColor;

uniform vec3 uCameraPosition;
uniform float uFocalLength;
uniform float uTime;

uniform vec3 uLightDir;
uniform float uNear;
uniform float uFar;
uniform int uSteps;
uniform float uEpsilon;
uniform float uNormalEps;
uniform mat4 uCameraMatrix;
uniform float uAspectRatio;
uniform vec2 uResolution = vec2(800.0, 600.0);
uniform float uWarp = 0.75; // simulate curvature of CRT monitor
uniform float uScan = 0.75; // simulate darkness between scanlines

const vec3 uLightColor = vec3(1.0);

const float Inf = 1.0 / 0.0;
const float NaN = 0.0 / 0.0;
const float PI = 3.1415926;

// everything in kilometers
const float oldmanHeight = 100.0;
const float oldmanWidth = 200.0;
const float oldmanBottomWidth = 50.0;
const float oldmanSectionHeight = 10.0;
const float oldmanSectionWidth = 50.0;
const float attackerSize = 2.5;

// colors
const vec3 starshipColor = vec3(0.4, 0.4, 0.4);

const int SKY_ID = 0;
const int STARSHIP_ID = 1;

// begin: structs

/* Struct to hold the signed distance and ID of the nearest object */
struct SD {
  float dist;
  int ID;
};

struct Intersection {
  float depth; // Current depth on the ray
  vec3 pos; // Current position on the ray
  int ID; // ID of the closest object
  int steps; // Number of steps taken
};

// end: structs

SD sdUnion(SD a, SD b) {
  return a.dist < b.dist ? a : b;
}

float degToRad(float deg) {
  return deg * (PI / 180.0);
}

vec3 rotateZ(vec3 pos, float angle) {
  float c = cos(angle);
  float s = sin(angle);

  mat3 m = mat3(
      c, s, 0.0,
      -s, c, 0.0,
      0.0, 0.0, 1.0
    );
  return m * pos;
}

vec3 rotateY(vec3 pos, float angle) {
  float c = cos(angle);
  float s = sin(angle);

  mat3 m = mat3(
      c, 0.0, s,
      0.0, 1.0, 0.0,
      -s, 0.0, c
    );
  return m * pos;
}

// begin: boolean operators

float intersectSDF(float distA, float distB) {
  return max(distA, distB);
}

float unionSDF(float distA, float distB) {
  return min(distA, distB);
}

float differenceSDF(float distA, float distB) {
  return max(distA, -distB);
}

// end: boolean operators

// begin: objects

float sdSphere(vec3 pos, float radius) {
  return length(pos) - radius;
}

float sdBox(vec3 pos, vec3 size) {
  vec3 q = abs(pos) - size;
  return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

// end: objects

// begin: scene construction

float constructOldManSection(vec3 rayPos, vec3 spawnPos) {
  vec3 localPos = rayPos + spawnPos;

  float sp = 6.2831853 / 12.0; // 12 sections = 30 degrees angle diff
  float an = atan(localPos.z, localPos.x);
  float id = floor(an / sp);

  float a1 = sp * id;
  float a2 = sp * (id + 1.0);
  vec2 r1 = mat2(cos(a1), -sin(a1), sin(a1), cos(a1)) * localPos.xz;
  vec2 r2 = mat2(cos(a2), -sin(a2), sin(a2), cos(a2)) * localPos.xz;

  float x_add = -oldmanWidth - oldmanSectionWidth * 0.8;
  float y = localPos.y - oldmanSectionHeight;
  vec3 size = vec3(oldmanSectionWidth, oldmanSectionHeight, oldmanSectionWidth);
  float d1 = sdBox(vec3(r1.x + x_add, y, r1.y), size);
  float d2 = sdBox(vec3(r2.x + x_add, y, r2.y), size);

  return min(d1, d2);
}

SD constructOldMan(vec3 rayPos, vec3 spawnPos) {
  float main_body = sdSphere(rayPos + spawnPos, oldmanWidth);
  main_body = differenceSDF(
      main_body,
      sdBox(
        rayPos + spawnPos + vec3(0.0, oldmanHeight, 0.0),
        vec3(oldmanWidth, oldmanHeight, oldmanWidth)
      )
    );
  float bottom_body = sdSphere(rayPos + spawnPos, oldmanBottomWidth);

  float result = unionSDF(main_body, bottom_body);
  result = unionSDF(result, constructOldManSection(rayPos, spawnPos));

  return SD(result, STARSHIP_ID);
}

/* Returns the signed distance to the object nearest to pos */
SD sdScene(vec3 pos) {
  SD result = SD(Inf, SKY_ID);

  vec3 fight_pos = vec3(600.0, 600.0, 600.0);

  result = sdUnion(result, constructOldMan(pos, fight_pos));

  vec3 attacker_distance = vec3(oldmanWidth * 4, 0.0, 0.0);
  vec3 attacker_origin = pos + fight_pos;
  for (int i = 0; i < 100; i++) {
    vec3 attacker_vec = rotateZ(attacker_distance, degToRad((i % 5) * (360 / 5)));
    attacker_vec = rotateY(attacker_vec, degToRad(i * (360 / 10)));

    float attacker = sdSphere(attacker_origin + attacker_vec, attackerSize);
    result = sdUnion(result, SD(attacker, STARSHIP_ID));
  }

  return result;
}

// end: scene construction

// begin: render

vec3 proceduralSky(vec3 rayDir) {
  // TODO : render stars
  return vec3(0.0);
}

vec3 proceduralSun(vec3 rayDir) {
  return pow(max(0.0, dot(rayDir, uLightDir)), 1000) * uLightColor;
}

float checkerPattern(vec3 pos) {
  vec3 p = floor(pos);
  return mod(p.x + p.y + p.z, 2.0);
}

/* Returns the color of the object ID at pos */
vec3 colorScene(vec3 pos, int ID) {
  switch (ID) {
    case STARSHIP_ID:
    return starshipColor;
    default:
    return vec3(1.0, 0.0, 0.0);
  }
}

/* Returns the normalized gradient of the signed distance field of our scene */
vec3 normalScene(vec3 pos) {
  vec2 h = vec2(uNormalEps, 0);
  return normalize(vec3(
      sdScene(pos + h.xyy).dist - sdScene(pos - h.xyy).dist,
      sdScene(pos + h.yxy).dist - sdScene(pos - h.yxy).dist,
      sdScene(pos + h.yyx).dist - sdScene(pos - h.yyx).dist
    ));
}

Intersection raymarchScene(vec3 rayOrigin, vec3 rayDir, float near, float far) {
  Intersection intsec = Intersection(
      0,
      rayOrigin,
      SKY_ID,
      0
    );

  float next_depth = near;

  while (
    next_depth > uEpsilon
      && intsec.depth < far
      && intsec.steps < uSteps
  ) {
    intsec.depth += next_depth;
    intsec.pos += rayDir * next_depth;

    SD sd = sdScene(intsec.pos);
    next_depth = sd.dist;
    intsec.ID = sd.ID;

    intsec.steps += 1;
  }

  return intsec;
}

void main() {
  // squared distance from center
  vec2 uv = gl_FragCoord.xy / uResolution;
  vec2 dc = abs(0.5 - uv);
  dc *= dc;
  // warp the fragment coordinates
  uv.x -= 0.5; uv.x *= 1.0 + (dc.y * (0.3 * uWarp)); uv.x += 0.5;
  uv.y -= 0.5; uv.y *= 1.0 + (dc.x * (0.4 * uWarp)); uv.y += 0.5;
  // sample outside boundaries set to black
  if (uv.y > 1.0 || uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0) {
    fragColor = vec3(0.0);
    return;
  }
  // warp uv from (0.0, 1.0) to (-1.0, 1.0)
  vec2 ndc = uv * 2.0 - 1.0;

  vec3 rayDir = normalize(viewDir); // Renormalize after interpolation
  vec3 rayOrigin = uCameraPosition;

  Intersection isec = raymarchScene(rayOrigin, rayDir, uNear, uFar);

  vec3 color;
  if (isec.depth >= uFar) {
    // No hit, render a procedural sky background
    color = proceduralSky(rayDir) + proceduralSun(rayDir);
  } else {
    // We hit something
    // The normal is the normalized gradient of the signed distance field
    vec3 normal = normalScene(isec.pos);
    // Lambert lighting term
    vec3 lighting = max(dot(normal, uLightDir), 0.0) * uLightColor;
    // Test if something lies between the hit point and the light source
    float shadow = raymarchScene(isec.pos, uLightDir, uNear, uFar).depth >= uFar ? 1.0 : 0.0;
    // Get the color of the hit point
    vec3 albedo = colorScene(isec.pos, isec.ID);
    // Calculate lighting
    color = (shadow * lighting) * albedo;
  }
  // determine if we are drawing in a scanline
  float apply = abs(sin(gl_FragCoord.y) * 0.5 * uScan);
  // sample the texture
  fragColor = mix(color, vec3(0.0), apply);
}

// end: render
