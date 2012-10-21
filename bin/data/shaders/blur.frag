
#define SAT_DECAY       0.95
#define ALPHA_DECAY     0.98

// vanilla frag shader
uniform sampler2DRect texSampler;

void main() {
    vec4 color = texture2DRect(texSampler, gl_TexCoord[0].xy);
    color *= vec4(SAT_DECAY,SAT_DECAY,SAT_DECAY,ALPHA_DECAY);
    gl_FragColor = color;
}