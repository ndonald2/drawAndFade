// Shader used to threshold and invert kinect depth image

uniform sampler2DRect depthTexture;
uniform sampler2DRect maskTexture;

void main() {

    vec4 depthPixel = texture2DRect(depthTexture, gl_TexCoord[0].xy);
    vec4 maskPixel = texture2DRect(maskTexture, gl_TexCoord[0].xy);
    gl_FragColor = maskPixel.a > 0.0 ? vec4(0.0,0.0,0.0,0.0) : depthPixel;
}