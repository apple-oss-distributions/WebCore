/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(WEBGL)

#include "GraphicsContext3D.h"
#include "GraphicsContext3DIOS.h"

#import "BlockExceptions.h"

#include "ANGLE/ShaderLang.h"
#include "CanvasRenderingContext.h"
#include <CoreGraphics/CGBitmapContext.h>
#include "Extensions3DOpenGL.h"
#include "GraphicsContext.h"
#include "HTMLCanvasElement.h"
#include "ImageBuffer.h"
#include "NotImplemented.h"
#import <OpenGLES/ES2/glext.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>
#import <QuartzCore/QuartzCore.h>
#include "WebGLLayer.h"
#include "WebGLObject.h"
#include <wtf/ArrayBuffer.h>
#include <wtf/ArrayBufferView.h>
#include <wtf/Int32Array.h>
#include <wtf/Float32Array.h>
#include <wtf/Uint8Array.h>
#include <wtf/UnusedParam.h>
#include <wtf/text/CString.h>

namespace WebCore {

// FIXME: This class is currently empty on Mac, but will get populated as 
// the restructuring in https://bugs.webkit.org/show_bug.cgi?id=66903 is done
class GraphicsContext3DPrivate {
public:
    GraphicsContext3DPrivate(GraphicsContext3D* graphicsContext3D)
        : m_graphicsContext3D(graphicsContext3D)
    {
    }
    
