#dv_include "uniforms.inc"
#dv_include "samplers.inc"
#dv_include "transform.inc"

VS_OUT_FS_IN(vec2 vTexCoord)
VS_OUT_FS_IN(vec4 vColor)


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


#ifdef COMPILEFS
void main()
{
    vec4 diffColor = cMatDiffColor * vColor;
    vec4 diffInput = texture(sDiffMap, vTexCoord);
    gl_FragColor = diffColor * diffInput;
}
#endif // def COMPILEFS
