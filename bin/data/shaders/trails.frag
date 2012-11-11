
#define SAT_DECAY       0.94
#define ALPHA_DECAY     0.98

uniform sampler2DRect texSampler;

void main() {
    vec4 color = texture2DRect(texSampler, gl_TexCoord[0].xy);
    color.rgb *= SAT_DECAY;
    color.a *= ALPHA_DECAY;
    color.a = color.a < 0.02 ? 0.0 : color.a;
    gl_FragColor = color;
}