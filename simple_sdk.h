
#ifndef _SIMPLE_SDK_
#define _SIMPLE_SDK_
/*
typedef	enum TexelFormat
{
		RGB,
		RGBA,
		BGR,
		BGRA
}TexelFormat;
*/

typedef struct
{
	bool intersected;
	float x;
	float y;
}ScreenResult;

static const unsigned TAG_TEX_SIZE = 64;

extern "C" void ogl_init();//1
extern "C" void ogl_resize(int width, int height);//2
extern "C" void ogl_set_projection(float fov, float near, float far);
extern "C" void ogl_set_background_texture(int width, int height, int format, const char* pData);
extern "C" void ogl_set_rect_texture(int width, int height, int format, const char* pData);
extern "C" void ogl_read_texels_from_renderbuffer();

extern "C" void ogl_set_background_texture_id(unsigned int id);
extern "C" void ogl_set_rect_texture_id(unsigned int id);//4
extern "C" void ogl_create_background_default_texture(int width, int height);//3

extern "C" void ogl_set_rect_size(int x, int y, int width, int height);
extern "C" void ogl_set_rect_location(float scale, float depth, float yaw, float pitch, float roll);
extern "C" void ogl_set_background_rotation(float yaw, float pitch, float roll);
extern "C" void ogl_update(ScreenResult& out, float* mat);//every time
extern "C" void ogl_render(float elpsedTime);//every time
extern "C" void ogl_destroy();

#endif
