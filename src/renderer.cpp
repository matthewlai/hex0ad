#include "renderer.h"

#include "flatbuffers/flatbuffers.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#include "platform_includes.h"
#include "texture_manager.h"

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
constexpr static float kFov = 60.0f;
constexpr static float kDefaultEyeDistance = 70.0f;
constexpr static float kDefaultEyeAzimuth = 0.0f;
constexpr static float kDefaultEyeElevation = 45.0f;
constexpr static float kZoomSpeed = 0.1f;

constexpr static int kShadowMapSize = 2048;

constexpr bool kDebugRenderDepth = false;
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
  if (context->pass != RenderPass::kGeometry) {
    return;
  }

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

  first_frame_ = true;
}

void Renderer::RenderFrame(const std::vector<Renderable*>& renderables) {
  if (first_frame_) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    first_frame_ = false;
    window_ = SDL_GL_GetCurrentWindow();

    glGenFramebuffers(1, &shadow_map_fb_);
    shadow_map_texture_ = TextureManager::GetInstance()->MakeDepthTexture(kShadowMapSize, kShadowMapSize);
    TextureManager::GetInstance()->BindTexture(shadow_map_texture_, GL_TEXTURE0 + kShadowTextureUnit);

    glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fb_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map_texture_, /*lod=*/0);
    GLenum no_buffer = GL_NONE;
    glDrawBuffers(1, &no_buffer);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  int window_width;
  int window_height;
  SDL_GL_GetDrawableSize(window_, &window_width, &window_height);

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  render_context_.window_width = window_width;
  render_context_.window_height = window_height;

  render_context_.eye_pos = EyePos();
  render_context_.light_pos = LightPos();

  // Shadow pass
  if (kDebugRenderDepth) {
    glViewport(0, 0, window_width, window_height);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  } else {
    glViewport(0, 0, kShadowMapSize, kShadowMapSize);
    glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fb_);
    glClear(GL_DEPTH_BUFFER_BIT);
  }

  float light_distance = glm::length(render_context_.light_pos);
  float shadow_near_z = 0.0f;
  float shadow_far_z = 4.0f * light_distance;
  glm::mat4 light_projection = glm::ortho(-50.0f, 50.0f, -50.0f, 50.0f, shadow_near_z, shadow_far_z);
  glm::mat4 light_view = glm::lookAt(render_context_.light_pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  render_context_.view = light_view;
  render_context_.projection = light_projection;
  render_context_.pass = RenderPass::kShadow;
  for (auto* renderable : renderables) {
    renderable->Render(&render_context_);
  }

  render_context_.light_transform = light_projection * light_view;

  if (!kDebugRenderDepth) {
    // Geometry pass
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window_width, window_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = glm::lookAt(render_context_.eye_pos, view_centre_, glm::vec3(0.0f, 0.0f, 1.0f));

    float near_z = 0.1f * eye_distance_;
    float far_z = 10.0f * eye_distance_;
    glm::mat4 projection =
        glm::perspective(glm::radians(kFov), static_cast<float>(window_width) / window_height, near_z, far_z);

    render_context_.view = view;
    render_context_.projection = projection;

    render_context_.pass = RenderPass::kGeometry;
    for (auto* renderable : renderables) {
      renderable->Render(&render_context_);
    }

    // UI pass.
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    render_context_.pass = RenderPass::kUi;
    for (auto* renderable : renderables) {
      renderable->Render(&render_context_);
    }
  }

  ++render_context_.frame_counter;

  SDL_GL_SwapWindow(window_);
}

void Renderer::MoveCamera(int32_t x_from, int32_t y_from, int32_t x_to, int32_t y_to) {
  glm::vec3 from = UnProjectToXY(x_from, y_from);
  glm::vec3 to = UnProjectToXY(x_to, y_to);
  auto diff = from - to;
  view_centre_ += diff;
  view_centre_.z = 0;
}

glm::vec3 Renderer::EyePos() {
  float eye_avimuth_rad = eye_azimuth_ * M_PI / 180.0f;
  float eye_elevation_rad = eye_elevation_ * M_PI / 180.0f;

  eye_distance_ += (eye_distance_target_ - eye_distance_) * kZoomSpeed;

  return view_centre_ + glm::normalize(
      glm::vec3(sin(eye_avimuth_rad), cos(eye_avimuth_rad), tan(eye_elevation_rad))) * eye_distance_;
}

glm::vec3 Renderer::LightPos() {
  // We are using directional lights, so only direction to objects matter, which is norm(LightPos).
  // We want to move the light with the camera, so we do that calculation first to figure out where
  // the light should be in world space, then normalize. This makes shadow calculations much easier.
  // We keep the light at some distance from the origin 
  glm::vec3 eye_to_centre = view_centre_ - render_context_.eye_pos;
  glm::vec3 light_pos = glm::normalize(glm::cross(eye_to_centre, glm::vec3(0.0f, 0.0f, 1.0f))) * glm::length(eye_to_centre) * 2.0f
      + EyePos();
  light_pos += glm::vec3(0.0f, 0.0f, 1.3f * render_context_.eye_pos.z);
  return glm::normalize(light_pos) * 100.0f;
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
