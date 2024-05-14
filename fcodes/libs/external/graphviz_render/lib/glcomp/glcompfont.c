/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: Details at https://graphviz.org
 *************************************************************************/

#include <cgraph/alloc.h>
#include <glcomp/glcompfont.h>
#include <glcomp/glcompset.h>
#include <glcomp/glpangofont.h>
#include <glcomp/glcomptexture.h>
#include <glcomp/glutils.h>
#include <GL/glut.h>
#include <stddef.h>

static void print_bitmap_string(void *font, char *s)
{
    if (s && strlen(s)) {
	while (*s) {
	    glutBitmapCharacter(font, *s);
	    s++;
	}
    }
}

void glprintfglut(void *font, GLfloat xpos, GLfloat ypos, GLfloat zpos,
		  char *bf)
{
    glRasterPos3f(xpos, ypos, zpos + 0.001f);
    print_bitmap_string(font, bf);


}

void glDeleteFont(glCompFont * f)
{
    free(f->fontdesc);
    if (f->tex)
	glCompDeleteTexture(f->tex);
    free(f);

}

glCompFont *glNewFont (glCompSet * s, char *text, glCompColor * c, char *fontdesc, int fs,int is2D)
{
    glCompFont *font = gv_alloc(sizeof(glCompFont));
    font->reference = 0;
    font->color.R = c->R;
    font->color.G = c->G;
    font->color.B = c->B;
    font->color.A = c->A;
    font->justify.VJustify = GL_FONTVJUSTIFY;
    font->justify.HJustify = GL_FONTHJUSTIFY;
    font->is2D=is2D;

    font->glutfont = NULL;

    font->fontdesc = gv_strdup(fontdesc);
    font->size = fs;
    font->transparent = 1;
    font->optimize = GL_FONTOPTIMIZE;
    if (text)
	font->tex =
	    glCompSetAddNewTexLabel(s, font->fontdesc, font->size, text,
				    is2D);
    return font;

}



glCompFont *glNewFontFromParent(glCompObj * o, char *text)
{
    glCompCommon *parent;
    glCompFont *font = gv_alloc(sizeof(glCompFont));
    parent = o->common.parent;
    if (parent) {
	parent = o->common.parent;
	font->reference = 1;
	font->color.R = parent->font->color.R;
	font->color.G = parent->font->color.G;
	font->color.B = parent->font->color.B;
	font->color.A = parent->font->color.A;

	font->glutfont = parent->font->glutfont;
	font->fontdesc = gv_strdup(parent->font->fontdesc);
	font->size = parent->font->size;
	font->transparent = parent->font->transparent;
	font->justify.VJustify = parent->font->justify.VJustify;
	font->justify.HJustify = parent->font->justify.HJustify;
	font->optimize = parent->font->optimize;
	font->is2D=parent->font->is2D;
	if (text) {
	    if (strlen(text))
		font->tex =
		    glCompSetAddNewTexLabel(parent->compset,
					    font->fontdesc, font->size,
					    text, parent->font->is2D);
	}
    } else {			/*no parent */

	glCompColor c;
	c.R = GLCOMPSET_FONT_COLOR_R;
	c.G = GLCOMPSET_FONT_COLOR_G;
	c.B = GLCOMPSET_FONT_COLOR_B;
	c.A = GLCOMPSET_FONT_COLOR_ALPHA;
	font = glNewFont(o->common.compset, text, &c,
		     GLCOMPSET_FONT_DESC, GLCOMPSET_FONT_SIZE,1);
    }
    return font;
}

/*texture base 3d text rendering*/
void glCompDrawText3D(glCompFont *f, GLfloat x, GLfloat y, double z, GLfloat w,
                      GLfloat h) {
	glEnable(GL_BLEND);		// Turn Blending On
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glBindTexture(GL_TEXTURE_2D,f->tex->id);
	glBegin(GL_QUADS);
		glTexCoord2d(0.0f, 1.0f);glVertex3d(x,y,z);
		glTexCoord2d(1.0f, 1.0f);glVertex3d(x+w,y,z);
		glTexCoord2d(1.0f, 0.0f);glVertex3d(x+w,y+h,z);
		glTexCoord2d(0.0f, 0.0f);glVertex3d(x,y+h,z);
	glEnd();

	glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);

}

void glCompDrawText(glCompFont * f,GLfloat x,GLfloat y)
{
    glRasterPos2f(x, y);
    glDrawPixels(f->tex->width, f->tex->height, GL_RGBA, GL_UNSIGNED_BYTE,  f->tex->data);
}

/*text rendering functions, depends on a globject to retrieve stats*/
void glCompRenderText(glCompFont * f, glCompObj * parentObj)
{
    GLfloat x, y;
    if (!f->tex)
	return;
    x = 0;
    y = 0;
    glCompCommon ref = parentObj->common;
    switch (f->justify.HJustify) 
    {
    case glFontHJustifyNone:
    case glFontHJustifyLeft:
	x = ref.refPos.x;
	break;
    case glFontHJustifyRight:
	x = ref.refPos.x + (ref.width - f->tex->width);
	break;
    case glFontHJustifyCenter:
	x = ref.refPos.x + (ref.width - f->tex->width) / 2.0f;
	break;
    }
    switch (f->justify.VJustify) {
    case glFontVJustifyNone:
    case glFontVJustifyBottom:
	y = ref.pos.y;
	break;
    case glFontVJustifyTop:
	x = ref.refPos.y + (ref.height - f->tex->height);
	break;
    case glFontVJustifyCenter:
	y = ref.refPos.y + (ref.height - f->tex->height) / 2.0f;
	break;
    }

    glCompSetColor(&f->color);
		glCompDrawText(f,x,y);

}
