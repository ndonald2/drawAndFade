
uniform sampler2DRect texSampler;
uniform float colorDecay;
uniform float alphaDecay;
uniform float alphaMin;

void main() {
    vec4 color = texture2DRect(texSampler, gl_TexCoord[0].xy);
    color.rgb *= colorDecay;
    color.a *= alphaDecay;
    color.a = color.a < alphaMin ? 0.0 : color.a;
    gl_FragColor = color;
}