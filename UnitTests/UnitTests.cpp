#include "pch.h"
#include "CppUnitTest.h"
#include <vector>
#include <iostream>
#include <unordered_set>

import SlotMap;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Unalmas;

namespace UnitTests
{
	struct NonMovable
	{
		NonMovable() = default;

		NonMovable(const NonMovable& other) = default;
		NonMovable& operator=(const NonMovable& other) = default;

		NonMovable(NonMovable&& other) = delete;
		NonMovable& operator=(NonMovable&& other) = delete;

		explicit NonMovable(int value_) : value{ value_ } {}

		int value;
	};

	struct NonCopyable
	{
		NonCopyable() = default;

		NonCopyable(const NonCopyable& other) = delete;
		NonCopyable& operator=(const NonCopyable& other) = delete;

		NonCopyable(NonCopyable&& other) = default;
		NonCopyable& operator=(NonCopyable&& other) = default;

		explicit NonCopyable(int value_) : value{ value_ } {}

		int value;
	};

	struct Noisy
	{
		Noisy()
		{
			Logger::WriteMessage("Default ctor.\n");
		}

		Noisy(int v) : value{ v }
		{
			Logger::WriteMessage("Single param ctor.\n");
		}

		Noisy(const Noisy& rhs)
		{
			Logger::WriteMessage("Copy ctor.\n");
			value = rhs.value;
		}

		Noisy(Noisy&& rhs)
		{
			Logger::WriteMessage("Move ctor.\n");
			value = rhs.value;
			rhs.value = -1;
		}

		Noisy& operator=(const Noisy& rhs)
		{
			Logger::WriteMessage("Copy assignment.\n");
			value = rhs.value;
			return *this;
		}

		Noisy& operator=(Noisy&& rhs)
		{
			Logger::WriteMessage("Move assignment.\n");
			value = rhs.value;
			return *this;
		}

		~Noisy()
		{
			if (value == -1)
			{
				Logger::WriteMessage("Dtor. Moved from.\n");
			}
			else
			{
				Logger::WriteMessage("Dtor.\n");

			}
		}

		int value{ 0 };
	};

	struct Base
	{
		virtual int GetType() const
		{
			return 0;
		}
	};

	struct Derived : public Base
	{
		int GetType() const override
		{
			return 1;
		}
	};

	TEST_CLASS(SlotMapItemPointerTests)
	{
	public:
		SlotMap<int> slotMap;
		SlotMapKey key;
		SlotMapItemPointer<int> pointer;

		TEST_METHOD_INITIALIZE(SetUp)
		{
			key = slotMap.Insert(42);
			pointer = SlotMapItemPointer<int>(&slotMap, key);
		}

		TEST_METHOD(DereferenceOperator)
		{
			Assert::AreEqual(42, *pointer);
		}

		TEST_METHOD(ArrowOperator)
		{
			Assert::AreEqual(42, *pointer.operator->());
		}

		TEST_METHOD(InvalidKey)
		{
			SlotMapKey invalidKey{ -1, 0 };
			SlotMapItemPointer<int> invalidPointer(&slotMap, invalidKey);
			auto func = [&]() { *invalidPointer; };
			Assert::ExpectException<std::out_of_range>(func);
		}

		TEST_METHOD(NullSlotMap)
		{
			SlotMapItemPointer<int> nullPointer(nullptr, key);
			auto func = [&]() { *nullPointer; };
			Assert::ExpectException<std::runtime_error>(func);
		}

		TEST_METHOD(LotsOfPointers)
		{
			SlotMap<int> slotmap(256);
			std::vector<SlotMapItemPointer<int>> pointers;

			for (int i = 0; i < 256; ++i)
			{
				const auto key = slotmap.Insert(i);
				pointers.push_back(SlotMapItemPointer<int>(&slotmap, key));	
			}

			for (int i = 0; i < 256; ++i)
			{
				Assert::IsTrue(*pointers[i] == i);
			}
		}
	};

