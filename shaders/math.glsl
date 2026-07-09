#line 2

const float Inf = 1.0 / 0.0;
const float NaN = 0.0 / 0.0;
const float PI = 3.1415926;
const float GOLDEN_ANGLE = PI * (3.0 - sqrt(5.0));

float degToRad(float deg) {
  return deg * (PI / 180.0);
}

/** Source: https://www.shadertoy.com/view/4djSRW?__cf_chl_f_tk=Jg2kxfvvaxD29iocH0qccA3TMetlbt34ZkU5OaAc46s-1783197884-1.0.1.1-YrSkhWk43eoKzdIxOTooWQUNfjt4IqXsC1i6Qil6muU */
float hash(vec3 p3) {
  p3 = fract(p3 * 0.1031);
  p3 += dot(p3, p3.zyx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

/** Returns a vector of length x pointing in dir */
vec3 xInDir(float x, vec3 dir) {
  return x * normalize(dir);
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
