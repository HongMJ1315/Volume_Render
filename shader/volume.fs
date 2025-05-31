#version 400 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler3D volumeTex;
uniform sampler1D tfTex;
uniform vec3 camPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform float stepSize;
uniform mat4 invViewProj;
uniform int isPhong;

const float shininess = 32.0;

void main(){
    vec4 ndc   = vec4(TexCoord * 2.0 - 1.0, 0.0, 1.0);
    vec4 world = invViewProj * ndc; world /= world.w;
    vec3 dir = normalize(world.xyz - camPos);

    vec3 bmin=vec3(-0.5), bmax=vec3(0.5), invD=1.0/dir;
    vec3 t0s=(bmin-camPos)*invD, t1s=(bmax-camPos)*invD;
    vec3 tminv=min(t0s,t1s), tmaxv=max(t0s,t1s);
    float tmin = max(max(tminv.x,tminv.y), max(tminv.z,0.0));
    float tmax = min(min(tmaxv.x,tmaxv.y), tmaxv.z);
    if(tmin>tmax) discard;

    // Ray Marching + Phong Shading
    vec4 col = vec4(0.0);

    float t = tmin;
    for(int i=0; i<512; ++i){
        if(t>tmax || col.a>=0.95) break;
        vec3 pos = camPos + dir*t;
        vec3 tc  = pos + vec3(0.5); 
        float val = texture(volumeTex, tc).r;
        vec4 samp = texture(tfTex, val);
        samp.a *= 0.1;

        if(isPhong == 1){
            float d = stepSize * 1.5;
            vec3 g;
            g.x = texture(volumeTex, pos+vec3(d,0,0)).r - texture(volumeTex, pos-vec3(d,0,0)).r;
            g.y = texture(volumeTex, pos+vec3(0,d,0)).r - texture(volumeTex, pos-vec3(0,d,0)).r;
            g.z = texture(volumeTex, pos+vec3(0,0,d)).r - texture(volumeTex, pos-vec3(0,0,d)).r;
            vec3 N = normalize(g);

            vec3 L = normalize(lightPos - pos);
            vec3 V = normalize(camPos - pos);
            vec3 H = normalize(L + V);
            float diff = max(dot(N, L), 0.0);
            float spec = pow(max(dot(N, H), 0.0), shininess);
            vec3 phong = ambientColor + lightColor * (diff + spec);
            samp.rgb *= phong;
        }

        col.rgb += (1.0 - col.a) * samp.a * samp.rgb;
        col.a   += (1.0 - col.a) * samp.a;
        t += stepSize;
    }

    FragColor = col;
}
