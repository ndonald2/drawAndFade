uniform sampler2DRect depthTex;
uniform float threshold;

void main() {

    vec4 color = texture2DRect(depthTex, gl_TexCoord[0].xy);
    color.a *= (1.0 - step(threshold, color.r))*0.7; // arbitrary alpha scaling, should be a uniform
    color.rgb = vec3(0.7,0.2,0.2);
    gl_FragColor = color;
    
}