//
// Created by emile on 24/02/2020.
//

#include "renderer/qgl.h"

#define QGLPROC(name, rettype, args) rettype (GL_APIENTRYP q##name) args;
#include "renderer/qgl_proc.h"

void loadQGL( void )
{
// load qgl function pointers
#define QGLPROC(name, rettype, args) \
	q##name = (rettype(GL_APIENTRYP)args)GLimp_ExtensionPointer(#name);

#include "renderer/qgl_proc.h"

}