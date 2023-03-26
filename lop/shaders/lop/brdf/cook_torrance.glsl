#ifndef SHADERS_LOP_BRDF_COOKTORRANCE_GLSL
#define SHADERS_LOP_BRDF_COOKTORRANCE_GLSL

// Sampling the GGX Distribution of Visible Normals, Eric Heitz,
// Journal of Computer Graphics Techniques Vol. 7, No. 4, 2018
// A. Complete Implementation of the GGX VNDF Sampling Routine
vec3 sampleGGXVNDF(vec3 Ve, float alpha_x, float alpha_y, float U1, float U2)
{
    // Section 3.2: transforming the view direction to the hemisphere configuration
    vec3 Vh = normalize(vec3(alpha_x * Ve.x, alpha_y * Ve.y, Ve.z));
    // Section 4.1: orthonormal basis (with special case if cross product is zero)
    float lensq = Vh.x * Vh.x + Vh.y * Vh.y;
    vec3  T1    = lensq > 0.f ? vec3(-Vh.y, Vh.x, 0.f) * inversesqrt(lensq) : vec3(1.f, 0.f, 0.f);
    vec3  T2    = cross(Vh, T1);
    // Section 4.2: parameterization of the projected area
    float r   = sqrt(U1);
    float phi = 2.f * Pi * U2;
    float t1  = r * cos(phi);
    float t2  = r * sin(phi);
    float s   = 0.5f * (1.f + Vh.z);
    t2        = (1.f - s) * sqrt(max(0.f, 1.f - t1 * t1)) + s * t2;
    // Section 4.3: reprojection onto hemisphere
    vec3 Nh = t1 * T1 + t2 * T2 + sqrt(max(0.f, 1.f - t1 * t1 - t2 * t2)) * Vh;
    // Section 3.4: transforming the normal back to the ellipsoid configuration
    vec3 Ne = normalize(vec3(alpha_x * Nh.x, alpha_y * Nh.y, max(0.f, Nh.z)));
    return Ne;
}

float smithG1GGX(float alpha2, float nS2) { return 2.0f / (sqrt(((alpha2 * (1.0f - nS2)) + nS2) / nS2) + 1.0f); }

float distributionGGX(float alphaSquared, float NdotH)
{
    // Reference: Jakub Boksansky. (2021). Crash Course in BRDF Implementation.
    float b = ((alphaSquared - 1.0f) * NdotH * NdotH + 1.0f);
    return alphaSquared / (PI * b * b);
}

// Implementing a Simple Anisotropic Rough Diffuse Material with Stochastic Evaluation, Eric Heitz and Jonathan Dupuy,
// Technical report, A. Masking and Shadowing
float smithG2OverG1HeightCorrelated(float alpha, float alphaSquared, float NdotL, float NdotV)
{
    float hlG1 = 1.f / (1.f + smithG1GGX(alphaSquared, NdotL * NdotL));
    float hvG1 = 1.f / (1.f + smithG1GGX(alphaSquared, NdotV * NdotV));
    return hlG1 / (hvG1 + hlG1 - hvG1 * hlG1);
}

float getPdfGGXVNDF(float alpha2, float nH, float nWo))
{
    nH = max(0.00001, nH);
    return (distributionGGX(max(0.00001, alpha2), nH) * smithG1GGX(alpha2, nWo * nWo)) / (4.0f * nWo);
}

#endif // SHADERS_LOP_BRDF_MICROFACETS_GLSL