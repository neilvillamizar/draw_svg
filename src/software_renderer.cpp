#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include <complex> // import complex<>

#include "triangulation.h"

using namespace std;

//////////////// Competitive Programming Version ///////////////
// typedef complex<double> pt;
// #define x_ real() // DO NOT USE x_ & y_ AS VARIABLE NAMES!!!
// #define y_ imag()

// // Products
// double dot(pt v, pt w) {return (conj(v)*w).x_;}
// double cross(pt v, pt w) {return (conj(v)*w).y_;}
// // < 0 c is left of ab, > 0 c is right, = 0 colinear
// double orient(pt a, pt b, pt c) {return cross(b-a,c-a);}

//////////////////////////////////////////////////////////////////

namespace CS248 {


// Implements SoftwareRenderer //


// fill a sample location with color
void SoftwareRendererImp::fill_sample(int x, int y, const Color &color) {
  if (x < 0 || x >= target_w) return;
  if (y < 0 || y >= target_h) return;

  Color pixel_color;
  float inv255 = 1.0 / 255.0;
  pixel_color.r = render_target[4 * (x + y * target_w)] * inv255;
  pixel_color.g = render_target[4 * (x + y * target_w) + 1] * inv255;
  pixel_color.b = render_target[4 * (x + y * target_w) + 2] * inv255;
  pixel_color.a = render_target[4 * (x + y * target_w) + 3] * inv255;

  pixel_color = ref->alpha_blending_helper(pixel_color, color);

  render_target[4 * (x + y * target_w)] = (uint8_t)(pixel_color.r * 255);
  render_target[4 * (x + y * target_w) + 1] = (uint8_t)(pixel_color.g * 255);
  render_target[4 * (x + y * target_w) + 2] = (uint8_t)(pixel_color.b * 255);
  render_target[4 * (x + y * target_w) + 3] = (uint8_t)(pixel_color.a * 255);

}

// fill samples in the entire pixel specified by pixel coordinates
void SoftwareRendererImp::fill_pixel(int x, int y, const Color &color) {

  // Task 2: Re-implement this function

  // check bounds
  if (x < 0 || x >= target_w) return;
  if (y < 0 || y >= target_h) return;

  for (int dx = 0; dx < this->sample_rate; dx++) {
    for (int dy = 0; dy < this->sample_rate; dy++) {
      ss_render_target[x][y][dx][dy] =
        ref->alpha_blending_helper(ss_render_target[x][y][dx][dy], color);
    }
  }

}

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  // set top level transformation
  transformation = canvas_to_screen;
  ss_render_target = vector<vector<vector<vector<Color>>>>(target_w,
                     vector<vector<vector<Color>>>(target_h,
                         vector<vector<Color>>(this->sample_rate,
                             vector<Color>(this->sample_rate, {0, 0, 0, 0}))));

  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y--;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y--;
  Vector2D c = transform(Vector2D(    0    , svg.height)); c.x--; c.y++;
  Vector2D d = transform(Vector2D(svg.width, svg.height)); d.x++; d.y++;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();
  ss_render_target.clear();
}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {

  // Task 2:
  // You may want to modify this for supersampling support
  this->sample_rate = sample_rate;

}

void SoftwareRendererImp::set_render_target( unsigned char* render_target,
    size_t width, size_t height ) {

  // Task 2:
  // You may want to modify this for supersampling support
  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;

}

