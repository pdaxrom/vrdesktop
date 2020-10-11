//precision mediump float;

uniform vec2      u_resolution;
uniform float     u_angle;
uniform sampler2D u_texture;

void main()
{
    vec2 uv = gl_FragCoord.xy / u_resolution;
    vec2 ndcPos = uv * 2.0 - 1.0;

    float aspect = u_resolution.x / u_resolution.y;
    
    float eye_angle = abs(u_angle);
    float half_angle = eye_angle/2.0;
    float half_dist = tan(half_angle);

    vec2  vp_scale = vec2(aspect, 1.0);
    vec2  P = ndcPos * vp_scale; 
    
    float vp_dia   = length(vp_scale);
    float rel_dist = length(P) / vp_dia;  
    vec2  rel_P = normalize(P) / normalize(vp_scale);

    vec2 pos_prj = ndcPos;
    if (u_angle > 0.0)
    {
        float beta = rel_dist * half_angle;
        pos_prj = rel_P * tan(beta) / half_dist;  
    }
    else if (u_angle < 0.0)
    {
        float beta = atan(rel_dist * half_dist);
        pos_prj = rel_P * beta / half_angle;
    }

    vec2 uv_prj = pos_prj * 0.5 + 0.5;
    vec2 rangeCheck = step(vec2(0.0), uv_prj) * step(uv_prj, vec2(1.0));
    if (rangeCheck.x * rangeCheck.y < 0.5)
        discard;

    vec4 texColor = texture2D(u_texture, uv_prj.st);
    gl_FragColor = vec4( texColor.rgb, 1.0 );
}
