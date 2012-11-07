
#define SAT_DECAY       0.98
#define ALPHA_DECAY     0.994

// vanilla frag shader
uniform sampler2DRect texSampler;

void main() {
    vec4 color = texture2DRect(texSampler, gl_TexCoord[0].xy);
    color.rgb *= SAT_DECAY;
    color.a = color.a*ALPHA_DECAY;
    if (color.a <= 0.025){
        color.a = 0.0;
    }
    gl_FragColor = color;
}