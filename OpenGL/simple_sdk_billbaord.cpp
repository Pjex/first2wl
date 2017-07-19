#include "simple_sdk.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#if 0
#include <GL\glew.h>
#else
#include <GLES2/gl2.h>
#endif

#include "simple_sdk_common.h"

#ifdef __cplusplus
extern "C"{
#endif

	static float star(unsigned size, unsigned x, unsigned y, float t)
	{
		float c = size / 2.0f;

		float i = (0.25f * sin(2.0f * PI * t) + 0.75f);
		float k = size * 0.046875f * i;

		float dist = sqrt((x - c) * (x - c) + (y - c) * (y - c));

		float salpha = 1.0f - dist / c;
		float xalpha = (float)x == c ? c : k / fabs(x  - c);
		float yalpha = (float)y == c ? c : k / fabs(y - c);

		return ogl_max(0.0f, ogl_min(1.0f, i * salpha * 0.2f + salpha * xalpha * yalpha));
	}

	void create_tag_image(TextureGL& tex, const Vertex3& color, unsigned size, float t)
	{
		unsigned char* pData = new unsigned char[size * size * 4];
		unsigned char* pOut = pData;
		for (unsigned y = 0; y < size; y++){
			for (unsigned x = 0; x < size; x++){
				*pOut++ = static_cast<unsigned char>(255 * color.x);
				*pOut++ = static_cast<unsigned char>(255 * color.y);
				*pOut++ = static_cast<unsigned char>(255 * color.z);
				*pOut++ = 255 * star(size, x, y, t);
			}
		}

		create_texture_internal(tex, (int)size, (int)size, GL_RGBA, (char*)pData);
		delete[] pData;
	}

	void update_billboard_buffer(Vertex* pData, Matrix4& rotate, const Billboard& info, int width, int height)
	{
		float hw = width * info.scale * 0.5f;
		float hh = height * info.scale * 0.5f;

		pData[0].position.x = -hw;
		pData[0].position.y = -hh;
		pData[0].position.z = -info.depth;

		pData[1].position.x = +hw;
		pData[1].position.y = -hh;
		pData[1].position.z = -info.depth;

		pData[2].position.x = -hw;
		pData[2].position.y = +hh;
		pData[2].position.z = -info.depth;

		pData[3].position.x = +hw;
		pData[3].position.y = +hh;
		pData[3].position.z = -info.depth;

		pData[0].texcoord.x = 0.0f;
		pData[0].texcoord.y = 0.0f;

		pData[1].texcoord.x = 1.0f;
		pData[1].texcoord.y = 0.0f;

		pData[2].texcoord.x = 0.0f;
		pData[2].texcoord.y = 1.0f;

		pData[3].texcoord.x = 1.0f;
		pData[3].texcoord.y = 1.0f;

		rotate = rotationYawPitchRoll(info.yaw, info.roll, info.pitch);
	}

	void update_billboard(RenderParams& render, ScreenResult& screen,
		const Size& window, const Projection& proj, const Matrix4& sceneRotation, const Billboard& billboard)
	{
		const Matrix4 projMat = perspective(proj.fov, (float)window.width / (float)window.height, proj.near, proj.far);
		render.sceneMVP = mulMat(projMat, sceneRotation);
		
		Matrix4 billboard_rotation;
		update_billboard_buffer(render.billBoardData, billboard_rotation, billboard, window.width, window.height);
		render.billboardMVP = mulMat(render.sceneMVP, billboard_rotation);

		const Matrix4 orthMat = ortho(0, +window.width, 0, +window.height, -1.0f, 1.0f);

		float img_x = (window.width - TAG_TEX_SIZE) / 2;
		float img_y = (window.height - TAG_TEX_SIZE) / 2;

		Matrix4 viewMat = identity();
		viewMat.m00 = TAG_TEX_SIZE * 0.5f;
		viewMat.m11 = TAG_TEX_SIZE * 0.5f;
		viewMat.m30 = img_x + viewMat.m00;
		viewMat.m31 = img_y + viewMat.m11;
		render.tagMVP = mulMat(orthMat, viewMat);

		Rect left_region = { 0/*left_region.x*/, /*0,*/window.height / 4,/*left_region.y*/window.width /2/*left_region.width*/, window.height/2 /*left_region.height*/ };
		viewMat.m00 = left_region.width * 0.5f + proj.trans ;//0.5666f;//here is width point ,it contral screen translation 加是平移，乘是缩放
		viewMat.m11 = left_region.height * 0.5f;//  + proj.trans * 2;//it height to ctrl screen up and down
		viewMat.m30 = left_region.x + viewMat.m00 + proj.trans ;//panjie这里应该是左边移动多少右边也移动多少达到平移效果而不是拉伸
		viewMat.m31 = left_region.y + viewMat.m11;// -  proj.trans * 2;
                //const float *b = viewMat.get();
		PJ_LOG("panjie left viewMat :left_region.width : %d, left_region.height:%d,left_region.x:%d,left_region.y:%d,window.height / 4:%d,window.width / 2:%d \n",left_region.width,left_region.height,left_region.x,left_region.y,window.height / 4,window.width / 2);
                PJ_LOG("panjie left viewMat :\n");
                int i =0;
                //for (int i = 0;i < 16; i++){
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m00);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m01);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m02);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m03);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m10);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m11);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m12);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m13);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m20);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m21);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m22);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m23);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m30);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m31);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m32);
                    PJ_LOG("leftviewMat[%d] : %f",i++, viewMat.m33);
                    
                //}
		render.leftSplitMVP = mulMat(orthMat, viewMat);

		Rect right_region = { window.width / 2, window.height / 4, window.width / 2, window.height / 2 };
		viewMat.m00 = right_region.width * 0.5f;
		viewMat.m11 = right_region.height * 0.5f;
		viewMat.m30 = right_region.x + viewMat.m00;
		viewMat.m31 = right_region.y + viewMat.m11;
                //const float *c = viewMat.get();
                /*PJ_LOG("panjie right viewMat :\n");
                for (int i = 0;i < 16; i++){
                    PJ_LOG("rightviewMat[%d] : %f",i,c[i]);
                    
                }*/
                PJ_LOG("panjie right viewMat :\n");
                int j =0;
                //for (int i = 0;i < 16; i++){
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m00);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m01);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m02);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m03);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m10);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m11);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m12);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m13);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m20);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m21);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m22);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m23);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m30);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m31);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m32);
                    PJ_LOG("leftviewMat[%d] : %f",j++, viewMat.m33);
		render.rightSplitMVP = mulMat(orthMat, viewMat);

		bool intersect;
		Vertex3 camera_pos;
		Vertex3 xAxis;
		Vertex3 yAxis;
		Vertex3 zAxis;
		Matrix4 modelView = mulMat(sceneRotation, billboard_rotation);
		decompseRigidMatrix(modelView, camera_pos, xAxis, yAxis, zAxis);
		zAxis.x *= -1.0f;
		zAxis.y *= -1.0f;
		zAxis.z *= -1.0f;

		float t, u, v;
		intersect = ogl_intersect_triangle(camera_pos, zAxis, render.billBoardData[0].position, render.billBoardData[1].position, render.billBoardData[2].position, &t, &u, &v);
		if (!intersect)
		{
			intersect = ogl_intersect_triangle(camera_pos, zAxis, render.billBoardData[1].position, render.billBoardData[2].position, render.billBoardData[3].position, &t, &u, &v);
		}

		intersect &= (t > 0.0f);
		if (intersect)
		{
			//				AND_LOG("camera_pos = (%f, %f, %f), dir = (%f, %f, %f), t = %f.\n", camera_pos.x, camera_pos.y, camera_pos.z, zAxis.x, zAxis.y, zAxis.z, t);
			Vertex3 point;
			point.x = camera_pos.x + zAxis.x * t;
			point.y = camera_pos.y + zAxis.y * t;
			point.z = camera_pos.z + zAxis.z * t;
			Vertex3 diff = point - render.billBoardData[0].position;
			float hw = window.width *  billboard.scale;
			float hh = window.height * billboard.scale;
			float screenX = diff.x / hw *  window.width;
			float screenY = window.height - diff.y / hh * window.height;

			screen.intersected = true;
			screen.x = screenX;
			screen.y = screenY;

//			printf("ScreenPos = (%f, %f).\n", screenX, screenY);
		}
		else
		{
			screen.intersected = false;
		}

		render.result = screen;
	}

#ifdef __cplusplus
}
#endif
