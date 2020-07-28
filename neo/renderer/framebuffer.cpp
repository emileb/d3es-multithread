/*
Emile Belanger
GPL3
*/
#include "renderer/tr_local.h"

static GLuint m_framebuffer;
static GLuint m_depthbuffer;
static int m_framebuffer_width, m_framebuffer_height;
static GLuint m_framebuffer_texture;

static int fixNpot(int v)
{
	return v;
}

void R_InitFrameBuffer(int width, int height, bool npot)
{
	m_framebuffer_width = width;
	m_framebuffer_height = height;
	
	if (!npot)
	{
		m_framebuffer_width = fixNpot(m_framebuffer_width);
		m_framebuffer_height = fixNpot(m_framebuffer_height);
	}
	
	// Create texture
	glGenTextures(1, &m_framebuffer_texture);
	glBindTexture(GL_TEXTURE_2D, m_framebuffer_texture);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_framebuffer_width, m_framebuffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	// Create renderbuffer
	glGenRenderbuffers(1, &m_depthbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_depthbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, m_framebuffer_width, m_framebuffer_height);
	
	// Create framebuffer
	glGenFramebuffers(1, &m_framebuffer);
}

void R_FrameBufferStart()
{
	// Render to framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, m_depthbuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_framebuffer_texture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthbuffer);

    GLenum result = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(result != GL_FRAMEBUFFER_COMPLETE)
	{
	    common->Error( "Error binding Framebuffer: %d\n", result );
	}
}

