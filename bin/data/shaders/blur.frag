
// vanilla frag shader
uniform sampler2DRect texSampler;

void main() {
    vec4 color = texture2DRect(texSampler, gl_TexCoord[0].xy);
    color[3] *= 0.98;
    gl_FragColor = color;
}