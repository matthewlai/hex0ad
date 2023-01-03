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
constexpr static float kZoomSpeed = 1e-5f;

constexpr static int kShadowMapSize = 2048;

constexpr bool kDebugRenderDepth = false;
}

/*static*/ void Renderable::SetLightParams(RenderContext* context, ShaderProgram* shader) {
  shader->SetUniform("light_transform"_name, context->light_transform);
  shader->SetUniform("light_pos"_name, context->light_pos);
  shader->SetUniform("eye_pos"_name, context->eye_pos);
  shader->SetUniform("shadow_texture"_name, kShadowTextureUnit);
}

TestTriangleRenderable::TestTriangleRenderable() {
  const static std::vector<GLfloat> vertices = { 0.0f, 1.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f,
                                      0.0f,  -1.0f, 0.0f, };
  const static std::vector<GLuint> indices = {
    0, 1, 2,
    2, 1, 0
  };

  simple_shader_ = GetShader("shaders/simple.vs", "shaders/simple.fs");

  vao_id_ = Renderer::MakeVAO({
    Renderer::VBOSpec(vertices, 0, GL_FLOAT, 3),
  },
  Renderer::EBOSpec(indices));
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

  Renderer::UseVAO(vao_id_);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*)0);
}

Renderer::Renderer() {
  #define GraphicsSetting(upper, lower, type, default, toggle_key) render_context_.lower = default;
  GRAPHICS_SETTINGS
  #undef GraphicsSetting

  eye_azimuth_ = kDefaultEyeAzimuth;
  eye_elevation_ = kDefaultEyeElevation;
  eye_distance_ = kDefaultEyeDistance;

  eye_distance_target_ = kDefaultEyeDistance;

  view_centre_ = glm::vec3(0.0, 0.0, 0.0);

  first_frame_ = true;

  render_context_.frame_counter = 0;
  render_context_.frame_start_time = GetTimeUs();
}

void Renderer::RenderFrame(const std::vector<Renderable*>& renderables) {
  int window_width;
  int window_height;

  if (first_frame_) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    first_frame_ = false;
    window_ = SDL_GL_GetCurrentWindow();
    SDL_GL_GetDrawableSize(window_, &window_width, &window_height);

    last_window_width_ = window_width;
    last_window_height_ = window_height;

    glGenFramebuffers(1, &shadow_map_fb_);
    shadow_map_texture_ = TextureManager::GetInstance()->MakeDepthTexture(kShadowMapSize, kShadowMapSize);
    TextureManager::GetInstance()->BindTexture(shadow_map_texture_, GL_TEXTURE0 + kShadowTextureUnit);

    glBindFramebuffer(GL_FRAMEBUFFER, shadow_map_fb_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map_texture_, /*lod=*/0);
    GLenum no_buffer = GL_NONE;
    glDrawBuffers(1, &no_buffer);
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      LOG_ERROR("Framebuffer incomplete");
    }

    glGenFramebuffers(1, &geometry_fb_);
    geometry_colour_texture_ = TextureManager::GetInstance()->MakeColourTexture(window_width, window_height);
    geometry_depth_texture_ = TextureManager::GetInstance()->MakeDepthTexture(window_width, window_height);
    TextureManager::GetInstance()->BindTexture(geometry_colour_texture_, GL_TEXTURE0 + kGeometryColourTextureUnit);

    glBindFramebuffer(GL_FRAMEBUFFER, geometry_fb_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, geometry_colour_texture_, /*lod=*/0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, geometry_depth_texture_, /*lod=*/0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      LOG_ERROR("Framebuffer incomplete");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::vector<float> positions;
    positions.push_back(0.0f); positions.push_back(0.0f); // v0
    positions.push_back(0.0f); positions.push_back(1.0f); // v1
    positions.push_back(1.0f); positions.push_back(0.0f); // v2
    positions.push_back(1.0f); positions.push_back(1.0f); // v3

    std::vector<GLuint> indices;
    indices.push_back(0); indices.push_back(2); indices.push_back(1);
    indices.push_back(3); indices.push_back(1); indices.push_back(2);

    quad_vao_id_ = MakeVAO({
      VBOSpec(positions, 0, GL_FLOAT, 2),
    },
    EBOSpec(indices));
  }

  SDL_GL_GetDrawableSize(window_, &window_width, &window_height);

  if (window_width != last_window_width_ || window_height != last_window_height_) {
    last_window_width_ = window_width;
    last_window_height_ = window_height;

    // Resize our textures.
    TextureManager::GetInstance()->ResizeColourTexture(geometry_colour_texture_, window_width, window_height);
    TextureManager::GetInstance()->ResizeDepthTexture(geometry_depth_texture_, window_width, window_height);
  }
  
  uint64_t time_now = GetTimeUs();
  int64_t time_since_last_frame = time_now - render_context_.frame_start_time;
  render_context_.frame_start_time = time_now;

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  render_context_.window_width = window_width;
  render_context_.window_height = window_height;

  UpdateEyePos(time_since_last_frame);

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
  float shadow_far_z = 4.0f * light_distance + 100.0f;
  float window_size = 0.5f * light_distance + 10.0f;
  glm::mat4 light_projection = glm::ortho(-window_size, window_size, -window_size, window_size, shadow_near_z, shadow_far_z);
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

