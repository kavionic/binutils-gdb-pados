// Test for -var-info-path-expression syntax error
// caused by PR 11912
#include <string.h>
#include <stdio.h>

// Ensure that the CU has namespace information for C++ scope lookup.
namespace N
{
  int value = 2;
}

class A
{
	public:
		int a;
};

class C : public A
{
	public:
		C()
		{
			a = 5;
		};
		void testLocation()
		{
			z = 1;
		};
		static void staticTestLocation(C *object)
		{
			object->z = N::value;
		};
		int z;
};

int main()
{
	C c;
	c.testLocation();
	C::staticTestLocation(&c);
	return 0;
}
