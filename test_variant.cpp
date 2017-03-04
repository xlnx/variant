#include <iostream>
#include <string>
#include "variant.h"
using namespace std;
using wvariant = unique_variant<std::string, int, double>;
int main()
{
	wvariant var = wvariant::create<string>("shit");
}
