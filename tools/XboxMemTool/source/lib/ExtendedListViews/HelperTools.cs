
//  © Copyright 2002, Pierre ARNAUD, OPaC bright ideas, Switzerland.
//    All rights reserved.

namespace SynapticEffect.Forms
{
	public class HelperTools
	{
		private HelperTools() { }
		
		static public System.Drawing.SizeF MeasureDisplayString(System.Drawing.Graphics graphics, string text, System.Drawing.Font font)
		{
			const int width = 32;
			
			System.Drawing.Bitmap   bitmap = new System.Drawing.Bitmap (width, 1, graphics);
			System.Drawing.SizeF    size   = graphics.MeasureString (text, font);
			System.Drawing.Graphics anagra = System.Drawing.Graphics.FromImage (bitmap);
			
			int measured_width = (int) size.Width;
			
			if (anagra != null) 
			{
				anagra.Clear (System.Drawing.Color.White);
				anagra.DrawString (text+"|", font, System.Drawing.Brushes.Black, width - measured_width, -font.Height / 2);
				
				for (int i = width-1; i >= 0; i--)
				{
					measured_width--;
					if (bitmap.GetPixel (i, 0).R == 0) 
					{
						break;
					}
				}
			}
			
			return new System.Drawing.SizeF (measured_width, size.Height);
		}
		
		static public int MeasureDisplayStringWidth(System.Drawing.Graphics graphics, string text, System.Drawing.Font font)
		{
			return (int) MeasureDisplayString (graphics, text, font).Width;
		}	
	}
}
