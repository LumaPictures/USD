//
// Copyright 2019 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
//
#include "pxr/imaging/glf/glew.h"
#include "pxr/imaging/glf/contextCaps.h"
#include "pxr/imaging/glf/diagnostic.h"

#include "pxr/imaging/hdx/utils.h"

#include "pxr/base/gf/vec4i.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdxUtils {

GfVec2i
GetScreenSize()
{
    // XXX Ideally we want screenSize to be passed in via the app. 
    // (see Presto Stagecontext/TaskGraph), but for now we query this from GL.
    //
    // Using GL_VIEWPORT here (or viewport from RenderParams) is in-correct!
    //
    // The gl_FragCoord we use in the OIT shaders is relative to the FRAMEBUFFER 
    // size (screen size), not the gl_viewport size.
    // We do various tricks with glViewport for Presto slate mode so we cannot
    // rely on it to determine the 'screenWidth' we need in the gl shaders.
    // 
    // The CounterBuffer is especially fragile to this because in the glsl shdr
    // we calculate a 'screenIndex' based on gl_fragCoord that indexes into
    // the CounterBuffer. If we did not make enough room in the CounterBuffer
    // we may be reading/writing an invalid index into the CounterBuffer.
    //

    GfVec2i s;

    GLint attachType = 0;
    glGetFramebufferAttachmentParameteriv(
        GL_DRAW_FRAMEBUFFER, 
        GL_COLOR_ATTACHMENT0,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,
        &attachType);

    GLint attachId = 0;
    glGetFramebufferAttachmentParameteriv(
        GL_DRAW_FRAMEBUFFER, 
        GL_COLOR_ATTACHMENT0,
        GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
        &attachId);

    // XXX Fallback to gl viewport in case we do not find a non-default FBO for
    // bakends that do not attach a custom FB. This is in-correct, but gl does
    // not let us query size properties of default framebuffer. For this we
    // need the screenSize to be passed in via app (see note above)
    if (attachId<=0) {
        GfVec4i viewport;
        glGetIntegerv(GL_VIEWPORT, &viewport[0]);
        s[0] = viewport[2];
        s[1] = viewport[3];
        return s;
    }

    GlfContextCaps const &caps = GlfContextCaps::GetInstance();

    if (ARCH_LIKELY(caps.directStateAccessEnabled) && glGetTextureLevelParameteriv) {
        if (attachType == GL_TEXTURE) {
            glGetTextureLevelParameteriv(attachId, 0, GL_TEXTURE_WIDTH, &s[0]);
            glGetTextureLevelParameteriv(attachId, 0, GL_TEXTURE_HEIGHT, &s[1]);
        } else if (attachType == GL_RENDERBUFFER) {
            glGetNamedRenderbufferParameteriv(
                attachId, GL_RENDERBUFFER_WIDTH, &s[0]);
            glGetNamedRenderbufferParameteriv(
                attachId, GL_RENDERBUFFER_HEIGHT, &s[1]);
        }
    } else {
        if (attachType == GL_TEXTURE) {
            int oldBinding;
            // This is either a multisampled or a normal texture 2d.
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldBinding);
            // Clear out any pending errors before attempting to bind to
            // GL_TEXTURE_2D - that way, if it errors, we know it was this
            // call, and we assume it's because it's actually a MULTISAMPLE
            // texture - this is ugly, but we don't know of any better way
            // to check this!
            GLF_POST_PENDING_GL_ERRORS();
            glBindTexture(GL_TEXTURE_2D, attachId);
            if (glGetError() != GL_NO_ERROR) {
                glBindTexture(GL_TEXTURE_2D, oldBinding);
                glGetIntegerv(GL_TEXTURE_BINDING_2D_MULTISAMPLE, &oldBinding);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, attachId);
                glGetTexLevelParameteriv(
                    GL_TEXTURE_2D_MULTISAMPLE, 0, GL_TEXTURE_WIDTH, &s[0]);
                glGetTexLevelParameteriv(
                    GL_TEXTURE_2D_MULTISAMPLE, 0, GL_TEXTURE_HEIGHT, &s[1]);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, oldBinding);
            } else {
                glGetTexLevelParameteriv(
                    GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &s[0]);
                glGetTexLevelParameteriv(
                    GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &s[1]);
                glBindTexture(GL_TEXTURE_2D, oldBinding);
            }
            
        } else if (attachType == GL_RENDERBUFFER) {
            int oldBinding;
            glGetIntegerv(GL_RENDERBUFFER_BINDING, &oldBinding);
            glBindRenderbuffer(GL_RENDERBUFFER, attachId);
            glGetRenderbufferParameteriv(
                GL_RENDERBUFFER,GL_RENDERBUFFER_WIDTH,&s[0]);
            glGetRenderbufferParameteriv(
                GL_RENDERBUFFER,GL_RENDERBUFFER_HEIGHT,&s[1]);
            glBindRenderbuffer(GL_RENDERBUFFER, oldBinding);
        }
    }

    return s;
}

}

PXR_NAMESPACE_CLOSE_SCOPE