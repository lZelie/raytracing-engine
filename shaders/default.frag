#version 430

// Camera UBO
layout (std140, binding = 0) uniform CameraBlock {
    vec2 windowSize;
    vec3 cameraPosition;
    vec3 cameraTarget;
    float cameraFov;
    float exposure_time;
    int time_samples;
    float focalDistance;
    float apertureSize;
} camera;

// Scene Objects UBO

// Material properties (could be extended to have different materials per object)
struct Material {
    vec3 diffuse;
    vec3 specular;
    vec3 ambient;
    float shininess;
    float reflection_coef;
    float refraction_coef;
    float refraction_index;
    float glossiness;
    vec3 absorption;
};

struct Sphere {
    vec3 position;
    float radius;
    vec3 velocity;
};

layout (std140, binding = 1) uniform ObjectsBlock {
    Sphere spheres[256];
    vec3 planes[256];// planes are stored as pairs (position, normal)
    vec3 triangles[768];// triangles are stored as triplets of vertices
    vec4 csgSpheres[4];// CSG operation spheres
    int numSpheres;
    int numPlanes;
    int numTriangles;
    Material sphere_materials[256];
    Material plane_materials[128];
    Material triangle_materials[256];
    Material csg_sphere_materials[4];
} objects;

// Lighting UBO
layout (std140, binding = 2) uniform LightingBlock {
    vec4 lightPosition;// xyz position, w intensity
    vec3 lightColor;
    vec3 ambientLight;
    int lightType;
    int sampleRate;
    uint recursionDepth;
    bool use_fresnel;
    float light_radius;
    int shadow_samples;
} lighting;

struct Hit {
    float distance;
    vec3 surface_normal;
    int surface_material_index;
};

struct Roth {
    int nb_hits;
    Hit hits[8];// Max 8 hits points
};

vec2 uv;// the UV coordinates of this pixel on the canvas

out vec4 fColor;// final color

void compute_primary_ray(in vec2 uv, out vec3 ray_pos, out vec3 ray_dir);

vec2 get_uv_plane_size();

float ray_sphere(vec3 ray_pos, vec3 ray_dir, int sphere_index, float time, out vec3 intersect_pt, out vec3 normal);// Test ray-sphere intersection, if intersect: return distance, point and normal

float ray_triangle(vec3 ray_pos, vec3 ray_dir, vec3 p0, vec3 p1, vec3 p2, out vec3 intersect_pt, out vec3 normal);// test ray-triangle intersection, if intersect: return distance, point and normal

float ray_plane(vec3 ray_pos, vec3 ray_dir, vec3 plane_pos, vec3 plane_normal, out vec3 intersec_pt, out vec3 normal);// test ray–plane intersection , if intersect : return distance, point and normal

float compute_nearest_intersection(vec3 ray_pos, vec3 ray_dir, float time, out vec3 intersec_i, out vec3 normal_i, out int object_id, out int object_type);// find nearest intersection in the scene, , if intersect: return distance, point and normal

vec2 compute_uv();

vec3 raycast(vec2 uv);// compute primary ray, computeIntersection, shade, return color

vec3 calculate_lighting(vec3 position, vec3 normal, vec3 view_dir, Material material, vec3 light_color);

vec3 phong_brdf(vec3 light_pos, vec3 normal, vec3 view_dir, Material material);

vec3 blinn_brdf(vec3 light_pos, vec3 normal, vec3 view_dir, Material material);

Material get_material(int object_type, int object_id, vec3 position);

float calculate_shadows(vec3 position, vec3 light_dir, float light_distance);

Roth unionCSG(Roth roth1, Roth roth2);
Roth intersectionCSG(Roth roth1, Roth roth2, vec3 rey_dir);
Roth complementCSG(Roth roth1);
Roth differenceCSG(Roth roth1, Roth roth2, vec3 ray_dir);
Roth ray_sphere_roth(vec3 ray_pos, vec3 ray_dir, vec3 sphere_pos, float sphere_radius, int material_index);
float rayCSG(vec3 ray_pos, vec3 ray_dir, out vec3 intersect_point, out vec3 normal, out int object_id, out int object_type);

int seed;

int xorshift(int value) {
    value ^= value << 13;
    value ^= value >> 17;
    value ^= value << 5;
    return value;
}

float random() {
    seed = xorshift(seed);
    return abs(fract(float(seed) / 3141.592653589793238));
}

// Function to create a random direction in the hemisphere around a normal
vec3 random_hemisphere_direction(vec3 normal) {
    // Generate random point on sphere
    float theta = 2.0f * 3.14159265359f * random();
    float phi = acos(2.0f * random() - 1.0f);

    // Convert to Cartesian coordinates
    vec3 random_dir = vec3(sin(phi) * cos(theta), sin(phi) * sin(phi), cos(phi));

    // Make sure it's in the right hemisphere
    if(dot(random_dir, normal) < 0.0f) {
        random_dir = -random_dir;
    }

    return normalize(random_dir);
}

