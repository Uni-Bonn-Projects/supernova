#line 2

const vec3 starshipColor = vec3(0.4, 0.4, 0.4);
const vec3 moonColor = vec3(0.96, 0.91, 0.77);
const vec3 sunColor = vec3(0.98, 0.81, 0.32);
const vec3 earthColor = vec3(0.0, 0.0, 0.62);

// TODO: below really used?

const int SKY_ID = 0;
const int STARSHIP_ID = 1;
const int MOON_ID = 3;
const int SUN_ID = 4;
const int EARTH_ID = 5;

/* Returns the color of the object ID at pos */
vec3 colorScene(vec3 pos, int ID) {
  switch (ID) {
    case STARSHIP_ID:
    return starshipColor;
    case MOON_ID:
    return moonColor;
    case SUN_ID:
    return sunColor;
    case EARTH_ID:
    return earthColor;
    default:
    return vec3(1.0, 0.0, 0.0);
  }
}

struct Intersection {
  float depth; // Current depth on the ray
  vec3 pos; // Current position on the ray
  int ID; // ID of the closest object
  bool glowing; // whether or not the closest object "glows"
  int steps; // Number of steps taken
};
