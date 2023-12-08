#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CMU462
{

  // Implements SoftwareRenderer //

  void SoftwareRendererImp::draw_svg(SVG &svg)
  {

    // set top level transformation
    transformation = svg_2_screen;

    // draw all elements
    for (size_t i = 0; i < svg.elements.size(); ++i)
    {
      draw_element(svg.elements[i]);
    }

    // draw canvas outline
    Vector2D a = transform(Vector2D(0, 0));
    a.x--;
    a.y--;
    Vector2D b = transform(Vector2D(svg.width, 0));
    b.x++;
    b.y--;
    Vector2D c = transform(Vector2D(0, svg.height));
    c.x--;
    c.y++;
    Vector2D d = transform(Vector2D(svg.width, svg.height));
    d.x++;
    d.y++;
    //printf("%f %f\n", svg.width, svg.height);
    rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
    rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
    rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
    rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

    // resolve and send to render target
    resolve();
  }

  void SoftwareRendererImp::set_sample_rate(size_t sample_rate)
  {

    // Task 4:
    // You may want to modify this for supersampling support
    this->sample_rate = sample_rate;
  }

  void SoftwareRendererImp::set_render_target(unsigned char *render_target,
                                              size_t width, size_t height)
  {

    // Task 4:
    // You may want to modify this for supersampling support
    this->render_target = render_target;
    this->target_w = width;
    this->target_h = height;
  }

  void SoftwareRendererImp::draw_element(SVGElement *element)
  {

    // Task 5 (part 1):
    // Modify this to implement the transformation stack

    switch (element->type)
    {
    case POINT:
      draw_point(static_cast<Point &>(*element));
      break;
    case LINE:
      draw_line(static_cast<Line &>(*element));
      break;
    case POLYLINE:
      draw_polyline(static_cast<Polyline &>(*element));
      break;
    case RECT:
      draw_rect(static_cast<Rect &>(*element));
      break;
    case POLYGON:
      draw_polygon(static_cast<Polygon &>(*element));
      break;
    case ELLIPSE:
      draw_ellipse(static_cast<Ellipse &>(*element));
      break;
    case IMAGE:
      draw_image(static_cast<Image &>(*element));
      break;
    case GROUP:
      draw_group(static_cast<Group &>(*element));
      break;
    default:
      break;
    }
  }

  // Primitive Drawing //

  void SoftwareRendererImp::draw_point(Point &point)
  {

    Vector2D p = transform(point.position);
    rasterize_point(p.x, p.y, point.style.fillColor);
  }

  void SoftwareRendererImp::draw_line(Line &line)
  {

    Vector2D p0 = transform(line.from);
    Vector2D p1 = transform(line.to);
    rasterize_line(p0.x, p0.y, p1.x, p1.y, line.style.strokeColor);
  }

  void SoftwareRendererImp::draw_polyline(Polyline &polyline)
  {

    Color c = polyline.style.strokeColor;

    if (c.a != 0)
    {
      int nPoints = polyline.points.size();
      for (int i = 0; i < nPoints - 1; i++)
      {
        Vector2D p0 = transform(polyline.points[(i + 0) % nPoints]);
        Vector2D p1 = transform(polyline.points[(i + 1) % nPoints]);
        rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
      }
    }
  }

  void SoftwareRendererImp::draw_rect(Rect &rect)
  {

    Color c;

    // draw as two triangles
    float x = rect.position.x;
    float y = rect.position.y;
    float w = rect.dimension.x;
    float h = rect.dimension.y;

    Vector2D p0 = transform(Vector2D(x, y));
    Vector2D p1 = transform(Vector2D(x + w, y));
    Vector2D p2 = transform(Vector2D(x, y + h));
    Vector2D p3 = transform(Vector2D(x + w, y + h));

    // draw fill
    c = rect.style.fillColor;
    if (c.a != 0)
    {
      rasterize_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c);
      rasterize_triangle(p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c);
    }

    // draw outline
    c = rect.style.strokeColor;
    if (c.a != 0)
    {
      rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
      rasterize_line(p1.x, p1.y, p3.x, p3.y, c);
      rasterize_line(p3.x, p3.y, p2.x, p2.y, c);
      rasterize_line(p2.x, p2.y, p0.x, p0.y, c);
    }
  }

  void SoftwareRendererImp::draw_polygon(Polygon &polygon)
  {

    Color c;

    // draw fill
    c = polygon.style.fillColor;
    if (c.a != 0)
    {

      // triangulate
      vector<Vector2D> triangles;
      triangulate(polygon, triangles);

      // draw as triangles
      for (size_t i = 0; i < triangles.size(); i += 3)
      {
        Vector2D p0 = transform(triangles[i + 0]);
        Vector2D p1 = transform(triangles[i + 1]);
        Vector2D p2 = transform(triangles[i + 2]);
        rasterize_triangle(p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c);
      }
    }

    // draw outline
    c = polygon.style.strokeColor;
    if (c.a != 0)
    {
      int nPoints = polygon.points.size();
      for (int i = 0; i < nPoints; i++)
      {
        Vector2D p0 = transform(polygon.points[(i + 0) % nPoints]);
        Vector2D p1 = transform(polygon.points[(i + 1) % nPoints]);
        rasterize_line(p0.x, p0.y, p1.x, p1.y, c);
      }
    }
  }

  void SoftwareRendererImp::draw_ellipse(Ellipse &ellipse)
  {

    // Extra credit
  }

  void SoftwareRendererImp::draw_image(Image &image)
  {

    Vector2D p0 = transform(image.position);
    Vector2D p1 = transform(image.position + image.dimension);

    rasterize_image(p0.x, p0.y, p1.x, p1.y, image.tex);
  }

  void SoftwareRendererImp::draw_group(Group &group)
  {

    for (size_t i = 0; i < group.elements.size(); ++i)
    {
      draw_element(group.elements[i]);
    }
  }

  // Rasterization //

  // The input arguments in the rasterization functions
  // below are all defined in screen space coordinates

  void SoftwareRendererImp::rasterize_point(float x, float y, Color color)
  {

    // fill in the nearest pixel
    int sx = (int)floor(x);
    int sy = (int)floor(y);

    // check bounds
    if (sx < 0 || sx >= target_w)
      return;
    if (sy < 0 || sy >= target_h)
      return;

    // fill sample - NOT doing alpha blending!
    render_target[4 * (sx + sy * target_w)] = (uint8_t)(color.r * 255);
    render_target[4 * (sx + sy * target_w) + 1] = (uint8_t)(color.g * 255);
    render_target[4 * (sx + sy * target_w) + 2] = (uint8_t)(color.b * 255);
    render_target[4 * (sx + sy * target_w) + 3] = (uint8_t)(color.a * 255);
  }

  void SoftwareRendererImp::rasterize_line(float x0, float y0,
                                           float x1, float y1,
                                           Color color)
  {

    // Task 2:
    // Implement line rasterization
    // 自己的实现
    float srcx, srcy;
    float desx, desy;
    if (x0 <= x1)
    {
      srcx = x0;
      srcy = y0;
      desx = x1;
      desy = y1;
    }
    else
    {
      srcx = x1;
      srcy = y1;
      desx = x0;
      desy = y0;
    }
    float deltax = desx - srcx < 1.0 ? 1.0 : desx - srcx;
    float slope = (desy - srcy) / (deltax); // 斜率，x每+1，y增加的数值

    float itv = 1.0;
    float factor = abs(slope) > 1.0 ? (abs(slope) > 1000 ? 1000 : abs(slope)) : 1.0; // 防止增加过快
    slope /= factor;
    itv /= factor;
    for (float xi = srcx, yi = srcy; xi <= desx || abs(yi - desy) > 1;)
    {
      rasterize_point(xi, yi, color);
      if (xi <= desx)
      { // x到终点后，还得等y到终点
        xi += itv;
      }
      yi += slope;
    }

    /*  // Bresenham's Line Drawing Algorithm
        int dx = x1 - x0;
        int dy = y1 - y0;
        int eps = 0;
        //步长
        int sx = dx > 0 ? 1 : -1;
        int sy = dy > 0 ? 1 : -1;
        dx = std::abs(dx);
        dy = std::abs(dy);
        float decision = 2 * dy - dx;//表示x固定走一格的情况下，y的增量
        float x = x0;
        float y = y0;
        for (int i = 0; i <= dx; ++i) {
            // Rasterize the current point
            rasterize_point(x, y, color);

            // Update coordinates based on decision variable
            if (decision > 0) {//y的增量大于0.5，所以向上走一格
                y += sy;
                decision -= 2 * dx;
            }
            decision += 2 * dy;

            x += sx;//x每次固定走一格
        }

     */
  }

  void SoftwareRendererImp::rasterize_triangle(float x0, float y0,
                                               float x1, float y1,
                                               float x2, float y2,
                                               Color color)
  {
    // Task 3:
    // Implement triangle rasterization
    //先找到三角形的bbox
    float x_lt = min(min(x0, x1), x2);
    float x_rd = max(max(x0, x1), x2);
    float y_lt = min(min(y0, y1), y2);
    float y_rd = max(max(y0, y1), y2);
    Vector3D AB(x1-x0, y1-y0, 0);
    Vector3D BC(x2-x1, y2-y1, 0);
    Vector3D CA(x0-x2, y0-y2, 0);
    for (float x = x_lt; x <= ceil(x_rd); x+=1) {
      for (float y = y_lt; y <= ceil(y_rd); y+=1) {
        Vector3D PA(x0-x,y0-y, 0);
        Vector3D PB(x1-x,y1-y, 0);
        Vector3D PC(x2-x,y2-y, 0);
        //只要计算叉积的z分量即可
        float z1 = PA.x*AB.y - PA.y*AB.x;
        float z2 = PB.x*BC.y - PB.y*BC.x;
        float z3 = PC.x*CA.y - PC.y*CA.x;
        if (z1>0&&z2>0&&z3>0 || z1<0&&z2<0&&z3<0) {
          rasterize_point(x, y, color);
        }
      }
    }
  }

  void SoftwareRendererImp::rasterize_image(float x0, float y0,
                                            float x1, float y1,
                                            Texture &tex)
  {
    // Task 6:
    // Implement image rasterization
  }

  // resolve samples to render target
  void SoftwareRendererImp::resolve(void)
  {

    // Task 4:
    // Implement supersampling
    // You may also need to modify other functions marked with "Task 4".
    return;
  }

} // namespace CMU462
