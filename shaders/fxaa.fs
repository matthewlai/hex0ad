#version 300 es

precision mediump float;

in vec2 tex_coords;

uniform sampler2D tex;

// (1.0 / render width, 1.0 / render_height)
uniform vec2 rcp_target_size;

out vec4 frag_colour;

// Quality setting. See fxaa.finc for other settings.
// Quality Preset 12
const int FXAA_QUALITY_PS = 12;
const float FXAA_QUALITY_P0 = 1.0;
const float FXAA_QUALITY_P1 = 1.5;
const float FXAA_QUALITY_P2 = 2.0;
const float FXAA_QUALITY_P3 = 4.0;
const float FXAA_QUALITY_P4 = 12.0;

// Unused
const float FXAA_QUALITY_P5 = 0.0;
const float FXAA_QUALITY_P6 = 0.0;
const float FXAA_QUALITY_P7 = 0.0;
const float FXAA_QUALITY_P8 = 0.0;
const float FXAA_QUALITY_P9 = 0.0;
const float FXAA_QUALITY_P10 = 0.0;
const float FXAA_QUALITY_P11 = 0.0;
const float FXAA_QUALITY_P12 = 0.0;

#include "fxaa.finc"

// Choose the amount of sub-pixel aliasing removal.
// This can effect sharpness.
//   1.00 - upper limit (softer)
//   0.75 - default amount of filtering
//   0.50 - lower limit (sharper, less sub-pixel aliasing removal)
//   0.25 - almost off
//   0.00 - completely off
const float kSubpix = 0.50f;

// The minimum amount of local contrast required to apply algorithm.
//   0.333 - too little (faster)
//   0.250 - low quality
//   0.166 - default
//   0.125 - high quality 
//   0.063 - overkill (slower)
const float kEdgeThreshold = 0.125f;

// Trims the algorithm from processing darks.
//   0.0833 - upper limit (default, the start of visible unfiltered edges)
//   0.0625 - high quality (faster)
//   0.0312 - visible limit (slower)
// Special notes when using FXAA_GREEN_AS_LUMA,
//   Likely want to set this to zero.
//   As colors that are mostly not-green
//   will appear very dark in the green channel!
//   Tune by looking at mostly non-green content,
//   then start at zero and increase until aliasing is a problem.
const float kEdgeThresholdMin = 0.0625f;

void main() {
  frag_colour = FxaaPixelShader(
    /*vec2 pos=*/ tex_coords,
    /*sampler2D tex=*/ tex,
    /*vec2 fxaaQualityRcpFrame=*/ rcp_target_size,
    /*float fxaaQualitySubpix=*/ kSubpix,
    /*float fxaaQualityEdgeThreshold=*/ kEdgeThreshold,
    /*float fxaaQualityEdgeThresholdMin=*/ kEdgeThresholdMin
  );
}
