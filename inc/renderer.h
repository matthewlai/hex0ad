#ifndef RENDERER_H
#define RENDERER_H

#include <chrono>
#include <cstdint>
#include <memory>

#include "flatbuffers/flatbuffers.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/mat4x4.hpp"

#include "graphics_settings.h"
#include "shaders.h"
#include "utils.h"

namespace {
// How often rendering stats are updated.
constexpr uint64_t kRenderStatsPeriod = 100000;
}

constexpr GLint kShadowTextureUnit = 8;

enum class RenderPass {
  // Shadow map pass. Depth testing enabled, MVP is ortho from light position.
  kShadow,

  // Standard geometry pass with normal MVP, depth testing enabled, alpha blending disabled.
  kGeometry,

  // UI pass with depth testing disabled, and alpha blending enabled. MVP ignored since we will be rendering in NDC directly.
  kUi,
};

class Renderable {
 public:
  struct RenderContext {
    uint64_t frame_counter;
    glm::mat4 view;
    glm::mat4 projection;

    // Light view + projection for geometry pass.
    glm::mat4 light_transform;

    glm::vec3 light_pos;
    glm::vec3 eye_pos;

    int32_t window_width;
    int32_t window_height;

    RenderPass pass;

    #define GraphicsSetting(upper, lower, type, default, toggle_key) type lower;
    GRAPHICS_SETTINGS
    #undef GraphicsSetting
  };
  virtual void Render(RenderContext* context) = 0;
};

class TestTriangleRenderable : public Renderable {
 public:
  TestTriangleRenderable();
  virtual ~TestTriangleRenderable();
  void Render(RenderContext* context) override;

 private:
  ShaderProgram* simple_shader_;
  GLuint vertices_vbo_id_;
  GLuint indices_vbo_id_;
};

class Renderer {
 public:
  Renderer();

  void RenderFrame(const std::vector<Renderable*>& renderables);

  void AddAzimuth(float diff_az) {
    eye_azimuth_ += diff_az;
    if (eye_azimuth_ < -180.0f) {
      eye_azimuth_ += 360.0f;
    }
    if (eye_azimuth_ > 180.0f) {
      eye_azimuth_ -= 360.0f;
    }
  }

  void AddElevation(float diff_ev) {
    eye_elevation_ += diff_ev;
    if (eye_elevation_ < 5.0f) {
      eye_elevation_ = 5.0f;
    }
    if (eye_elevation_ > 85.0f) {
      eye_elevation_ = 85.0f;
    }
  }

  void AddDistance(float diff_dist) {
    eye_distance_target_ *= exp(diff_dist);
    if (eye_distance_target_ > 1000.0f) {
      eye_distance_target_ = 1000.0f;
    }
    if (eye_distance_target_ < 25.0f) {
      eye_distance_target_ = 25.0f;
    }
  }

  void MoveCamera(int32_t x_from, int32_t y_from, int32_t x_to, int32_t y_to);

  #define GraphicsSetting(upper, lower, type, default, toggle_key) \
    void Toggle ## upper() { render_context_.lower ^= 0x1; }
    GRAPHICS_SETTINGS
  #undef GraphicsSetting

 private:
  glm::vec3 EyePos();
  glm::vec3 LightPos();

  // UnProject screen coordinates to a point on the z=0 plane.
  glm::vec3 UnProjectToXY(int32_t x, int32_t y);

  Renderable::RenderContext render_context_;
  uint64_t last_stat_time_us_;

  float eye_azimuth_;
  float eye_elevation_;
  float eye_distance_;

  // For smooth zooming.
  float eye_distance_target_;

  // We store view centre in double vector to minimize error accumulation during
  // dragging.
  glm::vec3 view_centre_;

  bool first_frame_;
  SDL_Window* window_;

  GLuint shadow_map_fb_;
  GLuint shadow_map_texture_;
};

template <typename T>
GLuint MakeAndUploadVBOImpl(GLenum binding_target, const T* buf, std::size_t size) {
  GLuint vbo_id;
  glGenBuffers(1, &vbo_id);
  CHECK_GL_ERROR
  glBindBuffer(binding_target, vbo_id);
  CHECK_GL_ERROR
  glBufferData(binding_target, sizeof(T) * size, buf, GL_STATIC_DRAW);
  CHECK_GL_ERROR
  return vbo_id;
}

template <typename T>
GLuint MakeAndUploadVBO(GLenum binding_target, const std::vector<T>& buf) {
  return MakeAndUploadVBOImpl<T>(binding_target, buf.data(), buf.size());
}

template <typename T>
GLuint MakeAndUploadVBO(GLenum binding_target, const flatbuffers::Vector<T>& buf) {
  return MakeAndUploadVBOImpl<T>(binding_target, buf.data(), static_cast<std::size_t>(buf.size()));
}

// Enable a VBO and bind it to a target and attribute location.
inline void UseVBO(GLenum binding_target, int attrib_location, GLenum gl_type,
            int components_per_element, GLuint vbo_id) {
  glEnableVertexAttribArray(attrib_location);
  glBindBuffer(binding_target, vbo_id);
  glVertexAttribPointer(attrib_location, components_per_element, gl_type, GL_FALSE, 0, (const void*) 0);
}

#endif // RENDERER_H

