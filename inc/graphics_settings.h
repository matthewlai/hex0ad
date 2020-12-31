#ifndef GRAPHICS_SETTINGS_H
#define GRAPHICS_SETTINGS_H

#define GRAPHICS_SETTINGS \
  GraphicsSetting(UseNormalMap, use_normal_map, bool, true, SDLK_n) \
  GraphicsSetting(UseLighting, use_lighting, bool, true, SDLK_l) \
  GraphicsSetting(UsePlayerColour, use_player_colour, bool, true, SDLK_p) \
  GraphicsSetting(UseSpecularHighlight, use_specular_highlight, bool, true, SDLK_h) \
  GraphicsSetting(UseAOMap, use_ao_map, bool, true, SDLK_a) \
  GraphicsSetting(UseShadows, use_shadows, bool, true, SDLK_s) \
  GraphicsSetting(UseFXAA, use_fxaa, bool, true, SDLK_f) \

#endif // GRAPHICS_SETTINGS_H
