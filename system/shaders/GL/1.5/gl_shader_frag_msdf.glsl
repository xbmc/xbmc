#version 150

uniform sampler2D m_samp0;
uniform vec4 m_unicol;
in vec4 m_cord0;
out vec4 fragColor;

// SM_TEXTURE shader



float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

float screenPxRange() {
    return 8.;
}

void main() {
    vec4 bgColor = vec4(m_unicol.rgb, 0.);
    vec3 msd = texture(m_samp0, m_cord0.xy).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = screenPxRange()*(sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    fragColor = mix(bgColor, m_unicol, opacity);
}