	TEST_CLASS(UnitTests)
	{
	public:
		TEST_METHOD(NoPolymorphismInSlotmap)
		{
			Unalmas::SlotMap<Base> bases;

			const auto b = bases.Insert(Base());
			const auto d = bases.Insert(Derived());

			Assert::IsTrue(bases[b].GetType() == 0);
			Assert::IsTrue(bases[d].GetType() == 0);
		}

		TEST_METHOD(NoisyWhenGrow)
		{
			std::vector<Unalmas::SlotMapKey> keys;
			Logger::WriteMessage("Test start\n");
			{
				Logger::WriteMessage("Scope start\n");
				Unalmas::SlotMap<Noisy> slotmap;
				Unalmas::SlotMapKey first;

				for (int i = 0; i < 10; ++i)
				{
					const auto key = slotmap.Insert(Noisy(i + 1));

					if (i == 2)
					{
						first = key;
					}

					keys.push_back(key);

					Assert::IsTrue(slotmap[key].value == i + 1);

					if (i >= 2)
					{
						const int v = slotmap[first].value;
						Assert::IsTrue(slotmap[first].value == 3);
					}
				}
				Assert::IsTrue(slotmap[first].value == 3);
				Logger::WriteMessage("Check\n");

				for (int i = 0; i < 1; ++i)
				{
					const auto& v = slotmap[keys[i]];
					Assert::IsTrue(v.value == i + 1);
				}

				Logger::WriteMessage("Scope exit\n");
			}
			Logger::WriteMessage("Test end\n");
		}

		TEST_METHOD(Keys)
		{
			Unalmas::SlotMapKey k(1, 0);

			//Copy
			Unalmas::SlotMapKey c(k);
			Assert::IsTrue(c.index == k.index && c.generation == k.generation);

			//Move
			Unalmas::SlotMapKey m(std::move(k));
			Assert::IsTrue(m.index == 1 && m.generation == 0);
		}

		TEST_METHOD(InsideUnorderedMap)
		{
			std::unordered_set<Unalmas::SlotMapKey> set;

			Unalmas::SlotMap<Noisy> map;
			for (int i = 0; i < 10; ++i)
			{
				const auto key = map.Insert(Noisy());
			}

			Assert::IsTrue(map.Size() == 10);
		}

		TEST_METHOD(NoisyInsertTwo)
		{
			Unalmas::SlotMap<Noisy> slotmap;
			const auto first = slotmap.Insert(Noisy(1));
			const auto second = slotmap.Insert(Noisy(2));
			const auto third = slotmap.Insert(Noisy(3));

			Assert::IsTrue(slotmap[first].value == 1);
			Assert::IsTrue(slotmap[second].value == 2);
			Assert::IsTrue(slotmap[third].value == 3);
		}

		TEST_METHOD(EmplaceByInsert)
		{
			Unalmas::SlotMap<NonCopyable> slotmap;
			auto first = slotmap.Insert(NonCopyable(123));

			Assert::IsTrue(slotmap[first].value == 123);
		}

		TEST_METHOD(NoisyInsert)
		{
			Logger::WriteMessage("-------- Creating slotmap --------\n");
			Unalmas::SlotMap<Noisy> slotmap;

			Logger::WriteMessage("--------\nCreate on stack.\n");
			Noisy n;
			n.value = 42;
			Logger::WriteMessage("--------\nInsert copy.\n");
			const auto key = slotmap.Insert(n);

			Assert::IsTrue(n.value == 42);
			const auto& value = slotmap[key];

			Assert::IsTrue(value.value == 42);

			Logger::WriteMessage("--------\nInsert move\n");
			slotmap.Insert(std::move(n));
			Logger::WriteMessage("--------\n");
		}

		TEST_METHOD(ConstructionTest)
		{
			Unalmas::SlotMap<int> slotmap;
			Assert::IsTrue(slotmap.Size() == 0);

			const auto key = slotmap.Insert(42);
			Assert::IsTrue(slotmap.Size() == 1);
		}

		TEST_METHOD(AddAndGet)
		{
			Unalmas::SlotMap<int> slotmap;
			const auto key = slotmap.Insert(42);
			Assert::IsTrue(slotmap[key] == 42);
		}

		TEST_METHOD(Clear)
		{
			Unalmas::SlotMap<int> slotmap;
			const auto key = slotmap.Insert(123);
			const auto otherKey = slotmap.Insert(456);

			Assert::IsTrue(slotmap.Size() == 2);

			slotmap.Clear();

			Assert::IsTrue(slotmap.Size() == 0);

			int value;
			Assert::IsFalse(slotmap.TryGet(key, value));
			Assert::IsFalse(slotmap.TryGet(otherKey, value));
		}

		TEST_METHOD(ClearAfterGrow)
		{
			Unalmas::SlotMap<double> slotmap;
			std::vector<Unalmas::SlotMapKey> keys;

			for (int i = 0; i < 100; ++i)
			{
				keys.push_back(slotmap.Insert(static_cast<double>(i)));
			}

			Assert::IsTrue(slotmap.Size() == 100);

			slotmap.Clear();

			double value;
			for (const auto& key : keys)
			{
				Assert::IsFalse(slotmap.TryGet(key, value));
			}

			Assert::IsTrue(slotmap.Size() == 0 && slotmap.Capacity() >= 100);
		}

		TEST_METHOD(Grow)
		{
			Unalmas::SlotMap<int> slotmap;		// Default capacity: 8
			std::vector<Unalmas::SlotMapKey> keys;
			for (int i = 0; i < 16; ++i)
			{
				const auto key = slotmap.Insert(i);
				keys.push_back(key);
			}

			Assert::IsTrue(slotmap.Size() == 16);
		}

		TEST_METHOD(SimpleErase)
		{
			Unalmas::SlotMap<int> slotmap;
			const auto key = slotmap.Insert(42);
			Assert::IsTrue(slotmap.Size() == 1);

			Assert::IsTrue(slotmap.Erase(key));
			Assert::IsTrue(slotmap.Size() == 0);
		}

		TEST_METHOD(CopySlotmap)
		{
			Unalmas::SlotMap<int> original;
			original.Insert(42);

			Assert::IsTrue(original.Size() == 1);

			Unalmas::SlotMap<int> copy(original);

			Assert::IsTrue(copy.Size() == 1);

			original.Clear();

			Assert::IsTrue(original.Size() == 0);
			Assert::IsTrue(copy.Size() == 1);
		}

		TEST_METHOD(MoveSlotmap)
		{
			Unalmas::SlotMap<float> original;
			original.Insert(123.4f);
			original.Insert(456.7f);

			Assert::IsTrue(original.Size() == 2);

			Unalmas::SlotMap<float> moved(std::move(original));

			Assert::IsTrue(moved.Size() == 2);
			Assert::IsTrue(original.Size() == 0);

			moved.Insert(789.0f);

			Assert::IsTrue(moved.Size() == 3);
		}

		TEST_METHOD(Iteration)
		{
			Unalmas::SlotMap<int> slotmap;
			for (int i = 0; i < 10; ++i)
			{
				slotmap.Insert(i);
			}

			Assert::IsTrue(slotmap.Size() == 10);

			for (const auto& item : slotmap)
			{
				const auto asString = ToString<int>(item);
				Logger::WriteMessage(asString.c_str());
				Logger::WriteMessage("\n");
			}

			for (auto& item : slotmap)
			{
				item = item * 2;
			}

			Logger::WriteMessage("After modification:\n");

			for (const auto& item : slotmap)
			{
				const auto asString = ToString<int>(item);
				Logger::WriteMessage(asString.c_str());
				Logger::WriteMessage("\n");
			}
		}

		TEST_METHOD(IterationByIndex)
		{
			Unalmas::SlotMap<int> slotmap;
			for (int i = 0; i < 10; ++i)
			{
				slotmap.Insert(i);
			}

			Assert::IsTrue(slotmap.Size() == 10);

			for (int i = 0; i < 10; ++i)
			{
				const auto& item = slotmap[i];
				const auto asString = ToString<int>(item);
				Logger::WriteMessage(asString.c_str());
				Logger::WriteMessage("\n");
			}
		}

		TEST_METHOD(KeyByIndex)
		{
			Unalmas::SlotMap<float> slotmap;
			std::vector<Unalmas::SlotMapKey> keys;
			for (int i = 0; i < 100; ++i)
			{
				keys.push_back(slotmap.Insert((float)i));
			}

			Assert::IsTrue(slotmap.Size() == 100);

			for (int i = 0; i < 100; ++i)
			{
				const auto& key = slotmap.GetKeyForIndex(i);

				const float valueByIndex = slotmap[i];
				const float valueByKey = slotmap[key];

				Assert::IsTrue(keys[i] == key);
				Assert::IsTrue(fabs(valueByIndex - valueByKey) < 0.00001f);
			}

		}

		TEST_METHOD(ConstIteration)
		{
			Unalmas::SlotMap<float> slotmap;
			Unalmas::SlotMapKey keyToRemove;

			for (int i = 0; i < 10; ++i)
			{
				const auto key = slotmap.Insert(i * 2.2f);
				if (i == 5)
				{
					keyToRemove = key;
				}
			}

			Assert::IsTrue(slotmap.Size() == 10);

			for (const auto& item : slotmap)
			{
				const auto asString = ToString<float>(item);
				Logger::WriteMessage(asString.c_str());
				Logger::WriteMessage("\n");
			}


			slotmap.Erase(keyToRemove);

			Assert::IsTrue(slotmap.Size() == 9);

			slotmap.Insert(123.456f);

			Logger::WriteMessage("---- After removal:\n");

			for (const auto& item : slotmap)
			{
				const auto asString = ToString<float>(item);
				Logger::WriteMessage(asString.c_str());
				Logger::WriteMessage("\n");
			}
		}

		TEST_METHOD(EraseNoisy)
		{
			Unalmas::SlotMap<Noisy> slotmap;
			const auto key = slotmap.Insert(Noisy());
			Assert::IsTrue(slotmap.Size() == 1);
			Assert::IsTrue(slotmap.Erase(key));
			Assert::IsTrue(slotmap.Size() == 0);
		}

		TEST_METHOD(EraseAndReuse)
		{
			Unalmas::SlotMap<int> slotmap;
			const auto key = slotmap.Insert(42);
			Assert::IsTrue(slotmap.Size() == 1);

			Assert::IsTrue(slotmap.Erase(key));
			Assert::IsTrue(slotmap.Size() == 0);

			Assert::IsFalse(slotmap.Erase(key));	// Key should be invalid by now.

			const auto newKey = slotmap.Insert(123);
			Assert::IsTrue(slotmap.Size() == 1);

			Assert::IsTrue(slotmap[newKey] == 123);
			Assert::IsTrue(slotmap.Erase(newKey));
			Assert::IsTrue(slotmap.Size() == 0);
		}

		TEST_METHOD(EraseAndReuse2)
		{
			Unalmas::SlotMap<int> slotmap;
			std::vector<Unalmas::SlotMapKey> keys;
			for (int i = 0; i < 7; ++i)
			{
				const auto key = slotmap.Insert(i);
				keys.push_back(key);
			}

			Assert::IsTrue(slotmap.Size() == 7);
			Assert::IsTrue(slotmap.Capacity() == 8);

			for (const auto& key : keys)
			{
				Assert::IsTrue(slotmap.Erase(key));
			}

			keys.clear();

			for (int i = 0; i < 7; ++i)
			{
				const auto newKey = slotmap.Insert(i * 2);
				keys.push_back(newKey);
			}

			Assert::IsTrue(slotmap.Size() == 7);
			Assert::IsTrue(slotmap.Capacity() == 8);

			for (int i = 0; i < 7; ++i)
			{
				const auto value = slotmap[keys[i]];
				Assert::IsTrue(value == i * 2);
			}
		}

		TEST_METHOD(EraseAndReuseFullCapacity)
		{
			Unalmas::SlotMap<int> slotmap(4);
			std::vector<Unalmas::SlotMapKey> keys;
			for (int i = 0; i < 4; ++i)
			{
				keys.push_back(slotmap.Insert(i));
			}

			Assert::IsTrue(slotmap.Size() == 4 && slotmap.Capacity() == 4);

			for (int i = 0; i < 4; ++i)
			{
				Assert::IsTrue(slotmap[keys[i]] == i);
			}

			for (const auto& key : keys)
			{
				Assert::IsTrue(slotmap.Erase(key));
			}

			Assert::IsTrue(slotmap.Size() == 0 && slotmap.Capacity() == 4);

			keys.clear();

			for (int i = 0; i < 4; ++i)
			{
				keys.push_back(slotmap.Insert(i));
			}

			Assert::IsTrue(slotmap.Size() == 4 && slotmap.Capacity() == 4);

			for (int i = 0; i < 4; ++i)
			{
				Assert::IsTrue(slotmap[keys[i]] == i);
			}
		}

		TEST_METHOD(NonMovableTest)
		{
			Assert::IsTrue(std::is_move_assignable<NonMovable>() == false);

			NonMovable a(42);

			NonMovable b(a);					// Copy ctor
			Assert::IsTrue(b.value == 42);

			NonMovable c = b;					// Copy assignment
			Assert::IsTrue(c.value == 42);
		}

		TEST_METHOD(EraseAndReuseNonMovable)
		{
			constexpr int itemCount = 100;
			Unalmas::SlotMap<NonMovable> slotmap;
			std::vector<Unalmas::SlotMapKey> keys;

			for (int i = 0; i < itemCount; ++i)
			{
				keys.push_back(slotmap.Insert(NonMovable(i)));
			}

			Assert::IsTrue(slotmap.Size() == itemCount);
			Assert::IsTrue(slotmap.Erase(keys[50]));
			Assert::IsTrue(slotmap.Size() == itemCount - 1);

			NonMovable result;
			Assert::IsFalse(slotmap.TryGet(keys[50], result));
			Assert::IsTrue(slotmap.TryGet(keys[itemCount - 1], result));
			Assert::IsTrue(result.value == itemCount - 1);
		}

		TEST_METHOD(EraseOutOfMany)
		{
			constexpr int itemCount = 100;
			Unalmas::SlotMap<int> slotmap;
			std::vector<Unalmas::SlotMapKey> keys;

			for (int i = 0; i < itemCount; ++i)
			{
				keys.push_back(slotmap.Insert(i));
			}

			int result{ 0 };

			for (int i = 0; i < itemCount; ++i)
			{
				bool canGet = slotmap.TryGet(keys[i], result);
				Assert::IsTrue(canGet);
				Assert::IsTrue(result == i);
			}

			Assert::IsTrue(slotmap.Size() == itemCount);

			Assert::IsTrue(slotmap.Erase(keys[50]));
			Assert::IsTrue(slotmap.Size() == itemCount - 1);

			Assert::IsFalse(slotmap.TryGet(keys[50], result));

			for (int i = 0; i < itemCount; ++i)
			{
				if (i != 50)
				{
					bool canGet = slotmap.TryGet(keys[i], result);
					Assert::IsTrue(canGet);
					Assert::IsTrue(result == i);
				}
			}

			for (int i = 0; i < itemCount; ++i)
			{
				if (i % 2 != 0)
				{
					Assert::IsTrue(slotmap.Erase(keys[i]));
				}
			}

			for (int i = 0; i < itemCount; ++i)
			{
				bool expectedToBeValid = (i != 50 && i % 2 == 0);
				bool isValid = slotmap.TryGet(keys[i], result);

				Assert::IsTrue(isValid == expectedToBeValid);
				Assert::IsTrue(!expectedToBeValid || result == i);
			}
		}

		TEST_METHOD(ManyInsertsClearsAndErases)
		{
			constexpr int itemCount = 1024;
			Unalmas::SlotMap<int> slotmap;
			std::vector<Unalmas::SlotMapKey> keys;

			for (int i = 0; i < itemCount; ++i)
			{
				keys.push_back(slotmap.Insert(i));
			}

			Assert::IsTrue(slotmap.Size() == itemCount);

			slotmap.Clear();
			Assert::IsTrue(slotmap.Size() == 0);

			int result = -1;
			for (int i = 0; i < itemCount; ++i)
			{
				Assert::IsFalse(slotmap.TryGet(keys[i], result));
			}

			keys.clear();

			for (int i = 0; i < itemCount; ++i)
			{
				keys.push_back(slotmap.Insert(i));
			}

			Assert::IsTrue(slotmap.Size() == itemCount);

			for (int i = 0; i < itemCount; ++i)
			{
				if (i % 3 == 0)
				{
					Assert::IsTrue(slotmap.Erase(keys[i]));
				}
			}

			for (int i = 0; i < itemCount; ++i)
			{
				const bool shouldExist = i % 3 != 0;
				Assert::IsTrue(slotmap.TryGet(keys[i], result) == shouldExist);
			}

			Assert::IsTrue(0 < slotmap.Size() && slotmap.Size() < itemCount);
		}
	};
}
