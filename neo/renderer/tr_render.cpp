/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "renderer/VertexCache.h"
#include "renderer/Cinematic.h"

#include "renderer/tr_local.h"

/*

  back end scene + lights rendering functions

*/

float RB_overbright = 1;

#include <arm_neon.h>

const int MAXIMUM_SHORTS_RENDER_INDICES = 1024 * 64;

static void RB_DrawElementsWithConversion( GLenum mode, GLsizei count, const int *indices ){

	static short shorts[MAXIMUM_SHORTS_RENDER_INDICES];

	GLenum type = GL_UNSIGNED_INT; // Default
	void *indicesRender = (void *)indices;

	if( glConfig.useShortIndexElements ){

        if(count >= MAXIMUM_SHORTS_RENDER_INDICES) // Just skip whole render if too many..
            return;

		type = GL_UNSIGNED_SHORT;
        indicesRender = shorts;

        // NOTE! This does not check if the 32bit integer is out of range for a short, so if it is it will produce corrupt rendering
#if 1
		int i = 0;
		for (; i < count - 3; i += 4) {
			int32x4_t v1 = vld1q_s32(&indices[i]); // Load four 32-bit integers
			int16x4_t v2 = vqmovn_s32(v1); // Narrow to four 16-bit integers
			vst1_s16(&shorts[i], v2); // Store four 16-bit integers
		}
		// Handle remaining elements
		for (; i < count; ++i) {
			shorts[i] = (int16_t)indices[i];
		}
#else
		for( int n = 0; n < count; n++) {
			shorts[n] = indices[n];
		}
#endif
	}

    // Draw!!!
	qglDrawElements( GL_TRIANGLES, count, type, indicesRender );
}

/*
================
RB_DrawElementsWithCounters
================
*/
void RB_DrawElementsWithCounters( const drawSurf_t *surf ) {

	backEnd.pc.c_drawElements++;

	if ( surf->indexCache ) {
		RB_DrawElementsWithConversion( GL_TRIANGLES, surf->numIndexes,  (int *)vertexCache.Position( surf->indexCache ) );
		backEnd.pc.c_vboIndexes += surf->numIndexes;
	} else {
		static bool bOnce = true;
		if (bOnce) {
			common->Warning("Attempting to draw without index caching. This is a bug.\n");
			bOnce = false;
		}
	}
}

/*
================
RB_DrawShadowElementsWithCounters

May not use all the indexes in the surface if caps are skipped
================
*/
void RB_DrawShadowElementsWithCounters( const drawSurf_t *surf , int numIndexes ) {

	backEnd.pc.c_shadowElements++;

	if ( surf->indexCache ) {
        RB_DrawElementsWithConversion( GL_TRIANGLES, numIndexes, (int *)vertexCache.Position( surf->indexCache ) );
		backEnd.pc.c_vboIndexes += numIndexes;
	} else {
		static bool bOnce = true;
		if (bOnce) {
			common->Warning("Attempting to draw without index caching. This is a bug.\n");
			bOnce = false;
		}
	}
}

/*
======================
RB_GetShaderTextureMatrix
======================
*/
void RB_GetShaderTextureMatrix( const float *shaderRegisters,
							   const textureStage_t *texture, float matrix[16] ) {
	matrix[0] = shaderRegisters[ texture->matrix[0][0] ];
	matrix[4] = shaderRegisters[ texture->matrix[0][1] ];
	matrix[8] = 0;
	matrix[12] = shaderRegisters[ texture->matrix[0][2] ];

	// we attempt to keep scrolls from generating incredibly large texture values, but
	// center rotations and center scales can still generate offsets that need to be > 1
	if ( matrix[12] < -40 || matrix[12] > 40 ) {
		matrix[12] -= (int)matrix[12];
	}

	matrix[1] = shaderRegisters[ texture->matrix[1][0] ];
	matrix[5] = shaderRegisters[ texture->matrix[1][1] ];
	matrix[9] = 0;
	matrix[13] = shaderRegisters[ texture->matrix[1][2] ];
	if ( matrix[13] < -40 || matrix[13] > 40 ) {
		matrix[13] -= (int)matrix[13];
	}

	matrix[2] = 0;
	matrix[6] = 0;
	matrix[10] = 1;
	matrix[14] = 0;

	matrix[3] = 0;
	matrix[7] = 0;
	matrix[11] = 0;
	matrix[15] = 1;
}

