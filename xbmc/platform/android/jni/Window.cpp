/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Window.h"
#include "WindowManager.h"
#include "View.h"

#include "jutils/jutils-details.hpp"

using namespace jni;

CJNIWindowManagerLayoutParams CJNIWindow::getAttributes()
{
  return call_method<jhobject>(m_object,
    "getAttributes", "()Landroid/view/WindowManager$LayoutParams;");
}

void CJNIWindow::setAttributes(const CJNIWindowManagerLayoutParams& attributes)
{
  call_method<void>(m_object,
                    "setAttributes", "(Landroid/view/WindowManager$LayoutParams;)V",
                    attributes.get_raw());


  if (xbmc_jnienv()->ExceptionCheck())
    xbmc_jnienv()->ExceptionClear();
}

CJNIView CJNIWindow::getDecorView()
{
  return call_method<jhobject>(m_object,
    "getDecorView", "()Landroid/view/View;");
}
