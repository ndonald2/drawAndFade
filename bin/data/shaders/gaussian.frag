uniform float sigma;     // The sigma value for the gaussian function: higher value means more blur
uniform float nBlurPixels;
uniform bool  isVertical;

uniform sampler2DRect blurTexture;  // Texture that will be blurred by this shader

// using ARB textures, so blur size will be 1 pixel -> 1 pixel
const float blurSize = 1.0;
const float pi = 3.14159265;


void main() {
    
    vec2 blurMultiplyVec = isVertical ? vec2(0.0, 1.0) : vec2(1.0, 0.0);
    
    // Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
    vec3 incrementalGaussian;
    incrementalGaussian.x = 1.0 / (sqrt(2.0 * pi) * sigma);
    incrementalGaussian.y = exp(-0.5 / (sigma * sigma));
    incrementalGaussian.z = incrementalGaussian.y * incrementalGaussian.y;
    
    vec4 avgValue = vec4(0.0, 0.0, 0.0, 0.0);
    float coefficientSum = 0.0;
    
    // Take the central sample first...
    avgValue += texture2DRect(blurTexture, gl_TexCoord[0].xy) * incrementalGaussian.x;
    coefficientSum += incrementalGaussian.x;
    incrementalGaussian.xy *= incrementalGaussian.yz;
    
    // Go through the remaining 8 vertical samples (4 on each side of the center)
    for (float i = 1.0; i <= nBlurPixels; i++) {
        avgValue += texture2DRect(blurTexture, gl_TexCoord[0].xy - i * blurSize *
                              blurMultiplyVec) * incrementalGaussian.x;
        avgValue += texture2DRect(blurTexture, gl_TexCoord[0].xy + i * blurSize *
                              blurMultiplyVec) * incrementalGaussian.x;
        coefficientSum += 2.0 * incrementalGaussian.x;
        incrementalGaussian.xy *= incrementalGaussian.yz;
    }
    
    gl_FragColor = avgValue / coefficientSum;
}