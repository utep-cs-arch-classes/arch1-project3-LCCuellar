#include "shape.h"

// compute union of two regions
void 
regionUnion(Region *rUnion, const Region *r1, const Region *r2)
{
  vec2Min(&rUnion->topLeft, &r1->topLeft, &r2->topLeft);
  vec2Max(&rUnion->botRight, &r1->botRight, &r2->botRight);
}

void
regionIntersect(Region *rIntersect, const Region *r1, const Region *r2) {
  vec2Max(&rIntersect->topLeft, &r1->topLeft, &r2->topLeft);
  vec2Min(&rIntersect->botRight, &r1->botRight, &r2->botRight);
}
// Trims extent of region to screen bounds
void regionClipScreen(Region *r)
{
  vec2Max(&r->topLeft, &r->topLeft, &vec2Zero);
  vec2Min(&r->botRight, &r->botRight, &screenSize);
}

