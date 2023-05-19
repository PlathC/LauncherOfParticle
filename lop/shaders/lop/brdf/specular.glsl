#ifndef SHADERS_LOP_BRDF_SPECULAR_GLSL
#define SHADERS_LOP_BRDF_SPECULAR_GLSL

// Found: https://github.com/boksajak/brdf/blob/master/brdf.h#L710
float getSmithG1GGX(float sn2, float alpha2) {
	return 2. / (sqrt(((alpha2 * (1. - sn2)) + sn2) / sn2) + 1.);
}

// Moving Frostbite to Physically Based Rendering by Lagarde & de Rousiers
// Found: https://github.com/boksajak/brdf/blob/master/brdf.h#L653
// Includes specular BRDF denominator
float getSmithG2GGX(float won, float win, float alpha2) 
{
    float ggxv = win * sqrt(won * won * (1. - alpha2) + alpha2);
    float ggxl = won * sqrt(win * win * (1. - alpha2) + alpha2);
    
    return 0.5 / (ggxv + ggxl);
}

// Found: https://github.com/boksajak/brdf/blob/master/brdf.h#L710
float getDGGX(float hn, float alpha2) 
{
    float b = ((alpha2 - 1.) * hn * hn + 1.);
	return alpha2 / max(1e-4, Pi * b * b);
}

// Eric Heitz, A Simpler and Exact Sampling Routine for the GGX Distribution of Visible Normals, 
// Technical report 2017
vec3 sampleGGXVNDF(vec3 V_, float alpha_x, float alpha_y, float U1, float U2)
{
    // stretch view
    vec3 V = normalize(vec3(alpha_x * V_.x, alpha_y * V_.y, V_.z));
    
    // orthonormal basis
    vec3 T1 = (V.z < 0.9999) ? normalize(cross(V, vec3(0,0,1))) : vec3(1,0,0);
    vec3 T2 = cross(T1, V);
    
    // sample point with polar coordinates (r, phi)
    float a = 1.0 / (1.0 + V.z);
    float r = sqrt(U1);
    float phi = (U2<a) ? U2/a * Pi : Pi + (U2-a)/(1.0-a) * Pi;
    float P1 = r*cos(phi);
    float P2 = r*sin(phi)*((U2<a) ? 1.0 : V.z);
    
    // compute normal
    vec3 N = P1*T1 + P2*T2 + sqrt(max(0.0, 1.0 - P1*P1 - P2*P2))*V;
    
    // unstretch
    N = normalize(vec3(alpha_x*N.x, alpha_y*N.y, max(0.0, N.z)));
    return N;
}

float evalSpecularReflection(Material material, vec3 wo, vec3 wi) 
{
    float roughness = max(1e-4, material.roughness);
    float alpha     = max(1e-4, roughness * roughness);
    float alpha2    = max(1e-4, alpha * alpha);  

    vec3  h   = normalize(wo + wi);
    float hn  = clamp(abs(h.z),  1e-4, 1.);
    float won = clamp(abs(wo.z), 1e-4, 1.);
    float win = clamp(abs(wi.z), 1e-4, 1.);

    float g = getSmithG2GGX(won, win, alpha2);
    float d = getDGGX(hn, alpha2);

    return g * d;
}

float getPdfSpecularReflection(Material material, vec3 wo, vec3 wi) 
{
    float roughness = max(1e-4, material.roughness);
    float alpha     = max(1e-4, roughness * roughness);
    float alpha2    = max(1e-4, alpha * alpha);  
    
	vec3  h   = normalize(wo + wi);
    float hn  = clamp(abs(h.z),   1e-4, 1.);
    float won = clamp(abs(wo.z),  1e-4, 1.);
    float win = clamp(abs(wi.z),  1e-4, 1.);
    float wih = clamp(dot(wi, h), 1e-4, 1.);

    float g1 = getSmithG1GGX(wih, alpha2);
    float d  = getDGGX(hn, alpha2);
    
    // Pdf of the VNDF times the Jacobian of the reflection operator
    return d * g1 * wih / max(1e-4, 4. * win * wih);
}

float evalSpecularTransmission(Material material, vec3 wo, vec3 wi) 
{
    float roughness = max(1e-4, material.roughness);
    float alpha     = max(1e-4, roughness * roughness);
    float alpha2    = max(1e-4, alpha * alpha);  

    float inside   = sign(wo.z);
    bool  isInside = inside < 0.; 

    const float AirIOR = 1.f;
    float etaI = isInside ? AirIOR : material.ior;
    float etaT = isInside ? material.ior : AirIOR;

    vec3  h   = normalize(-(etaI * wi + etaT * wo));
    float hn  = clamp(abs(h.z),        1e-4, 1.);
    float won = clamp(abs(wo.z),       1e-4, 1.);
    float woh = clamp(abs(dot(wo, h)), 1e-4, 1.);
    float win = clamp(abs(wi.z),       1e-4, 1.);
    float wih = clamp(abs(dot(wi, h)), 1e-4, 1.);

    float g2 = getSmithG1GGX(wih, alpha2) * getSmithG1GGX(woh, alpha2);
    float d  = getDGGX(hn, alpha2);
    float w  = wih * woh / max(1e-4, win * won);
    float s  = etaI * wih + etaT * woh;
    
    return w * etaT*etaT * g2 * d / max(1e-4, s*s);
}

float getPdfSpecularTransmission(Material material, vec3 wo, vec3 wi) 
{
    float roughness = max(1e-4, material.roughness);
    float alpha     = max(1e-4, roughness * roughness);
    float alpha2    = max(1e-4, alpha * alpha);  
    
    float inside   = sign(wo.z);
    bool  isInside = inside < 0.; 

    const float AirIOR = 1.f;
    float etaI = isInside ? AirIOR : material.ior;
    float etaT = isInside ? material.ior : AirIOR;

    vec3  h   = normalize(-(etaI * wi + etaT * wo));
    float hn  = clamp(abs(h.z),        1e-4, 1.);
    float won = clamp(abs(wo.z),       1e-4, 1.);
    float woh = clamp(abs(dot(wo, h)), 1e-4, 1.);
    float win = clamp(abs(wi.z),       1e-4, 1.);
    float wih = clamp(abs(dot(wi, h)), 1e-4, 1.);

    float g1 = getSmithG1GGX(wih, alpha2);
    float d  = getDGGX(hn, alpha2);
    
    float s                    = etaI * wih + etaT * woh;
    float transmissionJacobian = etaT*etaT * woh / max(1e-4, s*s);
    float vndf                 = g1 * wih * d    / win;
    
    return transmissionJacobian * vndf;
}

#endif // SHADERS_LOP_BRDF_SPECULAR_GLSL