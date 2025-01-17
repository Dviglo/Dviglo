// https://www.khronos.org/registry/OpenGL/extensions/OES/OES_standard_derivatives.txt
#extension GL_OES_standard_derivatives : enable

#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"

VS_OUT_FS_IN(vec2 vTexCoord)
VS_OUT_FS_IN(vec4 vColor)

#ifdef TEXT_EFFECT_SHADOW
uniform vec2 cShadowOffset;
uniform vec4 cShadowColor;
#endif

#ifdef TEXT_EFFECT_STROKE
uniform vec4 cStrokeColor;
#endif


#ifdef COMPILEVS
void main()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);

    vTexCoord = iTexCoord;
    vColor = iColor;
}
#endif // def COMPILEVS


/*
    1) Simplest SDF shader:

    float distance = texture(sDiffMap, vTexCoord).a;
    if (distance >= 0.5)
        gl_FragColor.a = vColor.a; // This is glyph
    else
        gl_FragColor.a = 0.0; // Outside glyph

    2) Glyph with antialiazed border:

    float distance = texture(sDiffMap, vTexCoord).a;
    gl_FragColor.a = vColor.a * smoothstep(0.495, 0.505, distance);

    3) Quality improvement for far and small text:

    float distance = texture(sDiffMap, vTexCoord).a;
    // How much "distance" is changed for neighboring pixels.
    // If text is far then width is big. Far text will be blurred.
    float width = fwidth(distance);
    gl_FragColor.a = vColor.a * smoothstep(0.5 - width, 0.5 + width, distance);
*/

#if defined(COMPILEFS) && defined(SIGNED_DISTANCE_FIELD)
    float GetAlpha(float distance, float width)
    {
        return smoothstep(0.5 - width, 0.5 + width, distance);
    }

    // Comment this define to turn off supersampling
    #define SUPERSAMPLING
#endif


#ifdef COMPILEFS
void main()
{
#ifdef SIGNED_DISTANCE_FIELD
    gl_FragColor.rgb = vColor.rgb;
    float distance = texture(sDiffMap, vTexCoord).a;

    #ifdef TEXT_EFFECT_STROKE
        #ifdef SUPERSAMPLING
            float outlineFactor = smoothstep(0.5, 0.525, distance); // Border of glyph
            gl_FragColor.rgb = mix(cStrokeColor.rgb, vColor.rgb, outlineFactor);
        #else
            if (distance < 0.525)
               gl_FragColor.rgb = cStrokeColor.rgb;
        #endif
    #endif

    #ifdef TEXT_EFFECT_SHADOW
        if (texture(sDiffMap, vTexCoord - cShadowOffset).a > 0.5 && distance <= 0.5)
            gl_FragColor = cShadowColor;
        #ifndef SUPERSAMPLING
        else if (distance <= 0.5)
            gl_FragColor.a = 0.0;
        #endif
        else
    #endif
        {
            float width = fwidth(distance);
            float alpha = GetAlpha(distance, width);

            #ifdef SUPERSAMPLING
                vec2 deltaUV = 0.354 * fwidth(vTexCoord); // (1.0 / sqrt(2.0)) / 2.0 = 0.354
                vec4 square = vec4(vTexCoord - deltaUV, vTexCoord + deltaUV);

                float distance2 = texture(sDiffMap, square.xy).a;
                float distance3 = texture(sDiffMap, square.zw).a;
                float distance4 = texture(sDiffMap, square.xw).a;
                float distance5 = texture(sDiffMap, square.zy).a;

                alpha += GetAlpha(distance2, width)
                       + GetAlpha(distance3, width)
                       + GetAlpha(distance4, width)
                       + GetAlpha(distance5, width);

                // For calculating of average correct would be dividing by 5.
                // But when text is blurred, its brightness is lost. Therefore divide by 4.
                alpha = alpha * 0.25;
            #endif

            gl_FragColor.a = alpha;
        }
#else
    #ifdef ALPHAMAP
        gl_FragColor.rgb = vColor.rgb;
        gl_FragColor.a = vColor.a * texture(sDiffMap, vTexCoord).r;
    #else
        gl_FragColor = vColor * texture(sDiffMap, vTexCoord);
    #endif
#endif
}
#endif // def COMPILEFS
