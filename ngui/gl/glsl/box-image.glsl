#vert
#include "_box.glsl"

out float f_opacity;
out vec2  f_tex_coord;

void main() {
  vec2 v;
  f_opacity = opacity;
  
  if ( gl_VertexID == 0) {
    v = vertex_ac.xy; f_tex_coord = vec2(0.0, 0.0);
  }
  else if ( gl_VertexID == 1) {
    v = vertex_ac.zy; f_tex_coord = vec2(1.0, 0.0);
  }
  else if ( gl_VertexID == 2) {
    v = vertex_ac.zw; f_tex_coord = vec2(1.0, 1.0);
  }
  else {
    v = vertex_ac.xw; f_tex_coord = vec2(0.0, 1.0);
  }
  
  gl_Position = r_matrix * v_matrix * vec4(v.xy, 0.0, 1.0);
}

#frag
#include "_version.glsl"

uniform sampler2D s_tex0;

in  lowp float f_opacity;
in  lowp vec2  f_tex_coord;
out lowp vec4  FragColor;

void main(void) {
  FragColor = texture(s_tex0, f_tex_coord) * vec4(1.0, 1.0, 1.0, f_opacity);
}