void SoftwareRendererImp::draw_element( SVGElement* element ) {

  // Task 3 (part 1):
  // Modify this to implement the transformation stack

  switch (element->type) {
  case POINT:
    draw_point(static_cast<Point&>(*element));
    break;
  case LINE:
    draw_line(static_cast<Line&>(*element));
    break;
  case POLYLINE:
    draw_polyline(static_cast<Polyline&>(*element));
    break;
  case RECT:
    draw_rect(static_cast<Rect&>(*element));
    break;
  case POLYGON:
    draw_polygon(static_cast<Polygon&>(*element));
    break;
  case ELLIPSE:
    draw_ellipse(static_cast<Ellipse&>(*element));
    break;
  case IMAGE:
    draw_image(static_cast<Image&>(*element));
    break;
  case GROUP:
    draw_group(static_cast<Group&>(*element));
    break;
  default:
    break;
  }

}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) {

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if ( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for ( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i + 0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i + 1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;

  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));

  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if ( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if ( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if ( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for ( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i + 0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i + 1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit

}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color ) {

  // fill in the nearest pixel
  int sx = (int)floor(x);
  int sy = (int)floor(y);

  // check bounds
  if (sx < 0 || sx >= target_w) return;
  if (sy < 0 || sy >= target_h) return;

  // fill sample - NOT doing alpha blending!
  // TODO: Call fill_pixel here to run alpha blending
  fill_pixel(sx, sy, color);
  /*render_target[4 * (sx + sy * target_w)] = (uint8_t)(color.r * 255);
  render_target[4 * (sx + sy * target_w) + 1] = (uint8_t)(color.g * 255);
  render_target[4 * (sx + sy * target_w) + 2] = (uint8_t)(color.b * 255);
  render_target[4 * (sx + sy * target_w) + 3] = (uint8_t)(color.a * 255);*/

}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
    float x1, float y1,
    Color color) {

  // Extra credit (delete the line below and implement your own)
  ref->rasterize_line_helper(x0, y0, x1, y1, target_w, target_h, color, this);

}

// screen point
struct scr_pt {
  double x, y;
};

// < 0 c is left of ab, > 0 c is right, = 0 colinear
double orient(scr_pt a, scr_pt b, scr_pt c) {
  return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
}

bool is_inside(scr_pt a, scr_pt b, scr_pt c, scr_pt sample) {

    double sign_0 = orient(a, b, sample);
    double sign_1 = orient(b, c, sample);
    double sign_2 = orient(c, a, sample);

    bool all_pos = sign_0 >= 0 && sign_1 >= 0 && sign_2 >= 0;
    bool all_neg = sign_0 <= 0 && sign_1 <= 0 && sign_2 <= 0;

    return all_pos || all_neg;

}

double get_horizontal_step(scr_pt a, scr_pt b) {
    return (b.y - a.y);
}

double get_vertical_step(scr_pt a, scr_pt b) {
    return (b.x - a.x);
}

int check_path(int value, int dir, int x, scr_pt a, scr_pt b, scr_pt c) {
    int it = 0;
    scr_pt sample = { x, value };
    while (is_inside(a, b, c, sample))
    {
        it++;
        if (dir > 0) sample.y += it;
        else sample.y -= it;
    }

    return it;
}

void update_boundaries(int& min, int& max, int x, scr_pt a, scr_pt b, scr_pt c){
    // Check if there is any room to grow up for the min
    int to_path = check_path(min, -1, x, a, b, c);

    if(to_path == 0) to_path = check_path(min, 1, x, a, b, c);

    min -= to_path;

    to_path = 0;
    // Same check up for the max
    to_path = check_path(max, 1, x, a, b, c);

    if (to_path == 0) to_path = check_path(min,-1, x, a, b, c);

    max += to_path;
}

int get_pair_y(scr_pt a, scr_pt b, scr_pt c, int to_find)
{
    if (floor(a.x) == to_find) return floor(a.y);
    else if (floor(b.x) == to_find) return floor(b.y);
    return floor(c.y);
}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
    float x1, float y1,
    float x2, float y2,
    Color color ) {
  // Task 1:
  // Implement triangle rasterization (you may want to call fill_sample here)

  //
  if (min({x0, x1, x2}) < 0 || max({x0, x1, x2}) >= target_w) return;
  if (min({y0, y1, y2}) < 0 || max({y0, y1, y2}) >= target_h) return;

  scr_pt a = {x0, y0}, b = {x1, y1}, c = {x2, y2};

  int min_tr_w = floor( min({x0, x1, x2}) );
  int max_tr_w = floor( max({x0, x1, x2}) );
  int min_tr_h = floor( min({y0, y1, y2}) );
  int max_tr_h = floor( max({y0, y1, y2}) );

  /*float jump = 1.0 / this->sample_rate;
  for (int x = min_tr_w; x <= max_tr_w; x++) {
    for (int y = min_tr_h; y <= max_tr_h; y++) {
      for (int dx = 0; dx < this->sample_rate; dx++) {
        for (int dy = 0; dy < this->sample_rate; dy++) {
          float new_x = x + jump * dx + jump / 2;
          float new_y = y + jump * dy + jump / 2;
          scr_pt sample = {new_x, new_y};
          if (is_inside(a, b, c, sample))
            ss_render_target[x][y][dx][dy] = color;
        }
      }
    }
  }*/

  int y_min = get_pair_y(a, b, c, min_tr_w);
  int y_max = y_min;

  for (int x = min_tr_w; x <= max_tr_w; x++) {
      update_boundaries(y_min, y_max, x, a, b, c);
      for (int y = y_min; y <= y_max; y++) {
          scr_pt sample = { x, y };
          if (is_inside(a, b, c, sample))
              fill_pixel(x, y, color);
          else
              fill_pixel(x, y, Color::Black);
      }
  }
}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
    float x1, float y1,
    Texture& tex ) {
  // Task 4:
  // Implement image rasterization (you may want to call fill_sample here)

}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {

  // Task 2:
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 2".
  int sample_cnt = this->sample_rate * this->sample_rate;
  for (int x = 0; x < target_w; x++) {
    for (int y = 0; y < target_h; y++) {
      Color col = {0, 0, 0, 0};
      for (int dx = 0; dx < this->sample_rate; dx++) {
        for (int dy = 0; dy < this->sample_rate; dy++) {
          float r = ss_render_target[x][y][dx][dy].r,
                g = ss_render_target[x][y][dx][dy].g,
                b = ss_render_target[x][y][dx][dy].b,
                a = ss_render_target[x][y][dx][dy].a;
          col.r += r * r;
          col.g += g * g;
          col.b += b * b;
          col.a += a;
        }
      }
      col.r = sqrt(col.r / sample_cnt);
      col.g = sqrt(col.g / sample_cnt);
      col.b = sqrt(col.b / sample_cnt);
      col.a = col.a / sample_cnt;
      fill_sample(x, y, col);
    }
  }
  return;

}

Color SoftwareRendererImp::alpha_blending(Color pixel_color, Color color)
{
  // Task 5
  // Implement alpha compositing
  return pixel_color;
}


} // namespace CS248
