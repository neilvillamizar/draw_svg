#include "viewport.h"

#include "CS248.h"

namespace CS248 {

void ViewportImp::set_viewbox( float x, float y, float span ) {

  // Task 3 (part 2): 
  // Set svg to normalized device coordinate transformation. Your input
  // arguments are defined as SVG canvans coordinates.

  this->x = x;
  this->y = y;
  this->span = span;

  double bx = x - span, by = y - span,
         Sx = 1.0 / (2.0 * span), Sy = Sx;

  Matrix3x3 canvas_to_norm = Matrix3x3::identity();

  canvas_to_norm(0, 0) = Sx; canvas_to_norm(0, 2) = Sx * (-bx);
  canvas_to_norm(1, 1) = Sy; canvas_to_norm(1, 2) = Sy * (-by);

  set_canvas_to_norm( canvas_to_norm );

}

void ViewportImp::update_viewbox( float dx, float dy, float scale ) { 
  
  this->x -= dx;
  this->y -= dy;
  this->span *= scale;
  set_viewbox( x, y, span );
}

} // namespace CS248
