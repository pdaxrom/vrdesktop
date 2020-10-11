attribute vec4 u_position;
attribute vec4 u_texture;

varying vec2 textureCoordinate;

void main()
{
	gl_Position = u_position;
	textureCoordinate = u_texture.xy;
}