// Function to generate a random point on a disk
vec2 random_disk() {
    float r = sqrt(random());
    float theta = 2.0 * 3.14159265359 * random();
    return vec2(r * cos(theta), r * sin(theta));
}

void main() {
    seed = int(gl_FragCoord.x) + int(camera.windowSize.x) * int(gl_FragCoord.y);
    
    vec3 color = vec3(0.0);
    int samples = max(1, lighting.sampleRate);

    float step_size = 1.0 / samples;

    for (int y = 0; y < samples; y++) {
        for (int x = 0; x < samples; x++) {
            vec2 offset = vec2(
            (float(x) + 0.5) * step_size - 0.5,
            (float(y) + 0.5) * step_size - 0.5
            ) / camera.windowSize;

            vec2 sample_coord = gl_FragCoord.xy + offset * camera.windowSize;
            vec2 sample_uv = (sample_coord / camera.windowSize - 0.5) * 2.0;

            float aspect = camera.windowSize.x / camera.windowSize.y;
            if (aspect >= 1.0) {
                sample_uv.x *= aspect;
            } else {
                sample_uv.y /= aspect;
            }

            // Accumulate color from this sample
            color += raycast(sample_uv);
        }
    }

    // Average the accumulated color values
    color /= float(samples * samples);

    // Output final color
    fColor = vec4(color, 1.0f);
}

void compute_primary_ray(in vec2 uv, out vec3 ray_pos, out vec3 ray_dir){
    vec3 from = camera.cameraPosition;
    vec3 to = camera.cameraTarget;
    float fovy = camera.cameraFov;
    vec2 uv_size = get_uv_plane_size();

    // Construct the camera coordinate system
    vec3 forward = normalize(from - to);
    vec3 right = normalize(cross(vec3(0.0f, 1.0f, 0.0f), forward));
    vec3 up = cross(forward, right);

    // Compute the distance to the image plane
    float dist = uv_size.y / tan(fovy / 2);

    // Construct a primary ray going through UV coordinate
    vec3 direction = normalize(uv.x * right + uv.y * up - dist * forward);

    // If aperture size is near zero, use a pinhole camera model
    if (camera.apertureSize < 0.001) {
        ray_pos = from;
        ray_dir = direction;
        return;
    }

    // Calculate point on focal plane
    vec3 focal_point = from + direction * camera.focalDistance;

    // Generate random offset on lens based on aperture size
    vec2 lens_offset = random_disk() * camera.apertureSize;

    // Offset the ray origin by the lens sample
    vec3 lens_pos = from + lens_offset.x * right + lens_offset.y * up;

    // Update the ray direction to go through the focal point
    ray_pos = lens_pos;
    ray_dir = normalize(focal_point - lens_pos);
}

// Function to get a sphere position at a specific time
vec3 get_sphere_position_at_time(int sphere_index, float time) {
    vec3 position = objects.spheres[sphere_index].position;
    vec3 velocity = objects.spheres[sphere_index].velocity;
    return position + velocity * time;
}

float ray_sphere(vec3 ray_pos, vec3 ray_dir, int sphere_index, float time, out vec3 intersect_pt, out vec3 normal) {
    vec3 sphere_pos = get_sphere_position_at_time(sphere_index, time);
    // Calculate the vector from ray origin to sphere center
    vec3 oc = ray_pos - sphere_pos;

    // Calculate quadratic equation coefficients
    // For ray-sphere intersection: |ray_pos + t*ray_dir - sphere_pos|^2 = sphere_radius^2
    float a = dot(ray_dir, ray_dir);// Length squared of ray direction
    float b = 2.0 * dot(oc, ray_dir);// 2 * dot product of oc and ray direction
    float c = dot(oc, oc) - objects.spheres[sphere_index].radius * objects.spheres[sphere_index].radius;// Length squared of oc minus radius squared

    // Calculate discriminant
    float discriminant = b * b - 4.0 * a * c;

    // If discriminant is negative, ray doesn't hit the sphere
    if (discriminant < 0.0) {
        return -2.0;// No intersection
    }

    // Calculate the nearest intersection distance
    float t = (-b - sqrt(discriminant)) / (2.0 * a);

    // If t is negative, the sphere is behind the ray
    if (t < 0.0) {
        // Try the other intersection point
        t = (-b + sqrt(discriminant)) / (2.0 * a);
        if (t < 0.0) {
            return -1.0;// Both intersections are behind the ray
        }
    }

    // Calculate the intersection point
    intersect_pt = ray_pos + t * ray_dir;

    // Calculate the normal at the intersection point
    normal = normalize(intersect_pt - sphere_pos);

    // Return the intersection distance
    return t;
}

