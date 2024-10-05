#include <iostream>
#include <vector>

import SlotMap;

int main()
{
	std::vector<int> v;
	for (int i = 0; i < 10; ++i)
	{
		v.emplace_back(i);
	}
}