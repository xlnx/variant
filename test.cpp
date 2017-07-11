#include <iostream>
#include <map>
#include <iostream>
#include "variant.h"
using namespace std;

struct A
{
	A () { cout << "A" << endl; }
	//A (A&&) { cout << "A&&" << endl; }
	~A () { cout << "~A" << endl; }
};

int main()
{
	variant<int, A, char> var = 1, b;//A();
	cout << std::is_move_constructible<A>::value << endl;
	//cout << a.as<int>() << endl;
	//cout << sizeof(var) << endl;
}