float ray_triangle(vec3 ray_pos, vec3 ray_dir, vec3 p0, vec3 p1, vec3 p2, out vec3 intersect_pt, out vec3 normal){
    // Calculate triangle normal
    vec3 edge1 = p1 - p0;
    vec3 edge2 = p2 - p0;
    normal = normalize(cross(edge1, edge2));

    // Check if ray and triangle are parallel
    float ndotray = dot(normal, ray_dir);
    if (abs(ndotray) < 0.000001)
    return -1.0;// They are parallel, no intersection

    // Calculate distance from ray origin to triangle plane
    float d = dot(normal, p0);
    float t = (dot(normal, ray_pos) - d) / -ndotray;

    // Check if triangle is behind the ray
    if (t < 0.0)
    return -1.0;

    // Calculate intersection point
    intersect_pt = ray_pos + t * ray_dir;

    // Check if intersection point is inside the triangle
    // Using barycentric coordinates
    vec3 c0 = cross(p1 - intersect_pt, p2 - intersect_pt);
    vec3 c1 = cross(p2 - intersect_pt, p0 - intersect_pt);
    vec3 c2 = cross(p0 - intersect_pt, p1 - intersect_pt);

    // Check if all vectors point in same direction as normal
    // (intersection point is inside triangle)
    if (dot(normal, c0) < 0.0 || dot(normal, c1) < 0.0 || dot(normal, c2) < 0.0)
    return -1.0;

    return t;
}

float ray_plane(vec3 ray_pos, vec3 ray_dir, vec3 plane_pos, vec3 plane_normal, out vec3 intersec_pt, out vec3 normal){
    // Normalize the plane normal
    plane_normal = normalize(plane_normal);

    // Check if ray and plane are parallel
    float denom = dot(plane_normal, ray_dir);
    if (abs(denom) < 0.000001) {
        return -1.0;// No intersection, ray is parallel to plane
    }

    // Calculate distance to intersection point
    float t = dot(plane_normal, plane_pos - ray_pos) / denom;

    // Check if plane is behind the ray
    if (t < 0.0) {
        return -1.0;
    }

    // Calculate intersection point
    intersec_pt = ray_pos + t * ray_dir;

    // Set output normal (same as plane normal if ray hits front side)
    normal = denom < 0.0 ? plane_normal : -plane_normal;

    return t;
}

float compute_nearest_intersection(vec3 ray_pos, vec3 ray_dir, float time, out vec3 intersec_i, out vec3 normal_i, out int object_id, out int object_type){
    float dist = 1e10f;
    bool hit = false;

    // Test intersection with sphere
    for (int i = 0; i < objects.numSpheres && i < 256; i++){
        vec3 intersec_point_sphere;
        vec3 normal_sphere;
        float sphere_dist = ray_sphere(ray_pos, ray_dir, i, time, intersec_point_sphere, normal_sphere);
        if (sphere_dist > 0.0 && sphere_dist < dist) {
            intersec_i = intersec_point_sphere;
            normal_i = normal_sphere;
            dist = sphere_dist;
            object_id = i;
            object_type = 0;
            hit = true;
        }
    }

    // Test intersection with planes
    for (int i = 0; i < objects.numPlanes && i < 128; i++){
        vec3 plane_pos = objects.planes[i * 2];
        vec3 plane_normal = objects.planes[i * 2 + 1];
        vec3 intersec_point_plane;
        vec3 normal_plane;
        float plane_dist = ray_plane(ray_pos, ray_dir, plane_pos, plane_normal, intersec_point_plane, normal_plane);
        if (plane_dist > 0.0 && plane_dist < dist) {
            intersec_i = intersec_point_plane;
            normal_i = normal_plane;
            dist = plane_dist;
            object_id = i;
            object_type = 1;
            hit = true;
        }
    }

    // Test intersection with triangles
    for (int i = 0; i < objects.numTriangles && i < 85; i++){
        vec3 p0 = objects.triangles[i * 3];
        vec3 p1 = objects.triangles[i * 3 + 1];
        vec3 p2 = objects.triangles[i * 3 + 2];
        vec3 intersec_point_triangle;
        vec3 normal_triangle;
        float triangle_dist = ray_triangle(ray_pos, ray_dir, p0, p1, p2, intersec_point_triangle, normal_triangle);
        if (triangle_dist > 0.0 && triangle_dist < dist) {
            intersec_i = intersec_point_triangle;
            normal_i = normal_triangle;
            dist = triangle_dist;
            object_id = i;
            object_type = 2;
            hit = true;
        }
    }

    vec3 csg_intersect_point;
    vec3 csg_normal;
    int csg_object_id, csg_object_type;
    float csg_dist = rayCSG(ray_pos, ray_dir, csg_intersect_point, csg_normal, csg_object_id, csg_object_type);

    if ((csg_dist > 0.0) && (dist < 0.0 || csg_dist < dist)){
        intersec_i = csg_intersect_point;
        normal_i = csg_normal;
        object_id = csg_object_id;
        object_type = csg_object_type;
        dist = csg_dist;
    }

    if (!hit && csg_dist <= 0.0) return -1.0;
    return dist;
}

