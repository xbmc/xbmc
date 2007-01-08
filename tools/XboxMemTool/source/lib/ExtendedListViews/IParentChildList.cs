// Author:  Bill Seddon
// Company: Lyquidity Solutions Limited
// 
// This work builds on code posted to SourceForge by
// Jon Rista (jrista@hotmail.com)
// 
// This code is provided "as is" and no warranty about
// it fitness for any specific task is expressed or 
// implied.  If you choose to use this code, you do so
// at your own risk.  
//
////////////////////////////////////////////////////////

using System;

namespace Lyquidity.Controls
{
	/// <summary>
	/// IParentChildList provides functions to navigate a mutliply-linked
	/// list organized in parent-child format. The current node may navigate
	/// upwards to its parent node, forward and backwards in the current
	/// level, and down to the next level of its children.
	/// </summary>
	public interface IParentChildList
	{
		IParentChildList ParentNode();

		IParentChildList PreviousSibling();
		IParentChildList NextSibling();

		IParentChildList FirstChild();
		IParentChildList NextChild();
		IParentChildList PreviousChild();
		IParentChildList LastChild();
	}
}
