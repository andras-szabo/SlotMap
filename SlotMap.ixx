//#define SLOTMAP_RELEASE
export module SlotMap;

#define DEFAULT_CAPACITY 8

// Guarantees:
// constant time lookup, erase, and insert
// dense, contiguous element storage
// stable keys, even if elements are moved around due to removal
// insert calls return a unique key
// keys to erased values won't work (until an overflow occurs)

// YouTube link: https://youtu.be/-8UZhDjgeZU?si=mZStcyE8M_fa-CkS

import <cassert>;
import <cstring>;
import <cstdlib>;
import <utility>;
import <type_traits>;
import <stdexcept>;

export namespace Unalmas
{
	struct SlotMapKey
	{
		int index{ -1 };
		int generation{ 0 };
		auto operator<=>(const SlotMapKey& other) const = default;

		bool IsValid() const noexcept
		{
			return index >= 0 && generation >= 0;
		}
	};

	template <typename T>
	struct SlotMapConstIterator
	{
		SlotMapConstIterator(T* ptr_, int size_, int index_) : ptr{ ptr_ }, size{ size_ }, index{ index_ }
		{}

		bool operator!=(const Unalmas::SlotMapConstIterator<T>& rhs) const
		{
			return	ptr != rhs.ptr || index != rhs.index;
		}

		SlotMapConstIterator& operator++()
		{
			++index;
			return *this;
		}

		const T& operator*() const
		{
#ifndef SLOTMAP_RELEASE
			if (index >= size)
			{
				throw std::out_of_range("[SlotMap] Trying to dereference invalid iterator.");
			}
#endif

			return *(ptr + index);
		}

	protected:
		T* ptr;
		int		index;
		int		size;
	};

	template <typename T>
	struct SlotMapIterator : public SlotMapConstIterator<T>
	{
		SlotMapIterator(T* ptr_, int size_, int index_) : SlotMapConstIterator<T>(ptr_, size_, index_) {}

		T& operator*() const
		{
#ifndef SLOTMAP_RELEASE
			if (this->index >= this->size)
			{
				throw std::out_of_range("[SlotMap] Trying to dereference invalid iterator.");
			}
#endif

			return *(this->ptr + this->index);
		}
	};

	template <typename T>
	class SlotMap
	{
	private:
		SlotMapKey* slots{ nullptr };
		T* values{ nullptr };
		unsigned int* valueToSlot{ nullptr };
		int						firstFreeSlot{ 0 };
		int						lastFreeSlot{ 0 };
		int						size{ 0 };
		int						capacity{ 0 };

	public:
		SlotMap();
		SlotMap(int capacity);

		SlotMap(const SlotMap& rhs);	// Only supported for trivially copyable T types
		SlotMap(SlotMap&& rhs);
		~SlotMap();

		SlotMap& operator=(const SlotMap& rhs) = delete;	// Copy and move assignment ops are deleted,
		SlotMap& operator=(SlotMap&& rhs) = delete;		// b/c I don't want to make a decision about how to
		// handle existing keys. E.g. when copy assigning
		// a slotmap, would existing keys be invalidated?
		// But what if those keys were compatible with the
		// original? No such issue in copy and move ctors,
		// because there _are_ no initial keys to invalidate,
		// so we can just say that all keys of the original
		// will be valid in the new copy (or moved-to) instance.

		T& operator[](const SlotMapKey& key) const;
		T& operator[](int index) const;
		bool					TryGet(const SlotMapKey& key, T& value) const;
		Unalmas::SlotMapKey		GetKeyForIndex(int index) const;

		int						Size() const { return size; }
		int						Capacity() const { return capacity; }

		template <typename U>
		SlotMapKey   			Insert(U&& value);

		bool					Erase(const SlotMapKey& key);
		void					Clear();

		SlotMapConstIterator<T>	begin() const;
		SlotMapConstIterator<T>	end() const;

		SlotMapIterator<T>		begin();

	private:
		void					Grow();
		void					DestructExistingItems();
	};

	template <typename T>
	Unalmas::SlotMapKey SlotMap<T>::GetKeyForIndex(int index) const
	{
#ifndef SLOTMAP_RELEASE
		if (index >= size)
		{
			throw std::out_of_range("[SlotMap] Trying to look up invalid index.");
		}
#endif

		return slots[valueToSlot[index]];
	}