vec2 get_uv_plane_size() {
    float aspect = camera.windowSize.x / camera.windowSize.y;// width / height

    vec2 uv_size;
    if (aspect >= 1.0) {
        // Width is larger than height
        uv_size = vec2(2.0 * aspect, 2.0);
    } else {
        // Height is larger than width
        uv_size = vec2(2.0, 2.0 / aspect);
    }

    return uv_size;
}

vec2 compute_uv(){
    // Get the aspect ratio (width / height)
    float aspect = camera.windowSize.x / camera.windowSize.y;

    // Calculate normalized device coordinates (NDC) in the range [-1, 1]
    vec2 ndc = (gl_FragCoord.xy / camera.windowSize - 0.5) * 2.0;

    // Scale coordinates so the shorter dimension has length 1 and
    // aspect ratio is maintained, ensuring a perfect circle
    if (aspect >= 1.0) {
        // Width is larger, scale X by 1/aspect ratio
        return vec2(ndc.x * aspect, ndc.y);
    } else {
        // Height is larger, scale Y by aspect ratio
        return vec2(ndc.x, ndc.y / aspect);
    }
}

// Function to create a reflection direction with some randomness based on glossiness
vec3 glossy_reflect(vec3 incident, vec3 normal, float glossiness) {
    // Perfect reflection vector
    vec3 reflection = reflect(incident, normal);
    
    if (glossiness < 0.001f) {
        return reflection; // Perfect reflection for glossiness = 0
    }

    // Random vector in hemisphere around the normal
    vec3 random_dir = random_hemisphere_direction(normal);

    // Interpolate between perfect reflection and random direction based on glossiness
    return normalize(mix(reflection, random_dir, glossiness));
}

