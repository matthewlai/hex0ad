#include "terrain.h"

#include "logger.h"
#include "renderer.h"
#include "resources.h"
#include "shaders.h"
#include "texture_manager.h"
#include "utils.h"

#include "terrain_generated.h"

namespace {
// 0AD "tiles" are 2m x 2m https://trac.wildfiregames.com/wiki/ArtScaleAndProportions
// Which means terrain textures should repeat at 22x22m.
// Models are supposed to be 2m = 1 unit.
constexpr static float kHexRingOffset = kGridSize * 0.07f;
constexpr static int kMapSize = 15;

static constexpr const char* kTerrainPathPrefix = "assets/art/terrains/";

TextureSet* TerrainTextureSet(const std::string& path) {
  static std::map<std::string, TextureSet> cache;
  auto it = cache.find(path);
  if (it == cache.end()) {
    std::vector<std::uint8_t> raw_buffer =
        ReadWholeFile(std::string(kTerrainPathPrefix) + path + ".fb");
    const data::Terrain* terrain_data = data::GetTerrain(raw_buffer.data());

    TextureSet textures = TextureManager::GetInstance()->LoadTextures(*terrain_data->textures());

    it = cache.insert(std::make_pair(path, textures)).first;
  }
  return &(it->second);
}

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
    constexpr static GLuint offsets[] = {
      0, 1, 7, 0, 7, 6, 1, 2, 8, 8, 7, 1, 8, 2, 9, 2, 3, 9,
      3, 10, 9, 3, 4, 10, 10, 4, 11, 4, 5, 11, 11, 5, 6, 5, 0, 6,
    };

    for (const auto& offset : offsets) {
      indices->push_back(base_index + offset);
    }
  } else {
    constexpr static GLuint offsets[] = {
      0, 1, 5, 0, 5, 4, 1, 2, 6, 6, 5, 1, 6, 2, 7, 2, 3, 7
    };

    for (const auto& offset : offsets) {
      indices->push_back(base_index + offset);
    }
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
    render_ground_(true),
    terrain_selection_(0) {}

void Terrain::Render(RenderContext* context) {
  if (!render_ground_) {
    return;
  }

  if (context->pass != RenderPass::kGeometry) {
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

    // Each 0ad tile is 2m x 2m, and a terrain texture is supposed to span 11x11 tiles.
    // https://trac.wildfiregames.com/wiki/ArtDesignDocument#TerrainTextures
    shader_->SetUniform("texture_scale"_name, (1.0f / 22.0f));

    std::size_t num_terrains = sizeof(kTestTerrainPaths) / sizeof(kTestTerrainPaths[0]);
    terrain_selection_ = Rand(0, num_terrains);
    LOG_INFO("Selected terrain % (%/%)", kTestTerrainPaths[terrain_selection_], terrain_selection_, num_terrains);

    initialized_ = true;
  }

  shader_->Activate();

  SetLightParams(context, shader_);

  // Graphics settings.
  #define GraphicsSetting(upper, lower, type, default, toggle_key) \
    shader_->SetUniform(NameLiteral(#lower), context->lower);
  GRAPHICS_SETTINGS
  #undef GraphicsSetting

  TextureSet* textures = TerrainTextureSet(kTestTerrainPaths[terrain_selection_]);

  TextureManager::GetInstance()->UseTextureSet(shader_, *textures);

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
