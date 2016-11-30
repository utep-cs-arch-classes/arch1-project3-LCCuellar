#include "shape.h"
#include "_abCircle.h"
#include <math.h>

char pacState;

// true if pixel is in circle centered at centerPos
int abCircleCheck(const AbCircle *circle, const Vec2 *centerPos, const Vec2 *pixel)
{
  u_char radius = circle->radius;
  int axis;
  Vec2 relPos;
  vec2Sub(&relPos, pixel, centerPos); /* vector from center to pixel */
  vec2Abs(&relPos);		      /* project to first quadrant */
  int within = (relPos.axes[0] <= radius && circle->chords[relPos.axes[0]] >= relPos.axes[1]);

  int x = pixel->axes[0] - centerPos->axes[0];
  int y = -(pixel->axes[1] - centerPos->axes[1]);

  
  if((x > 0 && y > 0) && (y < (x/pacState))) {
    within = 0;
  }
  if((x >= 0 && y <= 0) && (-y < (x/pacState))) {
    within = 0;
  }
  
  
  return within;
}
  
void
abCircleGetBounds(const AbCircle *circle, const Vec2 *centerPos, Region *bounds)
{
  u_char axis, radius = circle->radius;
  for (axis = 0; axis < 2; axis ++) {
    bounds->topLeft.axes[axis] = centerPos->axes[axis] - radius;
    bounds->botRight.axes[axis] = centerPos->axes[axis] + radius;
  }
  regionClipScreen(bounds);
}