	template <typename T>
	SlotMapConstIterator<T> SlotMap<T>::begin() const
	{
		return SlotMapConstIterator<T>(values, size, 0);
	}

	template <typename T>
	SlotMapIterator<T> SlotMap<T>::begin()
	{
		return SlotMapIterator<T>(values, size, 0);
	}

	template <typename T>
	SlotMapConstIterator<T> SlotMap<T>::end() const
	{
		return SlotMapConstIterator<T>(values, size, size);
	}

	template <typename T>
	T& SlotMap<T>::operator[](const SlotMapKey& key) const
	{
#ifndef SLOTMAP_RELEASE
		if (key.index >= capacity)
		{
			throw std::out_of_range("[SlotMap] Key index is out of bounds.");
		}
#endif

		const SlotMapKey& slot = slots[key.index];

#ifndef SLOTMAP_RELEASE
		if (slot.generation != key.generation)
		{
			throw std::invalid_argument("[SlotMap] Trying to use a key which is no longer valid.");
		}
#endif

		return values[slot.index];
	}

	template <typename T>
	T& SlotMap<T>::operator[](int index) const
	{
#ifndef SLOTMAP_RELEASE
		if (index >= size)
		{
			throw std::out_of_range("[SlotMap] Index is out of bounds.");
		}
#endif

		return values[index];
	}

	template <typename T>
	bool SlotMap<T>::TryGet(const SlotMapKey& key, T& value) const
	{
		if (0 <= key.index && key.index < capacity)
		{
			const SlotMapKey& slot = slots[key.index];
			if (slot.generation == key.generation)
			{
				value = values[slot.index];
				return true;
			}
		}

		return false;
	}

	template <typename T>
	SlotMap<T>::SlotMap() : SlotMap<T>(DEFAULT_CAPACITY)
	{
	}

	template <typename T>
	SlotMap<T>::SlotMap(int capacity_)
		: size{ 0 }, capacity{ capacity_ }, firstFreeSlot{ 0 }
	{
		slots = new SlotMapKey[capacity];
		values = static_cast<T*>(std::malloc(capacity * sizeof(T)));
		valueToSlot = new unsigned int[capacity];

		for (int i = 0; i < capacity - 1; ++i)
		{
			slots[i] = SlotMapKey(i + 1, 0);
		}

		// The last slot's index (showing the _next_ free slot)
		// should point to itself at start.
		slots[capacity - 1] = SlotMapKey(capacity - 1, 0);
		lastFreeSlot = capacity - 1;
	}

	template <typename T>
	SlotMap<T>::SlotMap(const SlotMap<T>& rhs)
	{
		static_assert(std::is_trivially_copyable<T>(), "You can only copy slotmaps with a trivially copyable element type.");

		size = rhs.size;
		capacity = rhs.capacity;
		firstFreeSlot = rhs.firstFreeSlot;
		lastFreeSlot = rhs.lastFreeSlot;

		slots = new SlotMapKey[capacity];
		values = static_cast<T*>(std::malloc(capacity * sizeof(T)));
		valueToSlot = new unsigned int[capacity];

		std::memcpy(slots, rhs.slots, capacity * sizeof(SlotMapKey));
		std::memcpy(valueToSlot, rhs.valueToSlot, capacity * sizeof(unsigned int));
		std::memcpy(values, rhs.values, capacity * sizeof(T));
	}


	template <typename T>
	SlotMap<T>::SlotMap(SlotMap<T>&& rhs)
	{
		size = rhs.size;
		capacity = rhs.capacity;
		firstFreeSlot = rhs.firstFreeSlot;

		slots = rhs.slots;
		values = rhs.values;
		valueToSlot = rhs.valueToSlot;

		rhs.slots = nullptr;
		rhs.values = nullptr;
		rhs.valueToSlot = nullptr;
		rhs.capacity = 0;
		rhs.size = 0;
	}

	template <typename T>
	SlotMap<T>::~SlotMap()
	{
		delete[] valueToSlot;

		DestructExistingItems();
		std::free(values);

		delete[] slots;
	}

	template <typename T>
	void SlotMap<T>::DestructExistingItems()
	{
		for (int i = 0; i < size; ++i)
		{
			values[i].~T();
		}
	}

