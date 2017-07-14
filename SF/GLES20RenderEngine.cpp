/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 * Not a Contribution
 *
 *
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <ui/Rect.h>

#include <utils/String8.h>
#include <utils/Trace.h>

#include <cutils/compiler.h>
#include <gui/ISurfaceComposer.h>
#include <math.h>

#include "GLES20RenderEngine.h"
#include "Program.h"
#include "ProgramCache.h"
#include "Description.h"
#include "Mesh.h"
#include "Texture.h"

//panjie
#include "../simpleSDK/simple_sdk.h"
// ---------------------------------------------------------------------------
namespace android {
// ---------------------------------------------------------------------------

GLES20RenderEngine::GLES20RenderEngine() :
        mVpWidth(0), mVpHeight(0) {

    //panjie add
    startFlag = true;
    isToOfB = false;
    mTexName = 0;
    mFboName = 0;
    //saveGLES = false;
    //tname_pj = 0;
    //name_pj = 0;

    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mMaxTextureSize);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, mMaxViewportDims);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);

    const uint16_t protTexData[] = { 0 };
    glGenTextures(1, &mProtectedTexName);
    glBindTexture(GL_TEXTURE_2D, mProtectedTexName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0,
            GL_RGB, GL_UNSIGNED_SHORT_5_6_5, protTexData);

    //mColorBlindnessCorrection = M;
}

GLES20RenderEngine::~GLES20RenderEngine() {
}


size_t GLES20RenderEngine::getMaxTextureSize() const {
    return mMaxTextureSize;
}

size_t GLES20RenderEngine::getMaxViewportDims() const {
    return
        mMaxViewportDims[0] < mMaxViewportDims[1] ?
            mMaxViewportDims[0] : mMaxViewportDims[1];
}

void GLES20RenderEngine::setViewportAndProjection(
        size_t vpw, size_t vph, Rect sourceCrop, size_t hwh, bool yswap,
        Transform::orientation_flags rotation) {

    size_t l = sourceCrop.left;
    size_t r = sourceCrop.right;

    // In GL, (0, 0) is the bottom-left corner, so flip y coordinates
    size_t t = hwh - sourceCrop.top;
    size_t b = hwh - sourceCrop.bottom;

    mat4 m;
    if (yswap) {
        m = mat4::ortho(l, r, t, b, 0, 1);
    } else {
        m = mat4::ortho(l, r, b, t, 0, 1);
    }

    // Apply custom rotation to the projection.
    float rot90InRadians = 2.0f * static_cast<float>(M_PI) / 4.0f;
    switch (rotation) {
        case Transform::ROT_0:
            break;
        case Transform::ROT_90:
            m = mat4::rotate(rot90InRadians, vec3(0,0,1)) * m;
            break;
        case Transform::ROT_180:
            m = mat4::rotate(rot90InRadians * 2.0f, vec3(0,0,1)) * m;
            break;
        case Transform::ROT_270:
            m = mat4::rotate(rot90InRadians * 3.0f, vec3(0,0,1)) * m;
            break;
        case Transform::FLIP_H:
            m = mat4::scale(vec4(-1, 1, 1, 1)) * m;
            break;
        case Transform::FLIP_V:
            m = mat4::scale(vec4(1, -1, 1, 1)) * m;
            break;
        default:
            break;
    }

    glViewport(0, 0, vpw, vph);
    mState.setProjectionMatrix(m);
    mVpWidth = vpw;
    mVpHeight = vph;
}