    ~GraphicsContext3DPrivate() { }

private:
    GraphicsContext3D* m_graphicsContext3D; // Weak back-pointer
};


PassRefPtr<GraphicsContext3D> GraphicsContext3D::create(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
{
    // This implementation doesn't currently support rendering directly to the HostWindow.
    if (renderStyle == RenderDirectlyToHostWindow)
        return 0;
    RefPtr<GraphicsContext3D> context = adoptRef(new GraphicsContext3D(attrs, hostWindow, false));
    return context->m_contextObj ? context.release() : 0;
}

GraphicsContext3D::GraphicsContext3D(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, bool)
    : m_currentWidth(0)
    , m_currentHeight(0)
    , m_contextObj(0)
    , m_attrs(attrs)
    , m_texture(0)
    , m_compositorTexture(0)
    , m_fbo(0)
    , m_depthStencilBuffer(0)
    , m_layerComposited(false)
    , m_internalColorFormat(0)
    , m_boundFBO(0)
    , m_activeTexture(0)
    , m_boundTexture0(0)
    , m_multisampleFBO(0)
    , m_multisampleDepthStencilBuffer(0)
    , m_multisampleColorBuffer(0)
    , m_private(adoptPtr(new GraphicsContext3DPrivate(this)))
{
    UNUSED_PARAM(hostWindow);

    m_contextObj = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    makeContextCurrent();
    
    validateAttributes();

    // Create the WebGLLayer
    BEGIN_BLOCK_OBJC_EXCEPTIONS
        m_webGLLayer.adoptNS([[WebGLLayer alloc] initWithGraphicsContext3D:this]);
        [m_webGLLayer.get() setOpaque:0];
#ifndef NDEBUG
        [m_webGLLayer.get() setName:@"WebGL Layer"];
#endif    
    END_BLOCK_OBJC_EXCEPTIONS
    
    ::glGenRenderbuffers(1, &m_texture);
    
    // create an FBO
    ::glGenFramebuffersEXT(1, &m_fbo);
    ::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
    
    m_boundFBO = m_fbo;
    if (!m_attrs.antialias && (m_attrs.stencil || m_attrs.depth))
        ::glGenRenderbuffersEXT(1, &m_depthStencilBuffer);

    // create an multisample FBO
    if (m_attrs.antialias) {
        ::glGenFramebuffersEXT(1, &m_multisampleFBO);
        ::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_multisampleFBO);
        m_boundFBO = m_multisampleFBO;
        ::glGenRenderbuffersEXT(1, &m_multisampleColorBuffer);
        if (m_attrs.stencil || m_attrs.depth)
            ::glGenRenderbuffersEXT(1, &m_multisampleDepthStencilBuffer);
    }
    
    // ANGLE initialization.

    ShBuiltInResources ANGLEResources;
    ShInitBuiltInResources(&ANGLEResources);

    getIntegerv(GraphicsContext3D::MAX_VERTEX_ATTRIBS, &ANGLEResources.MaxVertexAttribs);
    getIntegerv(GraphicsContext3D::MAX_VERTEX_UNIFORM_VECTORS, &ANGLEResources.MaxVertexUniformVectors);
    getIntegerv(GraphicsContext3D::MAX_VARYING_VECTORS, &ANGLEResources.MaxVaryingVectors);
    getIntegerv(GraphicsContext3D::MAX_VERTEX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxVertexTextureImageUnits);
    getIntegerv(GraphicsContext3D::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxCombinedTextureImageUnits);
    getIntegerv(GraphicsContext3D::MAX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxTextureImageUnits);
    getIntegerv(GraphicsContext3D::MAX_FRAGMENT_UNIFORM_VECTORS, &ANGLEResources.MaxFragmentUniformVectors);

    // Always set to 1 for OpenGL ES.
    ANGLEResources.MaxDrawBuffers = 1;
    
    m_compiler.setResources(ANGLEResources);
    

    ::glClearColor(0, 0, 0, 0);
}

GraphicsContext3D::~GraphicsContext3D()
{
    if (m_contextObj) {
        makeContextCurrent();
        ::glDeleteRenderbuffers(1, &m_texture);
        ::glDeleteRenderbuffers(1, &m_compositorTexture);
        if (m_attrs.antialias) {
            ::glDeleteRenderbuffersEXT(1, &m_multisampleColorBuffer);
            if (m_attrs.stencil || m_attrs.depth)
                ::glDeleteRenderbuffersEXT(1, &m_multisampleDepthStencilBuffer);
            ::glDeleteFramebuffersEXT(1, &m_multisampleFBO);
        } else {
            if (m_attrs.stencil || m_attrs.depth)
                ::glDeleteRenderbuffersEXT(1, &m_depthStencilBuffer);
        }
        ::glDeleteFramebuffersEXT(1, &m_fbo);
        [EAGLContext setCurrentContext:0];
        [static_cast<EAGLContext*>(m_contextObj) release];
    }
}

bool GraphicsContext3D::setRenderbufferStorageFromDrawable(GC3Dsizei width, GC3Dsizei height)
{
    [m_webGLLayer.get() setBounds:CGRectMake(0, 0, width, height)];
    return [m_contextObj renderbufferStorage:GL_RENDERBUFFER fromDrawable:static_cast<NSObject<EAGLDrawable>*>(m_webGLLayer.get())];
}

bool GraphicsContext3D::makeContextCurrent()
{
    if (!m_contextObj)
        return false;

    if ([EAGLContext currentContext] != m_contextObj)
        return [EAGLContext setCurrentContext:static_cast<EAGLContext*>(m_contextObj)];
    return true;
}

void GraphicsContext3D::endPaint()
{
    makeContextCurrent();
    ::glFlush();
    ::glBindRenderbuffer(GL_RENDERBUFFER, m_texture);
    [static_cast<EAGLContext*>(m_contextObj) presentRenderbuffer:GL_RENDERBUFFER];
    [EAGLContext setCurrentContext:nil];
}

bool GraphicsContext3D::isGLES2Compliant() const
{
    return false;
}

void GraphicsContext3D::setContextLostCallback(PassOwnPtr<ContextLostCallback>)
{
}

void GraphicsContext3D::setErrorMessageCallback(PassOwnPtr<ErrorMessageCallback>)
{
}

}

#endif // ENABLE(WEBGL)
