// Embedded by the editor material builder; no runtime shader-directory mapping.
// RGB is the internal light/shading field, A is geometric coverage.
float2 p = UV * 2.0 - 1.0;
float t = FlowTime;
float mask = 0., light = 1.;
// Smooth, advected value noise. Explicit octaves keep cost bounded on every tier.
float2 q = p * 3.2 + float2(t * .31, -t * .82);
float n = 0.;
if (Shape >= 5.5 && Shape < 8.5) {
    float amplitude = .57;
    [unroll] for (int octave = 0; octave < 3; ++octave) {
        float2 cell = floor(q), f = frac(q); f = f*f*(3.-2.*f);
        float4 h = frac(sin(float4(dot(cell,float2(127.1,311.7)),
            dot(cell+float2(1,0),float2(127.1,311.7)),
            dot(cell+float2(0,1),float2(127.1,311.7)),
            dot(cell+1.,float2(127.1,311.7)))) * 43758.5453);
        n += lerp(lerp(h.x,h.y,f.x),lerp(h.z,h.w,f.x),f.y)*amplitude;
        q = float2(q.x*.8-q.y*.6,q.x*.6+q.y*.8)*2.13 + 7.1;
        amplitude *= .48;
    }
}
float aa = max(fwidth(p.x)+fwidth(p.y), .006);
if (Shape < .5) {
    float y = abs(p.y);
    float core = exp2(-y*y*85.);
    float sheath = exp2(-y*y*13.);
    mask = saturate(core*.72 + sheath*.23 + exp2(-y*y*3.)*.045);
    light = .55 + core*.9;
} else if (Shape < 1.5) {
    float r2 = dot(p,p);
    mask = exp2(-r2*7.) * (1.-smoothstep(.65,1.,sqrt(r2)));
    light = .38 + exp2(-r2*24.)*1.25;
} else if (Shape < 2.5) {
    float d = abs(p.x)*.72 + abs(p.y);
    mask = 1.-smoothstep(.8-aa,.8+aa,d);
    light = lerp(.18,.95,saturate(.5+p.x*.65-p.y*.3));
    light += exp2(-pow((d-.73)*35.,2.))*.85;
} else if (Shape < 3.5) {
    float d = max(abs(p.x),abs(p.y));
    mask = 1.-smoothstep(.78-aa,.78+aa,d);
    light = .32 + .55*saturate(.5-p.y*.4+p.x*.2);
    light += exp2(-pow((d-.68)*38.,2.))*.65;
} else if (Shape < 4.5) {
    float2 h = p*1.25; h.y = -h.y+.12;
    float z = dot(h,h)-1.;
    float d = z*z*z-h.x*h.x*h.y*h.y*h.y;
    mask = 1.-smoothstep(-aa,aa,d);
    light = .35 + .55*saturate(1.-length(p-float2(-.23,-.25)));
    light += exp2(-dot(p-float2(-.28,-.3),p-float2(-.28,-.3))*45.)*.65;
} else if (Shape < 5.5) {
    float2 a = abs(p);
    float star = exp2(-a.x*42.)*pow(saturate(1.-a.y),2.)
        + exp2(-a.y*42.)*pow(saturate(1.-a.x),2.);
    mask = saturate(star + exp2(-dot(p,p)*30.)*.7);
    light = .65 + exp2(-dot(p,p)*65.);
} else if (Shape < 6.5) {
    float warp = sin(UV.x*19.-t*3.2)*.12 + (n-.48)*.58;
    float y = abs(p.y+warp);
    float fibres = pow(saturate(.5+.5*sin(UV.x*53.+n*13.-t*5.)),5.);
    mask = pow(saturate(1.-y),2.)*smoothstep(.16,.65,n)*(.22+.78*fibres);
    light = .38 + fibres*.85 + exp2(-y*y*28.)*.28;
} else if (Shape < 7.5) {
    float h = saturate(1.-UV.y);
    float width = max(.025,pow(1.-h,.76)*.9);
    float sway = sin(h*9.-t*3.)*.17*h+(n-.45)*.45;
    float body = saturate(1.-abs(p.x+sway)/width);
    mask = pow(body,1.45)*smoothstep(.16,.64,n)*smoothstep(0.,.09,h)*pow(1.-h,.25);
    light = .28 + body*.8 + smoothstep(.65,.85,n)*.45;
} else if (Shape < 8.5) {
    float r = length(p+float2(n-.45,(n-.5)*.6)*.35);
    mask = pow(saturate(1.-r*r),1.8)*smoothstep(.12,.67,n);
    // Broad self-shading gives clouds volume instead of additive neon cotton.
    light = .2 + .7*saturate(n+.25-p.y*.2-p.x*.18);
} else if (Shape < 9.5) {
    float r = length(p);
    float rim = exp2(-pow((r-.69)*22.,2.));
    float fresnel = .2 + .8*pow(saturate(r/.75),3.);
    float glint = exp2(-dot(p-float2(-.24,-.35),p-float2(-.24,-.35))*95.);
    mask = saturate(rim*fresnel+glint+exp2(-dot(p,p)*4.)*.025);
    light = .45+glint;
} else if (Shape < 10.5) {
    // Scattered light inside an energy shaft, separate from its fine filaments.
    float y = saturate(UV.y);
    float edge = exp2(-p.x*p.x*3.5)*(1.-smoothstep(.7,1.,abs(p.x)));
    float band = .6+.22*sin(y*19.-t*2.4)+.12*sin(y*37.+p.x*4.+t);
    mask = edge*smoothstep(0.,.12,y)*(1.-smoothstep(.72,1.,y))*band;
    light = .24+exp2(-p.x*p.x*16.)*.42;
} else {
    // Velocity-aligned incandescent spark: tapered head, long feathered tail.
    float width = .06 + .22*pow(saturate(UV.x),2.);
    float body = exp2(-pow(p.y/width,2.)*2.);
    mask = body*pow(saturate(UV.x),.65)*(1.-smoothstep(.89,1.,UV.x));
    light = .4 + .9*pow(saturate(UV.x),4.);
}
return float4(light,light,light,saturate(mask));