	template <typename T>
	void SlotMap<T>::Clear()
	{
		DestructExistingItems();
		for (int i = 0; i < capacity; ++i)
		{
			SlotMapKey& key = slots[i];
			key.index = i + 1;
			key.generation += 1;
		}

		firstFreeSlot = 0;
		size = 0;
	}

	template <typename T>
	bool SlotMap<T>::Erase(const SlotMapKey& key)
	{
		SlotMapKey& slot = slots[key.index];
		if (slot.generation == key.generation)
		{
			slot.generation++;
			const int valueIndex = slot.index;
			const int removedSlotIndex = valueToSlot[valueIndex];

			// Destruct existing item
			values[valueIndex].~T();

			// Move (or copy) last item into newly freed value allocation
			if (size > 1)
			{
				if constexpr (std::is_move_assignable<T>())
				{
					values[valueIndex] = static_cast<T&&>(values[size - 1]);
				}
				else
				{
					values[valueIndex] = values[size - 1];
					values[size - 1].~T();
				}

				valueToSlot[valueIndex] = valueToSlot[size - 1];
			}

			// If the first free slot is created by this removal, set both first and last
			// to the removed slot, and make sure the slot's index is also pointing at itself.
			if (firstFreeSlot == -1)
			{
				firstFreeSlot = removedSlotIndex;
			}
			else
			{
				slots[lastFreeSlot].index = removedSlotIndex;
			}

			slots[removedSlotIndex].index = removedSlotIndex;
			lastFreeSlot = removedSlotIndex;

			// Adjust the affected slot:
			SlotMapKey& movedSlot = slots[valueToSlot[valueIndex]];
			movedSlot.index = valueIndex;

			size--;

			return true;
		}

		return false;
	}

	template <typename T>
	template <typename U>
	SlotMapKey SlotMap<T>::Insert(U&& value)
	{
		while (capacity <= size)
		{
			Grow();
		}

		const int newValueIndex = size++;

		if constexpr (std::is_move_assignable<T>())
		{
			new (&values[newValueIndex]) T(std::forward<U>(value));
		}
		else
		{
			new (&values[newValueIndex]) T(value);
		}

		const int slotIndex = firstFreeSlot;
		SlotMapKey& slot = slots[slotIndex];

		valueToSlot[newValueIndex] = slotIndex;

		if (slot.index == firstFreeSlot)
		{
			firstFreeSlot = -1;				// Ran out of free slots!
			lastFreeSlot = -1;
		}
		else
		{
			firstFreeSlot = slot.index;
		}

		slot.index = newValueIndex;

		return SlotMapKey(slotIndex, slot.generation);
	}

	template <typename T>
	void SlotMap<T>::Grow()
	{
		const int newCapacity = capacity * 2;

		T* newValues = static_cast<T*>(std::malloc(newCapacity * sizeof(T)));

		SlotMapKey* newSlots = new SlotMapKey[newCapacity];
		unsigned int* newValueToSlot = new unsigned int[newCapacity];

		memcpy(newValueToSlot, valueToSlot, capacity * sizeof(unsigned int));
		memcpy(newSlots, slots, capacity * sizeof(SlotMapKey));

		if constexpr (std::is_trivially_copyable_v<T>)
		{
			memcpy(newValues, values, capacity * sizeof(T));
		}
		else
		{
			for (int i = 0; i < size; ++i)
			{
				if constexpr (std::is_move_assignable<T>())
				{
					new (&newValues[i]) T(std::move(values[i]));
				}
				else
				{
					new (&newValues[i]) T(values[i]);
					values[i].~T();
				}
			}
		}

		delete[] slots;
		std::free(values);
		delete[] valueToSlot;

		slots = newSlots;
		values = newValues;
		valueToSlot = newValueToSlot;

		for (int i = capacity; i < newCapacity - 1; ++i)
		{
			slots[i].index = i + 1;
		}

		slots[newCapacity - 1].index = newCapacity - 1;

		firstFreeSlot = capacity;
		lastFreeSlot = newCapacity - 1;

		capacity = newCapacity;
	}

} // namespace Unalmas

template<>
struct std::hash<Unalmas::SlotMapKey>
{
	std::size_t operator()(const Unalmas::SlotMapKey& key) const noexcept
	{
		return key.index;
	}
};