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

    if (attachType == GL_TEXTURE) {
        glGetTextureLevelParameteriv(attachId, 0, GL_TEXTURE_WIDTH, &s[0]);
        glGetTextureLevelParameteriv(attachId, 0, GL_TEXTURE_HEIGHT, &s[1]);
    } else if (attachType == GL_RENDERBUFFER) {
        glGetNamedRenderbufferParameteriv(attachId,GL_RENDERBUFFER_WIDTH,&s[0]);
        glGetNamedRenderbufferParameteriv(attachId,GL_RENDERBUFFER_HEIGHT,&s[1]);
    } else {
        TF_CODING_ERROR("Unknown framebuffer attachment");
        return s;
    }

    return s;
}

}

PXR_NAMESPACE_CLOSE_SCOPE
