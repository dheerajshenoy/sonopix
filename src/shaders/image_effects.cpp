const char *k_image_frag_shader = R"glsl(
uniform sampler2D texture;
uniform vec2  u_texel_size;
uniform float u_grayscale;
uniform float u_brightness;
uniform float u_saturation;
uniform float u_contrast;
uniform float u_hue;
uniform float u_blur;
uniform float u_sharpen;
uniform float u_threshold;
uniform int   u_invert;

vec3 rgb2hsv(vec3 c) {
    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
    float d = q.x - min(q.w, q.y);
    float e = 1.0e-10;
    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec2 uv = gl_TexCoord[0].xy;
    vec4 pixel;

    // Gaussian 3×3 blur — kernel radius scaled by u_blur
    if (u_blur > 0.0) {
        vec2 d = u_texel_size * u_blur;
        pixel = texture2D(texture, uv + vec2(-1.0,-1.0)*d) * 0.0625
              + texture2D(texture, uv + vec2( 0.0,-1.0)*d) * 0.125
              + texture2D(texture, uv + vec2( 1.0,-1.0)*d) * 0.0625
              + texture2D(texture, uv + vec2(-1.0, 0.0)*d) * 0.125
              + texture2D(texture, uv)                     * 0.25
              + texture2D(texture, uv + vec2( 1.0, 0.0)*d) * 0.125
              + texture2D(texture, uv + vec2(-1.0, 1.0)*d) * 0.0625
              + texture2D(texture, uv + vec2( 0.0, 1.0)*d) * 0.125
              + texture2D(texture, uv + vec2( 1.0, 1.0)*d) * 0.0625;
    } else {
        pixel = texture2D(texture, uv);
    }

    // Laplacian sharpen applied to the (possibly blurred) sample
    if (u_sharpen > 0.0) {
        vec2 d = u_texel_size;
        vec4 sharp = pixel * (1.0 + 4.0 * u_sharpen)
                   - texture2D(texture, uv + vec2( 0.0,-1.0)*d) * u_sharpen
                   - texture2D(texture, uv + vec2( 0.0, 1.0)*d) * u_sharpen
                   - texture2D(texture, uv + vec2(-1.0, 0.0)*d) * u_sharpen
                   - texture2D(texture, uv + vec2( 1.0, 0.0)*d) * u_sharpen;
        pixel = clamp(sharp, 0.0, 1.0);
    }

    vec3 color = pixel.rgb;

    // brightness (additive shift)
    color = clamp(color + u_brightness, 0.0, 1.0);

    // contrast (scale around mid-grey)
    color = clamp((color - 0.5) * u_contrast + 0.5, 0.0, 1.0);

    // saturation
    float lum = dot(color, vec3(0.299, 0.587, 0.114));
    color = clamp(mix(vec3(lum), color, u_saturation), 0.0, 1.0);

    // hue shift
    if (u_hue != 0.0) {
        vec3 hsv = rgb2hsv(color);
        hsv.x = mod(hsv.x + u_hue / 360.0, 1.0);
        color = hsv2rgb(hsv);
    }

    // grayscale
    lum = dot(color, vec3(0.299, 0.587, 0.114));
    color = mix(color, vec3(lum), u_grayscale);

    // invert
    if (u_invert != 0) color = 1.0 - color;

    // threshold (applied last; u_threshold < 0 means disabled)
    if (u_threshold >= 0.0) {
        lum = dot(color, vec3(0.299, 0.587, 0.114));
        color = vec3(step(u_threshold, lum));
    }

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), pixel.a);
}
)glsl";
