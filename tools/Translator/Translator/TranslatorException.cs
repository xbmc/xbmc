using System;

namespace TeamXBMC.TranslatorCore
{
	/// <summary>
	/// Exception thrown by the TranslatorCore namespace
	/// </summary>
	public class TranslatorException : ApplicationException
	{
		#region Constructors

		public TranslatorException(string message, Exception innerException) : base(message, innerException)
		{

		}

		public TranslatorException(string message) : base(message)
		{

		}

		#endregion
	}
}
