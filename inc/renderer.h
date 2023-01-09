#ifndef RENDERER_H
#define RENDERER_H

#include <chrono>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>

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

// For Geometry pass output, SMAA pass input.
constexpr GLint kGeometryColourTextureUnit = 9;

enum class RenderPass {
  // Shadow map pass. Depth testing enabled, MVP is ortho from light position.
  kShadow,

  // Standard geometry pass with normal MVP, depth testing enabled, alpha blending disabled.
  kGeometry,

  // Screen space passes are inserted here (no RenderPass value needed because no renderables are asked to render).

  // UI pass with depth testing disabled, and alpha blending enabled. MVP ignored since we will be rendering in NDC directly.
  kUi,
};

class Renderable {
 public:
  struct RenderContext {
    uint64_t frame_counter;
    uint64_t frame_start_time;

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

  // Set light_pos, eye_pos, shadow texture, and light transform uniforms from render_context_.
  static void SetLightParams(RenderContext* context, ShaderProgram* shader);
};

class TestTriangleRenderable : public Renderable {
 public:
  TestTriangleRenderable();
  virtual ~TestTriangleRenderable() {}
  void Render(RenderContext* context) override;

 private:
  ShaderProgram* simple_shader_;
  GLuint vao_id_;
};

class Renderer {
 public:
  struct VBOSpec {
    // Pointer to data to buffer.
    const void* data;

    // Size of the buffer in bytes.
    std::size_t data_size;

    int attrib_location;
    GLenum gl_type;
    int components_per_element;

    bool IsInt() const {
      static constexpr GLenum kIntTypes[] = {
        GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT
      };
      for (const auto& t : kIntTypes) {
        if (t == gl_type) {
          return true;
        }
      }
      return false;
    }

    template <typename T>
    VBOSpec(const std::vector<T>& buf, int attrib_location_i, GLenum gl_type_i, int components_per_element_i) 
        : data(buf.data()), data_size(buf.size() * sizeof(T)), attrib_location(attrib_location_i), gl_type(gl_type_i),
          components_per_element(components_per_element_i) {}

    template <typename T>
    VBOSpec(const flatbuffers::Vector<T>& buf, int attrib_location_i, GLenum gl_type_i, int components_per_element_i) 
        : data(buf.data()), data_size(static_cast<std::size_t>(buf.size()) * sizeof(T)), attrib_location(attrib_location_i),
          gl_type(gl_type_i), components_per_element(components_per_element_i) {}
  };

  struct EBOSpec {
    // Pointer to data to buffer.
    const void* data;

    // Size of the buffer in elements.
    std::size_t data_size;

    // Size per element.
    std::size_t element_size;

    template <typename T>
    EBOSpec(const std::vector<T>& buf) 
        : data(buf.data()), data_size(buf.size() * sizeof(T)) {}

    template <typename T>
    EBOSpec(const flatbuffers::Vector<T>& buf) 
        : data(buf.data()), data_size(static_cast<std::size_t>(buf.size()) * sizeof(T)) {}
  };

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

  static GLuint MakeVAO(std::initializer_list<VBOSpec> vbos, const EBOSpec& ebo);

  static void UseVAO(GLuint vao);

  // UnProject screen coordinates to a point on the z=0 plane.
  glm::vec3 UnProjectToXY(int32_t x, int32_t y);

  #define GraphicsSetting(upper, lower, type, default, toggle_key) \
    void Toggle ## upper() { render_context_.lower ^= 0x1; }
    GRAPHICS_SETTINGS
  #undef GraphicsSetting

  #define GraphicsSetting(upper, lower, type, default, toggle_key) \
    void Set ## upper(type new_val) { render_context_.lower = new_val; }
    GRAPHICS_SETTINGS
  #undef GraphicsSetting

  #define GraphicsSetting(upper, lower, type, default, toggle_key) \
    type upper() const { return render_context_.lower; }
    GRAPHICS_SETTINGS
  #undef GraphicsSetting

 private:
  class FrameBuffer {
   public:
    FrameBuffer(int width, int height, bool have_colour, bool have_depth);
    void Resize(int width, int height);
    void Bind();

    GLuint ColourTex() const { return *colour_tex_; }
    GLuint DepthTex() const { return *depth_tex_; }

    FrameBuffer(const FrameBuffer&) = default;
    FrameBuffer& operator=(const FrameBuffer&) = default;

   private:
    GLuint fbo_;
    std::optional<GLuint> colour_tex_;
    std::optional<GLuint> depth_tex_;
  };

  glm::vec3 EyePos();
  void UpdateEyePos(int64_t elapsed_time_us);
  glm::vec3 LightPos();

  void DrawFullScreen();

  static GLuint MakeAndUploadBuf(GLenum binding_target, const void* buf, std::size_t size);

  Renderable::RenderContext render_context_;

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

  int last_window_width_;
  int last_window_height_;

  // Target of the shadow map pass.
  std::optional<FrameBuffer> shadow_fb_;

  GLuint fullscreen_vao_id_;
};

#endif // RENDERER_H

