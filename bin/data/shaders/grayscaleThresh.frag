// Shader used to threshold and invert kinect depth image

uniform sampler2DRect texture;
uniform float threshold;

void main() {

    vec4 color = texture2DRect(texture, gl_TexCoord[0].xy);
    color.a *= (1.0 - step(threshold, color.r));
    color.rgb = vec3(1.0,1.0,1.0) - color.rgb;
    //color.rgb = vec3(1.0,1.0,1.0);
    gl_FragColor = color;
}