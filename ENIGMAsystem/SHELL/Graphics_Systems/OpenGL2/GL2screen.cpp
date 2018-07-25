/** Copyright (C) 2008-2014 Josh Ventura, Harijs Grinbergs, Robert B. Colton
***
*** This file is a part of the ENIGMA Development Environment.
***
*** ENIGMA is free software: you can redistribute it and/or modify it under the
*** terms of the GNU General Public License as published by the Free Software
*** Foundation, version 3 of the license or any later version.
***
*** This application and its source code is distributed AS-IS, WITHOUT ANY
*** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
*** details.
***
*** You should have received a copy of the GNU General Public License along
*** with this code. If not, see <http://www.gnu.org/licenses/>
**/

#include "Bridges/General/GL2Context.h"
#include "Graphics_Systems/General/GStextures.h"
#include "Graphics_Systems/General/GSscreen.h"
#include "Graphics_Systems/General/GSmatrix.h"
#include "Graphics_Systems/General/GScolors.h"
#include "OpenGLHeaders.h"

#include "Universal_System/image_formats.h"
#include "Universal_System/roomsystem.h"
#include "Platforms/General/PFwindow.h"

#include <string>
#include <cstdio>

//WE SHOULDN'T DO THIS! Don't specify namespaces like this - Harijs
using namespace std;

using namespace enigma;
using namespace enigma_user;

namespace enigma
{
extern GLuint msaa_fbo;
unsigned int bound_framebuffer = 0; //Shows the bound framebuffer, so glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &fbo); don't need to be called (they are very slow)
int viewport_x, viewport_y, viewport_w, viewport_h; //These are used by surfaces, to set back the viewport

void scene_begin() {
  //oglmgr->EndShapesBatching(); //If called inside bound surface we need to finish drawing
  oglmgr->BeginScene();
}

void scene_end() {
  oglmgr->EndScene();

  if (enigma::msaa_fbo != 0) {
    GLint fbo;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
    //glBindFramebuffer(GL_READ_FRAMEBUFFER, enigma::msaa_fbo); TODO: draw me
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //TODO: Change the code below to fix this to size properly to views
    //TODO: draw fullscreen quad
    //glBlitFramebuffer(0, 0, window_get_region_width_scaled(), window_get_region_height_scaled(), 0, 0, window_get_region_width_scaled(), window_get_region_height_scaled(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  }
}

}

namespace enigma_user
{

void screen_init()
{
  oglmgr->EndShapesBatching();
  enigma::gui_width = window_get_region_width();
  enigma::gui_height = window_get_region_height();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (!view_enabled)
  {
    glClearColor(0,0,0,0);
    screen_set_viewport(0, 0, window_get_region_width(), window_get_region_height());
    d3d_set_projection_ortho(0, 0, room_width, room_height, 0);
  } else {
    for (view_current = 0; view_current < 7; view_current++)
    {
      if (view_visible[(int)view_current])
      {
        int vc = (int)view_current;

        glClearColor(0,0,0,0);

        screen_set_viewport(view_xport[vc], view_yport[vc], view_wport[vc], view_hport[vc]);
        d3d_set_projection_ortho(view_xview[vc], view_yview[vc], view_wview[vc], view_hview[vc], view_angle[vc]);
        break;
      }
    }
  }

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glEnable(GL_SCISSOR_TEST);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  texture_reset();
  draw_set_color(c_white);
}

int screen_save(string filename) //Assumes native integers are little endian
{
  oglmgr->EndShapesBatching();
  unsigned int w=window_get_width(),h=window_get_height(),sz=w*h;

  string ext = enigma::image_get_format(filename);

  unsigned char *rgbdata = new unsigned char[sz*4];
  GLint prevFbo;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadPixels(0,0,w,h, GL_BGRA, GL_UNSIGNED_BYTE, rgbdata);
  glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);

  int ret = image_save(filename, rgbdata, w, h, w, h, false);

  delete[] rgbdata;
  return ret;
}

int screen_save_part(string filename,unsigned x,unsigned y,unsigned w,unsigned h) //Assumes native integers are little endian
{
  oglmgr->EndShapesBatching();
  unsigned sz = w*h;

  string ext = enigma::image_get_format(filename);

  unsigned char *rgbdata = new unsigned char[sz*4];
  GLint prevFbo;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glReadPixels(x,window_get_region_height_scaled()-h-y,w,h, GL_BGRA, GL_UNSIGNED_BYTE, rgbdata);
  glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);

  int ret = image_save(filename, rgbdata, w, h, w, h, false);

  delete[] rgbdata;
  return ret;
}

void screen_set_viewport(gs_scalar x, gs_scalar y, gs_scalar width, gs_scalar height) {
  x = (x / window_get_region_width()) * window_get_region_width_scaled();
  y = (y / window_get_region_height()) * window_get_region_height_scaled();
  width = (width / window_get_region_width()) * window_get_region_width_scaled();
  height = (height / window_get_region_height()) * window_get_region_height_scaled();
  gs_scalar sx, sy;
  sx = (window_get_width() - window_get_region_width_scaled()) / 2;
  sy = (window_get_height() - window_get_region_height_scaled()) / 2;
  viewport_x = sx + x;
  viewport_y = window_get_height() - (sy + y) - height;
  viewport_w = width;
  viewport_h = height;
  //NOTE: OpenGL viewports are bottom left unlike Direct3D viewports which are top left
  glViewport(viewport_x, viewport_y, viewport_w, viewport_h);
  glScissor(viewport_x, viewport_y, viewport_w, viewport_h);
}

//TODO: These need to be in some kind of General
void display_set_gui_size(unsigned int width, unsigned int height) {
  enigma::gui_width = width;
  enigma::gui_height = height;
}

}