vec3 raycast(vec2 uv) {
    const int time_samples = camera.exposure_time < 0.0001 && camera.time_samples > 0 ? 1 : camera.time_samples;
    vec3 color = vec3(0.0f);
    
    for (int t = 0; t < time_samples; t++) { 
        float time = camera.exposure_time * random();
        // Array to store ray positions for primary and secondary rays
        vec3 ray_pos[16];
        // Array to store ray directions
        vec3 ray_dir[16];
        // Initialize with 1 primary ray
        uint nb_rays = 1;
        // Calculate the initial ray position and direction based on screen coordinates
        compute_primary_ray(uv, ray_pos[0], ray_dir[0]);

        // Initialize color accumulator for final pixel color
        vec3 sample_color = vec3(0.0);
        // Mask array to track contribution weight of each ray (for blending reflections/refractions)
        vec3 mask[16];
        // Set initial ray's contribution to 100%
        mask[0] = vec3(1.0);

        // Track current index of refraction for each ray
        float current_ior[16];
        current_ior[0] = 1.0f;// Start with air's IOR

        // Track which rays should be skipped (terminated)
        bool skips[16];
        skips[0] = false;

        float eta_stack[16][16];
        int nb_eta[16];
        nb_eta[0] = 0;

        // Track path length through material for Beer's law
        float path_length[16];
        path_length[0] = 0.0;

        // Material absorption coefficients for Beer's law
        vec3 absorption_coef[16];
        absorption_coef[0] = vec3(0.0);

        // Main ray tracing loop - iterates up to maximum recursion depth
        int depth = 0;
        for (; depth <= lighting.recursionDepth; depth++) {
            // Process all active rays
            for (uint i = 0; i < nb_rays; i++){
                // Only process rays that haven't been terminated
                if (!skips[i]){
                    // Variables to store intersection data
                    vec3 intersect_point;
                    vec3 normal;
                    int object_id, object_type;

                    // Find the nearest object hit by the ray
                    float dist = compute_nearest_intersection(ray_pos[i], ray_dir[i], time, intersect_point, normal, object_id, object_type);

                    // If we hit something (dist > 0)
                    if (dist > 0.0) {
                        // Get material properties of the hit object
                        Material material = get_material(object_type, object_id, intersect_point);

                        // Calculate view direction (from intersection point to ray origin)
                        vec3 view_dir = normalize(ray_pos[i] - intersect_point);

                        // Calculate direct lighting at intersection point
                        vec3 direct_lighting = calculate_lighting(intersect_point, normal, view_dir, material, lighting.lightColor);

                        // Check if material has refraction or reflection properties
                        bool has_refraction = material.refraction_coef > 0.0f;
                        bool has_reflection = material.reflection_coef > 0.0f;

                        // Handle refraction (if material refracts and we haven't hit recursion limit)
                        if (has_refraction && depth < lighting.recursionDepth) {
                            float eta;
                            // If ray is entering an object, flip the normal to point inward
                            bool is_entering = dot(-view_dir, normal) < 0.0f;
                            // Calculate refraction ratio based on current IOR and material's IOR
                            float eta_from = current_ior[i];
                            float eta_to;
                            if (is_entering){
                                eta_to = material.refraction_index;
                                eta_stack[i][nb_eta[i]] = eta_to;
                                nb_eta[i]++;

                                path_length[i] = dist;
                            }
                            else {
                                normal = -normal;
                                absorption_coef[i] = material.absorption;
                                vec3 absorption = exp(-absorption_coef[i] * path_length[i]);
                                mask[i] *= absorption;

                                path_length[i] = 0.0f;
                                if (nb_eta[i] > 0){
                                    nb_eta[i]--;
                                    eta_to = eta_stack[i][nb_eta[i]];
                                }
                                else {
                                    eta_from = material.refraction_index;
                                    eta_to = 1.0f;
                                }
                            }
                            eta = eta_from / eta_to;
                            current_ior[i] = eta_to;

                            // Calculate cosine of angle between ray and normal
                            float cos_theta_i = abs(dot(-view_dir, normal));
                            float r0 = pow((eta_from - eta_to) / (eta_from + eta_to), 2);

                            vec3 refracted = refract(-view_dir, normal, eta);
                            ray_pos[i] = intersect_point - normal * 1e-3f;

                            float fresnel;
                            if (lighting.use_fresnel) {
                                if (eta_from > eta_to){
                                    float sin_theta = sqrt(1.0 - cos_theta_i * cos_theta_i);
                                    bool total_internal_reflection = (eta_from / eta_to) * (eta_from / eta_to) * sin_theta >= 1.0;

                                    if (total_internal_reflection) {
                                        fresnel = 1.0f;
                                    }
                                    else {
                                        float cos_theta_t = abs(dot(-refracted, normal));
                                        fresnel = r0 + (1.0 - r0) * pow(1.0 - cos_theta_t, 5.0);
                                    }
                                }
                                else {
                                    fresnel = r0 + (1.0 - r0) * pow(1.0 - cos_theta_i, 5.0);
                                }
                            }

                            ray_dir[i] = refracted;

                            float refraction_coef = material.refraction_coef - material.refraction_coef * fresnel;
                            float reflection_coef = material.reflection_coef + material.refraction_coef * fresnel;

                            has_reflection = reflection_coef > 0.0f;

                            // Handle both reflection and refraction (spawn additional ray if possible)
                            if (has_reflection && nb_rays < 16){
                                // Create a new reflected ray
                                ray_dir[nb_rays] = glossy_reflect(-view_dir, normal, material.glossiness);
                                // Offset position to avoid self-intersection
                                ray_pos[nb_rays] = intersect_point + normal * 1e-3f;

                                // Calculate new ray's contribution based on reflection coefficient
                                mask[nb_rays] = mask[i] * (reflection_coef);

                                path_length[nb_rays] = path_length[i];
                                absorption_coef[nb_rays] = absorption_coef[i];

                                for (int j = 0; j < nb_eta[i]; j++){
                                    eta_stack[nb_rays][j] = eta_stack[i][j];
                                }
                                skips[nb_rays] = false;
                                // Increment ray count
                                nb_rays++;
                            }

                            // Scale current ray's contribution by refraction coefficient
                            mask[i] *= refraction_coef;
                            // Add the direct lighting contribution to final sample_color, scaled by current ray mask 
                            sample_color += mask[i] * direct_lighting;
                        }
                        // Handle pure reflection (no refraction)
                        else if (has_reflection) {
                            // Calculate reflection direction
                            ray_dir[i] = glossy_reflect(-view_dir, normal, material.glossiness);
                            // Offset position to avoid self-intersection
                            ray_pos[i] = intersect_point + normal * 1e-3f;

                            // Scale ray's contribution by reflection coefficient
                            mask[i] *= material.reflection_coef;
                            // Add the direct lighting contribution to final sample_color, scaled by current ray mask 
                            sample_color += mask[i] * direct_lighting;
                        }
                        else {
                            // No reflection or refraction, terminate this ray
                            // Add the direct lighting contribution to final sample_color, scaled by current ray mask 
                            sample_color += mask[i] * direct_lighting;

                            skips[i] = true;
                        }
                    } else {
                        // Ray didn't hit anything - add background sample_color contribution
                        sample_color += mask[i] * vec3(0.2f, 0.3f, 0.4f);// Sky blue background
                        // Terminate this ray
                        skips[i] = true;
                    }
                }
            }
        } 
        color += sample_color;
    }
    // Return final accumulated color
    return color / float(time_samples);
}