#ifdef USE_HWC2
void GLES20RenderEngine::setupLayerBlending(bool premultipliedAlpha,
        bool opaque, float alpha) {
#else
void GLES20RenderEngine::setupLayerBlending(
    bool premultipliedAlpha, bool opaque, int alpha) {
#endif

    mState.setPremultipliedAlpha(premultipliedAlpha);
    mState.setOpaque(opaque);
#ifdef USE_HWC2
    mState.setPlaneAlpha(alpha);

    if (alpha < 1.0f || !opaque) {
#else
    mState.setPlaneAlpha(alpha / 255.0f);

    if (alpha < 0xFF || !opaque) {
#endif
        glEnable(GL_BLEND);
        glBlendFunc(premultipliedAlpha ? GL_ONE : GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    } else {
        glDisable(GL_BLEND);
    }
}

#ifdef USE_HWC2
void GLES20RenderEngine::setupDimLayerBlending(float alpha) {
#else
void GLES20RenderEngine::setupDimLayerBlending(int alpha) {
#endif
    mState.setPlaneAlpha(1.0f);
    mState.setPremultipliedAlpha(true);
    mState.setOpaque(false);
#ifdef USE_HWC2
    mState.setColor(0, 0, 0, alpha);
#else
    mState.setColor(0, 0, 0, alpha/255.0f);
#endif
    mState.disableTexture();

#ifdef USE_HWC2
    if (alpha == 1.0f) {
#else
    if (alpha == 0xFF) {
#endif
        glDisable(GL_BLEND);
    } else {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
}

#ifdef USE_HWC2
void GLES20RenderEngine::setupDimLayerBlendingWithColor(uint32_t color, float alpha) {
#else
void GLES20RenderEngine::setupDimLayerBlendingWithColor(uint32_t color, int alpha) {
#endif
    // SF Client sets the color on Dim Layer in RGBA format
    float r = float((color & 0xFF000000) >> 24);
    float g = float((color & 0x00FF0000) >> 16);
    float b = float((color & 0x0000FF00) >> 8);
    float a = float(color & 0x000000FF);

    mState.setPlaneAlpha(1.0f);
    mState.setPremultipliedAlpha(true);
    mState.setOpaque(false);
#ifdef USE_HWC2
    mState.setColor(r, g, b, (a / 255.0f) * alpha);
#else
    mState.setColor(r, g, b, (a / 255.0f) * (alpha / 255.0f));
#endif
    mState.disableTexture();

#ifdef USE_HWC2
    if (alpha == 1.0f) {
#else
    if (alpha == 0xFF) {
#endif
        glDisable(GL_BLEND);
    } else {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    }
}

void GLES20RenderEngine::setupLayerTexturing(const Texture& texture) {
    GLuint target = texture.getTextureTarget();
    glBindTexture(target, texture.getTextureName());
    GLenum filter = GL_NEAREST;
    if (texture.getFiltering()) {
        filter = GL_LINEAR;
    }
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);

    mState.setTexture(texture);
}

void GLES20RenderEngine::setupLayerBlackedOut() {
    glBindTexture(GL_TEXTURE_2D, mProtectedTexName);
    Texture texture(Texture::TEXTURE_2D, mProtectedTexName);
    texture.setDimensions(1, 1); // FIXME: we should get that from somewhere
    mState.setTexture(texture);
}

mat4 GLES20RenderEngine::setupColorTransform(const mat4& colorTransform) {
    mat4 oldTransform = mState.getColorMatrix();
    mState.setColorMatrix(colorTransform);
    return oldTransform;
}

void GLES20RenderEngine::disableTexturing() {
    mState.disableTexture();
}

void GLES20RenderEngine::disableBlending() {
    glDisable(GL_BLEND);
}


void GLES20RenderEngine::bindImageAsFramebuffer(EGLImageKHR image,
        uint32_t* texName, uint32_t* fbName, uint32_t* status) {
    GLuint tname, name;
    // turn our EGLImage into a texture
    glGenTextures(1, &tname);
    glBindTexture(GL_TEXTURE_2D, tname);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)image);

    // create a Framebuffer Object to render into
    glGenFramebuffers(1, &name);
    glBindFramebuffer(GL_FRAMEBUFFER, name);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tname, 0);

    *status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    *texName = tname;
    *fbName = name;
}

void GLES20RenderEngine::unbindFramebuffer(uint32_t texName, uint32_t fbName) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbName);
    glDeleteTextures(1, &texName);
}

void GLES20RenderEngine::setupFillWithColor(float r, float g, float b, float a) {
    mState.setPlaneAlpha(1.0f);
    mState.setPremultipliedAlpha(true);
    mState.setOpaque(false);
    mState.setColor(r, g, b, a);
    mState.disableTexture();
    glDisable(GL_BLEND);
}

//panjie add for to render offline buffer
void GLES20RenderEngine::beginGroup(const mat4& colorTransform, bool isGLES) {
    //GLuint tname, name;
    if (isGLES) {
        isToOfB = true;
    } else {
        isToOfB = false;
        return;
        //startFlag = true;
    }
    ALOGE("panjie beginGroup startFlag is:%d   ",startFlag);
    if (startFlag) {
            startFlag = false;
            //panjie init opengl env
            ogl_init();
            ogl_resize(/*mVpHeight,*//*2880*/mVpWidth, /*1704*/mVpHeight);
            ogl_create_background_default_texture(/*2880*/mVpWidth , /*1704*/mVpHeight/*, mVpWidth*/);
            //panjie end
            GLuint tname, name;
            // create the texture
            glGenTextures(1, &tname);
            glBindTexture(GL_TEXTURE_2D, tname);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mVpWidth, mVpHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            // create a Framebuffer Object to render into
            glGenFramebuffers(1, &name);
            ALOGE("panjie name is : %d      tname is :  %d   ",name,tname);
            glBindFramebuffer(GL_FRAMEBUFFER, name);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tname, 0);
        //}
        //Group group;
        group.texture = tname;
        group.fbo = name;
        group.width = mVpWidth;
        group.height = mVpHeight;
        group.colorTransform = colorTransform;
        if(GL_FALSE == glIsFramebuffer(mFboName)){
            ALOGE("GL_FALSE == glIsFramebuffer  mFboName = %d ", mFboName);
            glGenTextures(1, &mTexName);
            glBindTexture(GL_TEXTURE_2D, mTexName);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mVpWidth, mVpHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            //original fbo, delete it then we get error
            glGenFramebuffers(1, &mFboName);
        }
    }
    Group mGroup;
    mGroup.texture = mTexName;
    mGroup.fbo = mFboName;
    mGroup.width = mVpWidth;
    mGroup.height = mVpHeight;
    //group.colorTransform = colorTransform;
    mGroupStack.push(mGroup);
}

void GLES20RenderEngine::endGroup(int mode) {//增加int来判断当前是要渲染还是要清空buffer内容，这样就省去了clearFbo
    if (mode == 1) {
    ALOGE("panjie GLES20RenderEngine::endGroup mode is 1,fbo :%d,   texture:%d",group.fbo, group.texture);
    //const Group group(mGroupStack.top());
    mGroupStack.pop();
    // activate the previous render target
    GLuint fbo = 0;
    if (!mGroupStack.isEmpty()) {
        fbo = mGroupStack.top().fbo;//group.fbo;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // set our state
    Texture texture(Texture::TEXTURE_2D, group.texture);
    texture.setDimensions(group.width, group.height);
    ALOGE("panjie GLES20RenderEngine::endGroup   group.width:%d     group.height:%d",group.width, group.height);
    glBindTexture(GL_TEXTURE_2D, group.texture);
    mState.setPlaneAlpha(1.0f);
    mState.setPremultipliedAlpha(true);
    mState.setOpaque(false);
    mState.setTexture(texture);
    mState.setColorMatrix(group.colorTransform);
    glDisable(GL_BLEND);

    glClearColor(0,1,0,0);
    glClear(GL_COLOR_BUFFER_BIT);
    ogl_set_rect_texture_id(group.texture);
    //ogl_set_projection(90.0, 0.5, 120);//panjie set screen size and location
    //ogl_set_rect_size(300,480,100,160);
    ScreenResult out;
    float mat[16]={0};
    mat[0] = mat[5] =mat[10] = mat[15] = 1.0f;
    //getNumberMat(mat);//no binder get rotation number panjie
    ogl_update(out,mat);
    ogl_render(1);
    ALOGE("panjie end ogl render");
    //if (out.intersected) ALOGE("panjieEnable   x: %f          y : %f ",out.x,out.y);
    }

    if (mode == 2) {
        ALOGE("panjie GLES20RenderEngine::endGroup mode is    2 clear fbo");
        glBindFramebuffer(GL_FRAMEBUFFER, group.fbo);
        glClearColor(0,0,0,0);
        glClear(GL_COLOR_BUFFER_BIT);

        //After we clear fbo,we must bind the original fbo to current
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}

void GLES20RenderEngine::drawMesh(const Mesh& mesh) {

    //panjie add to choose render to offline
    if (isToOfB) {
        ALOGE("panjie GLES20RenderEngine::drawMesh isToOfB is true");
        glBindFramebuffer(GL_FRAMEBUFFER, group.fbo);
    } else {
        ALOGE("panjie GLES20RenderEngine::drawMesh isToOfB is false 23333");
    }
    ProgramCache::getInstance().useProgram(mState);

    if (mesh.getTexCoordsSize()) {
        glEnableVertexAttribArray(Program::texCoords);
        glVertexAttribPointer(Program::texCoords,
                mesh.getTexCoordsSize(),
                GL_FLOAT, GL_FALSE,
                mesh.getByteStride(),
                mesh.getTexCoords());
    }

    glVertexAttribPointer(Program::position,
            mesh.getVertexSize(),
            GL_FLOAT, GL_FALSE,
            mesh.getByteStride(),
            mesh.getPositions());

    glDrawArrays(mesh.getPrimitive(), 0, mesh.getVertexCount());

    if (mesh.getTexCoordsSize()) {
        glDisableVertexAttribArray(Program::texCoords);
    }
}

void GLES20RenderEngine::dump(String8& result) {
    RenderEngine::dump(result);
}

// ---------------------------------------------------------------------------
}; // namespace android
// ---------------------------------------------------------------------------

#if defined(__gl_h_)
#error "don't include gl/gl.h in this file"
#endif
