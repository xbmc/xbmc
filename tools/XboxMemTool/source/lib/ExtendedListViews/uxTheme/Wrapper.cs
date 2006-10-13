using System;
using System.Runtime.InteropServices;

namespace Lyquidity.Controls.ExtendedListViews.uxTheme
{
	public class Wrapper
	{
		private Wrapper() { }
		
		[ DllImport("OPaC.uxTheme.Win32.dll", CharSet=System.Runtime.InteropServices.CharSet.Unicode, EntryPoint="_Wrapper_IsAppThemed@0") ]
		public static extern bool IsAppThemed();
		
		[ DllImport("OPaC.uxTheme.Win32.dll", CharSet=System.Runtime.InteropServices.CharSet.Unicode, EntryPoint="_Wrapper_DrawBackground@48") ]
		public static extern bool DrawBackground(string name, string part, string state, System.IntPtr hdc, int ox, int oy, int dx, int dy, int clip_ox, int clip_oy, int clip_dx, int clip_dy);
		
		[ DllImport("OPaC.uxTheme.Win32.dll", CharSet=System.Runtime.InteropServices.CharSet.Unicode, EntryPoint="_Wrapper_DrawTabPageBackground@36") ]
		public static extern bool DrawTabPageBackground(System.IntPtr hdc, int ox, int oy, int dx, int dy, int clip_ox, int clip_oy, int clip_dx, int clip_dy);
		
		[ DllImport("OPaC.uxTheme.Win32.dll", CharSet=System.Runtime.InteropServices.CharSet.Unicode, EntryPoint="_Wrapper_DrawGroupBoxBackground@36") ]
		public static extern bool DrawGroupBoxBackground(System.IntPtr hdc, int ox, int oy, int dx, int dy, int clip_ox, int clip_oy, int clip_dx, int clip_dy);
		
		[ DllImport("OPaC.uxTheme.Win32.dll", CharSet=System.Runtime.InteropServices.CharSet.Unicode, EntryPoint="_Wrapper_DrawThemeParentBackground@8") ]
		public static extern bool DrawThemeParentBackground(System.IntPtr hwnd, System.IntPtr hdc);
		
		[ DllImport("OPaC.uxTheme.Win32.dll", CharSet=System.Runtime.InteropServices.CharSet.Unicode, EntryPoint="_Wrapper_DrawThemeParentBackgroundRect@24") ]
		public static extern bool DrawThemeParentBackground(System.IntPtr hwnd, System.IntPtr hdc, int ox, int oy, int dx, int dy);
		
		[ DllImport("OPaC.uxTheme.Win32.dll", CharSet=System.Runtime.InteropServices.CharSet.Unicode, EntryPoint="_Wrapper_GetTextColor@24") ]
		public static extern bool GetTextColor(string name, string part, string state, out int r, out int g, out int b);

		[ DllImport("OPaC.uxTheme.Win32.Dll", CharSet=System.Runtime.InteropServices.CharSet.Unicode, EntryPoint="_Wrapper_DrawThemeEdge@48") ]
		public static extern bool DrawThemeEdge(string name, string part, string state, System.IntPtr hdc, int ox, int oy, int dx, int dy, int clip_ox, int clip_oy, int clip_dx, int clip_dy);
	}
}
