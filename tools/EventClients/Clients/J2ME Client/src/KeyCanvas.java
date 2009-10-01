/*
* XBMC Media Center
* UDP Event Server
* Copyright (c) 2008 topfs2
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
import javax.microedition.lcdui.*;

public class KeyCanvas extends Canvas
{
  private KeyHandler m_SendKey = null;
  private Image Cover = null;
  private boolean SetCover = false;

  public KeyCanvas(KeyHandler sendKey)
  {
    m_SendKey = sendKey;
    try
    {
//      Cover = Image.createImage("home.png");
    }
    catch (Exception e)
    { }
  }

  public void SetCover(Image newCover)
  {
    if (newCover != null)
    {
      SetCover = true;
      Cover = newCover;
    }
  }

  public void paint(Graphics g)
  {
    g.drawRect(0, 0, this.getWidth(), this.getHeight());
    if (Cover != null)
      g.drawImage(Cover, 0, 0, g.TOP|g.LEFT);
  }

  public void keyPressed(int keyCode)
  {
    if (m_SendKey != null)
      m_SendKey.keyPressed(keyCode);
  }

  public void keyRepeated(int keyCode)
  {
    if (m_SendKey != null)
      m_SendKey.keyPressed(keyCode);
  }
}
