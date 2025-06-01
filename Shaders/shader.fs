#version 330 core
out vec4 FragColor;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentLightPos;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

uniform bool normalMapping;

uniform bool drawAsSphere;

void main()
{  
    // if(drawAsSphere){
    //     vec2 coord = gl_PointCoord * 2.0 - 1.0;
    //     float dist = dot(coord, coord);
    //     if(dist > 1.0) {
    //         discard; // discard fragments outside the sphere
    //     }

    //     vec3 normal = normalize(vec3(coord, sqrt(1.0 - dist)));
    //     vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
    //     float diffuse = max(dot(normal, lightDir), 0.0);

    //     vec3 color = vec3(1.0, 0.3, 0.3); // 球体颜色
    //     FragColor = vec4(color * diffuse, 1.0);
    //     return;
    // }

    // normal [0, 1] -> [-1, 1]
    vec3 normal = texture(texture_normal1, fs_in.TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    vec3 color = texture(texture_diffuse1, fs_in.TexCoords).rgb;
    // ambient
    vec3 ambient = 1.0 * color;
    // diffuse
    vec3 lightDir = normalize(fs_in.TangentLightPos - fs_in.TangentFragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;

    // specular
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = vec3(0.2) * spec;

    //FragColor = vec4(ambient + diffuse + specular, 1.0);
    FragColor = vec4(ambient, 1.0);
}