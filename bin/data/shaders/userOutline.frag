

// vanilla frag shader
uniform sampler2DRect texSampler;
uniform float threshold;

void main() {
    vec4 color = texture2DRect(texSampler, gl_TexCoord[0].xy);
    if (color.r >= threshold){
        color.a = 0.0;
    }
    gl_FragColor = color;
}