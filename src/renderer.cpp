#include "renderer.h"

#include "flatbuffers/flatbuffers.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#include "platform_includes.h"

#include <cstddef>
#include <iostream>

namespace {
constexpr static float kFov = 60.0f;
constexpr static float kDefaultEyeDistance = 70.0f;
constexpr static float kDefaultEyeAzimuth = 0.0f;
constexpr static float kDefaultEyeElevation = 45.0f;
constexpr static float kZoomSpeed = 0.1f;
}

TestTriangleRenderable::TestTriangleRenderable() {
  const static GLfloat vertices[] = { 0.0f, 1.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f,
                                      0.0f,  -1.0f, 0.0f, };
  const static GLuint indices[] = {
    0, 1, 2,
    2, 1, 0
  };

  simple_shader_ = GetShader("shaders/simple.vs", "shaders/simple.fs");
  glGenBuffers(1, &vertices_vbo_id_);
  glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo_id_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 3 * 3, vertices, GL_STATIC_DRAW);

  glGenBuffers(1, &indices_vbo_id_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo_id_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 3 * 2, indices, GL_STATIC_DRAW);
}

TestTriangleRenderable::~TestTriangleRenderable() {
  glDeleteBuffers(1, &vertices_vbo_id_);
  glDeleteBuffers(1, &indices_vbo_id_);
}

void TestTriangleRenderable::Render(RenderContext* context) {
  glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(float(context->frame_counter % 360)),
    glm::vec3(0.0f, 1.0f, 0.0f));
  glm::mat4 mvp = context->projection * context->view * model;

  simple_shader_->Activate();
  simple_shader_->SetUniform("mvp"_name, mvp);

  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, vertices_vbo_id_);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const void*) 0);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_vbo_id_);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);

  glDisableVertexAttribArray(0);
}

Renderer::Renderer() {
  #define GraphicsSetting(upper, lower, type, default, toggle_key) render_context_.lower = default;
  GRAPHICS_SETTINGS
  #undef GraphicsSetting

  render_context_.last_frame_time_us = GetTimeUs();

  last_stat_time_us_ = GetTimeUs();

  #ifdef USE_OPENGL
  // OpenGL core profile doesn't have a default VAO. Use a single VAO for now.
  unsigned int vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao); 
  #endif

  eye_azimuth_ = kDefaultEyeAzimuth;
  eye_elevation_ = kDefaultEyeElevation;
  eye_distance_ = kDefaultEyeDistance;

  eye_distance_target_ = kDefaultEyeDistance;

  view_centre_ = glm::vec3(0.0, 0.0, 0.0);
}

void Renderer::RenderFrameBegin() {
  // Make sure the last frame swap is done. This doesn't result in much
  // performance penalty because with double buffering (as opposed to triple
  // buffering), most GL calls we make below will require the swap to be done
  // anyways. Calling glFinish() here allows us to get a better time measurement.
  // All non-graphics CPU work should have been done by this point.
  glFinish();

  // Unless we are falling terribly behind (code before this point taking more than
  //  an entire frame period), this is when the buffer swap happens.
  render_context_.frame_start_time = GetTimeUs();

#ifdef __EMSCRIPTEN__
  int window_width = EM_ASM_INT({ return document.getElementById('canvas').width; });
  int window_height = EM_ASM_INT({ return document.getElementById('canvas').height; });
#else
  SDL_Window* window = SDL_GL_GetCurrentWindow();
  int window_width;
  int window_height;
  SDL_GL_GetDrawableSize(window, &window_width, &window_height);
#endif

  glViewport(0, 0, window_width, window_height);
  render_context_.window_width = window_width;
  render_context_.window_height = window_height;

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  float eye_avimuth_rad = eye_azimuth_ * M_PI / 180.0f;
  float eye_elevation_rad = eye_elevation_ * M_PI / 180.0f;

  eye_distance_ += (eye_distance_target_ - eye_distance_) * kZoomSpeed;

  render_context_.eye_pos = view_centre_ + glm::normalize(
      glm::vec3(sin(eye_avimuth_rad), cos(eye_avimuth_rad), tan(eye_elevation_rad))) * eye_distance_;

  glm::mat4 view = glm::lookAt(render_context_.eye_pos, view_centre_, glm::vec3(0.0f, 0.0f, 1.0f));

  float near_z = 0.1f * eye_distance_;
  float far_z = 10.0f * eye_distance_;
  glm::mat4 projection = glm::perspective(glm::radians(kFov),
    static_cast<float>(window_width) / window_height,
    near_z, far_z);

  render_context_.view = view;
  render_context_.projection = projection;

  // Put the light 45 degrees from eye.
  render_context_.light_pos = glm::normalize(
      glm::vec3(sin(eye_avimuth_rad + M_PI / 4.0f), cos(eye_avimuth_rad + M_PI / 4.0f), 0.25f)) * eye_distance_;
}

void Renderer::Render(Renderable* renderable) {
  renderable->Render(&render_context_);
}

void Renderer::RenderFrameEnd() {
  // This is when all the draw calls have been issued (all the CPU work is done).
  uint64_t draw_calls_end_time = GetTimeUs();

  glFinish();

  // This is when all the drawing is done (to the back buffer).
  uint64_t frame_end_time = GetTimeUs();

  ++render_context_.frame_counter;
  if ((render_context_.frame_start_time - last_stat_time_us_) > kRenderStatsPeriod) {
    uint64_t time_since_last_frame_us = render_context_.frame_start_time - render_context_.last_frame_time_us;
    float frame_rate = 1000000.0f / time_since_last_frame_us;
    LOG_INFO("Frame rate: %, draw calls time: % us, render time: % us",
             frame_rate,
             (draw_calls_end_time - render_context_.frame_start_time),
             (frame_end_time - render_context_.frame_start_time));
    last_stat_time_us_ = render_context_.frame_start_time;
  }
  render_context_.last_frame_time_us = render_context_.frame_start_time;
}

void Renderer::MoveCamera(int32_t x_from, int32_t y_from, int32_t x_to, int32_t y_to) {
  glm::vec3 from = UnProjectToXY(x_from, y_from);
  glm::vec3 to = UnProjectToXY(x_to, y_to);
  auto diff = from - to;
  view_centre_ += diff;
  view_centre_.z = 0;
}

glm::vec3 Renderer::UnProjectToXY(int32_t x, int32_t y) {
  // First we need two points to create the ray corresponding to (x, y) on screen. We need two points for that.
  // For the first point we unproject from an arbitrary depth = 1. The second point is the camera position.
  glm::vec3 point0 = glm::unProject(
      glm::vec3(x, render_context_.window_height - y, 1.0), render_context_.view, render_context_.projection,
      glm::vec4(0.0f, 0.0f, render_context_.window_width, render_context_.window_height));
  glm::vec3 point1 = render_context_.eye_pos;

  glm::vec3 dir = glm::normalize(point1 - point0);

  // Then we are just looking for where the camera -> point ray intersects z = 0.
  // point0.z + c * dir.z = 0
  // c = -point0.z / dir.z
  // point0 + c * dir = vec3(x, y, 0)
  return point0 + (-point0.z / dir.z) * dir;
}