void Renderer::DrawQuad() {
  UseVAO(quad_vao_id_);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (const void*) 0);
}

glm::vec3 Renderer::EyePos() {
  float eye_avimuth_rad = eye_azimuth_ * M_PI / 180.0f;
  float eye_elevation_rad = eye_elevation_ * M_PI / 180.0f;

  return view_centre_ + glm::normalize(
      glm::vec3(sin(eye_avimuth_rad), cos(eye_avimuth_rad), tan(eye_elevation_rad))) * eye_distance_;
}

void Renderer::UpdateEyePos(int64_t elapsed_time_us) {
  eye_distance_ += (eye_distance_target_ - eye_distance_) * (1.0f - exp(-kZoomSpeed * static_cast<float>(elapsed_time_us)));
}

glm::vec3 Renderer::LightPos() {
  glm::vec3 eye_to_centre = view_centre_ - render_context_.eye_pos;
  glm::vec3 light_pos = glm::normalize(glm::cross(eye_to_centre, glm::vec3(0.0f, 0.0f, 1.0f))) * glm::length(eye_to_centre) * 2.0f
      + EyePos();
  light_pos += glm::vec3(0.0f, 0.0f, 1.3f * render_context_.eye_pos.z);

  // Apply a minimum distance.
  float distance = std::max(10.0f, glm::length(view_centre_ - light_pos));
  light_pos = glm::normalize(light_pos - view_centre_) * distance + view_centre_;
  return light_pos;
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

/*static*/ GLuint Renderer::MakeVAO(std::initializer_list<Renderer::VBOSpec> vbos, const Renderer::EBOSpec& ebo) {
  GLuint vao;
  glGenVertexArrays(1, &vao);
  CHECK_GL_ERROR
  glBindVertexArray(vao);
  CHECK_GL_ERROR
  for (const Renderer::VBOSpec& vbo : vbos) {
    glEnableVertexAttribArray(vbo.attrib_location);
    CHECK_GL_ERROR
    GLuint vbo_id = MakeAndUploadBuf(GL_ARRAY_BUFFER, vbo.data, vbo.data_size);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    CHECK_GL_ERROR
    if (vbo.IsInt()) {
      glVertexAttribIPointer(vbo.attrib_location, vbo.components_per_element, vbo.gl_type, 0, (const void*) 0);
    } else {
      glVertexAttribPointer(vbo.attrib_location, vbo.components_per_element, vbo.gl_type, GL_FALSE, 0, (const void*) 0);
    }
    CHECK_GL_ERROR
  }
  GLuint ebo_id = MakeAndUploadBuf(GL_ELEMENT_ARRAY_BUFFER, ebo.data, ebo.data_size);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id);
  CHECK_GL_ERROR
  return vao;
}

/*static*/ void Renderer::UseVAO(GLuint vao) {
  static GLuint current_vao = 0;
  if (vao != current_vao) {
    glBindVertexArray(vao);
    current_vao = vao;
  }
}

/*static*/ GLuint Renderer::MakeAndUploadBuf(GLenum binding_target, const void* buf, std::size_t size) {
  GLuint buf_id;
  glGenBuffers(1, &buf_id);
  CHECK_GL_ERROR
  glBindBuffer(binding_target, buf_id);
  CHECK_GL_ERROR
  glBufferData(binding_target, size, buf, GL_STATIC_DRAW);
  CHECK_GL_ERROR
  return buf_id;
}
