attribute vec4 u_position;
attribute vec4 u_texture;

uniform float u_width;
uniform float u_height;
uniform vec3 u_angles;
uniform vec3 u_translation;

varying vec2 textureCoordinate;

#define M_PI 3.141592653589793238462643383279

mat4 mat4_identity() {
    return mat4(1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0);
}

mat4 mat4_perspective(float fov, float aspect, float near, float far) {
    float radians = fov / 2.0 * M_PI / 180.0;
    float depth = far - near;
    float sine = sin(radians);
    float cosine = cos(radians);
    if (depth != 0.0 && sine != 0.0 && cosine != 0.0) {
	float cotangent = cosine / sine;
	return mat4(cotangent / aspect, 0.0, 0.0, 0.0,
		    0.0, cotangent, 0.0, 0.0,
		    0.0, 0.0, -(far + near) / depth, -1.0,
		    0.0, 0.0, -2.0 * near * far / depth, 0.0);
    }
    return mat4(1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0);
}

mat4 mat4_frustum(float left, float right, float bottom, float top, float near, float far) {
    return mat4(2.0 * near / (right - left), 0.0, (right + left) / (right - left), 0.0,
		0.0, 2.0 * near / (top - bottom), (top + bottom) / (top - bottom), 0.0,
		0.0, 0.0, -(far + near) / (far - near), -2.0 * far * near / (far - near),
		0.0, 0.0, -1.0, 0.0);
}

mat4 mat4_rotate_x(float angle)
{
    float c = cos(angle * M_PI / 180.0);
    float s = sin(angle * M_PI / 180.0);

    return mat4(1.0, 0.0, 0.0, 0.0,
		0.0,   c,  -s, 0.0,
		0.0,   s,   c, 0.0,
		0.0, 0.0, 0.0, 1.0);
}

mat4 mat4_rotate_y(float angle)
{
    float c = cos(angle * M_PI / 180.0);
    float s = sin(angle * M_PI / 180.0);

    return mat4(  c, 0.0,   s, 0.0,
		0.0, 1.0, 0.0, 0.0,
		 -s, 0.0,   c, 0.0,
		0.0, 0.0, 0.0, 1.0);
}

mat4 mat4_rotate_z(float angle)
{
    float c = cos(angle * M_PI / 180.0);
    float s = sin(angle * M_PI / 180.0);

    return mat4(  c,  -s, 0.0, 0.0,
		  s,   c, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0);
}

mat4 mat4_translate(vec3 v)
{
    return mat4(1.0, 0.0, 0.0, v.x,
		0.0, 1.0, 0.0, v.y,
		0.0, 0.0, 1.0, v.z,
		0.0, 0.0, 0.0, 1.0);
}

void main()
{
    mat4 modelView = mat4_identity();
    modelView *= mat4_rotate_x(u_angles.x) * mat4_rotate_y(u_angles.y) * mat4_rotate_z(u_angles.z);
    modelView *= mat4_translate(u_translation);
    mat4 projectionView = mat4_perspective(15.0, u_width / u_height, 1.5, 15.0);
    gl_Position = u_position * modelView * projectionView;
//    gl_Position = u_position;

    textureCoordinate = u_texture.xy;
}
