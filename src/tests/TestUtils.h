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
				      "Actual: " + std::string(xString) +
				      " Expected: " + std::string(yString);
			}
		}

		template <class T>
		static int runTest(class TestList<T>& tests) throw ()
		{
			try
			{
				T testInstance;
				for (int i=0; i < tests.size(); i++)
				{
					tests.runTest(&testInstance,i);
				}
				std::cout << "Test passed" << std::endl;
				return 0;
			}
			catch (...)
			{
				std::cout << "Test failed" << std::endl;
				return 1;
			}
		}
};

#define TEST_COMPARE(x,y) \
	TestUtils::compare(x,y,#x,#y);

