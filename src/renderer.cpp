#include "renderer.h"

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

#include "platform_includes.h"

#include <cstddef>
#include <iostream>

TestTriangleRenderable::TestTriangleRenderable() {
  const static GLfloat vertices[] = { 0.0f, 1.0f, 0.0f,
                                      0.0f, 0.0f, 1.0f,
                                      0.0f,  -1.0f, 0.0f, };
  const static GLuint indices[] = {
    0, 1, 2,
    2, 1, 0
  };

  simple_shader_ = GetShader("shaders/simple.vs", "shaders/simple.fs");
  simple_shader_mvp_loc_ = simple_shader_->GetUniformLocation("mvp");
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
  glm::mat4 mvp = context->vp * model;
  glUniformMatrix4fv(simple_shader_mvp_loc_, 1, GL_FALSE, glm::value_ptr(mvp));

  simple_shader_->Activate();
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

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  double camera_angle_rad = render_context_.frame_start_time / 3000000.0;

  float eye_x = 30.0f * sin(camera_angle_rad);
  float eye_y = 30.0f * cos(camera_angle_rad);

  render_context_.eye_pos = glm::vec3(eye_x, eye_y, 20.0f);

  //float distance_ratio = 3.0f;
  float distance_ratio = 3.0f;

  render_context_.eye_pos *= distance_ratio;

  float near_z = 1.0f * distance_ratio;
  float far_z = 100.0f * distance_ratio;

  glm::mat4 view = glm::lookAt(render_context_.eye_pos, glm::vec3(0.0f, 0.0f, 5.0f),
    glm::vec3(0.0f, 0.0f, 1.0f));

  glm::mat4 projection;

  if (window_width > 0 && window_height > 0) {
    projection = glm::perspective(glm::radians(90.0f),
      static_cast<float>(window_width) / window_height,
      near_z, far_z);
  } else {
    projection = glm::perspective(glm::radians(90.0f), 1.0f, near_z, far_z);
    LOG_ERROR("Invalid window size: % %", window_width, window_height);
  }

  render_context_.vp = projection * view;

  // Put the light 45 degrees from eye.
  render_context_.light_pos = glm::vec3(30.0f * sin(camera_angle_rad + M_PI / 4.0f),
                                        30.0f * cos(camera_angle_rad + M_PI / 4.0f), 7.0f) * distance_ratio;
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
