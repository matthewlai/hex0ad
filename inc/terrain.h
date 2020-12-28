#ifndef TERRAIN_H
#define TERRAIN_H

#include "hex.h"
#include "platform_includes.h"
#include "renderer.h"

class Terrain : public Renderable {
 public:
  Terrain();

  void Render(RenderContext* context) override;

  void ToggleRenderGround() { render_ground_ ^= 1; }

  virtual ~Terrain() {}

 private:
  ShaderProgram* shader_;
  bool initialized_;
  GLuint vertices_vbo_id_;
  GLuint indices_vbo_id_;
  std::size_t num_indices_;
  GLuint edges_vertices_vbo_id_;
  GLuint edges_indices_vbo_id_;
  std::size_t edges_num_indices_;
  GLint mvp_loc_;
  GLint is_edge_loc_;

  bool render_ground_;
};

#endif // TERRAIN_H