/*
======================
RB_BindVariableStageImage

Handles generating a cinematic frame if needed
======================
*/
void RB_BindVariableStageImage( const textureStage_t *texture, const float *shaderRegisters ) {
	if ( texture->cinematic ) {

		if ( r_skipDynamicTextures.GetBool() ) {
			globalImages->defaultImage->Bind();
			return;
		}

		// For multithreading. Images will be 1 fame behind..oh well
		if ( texture->image ) {
			// The first time the image will be invalid so wont bind, so bind black image
			if ( texture->image->Bind() == false )
			{
				globalImages->blackImage->Bind();
			}
			// Save time to display
			texture->image->cinmaticNextTime = (int)(1000 * ( backEnd.viewDef->floatTime + backEnd.viewDef->renderView.shaderParms[11] ) );
			// Update next time
			globalImages->AddAllocList( texture->image );
		}
	} else {
		//FIXME: see why image is invalid
		if (texture->image) {
			texture->image->Bind();
		}
	}
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView (void) {

	// set the window clipping
	qglViewport( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1,
				 tr.viewportOffset[1] + backEnd.viewDef->viewport.y1,
				 backEnd.viewDef->viewport.x2 + 1 - backEnd.viewDef->viewport.x1,
				 backEnd.viewDef->viewport.y2 + 1 - backEnd.viewDef->viewport.y1 );

	// the scissor may be smaller than the viewport for subviews
	qglScissor( tr.viewportOffset[0] + backEnd.viewDef->viewport.x1 + backEnd.viewDef->scissor.x1,
				tr.viewportOffset[1] + backEnd.viewDef->viewport.y1 + backEnd.viewDef->scissor.y1,
				backEnd.viewDef->scissor.x2 + 1 - backEnd.viewDef->scissor.x1,
				backEnd.viewDef->scissor.y2 + 1 - backEnd.viewDef->scissor.y1 );
	backEnd.currentScissor = backEnd.viewDef->scissor;

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );

	// we don't have to clear the depth / stencil buffer for 2D rendering
	if ( backEnd.viewDef->viewEntitys ) {
		qglStencilMask( 0xff );
		// some cards may have 7 bit stencil buffers, so don't assume this
		// should be 128
		qglClearStencil( 1<<(glConfig.stencilBits-1) );
		qglClear( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
		qglEnable( GL_DEPTH_TEST );
	} else {
		qglDisable( GL_DEPTH_TEST );
		qglDisable( GL_STENCIL_TEST );
	}

	backEnd.glState.faceCulling = -1;		// force face culling to set next time
	GL_Cull( CT_FRONT_SIDED );

}

/*
==================
RB_SetDrawInteractions
==================
*/
void RB_SetDrawInteraction( const shaderStage_t *surfaceStage, const float *surfaceRegs,
						  idImage **image, idVec4 matrix[2], float color[4] ) {
	*image = surfaceStage->texture.image;
	if ( surfaceStage->texture.hasMatrix ) {
		matrix[0][0] = surfaceRegs[surfaceStage->texture.matrix[0][0]];
		matrix[0][1] = surfaceRegs[surfaceStage->texture.matrix[0][1]];
		matrix[0][2] = 0;
		matrix[0][3] = surfaceRegs[surfaceStage->texture.matrix[0][2]];

		matrix[1][0] = surfaceRegs[surfaceStage->texture.matrix[1][0]];
		matrix[1][1] = surfaceRegs[surfaceStage->texture.matrix[1][1]];
		matrix[1][2] = 0;
		matrix[1][3] = surfaceRegs[surfaceStage->texture.matrix[1][2]];

		// we attempt to keep scrolls from generating incredibly large texture values, but
		// center rotations and center scales can still generate offsets that need to be > 1
		if ( matrix[0][3] < -40 || matrix[0][3] > 40 ) {
			matrix[0][3] -= (int)matrix[0][3];
		}
		if ( matrix[1][3] < -40 || matrix[1][3] > 40 ) {
			matrix[1][3] -= (int)matrix[1][3];
		}
	} else {
		matrix[0][0] = 1;
		matrix[0][1] = 0;
		matrix[0][2] = 0;
		matrix[0][3] = 0;

		matrix[1][0] = 0;
		matrix[1][1] = 1;
		matrix[1][2] = 0;
		matrix[1][3] = 0;
	}

	if ( color ) {
		for ( int i = 0 ; i < 4 ; i++ ) {
			color[i] = surfaceRegs[surfaceStage->color.registers[i]];
			// clamp here, so card with greater range don't look different.
			// we could perform overbrighting like we do for lights, but
			// it doesn't currently look worth it.
			if ( color[i] < 0 ) {
				color[i] = 0;
			} else if ( color[i] > 1.0 ) {
				color[i] = 1.0;
			}
		}
	}
}

/*
=================
RB_SubmittInteraction
=================
*/
void RB_SubmittInteraction( drawInteraction_t *din, void (*DrawInteraction)(const drawInteraction_t *) ) {
	if ( !din->bumpImage ) {
		return;
	}

	if ( !din->diffuseImage || r_skipDiffuse.GetBool() ) {
		din->diffuseImage = globalImages->blackImage;
	}
	if ( !din->specularImage || r_skipSpecular.GetBool() || din->ambientLight ) {
		din->specularImage = globalImages->blackImage;
	}
	if ( !din->bumpImage || r_skipBump.GetBool() ) {
		din->bumpImage = globalImages->flatNormalMap;
	}

	// if we wouldn't draw anything, don't call the Draw function
	if (
		( ( din->diffuseColor[0] > 0 ||
		din->diffuseColor[1] > 0 ||
		din->diffuseColor[2] > 0 ) && din->diffuseImage != globalImages->blackImage )
		|| ( ( din->specularColor[0] > 0 ||
		din->specularColor[1] > 0 ||
		din->specularColor[2] > 0 ) && din->specularImage != globalImages->blackImage ) ) {

				din->diffuseColor[0] *= RB_overbright;
				din->diffuseColor[1] *= RB_overbright;
				din->diffuseColor[2] *= RB_overbright;
		DrawInteraction( din );
	}
}

/*
=============
RB_DrawView
=============
*/
void RB_DrawView( const void *data ) {
	const drawSurfsCommand_t	*cmd;

	cmd = (const drawSurfsCommand_t *)data;

	// with r_lockSurfaces enabled, we set the locked render view
	// for the primary viewDef for all the "what should be drawn" calculations.
	// now it must be reverted to the real render view so the scene gets rendered
	// from the actual current players point of view
	if(r_lockSurfaces.GetBool() && tr.primaryView == cmd->viewDef) {
		viewDef_t* parms = cmd->viewDef;
		const viewDef_t origParms = *parms;

		*parms = tr.lockSurfacesRealViewDef; // actual current player/camera position
		parms->renderWorld = origParms.renderWorld;
		parms->floatTime = origParms.floatTime;
		parms->drawSurfs = origParms.drawSurfs;
		parms->numDrawSurfs = origParms.numDrawSurfs;
		parms->maxDrawSurfs = origParms.maxDrawSurfs;
		parms->viewLights = origParms.viewLights;
		parms->viewEntitys = origParms.viewEntitys;
		parms->connectedAreas = origParms.connectedAreas;

		for( viewEntity_t* vModel = parms->viewEntitys ; vModel ; vModel = vModel->next ) {
			myGlMultMatrix( vModel->modelMatrix,
				parms->worldSpace.modelViewMatrix,
				vModel->modelViewMatrix );
		}
	}

	backEnd.viewDef = cmd->viewDef;

	// we will need to do a new copyTexSubImage of the screen
	// when a SS_POST_PROCESS material is used
	backEnd.currentRenderCopied = false;

	// if there aren't any drawsurfs, do nothing
	if ( !backEnd.viewDef->numDrawSurfs ) {
		return;
	}

	// skip render bypasses everything that has models, assuming
	// them to be 3D views, but leaves 2D rendering visible
	if ( r_skipRender.GetBool() && backEnd.viewDef->viewEntitys ) {
		return;
	}

	backEnd.pc.c_surfaces += backEnd.viewDef->numDrawSurfs;

	// render the scene, jumping to the hardware specific interaction renderers
	RB_RenderView();
}
