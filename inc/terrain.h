#ifndef TERRAIN_H
#define TERRAIN_H

#include "hex.h"
#include "platform_includes.h"
#include "renderer.h"

constexpr static float kGridSize = 5.0f;

class Terrain : public Renderable {
 public:
  Terrain();

  void Render(RenderContext* context) override;

  void ToggleRenderGround() { render_ground_ ^= 1; }

  glm::vec2 SnapToGrid(const glm::vec2& coords) {
    return Hex::SnapToGrid(coords, kGridSize);
  }

  Hex CoordsToHex(const glm::vec2& coords) {
    return Hex::CartesianToHex(coords, kGridSize);
  }

  virtual ~Terrain() {}

 private:
  ShaderProgram* shader_;
  bool initialized_;
  GLuint vao_id_;
  std::size_t num_indices_;
  GLuint edges_vao_id_;
  std::size_t edges_num_indices_;

  bool render_ground_;
  std::size_t terrain_selection_;
};

#endif // TERRAIN_H
