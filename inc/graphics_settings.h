#ifndef GRAPHICS_SETTINGS_H
#define GRAPHICS_SETTINGS_H

#define GRAPHICS_SETTINGS \
  GraphicsSetting(UseVsync, use_vsync, bool, true, SDLK_v) \
  GraphicsSetting(UseNormalMap, use_normal_map, bool, true, SDLK_n) \
  GraphicsSetting(UseLighting, use_lighting, bool, true, SDLK_l) \
  GraphicsSetting(UseSpecularHighlight, use_specular_highlight, bool, true, SDLK_h) \
  GraphicsSetting(UseAOMap, use_ao_map, bool, true, SDLK_a) \
  GraphicsSetting(UseShadows, use_shadows, bool, true, SDLK_s) \

#endif // GRAPHICS_SETTINGS_H