// Determine if a point is in shadow
float calculate_shadows(vec3 position, vec3 light_dir, float light_distance) {
    const int time_samples = camera.exposure_time < 0.0001 && camera.time_samples > 0 ? 1 : camera.time_samples;
    float shadow_intensity = 0.0f;

    for (int t = 0; t < time_samples; t++) {
        float time = camera.exposure_time * random();
        // Variables to store random sampling data
        vec3 light_pos = lighting.lightPosition.xyz;
        float light_radius = lighting.light_radius;
        int shadow_samples = lighting.shadow_samples;
        float time_shadow_intensity = 0.0f;

        // Create a coordinate system around the light direction
        vec3 up = vec3(0.0f, 1.0f, 0.0f);
        if (abs(dot(light_dir, up)) > 0.99f) {
            up = vec3(1.0f, 0.0f, 0.0f);
        }
        vec3 right = normalize(cross(up, light_dir));
        up = normalize(cross(light_dir, right));

        // Cast multiple shadow rays toward the light source
        for (int i = 0; i < shadow_samples; i++) {
            // Generate random points on disc around light
            float radius = light_radius * sqrt(random());
            float theta = 2.0f * 3.14159265389793238f * random();

            // Calculate offset point on light using disc sampling
            vec3 offset = radius * (right * cos(theta) + up * sin(theta));
            vec3 sample_light_pos = light_pos + offset;
            vec3 sample_light_dir = normalize(sample_light_pos - position);
            float sample_light_distance = distance(position, sample_light_pos);

            // Offset the ray origin slightly to avoid self-intersection
            vec3 offset_pos = position + sample_light_dir * 0.001f;

            // Check for intersection between point and light sample
            vec3 shadow_intersec;
            vec3 shadow_normal;
            int shadow_obj_id;
            int shadow_obj_type;

            float shadow_dist = compute_nearest_intersection(offset_pos, sample_light_dir, time, shadow_intersec, shadow_normal, shadow_obj_id, shadow_obj_type);

            // If no intersection or intersection is beyond light, count this ray as unoccluded
            if (shadow_dist < 0.0f || shadow_dist > sample_light_distance) {
                time_shadow_intensity += 1.0f;
            }
        }

        shadow_intensity += time_shadow_intensity / float(shadow_samples);
    }
    return shadow_intensity / float(time_samples);
}

vec3 calculate_lighting(vec3 position, vec3 normal, vec3 view_dir, Material material, vec3 light_color) {
    // Ambient
    vec3 ambient = material.ambient * lighting.ambientLight;

    vec3 light_pos = lighting.lightPosition.xyz;
    float light_intensity = lighting.lightPosition.w;

    vec3 light_dir = normalize(light_pos - position);
    float light_distance = distance(light_pos, position);

    // Diffuse
    float diff = max(dot(normal, light_dir), 0.0);
    vec3 diffuse = diff * material.diffuse * vec3(1.0);

    // Specular - choose between Phong and Blinn-Phong based on lighting type
    vec3 specular = lighting.lightType == 0 ?
    phong_brdf(light_dir, normal, view_dir, material) :
    blinn_brdf(light_dir, normal, view_dir, material);

    // Attenuation (light falloff with distance)
    float attenuation = 1.0 / (1.0 + 0.09 * light_distance + 0.032 * light_distance * light_distance);

    float sl = calculate_shadows(position, light_dir, light_distance);
    vec3 result = ambient + sl * (diffuse + specular * light_intensity);
    result *= light_color;
    return result;
}

vec3 phong_brdf(vec3 light_dir, vec3 normal, vec3 view_dir, Material material) {
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular = spec * material.specular;
    return specular;
}

vec3 blinn_brdf(vec3 light_dir, vec3 normal, vec3 view_dir, Material material) {
    vec3 halfway = normalize(light_dir + view_dir);
    float spec = pow(max(dot(halfway, normal), 0.0), material.shininess);
    vec3 specular = spec * material.specular;
    return specular;
}

