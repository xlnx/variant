#include <iostream>
#include <string>
#include "variant.h"
using namespace std;
int main()
{
	variant<int, double, string> var = 3, v = 1.5, c = string("asd");
	var.make_match<int, double>(
		[](int& n){ std::cout << n << std::endl; },
		[](double&){}
	);
}
