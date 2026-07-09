#include "sdf.glsl"
#include "math.glsl"
#line 4

uniform vec3 u_oldman_pos;
uniform float u_oldman_active;
uniform float u_oldman_scale;

uniform vec3 u_attacker_pos;
uniform float u_attacker_active;
uniform float u_attacker_scale;

// everything in kilometers
const float oldmanHeight = 100.0;
const float oldmanWidth = 200.0;
const float oldmanBottomWidth = 50.0;
const float oldmanSectionHeight = 10.0;
const float oldmanSectionWidth = 50.0;
const float attackerSize = 2.5;
const float moonSize = 3474.0;
const float earthSize = 12756.0;
const float earthMoonDist = 384400.0;

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

  return SD(result, STARSHIP_ID, false);
}

/* Returns the signed distance to the object nearest to pos */
SD sdScene(vec3 pos) {
  SD result = SD(Inf, SKY_ID, true);

  if (u_oldman_active > 0.5) {
    vec3 scaledPos = pos / u_oldman_scale;
    /* constructOldMan wants rayPos + spawnPos so we use -u_oldman_pos to arrive at +u_oldman_pos */
    SD oldmanSD = constructOldMan(scaledPos, -u_oldman_pos / u_oldman_scale);
    oldmanSD.dist *= u_oldman_scale;

    result = sdUnion(result, oldmanSD);
  }

  if (u_attacker_active > 0.5) {
    vec3 attacker_distance = vec3(oldmanWidth * 4, 0.0, 0.0);
    vec3 attacker_origin = pos - u_attacker_pos;

    for (int i = 0; i < 13; i++) {
      vec3 attacker_vec = rotateZ(attacker_distance, degToRad((i % 5) * (360 / 5)));
      attacker_vec = rotateY(attacker_vec, degToRad(i * (360 / 10)));

      float attacker = sdSphere(attacker_origin + attacker_vec, attackerSize * u_attacker_scale);
      result = sdUnion(result, SD(attacker, STARSHIP_ID, false));
    }
  }

  // moon
  vec3 moonPos = pos + xInDir(10000, vec3(1, 2, 1));
  result = sdUnion(result, SD(
        sdSphere(moonPos, moonSize),
        MOON_ID,
        false
      ));

  // earth
  vec3 earthPos = moonPos + xInDir(earthMoonDist, vec3(-0.2, -0.3, -1));
  result = sdUnion(result, SD(
        sdSphere(earthPos, earthSize),
        EARTH_ID,
        false
      ));

  return result;
}