Material get_material(int object_type, int object_id, vec3 position) {
    Material mat;

    mat.reflection_coef = 0.0f;
    mat.refraction_coef = 0.0f;
    mat.refraction_index = 1.0f;
    mat.absorption = vec3(0.0);

    // Different material properties based on object type
    if (object_type == 0) { // Sphere
        return objects.sphere_materials[object_id];
    }
    else if (object_type == 1) { // Plane
        // Create checkerboard pattern based on the position
        // Get plane data to determine orientation
        vec3 plane_pos = objects.planes[object_id * 2];
        vec3 plane_normal = normalize(objects.planes[object_id * 2 + 1]);

        // Create a coordinate system for the plane
        vec3 u_axis = normalize(cross(plane_normal, abs(plane_normal.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0)));
        vec3 v_axis = normalize(cross(plane_normal, u_axis));

        // Project the position onto the plane's coordinate system
        float u = dot(position - plane_pos, u_axis);
        float v = dot(position - plane_pos, v_axis);

        // Create the checkerboard pattern
        float scale = 1.0;// Size of checker squares
        bool isEvenU = mod(floor(u * scale), 2.0) < 1.0;
        bool isEvenV = mod(floor(v * scale), 2.0) < 1.0;
        bool isBlack = isEvenU != isEvenV;// XOR for checkerboard pattern

        mat.ambient = vec3(0.1, 0.1, 0.1);
        mat.specular = vec3(0.2, 0.2, 0.2);
        mat.shininess = 4.0;

        if (isBlack) {
            // Black square
            mat.diffuse = vec3(0.1, 0.1, 0.1);// Dark grey/black
        } else {
            // White square
            mat.diffuse = vec3(0.9, 0.9, 0.9);// White
        }
    }
    else if (object_type == 2) { // Triangle/Mesh
        return objects.triangle_materials[object_id];
    }
    else {
        return objects.csg_sphere_materials[object_id];
    }

    return mat;
}

Roth ray_sphere_roth(vec3 ray_pos, vec3 ray_dir, vec3 sphere_pos, float sphere_radius, int material_index) {
    Roth result;
    result.nb_hits = 0;

    vec3 oc = ray_pos - sphere_pos;

    float a = dot(ray_dir, ray_dir);
    float b = 2.0 * dot(oc, ray_dir);
    float c = dot(oc, oc) - sphere_radius * sphere_radius;

    float discriminant = b * b - 4.0 * a * c;

    if (discriminant < 0.0) {
        return result;// No intersection
    }

    // Calculate the two intersection distances
    float t1 = (-b - sqrt(discriminant)) / (2.0 * a);
    float t2 = (-b + sqrt(discriminant)) / (2.0 * a);

    // Add entry point
    vec3 intersect_pt1 = ray_pos + t1 * ray_dir;
    vec3 normal1 = normalize(intersect_pt1 - sphere_pos);

    result.hits[result.nb_hits].distance = t1;
    result.hits[result.nb_hits].surface_normal = normal1;
    result.hits[result.nb_hits].surface_material_index = material_index;
    result.nb_hits++;

    // Add exit point
    vec3 intersect_pt2 = ray_pos + t2 * ray_dir;
    vec3 normal2 = normalize(intersect_pt2 - sphere_pos);

    result.hits[result.nb_hits].distance = t2;
    result.hits[result.nb_hits].surface_normal = -normal2;// Negative normal for exit point
    result.hits[result.nb_hits].surface_material_index = material_index;
    result.nb_hits++;

    return result;
}

// Union operation: Combines two objects, returns all intersection points sorted by distance
Roth unionCSG(Roth roth1, Roth roth2) {
    Roth result;
    result.nb_hits = 0;

    int i = 0, j = 0;

    // Merge hits from both roths, keeping them ordered by distance
    int inside_count = 0;
    while (i < roth1.nb_hits && j < roth2.nb_hits && result.nb_hits < 8) {
        if (roth1.hits[i].distance < roth2.hits[j].distance) {
            if (i % 2 != 0) {
                if (inside_count == 1) {
                    result.hits[result.nb_hits] = roth1.hits[i];
                    result.nb_hits++;
                }
                inside_count--;
            }
            else {
                if (inside_count == 0) {
                    result.hits[result.nb_hits] = roth1.hits[i];
                    result.nb_hits++;
                }
                inside_count++;
            }
            i++;
        } else {
            if (j % 2 != 0) {
                if (inside_count == 1) {
                    result.hits[result.nb_hits] = roth2.hits[j];
                    result.nb_hits++;
                }
                inside_count--;
            }
            else {
                if (inside_count == 0) {
                    result.hits[result.nb_hits] = roth2.hits[j];
                    result.nb_hits++;
                }
                inside_count++;
            }
            j++;
        }
    }

    // Add remaining hits from roth1
    while (i < roth1.nb_hits && result.nb_hits < 8) {
        result.hits[result.nb_hits] = roth1.hits[i];
        i++;
        result.nb_hits++;
    }

    // Add remaining hits from roth2
    while (j < roth2.nb_hits && result.nb_hits < 8) {
        result.hits[result.nb_hits] = roth2.hits[j];
        j++;
        result.nb_hits++;
    }

    return result;
}

