

// vanilla frag shader
uniform sampler2DRect depthTex;
uniform float threshold;

void main() {
    vec4 color = texture2DRect(depthTex, gl_TexCoord[0].xy);
    color.a *= (1.0 - step(threshold, color.r))*0.6;
    gl_FragColor = color;
}