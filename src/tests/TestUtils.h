#pragma once

#include <iostream>
#include <functional>
#include <string>
#include <vector>

template <class T>
class TestList
{
	public:
		void addTest(void (T::*test)())
		{
			m_tests.push_back(std::mem_fun(test));
		}

		int size() const
		{
			return m_tests.size();
		}

		void runTest(T* testInstance, int i)
		{
			m_tests.at(i)(testInstance);
		}

	private:
		std::vector<std::mem_fun_t<void,T> > m_tests;
};

class TestUtils
{
	public:
		template <class X, class Y>
		static void compare(const X& x, const Y& y, const char* xString, const char* yString)
		{
			if (x != y)
			{
				throw "Actual and expected values differ.  "
				      "Actual: " + toString(x,xString) +
				      " Expected: " + toString(y,yString);
			}
		}

		template <typename T>
		static std::string toString(T value, const char* context)
		{
			return "Unprintable: " + std::string(context);
		}

		template <class T>
		static int runTest(class TestList<T>& tests) throw ()
		{
			std::string errorText;
			try
			{
				T testInstance;
				for (int i=0; i < tests.size(); i++)
				{
					tests.runTest(&testInstance,i);
				}
			}
			catch (const std::exception& ex)
			{
				errorText = ex.what();
			}
			catch (const std::string& error)
			{
				errorText = error;
			}
			catch (...)
			{
				errorText = "Unknown exception";
			}

			if (errorText.empty())
			{
				std::cout << "Test passed" << std::endl;
				return 0;
			}
			else
			{
				std::cout << "Test failed: " << errorText << std::endl;
				return 1;
			}
		}
};

template <>
inline std::string TestUtils::toString(const std::string& value, const char*)
{
	return value;
}
template <>
inline std::string TestUtils::toString(std::string value, const char*)
{
	return value;
}
template <>
inline std::string TestUtils::toString(const char* value, const char*)
{
	return value;
}

#define TEST_COMPARE(x,y) \
	TestUtils::compare(x,y,#x,#y);