// Intersection operation: Returns intersection points where both objects overlap
Roth intersectionCSG(Roth roth1, Roth roth2, vec3 ray_dir) {
    Roth result;
    result.nb_hits = 0;

    int i = 0, j = 0;

    // Merge hits from both roths, keeping them ordered by distance
    int inside_count = 0;
    while (i < roth1.nb_hits && j < roth2.nb_hits && result.nb_hits < 8) {
        if (roth1.hits[i].distance < roth2.hits[j].distance) {
            if (i % 2 != 0) {
                if (inside_count == 2) {
                    result.hits[result.nb_hits] = roth1.hits[i];
                    result.nb_hits++;
                }
                inside_count--;
            }
            else {
                if (inside_count == 1) {
                    result.hits[result.nb_hits] = roth1.hits[i];
                    result.nb_hits++;
                }
                inside_count++;
            }
            i++;
        } else {
            if (j % 2 != 0) {
                if (inside_count == 2) {
                    result.hits[result.nb_hits] = roth2.hits[j];
                    result.nb_hits++;
                }
                inside_count--;
            }
            else {
                if (inside_count == 1) {
                    result.hits[result.nb_hits] = roth2.hits[j];
                    result.nb_hits++;
                }
                inside_count++;
            }
            j++;
        }
    }

    return result;
}

// Complement operation: Inverts an object, turning inside to outside
Roth complementCSG(Roth roth) {
    Roth result;

    // For empty Roth, complement is a special case
    if (roth.nb_hits == 0) {
        // Create a "universe" hit at infinity
        result.nb_hits = 2;
        result.hits[0].distance = 0.0;
        result.hits[0].surface_normal = vec3(0.0, 0.0, 0.0);
        result.hits[0].surface_material_index = 0;

        result.hits[1].distance = 1.0e30;// Very far away
        result.hits[1].surface_normal = vec3(0.0, 0.0, 0.0);
        result.hits[1].surface_material_index = 0;
        return result;
    }

    result.nb_hits = roth.nb_hits;

    // Swap entry and exit points by reversing the array
    for (int i = 0; i < roth.nb_hits; i++) {
        result.hits[i] = roth.hits[roth.nb_hits - 1 - i];
    }

    return result;
}

// Difference operation: Subtracts the second object from the first
Roth differenceCSG(Roth roth1, Roth roth2, vec3 ray_dir) {
    Roth complement = complementCSG(roth2);
    return intersectionCSG(roth1, complement, ray_dir);
}

// Main CSG ray function that performs (Sphere1 ∩ Sphere2) + Sphere3) – Sphere4
float rayCSG(vec3 ray_pos, vec3 ray_dir, out vec3 intersect_point, out vec3 normal, out int object_id, out int object_type) {
    // Get sphere data from uniform array
    vec3 sphere1_pos = objects.csgSpheres[0].xyz;
    float sphere1_radius = objects.csgSpheres[0].w;

    vec3 sphere2_pos = objects.csgSpheres[1].xyz;
    float sphere2_radius = objects.csgSpheres[1].w;

    vec3 sphere3_pos = objects.csgSpheres[2].xyz;
    float sphere3_radius = objects.csgSpheres[2].w;

    vec3 sphere4_pos = objects.csgSpheres[3].xyz;
    float sphere4_radius = objects.csgSpheres[3].w;

    // Calculate CSG operations step by step
    // 1. Get intersections with each sphere
    Roth roth1 = ray_sphere_roth(ray_pos, ray_dir, sphere1_pos, sphere1_radius, 0);
    Roth roth2 = ray_sphere_roth(ray_pos, ray_dir, sphere2_pos, sphere2_radius, 1);
    Roth roth3 = ray_sphere_roth(ray_pos, ray_dir, sphere3_pos, sphere3_radius, 2);
    Roth roth4 = ray_sphere_roth(ray_pos, ray_dir, sphere4_pos, sphere4_radius, 3);

    // 2. Perform (Sphere1 ∩ Sphere2) + Sphere3) – Sphere4
    Roth intersection_result = intersectionCSG(roth1, roth2, ray_dir);// Sphere1 ∩ Sphere2
    Roth union_result = unionCSG(intersection_result, roth3);// (Sphere1 ∩ Sphere2) + Sphere3
    Roth final_result = differenceCSG(union_result, roth4, ray_dir);// ((Sphere1 ∩ Sphere2) + Sphere3) - Sphere4

    // 3. Find closest hit point in the final result
    if (final_result.nb_hits > 0) {
        intersect_point = ray_pos + final_result.hits[0].distance * ray_dir;
        normal = final_result.hits[0].surface_normal;
        object_id = final_result.hits[0].surface_material_index;
        object_type = 3;// Special type for CSG objects
        return final_result.hits[0].distance;
    }

    return -1.0;// No intersection
}