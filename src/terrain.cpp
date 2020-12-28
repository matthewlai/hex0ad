#include "terrain.h"

#include "logger.h"
#include "renderer.h"
#include "shaders.h"
#include "texture_manager.h"
#include "utils.h"

namespace {
// 0AD "tiles" are 2m x 2m https://trac.wildfiregames.com/wiki/ArtScaleAndProportions
// Which means terrain textures should repeat at 22x22m.
// Models are supposed to be 2m = 1 unit.
constexpr static float kGridSize = 5.0f;
constexpr static float kHexRingOffset = kGridSize * 0.05f;
constexpr static int kMapSize = 15;

void DrawHex(const Hex& hex, std::vector<float>* positions, std::vector<GLuint>* indices) {
  GLuint base_index = positions->size() / 3;

  hex.ForEachVertex(kGridSize, kGridSize - kHexRingOffset, [&](const glm::vec2& vertex, int index) {
    (void) index;
    positions->push_back(vertex.x);
    positions->push_back(vertex.y);
    positions->push_back(0.0f);
  });

  for (int i = 0; i < 4; ++i) {
    indices->push_back(base_index);
    indices->push_back(base_index + i + 1);
    indices->push_back(base_index + i + 2);
  }
}

// Draw a ring around the hex.
void DrawHexRing(const Hex& hex, std::vector<float>* positions, std::vector<GLuint>* indices, bool is_outside) {
  GLuint base_index = positions->size() / 3;

  // As an optimization we only draw half the ring for inner hexs (there is still some extra drawing).
  hex.ForEachVertex(kGridSize, kGridSize + kHexRingOffset, [&](const glm::vec2& vertex, int index) {
    if (is_outside || index < 4) {
      positions->push_back(vertex.x);
      positions->push_back(vertex.y);
      positions->push_back(0.0f);
    }
  });

  hex.ForEachVertex(kGridSize, kGridSize - kHexRingOffset, [&](const glm::vec2& vertex, int index) {
    if (is_outside || index < 4) {
      positions->push_back(vertex.x);
      positions->push_back(vertex.y);
      positions->push_back(0.0f);
    }
  });

  // 12 triangles for outer ring, 6 for inside.
  if (is_outside) {
    indices->push_back(base_index + 0);
    indices->push_back(base_index + 1);
    indices->push_back(base_index + 7);
    indices->push_back(base_index + 0);
    indices->push_back(base_index + 7);
    indices->push_back(base_index + 6);
    indices->push_back(base_index + 1);
    indices->push_back(base_index + 2);
    indices->push_back(base_index + 8);
    indices->push_back(base_index + 8);
    indices->push_back(base_index + 7);
    indices->push_back(base_index + 1);
    indices->push_back(base_index + 8);
    indices->push_back(base_index + 2);
    indices->push_back(base_index + 9);
    indices->push_back(base_index + 2);
    indices->push_back(base_index + 3);
    indices->push_back(base_index + 9);

    indices->push_back(base_index + 3);
    indices->push_back(base_index + 10);
    indices->push_back(base_index + 9);
    indices->push_back(base_index + 3);
    indices->push_back(base_index + 4);
    indices->push_back(base_index + 10);
    indices->push_back(base_index + 10);
    indices->push_back(base_index + 4);
    indices->push_back(base_index + 11);
    indices->push_back(base_index + 4);
    indices->push_back(base_index + 5);
    indices->push_back(base_index + 11);
    indices->push_back(base_index + 11);
    indices->push_back(base_index + 5);
    indices->push_back(base_index + 6);
    indices->push_back(base_index + 5);
    indices->push_back(base_index + 0);
    indices->push_back(base_index + 6);
  } else {
    indices->push_back(base_index + 0);
    indices->push_back(base_index + 1);
    indices->push_back(base_index + 5);
    indices->push_back(base_index + 0);
    indices->push_back(base_index + 5);
    indices->push_back(base_index + 4);
    indices->push_back(base_index + 1);
    indices->push_back(base_index + 2);
    indices->push_back(base_index + 6);
    indices->push_back(base_index + 6);
    indices->push_back(base_index + 5);
    indices->push_back(base_index + 1);
    indices->push_back(base_index + 6);
    indices->push_back(base_index + 2);
    indices->push_back(base_index + 7);
    indices->push_back(base_index + 2);
    indices->push_back(base_index + 3);
    indices->push_back(base_index + 7);
  }
}

}

Terrain::Terrain() : 
    shader_(nullptr),
    initialized_(false),
    vertices_vbo_id_(0),
    indices_vbo_id_(0),
    num_indices_(0),
    edges_vertices_vbo_id_(0),
    edges_indices_vbo_id_(0),
    edges_num_indices_(0),
    render_ground_(true) {}

void Terrain::Render(RenderContext* context) {
  if (!render_ground_) {
    return;
  }

  if (!initialized_) {
    Hex origin;

    std::vector<float> positions;
    std::vector<GLuint> indices;
    std::vector<float> edge_positions;
    std::vector<GLuint> edge_indices;

    for (int i = 0; i < kMapSize; ++i) {
      origin.ForEachHexAtDist(i, [&](const Hex& hex) {
        DrawHex(hex, &positions, &indices);
        DrawHexRing(hex, &edge_positions, &edge_indices, /*is_outside=*/i == (kMapSize - 1));
      });
    }

    vertices_vbo_id_ = MakeAndUploadVBO(GL_ARRAY_BUFFER, positions);
    indices_vbo_id_ = MakeAndUploadVBO(GL_ELEMENT_ARRAY_BUFFER, indices);
    num_indices_ = indices.size();

    edges_vertices_vbo_id_ = MakeAndUploadVBO(GL_ARRAY_BUFFER, edge_positions);
    edges_indices_vbo_id_ = MakeAndUploadVBO(GL_ELEMENT_ARRAY_BUFFER, edge_indices);
    edges_num_indices_ = edge_indices.size();

    shader_ = GetShader("shaders/terrain.vs", "shaders/terrain.fs");
    shader_->Activate();

    initialized_ = true;
  }

  shader_->Activate();

  glm::mat4 mvp = context->projection * context->view;
  shader_->SetUniform("mvp"_name, mvp);
  shader_->SetUniform("is_edge"_name, 0);
  UseVBO(GL_ARRAY_BUFFER, 0, GL_FLOAT, 3, vertices_vbo_id_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo_id_);
  glDrawElements(GL_TRIANGLES, num_indices_, GL_UNSIGNED_INT, (const void*) 0);

  shader_->SetUniform("is_edge"_name, 1);
  UseVBO(GL_ARRAY_BUFFER, 0, GL_FLOAT, 3, edges_vertices_vbo_id_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edges_indices_vbo_id_);
  glDrawElements(GL_TRIANGLES, edges_num_indices_, GL_UNSIGNED_INT, (const void*) 0);

  glDisableVertexAttribArray(0);
}
