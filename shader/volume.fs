#version 400 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler3D volumeTex;
uniform sampler1D tfTex;
uniform vec3 camPos;
uniform mat4 invViewProj;
uniform float stepSize;

void main(){
    // 1) 重建 ray
    vec4 ndc   = vec4(TexCoord * 2.0 - 1.0, 0.0, 1.0);
    vec4 world = invViewProj * ndc;
    world /= world.w;
    vec3 dir = normalize(world.xyz - camPos);

    // 2) 與單位盒相交
    vec3 boxMin = vec3(0.0), boxMax = vec3(1.0);
    vec3 invD = 1.0 / dir;
    vec3 t0s = (boxMin - camPos) * invD;
    vec3 t1s = (boxMax - camPos) * invD;
    vec3 tminv = min(t0s, t1s), tmaxv = max(t0s, t1s);
    float tmin = max(max(tminv.x, tminv.y), max(tminv.z, 0.0));
    float tmax = min(min(tmaxv.x, tmaxv.y), tmaxv.z);
    if(tmin > tmax) discard;

    // 3) 積分
    vec4 col = vec4(0.0);
    float t = tmin;
    for(int i = 0; i < 512; ++i){
        if(t > tmax || col.a >= 0.95) break;
        vec3 pos = camPos + dir * t;
        float val = texture(volumeTex, pos).r;
        vec4 samp = texture(tfTex, val);
        samp.a *= 0.05;                    // alpha scale
        col.rgb += (1.0 - col.a) * samp.a * samp.rgb;
        col.a   += (1.0 - col.a) * samp.a;
        t += stepSize;
    }
    FragColor = col;
}