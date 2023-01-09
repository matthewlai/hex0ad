#include "renderer.h"

#include "flatbuffers/flatbuffers.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#include "smaa/AreaTex.h"
#include "smaa/SearchTex.h"

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

constexpr static glm::vec3 kLightPos(150.0f, 0.0f, 75.0f);
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

  simple_shader_ = GetShader("simple.vs", "simple.fs");

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

    shadow_fb_ = FrameBuffer(kShadowMapSize, kShadowMapSize, /*have_colour=*/false, /*have_depth=*/true);
    TextureManager::GetInstance()->BindTexture(shadow_fb_->DepthTex(), GL_TEXTURE0 + kShadowTextureUnit);

    geometry_fb_ = FrameBuffer(window_width, window_height, /*have_colour=*/true, /*have_depth=*/true);
    TextureManager::GetInstance()->BindTexture(geometry_fb_->ColourTex(), GL_TEXTURE0 + kGeometryColourTextureUnit);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Oversized triangle.
    std::vector<float> positions;
    positions.push_back(-1.0f); positions.push_back(-1.0f); // v0
    positions.push_back(3.0f); positions.push_back(-1.0f); // v1
    positions.push_back(-1.0f); positions.push_back(3.0f); // v2

    std::vector<GLuint> indices;
    indices.push_back(0); indices.push_back(1); indices.push_back(2);

    fullscreen_vao_id_ = MakeVAO({
      VBOSpec(positions, 0, GL_FLOAT, 2),
    },
    EBOSpec(indices));

    // SMAA
    smaa_data_ = SMAAData();
    smaa_data_->area_tex_ = TextureFromMemory(
      AREATEX_WIDTH, AREATEX_HEIGHT, /*internal_format=*/GL_RG8, /*format=*/GL_RG,
      /*type=*/GL_UNSIGNED_BYTE, areaTexBytes);
    smaa_data_->search_tex_ = TextureFromMemory(
      SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, /*internal_format=*/GL_R8, /*format=*/GL_RED,
      /*type=*/GL_UNSIGNED_BYTE, searchTexBytes);
    TextureManager::GetInstance()->BindTexture(smaa_data_->area_tex_, GL_TEXTURE0 + kSmaaAreaTexTextureUnit);
    TextureManager::GetInstance()->BindTexture(smaa_data_->search_tex_, GL_TEXTURE0 + kSmaaSearchTexTextureUnit);
    smaa_data_->edges_shader_ = GetShader("smaa_edges.vs", "smaa_edges_luma.fs");
    smaa_data_->weights_shader_ = GetShader("smaa_weights.vs", "smaa_weights.fs");
    smaa_data_->blending_shader_ = GetShader("smaa_blend.vs", "smaa_blend.fs");
  }

  SDL_GL_GetDrawableSize(window_, &window_width, &window_height);

  if (window_width != last_window_width_ || window_height != last_window_height_) {
    last_window_width_ = window_width;
    last_window_height_ = window_height;

    // Resize our textures (don't resize shadow map).
    if (geometry_fb_) {
      geometry_fb_->Resize(window_width, window_height);
    }

    if (smaa_data_) {
      smaa_data_->edge_fbo_->Resize(window_width, window_height);
      smaa_data_->weights_fbo_->Resize(window_width, window_height);
    }
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
  if (UseShadows()) {
    glViewport(0, 0, kShadowMapSize, kShadowMapSize);
    shadow_fb_->Bind();
    glClear(GL_DEPTH_BUFFER_BIT);
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
  }

  // Geometry pass
  // If we are doing SMAA (or any other post processing), we have to render into a framebuffer. Otherwise
  // we can render into the back buffer directly.
  if (UseSMAA()) {
    geometry_fb_->Bind();
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
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

  glDisable(GL_DEPTH_TEST);

  if (UseSMAA()) {
    // First pass - detect edges using luma.
    smaa_data_->edges_shader_->Activate();
    smaa_data_->edges_shader_->SetUniform("resolution"_name, glm::vec2(window_width, window_height));
    smaa_data_->edges_shader_->SetUniform("colorTex"_name, kGeometryColourTextureUnit);
    if (!smaa_data_->edge_fbo_) {
      smaa_data_->edge_fbo_ = FrameBuffer(window_width, window_height, /*have_colour=*/true, /*have_depth=*/false);
      TextureManager::GetInstance()->BindTexture(smaa_data_->edge_fbo_->ColourTex(), GL_TEXTURE0 + kSmaaEdgesTextureUnit);
    }
    smaa_data_->edge_fbo_->Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    DrawFullScreen();

    // Second pass - calculate blending weights.
    smaa_data_->weights_shader_->Activate();
    smaa_data_->weights_shader_->SetUniform("resolution"_name, glm::vec2(window_width, window_height));
    smaa_data_->weights_shader_->SetUniform("SMAA_RT_METRICS"_name, glm::vec4(1.0f / window_width, 1.0f / window_height,
                                                                              window_width, window_height));
    smaa_data_->weights_shader_->SetUniform("edgesTex"_name, kSmaaEdgesTextureUnit);
    smaa_data_->weights_shader_->SetUniform("areaTex"_name, kSmaaAreaTexTextureUnit);
    smaa_data_->weights_shader_->SetUniform("searchTex"_name, kSmaaSearchTexTextureUnit);
    if (!smaa_data_->weights_fbo_) {
      smaa_data_->weights_fbo_ = FrameBuffer(window_width, window_height, /*have_colour=*/true, /*have_depth=*/false);
      TextureManager::GetInstance()->BindTexture(smaa_data_->weights_fbo_->ColourTex(), GL_TEXTURE0 + kSmaaWeightsTextureUnit);
    }
    smaa_data_->weights_fbo_->Bind();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    DrawFullScreen();

    // Third pass - actual blending into back buffer.
    smaa_data_->blending_shader_->Activate();
    smaa_data_->blending_shader_->SetUniform("resolution"_name, glm::vec2(window_width, window_height));
    smaa_data_->blending_shader_->SetUniform("colorTex"_name, kGeometryColourTextureUnit);
    smaa_data_->blending_shader_->SetUniform("blendTex"_name, kSmaaWeightsTextureUnit);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    DrawFullScreen();
  }

  // UI pass.
  glEnable(GL_BLEND);

  render_context_.pass = RenderPass::kUi;
  for (auto* renderable : renderables) {
    renderable->Render(&render_context_);
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

void Renderer::DrawFullScreen() {
  UseVAO(fullscreen_vao_id_);
  glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (const void*) 0);
}

Renderer::FrameBuffer::FrameBuffer(int width, int height, bool have_colour, bool have_depth) {
  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  if (have_colour) {
    colour_tex_ = TextureManager::GetInstance()->MakeColourTexture(width, height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *colour_tex_, /*lod=*/0);
  } else {
    GLenum no_buffer = GL_NONE;
    glDrawBuffers(1, &no_buffer);
    glReadBuffer(GL_NONE);
  }
  if (have_depth) {
    depth_tex_ = TextureManager::GetInstance()->MakeDepthTexture(width, height);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, *depth_tex_, /*lod=*/0);
  }
  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      LOG_ERROR("Framebuffer incomplete");
  }
}

void Renderer::FrameBuffer::Resize(int width, int height) {
  if (colour_tex_.has_value()) {
    TextureManager::GetInstance()->ResizeColourTexture(*colour_tex_, width, height);
  }
  if (depth_tex_.has_value()) {
    TextureManager::GetInstance()->ResizeDepthTexture(*depth_tex_, width, height);
  }
}

void Renderer::FrameBuffer::Bind() {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
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
  return kLightPos;
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
