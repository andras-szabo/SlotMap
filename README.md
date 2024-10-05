# SlotMap

A simple, single module implementation of a SlotMap data structure, based on a talk by Allan Deutsch at CppCon 2017. (https://www.youtube.com/watch?v=-8UZhDjgeZU)

## Usage

// Create a slotmap of a given type, with a default capacity
'Unalmas::SlotMap<int> slotmap;'

// Create a slotmap of a given type with a specific capacity
Unalmas::SlotMap<int> slotmap(32);

// Insert a value into the map, and keep the key
Unalmas::SlotMapKey key = slotmap.Insert(123);

// Look up a value - may throw exceptions if the key is not found or invalid
const auto value = slotmap[key];

// Try to look up a value, handle errors gracefully
int value;
if (slotmap.TryGet(key, value)) { ... }

// Erase a key-value pair
bool couldErase = slotmap.Erase(key);

// Clear the map altogether
slotmap.Clear();

// Iterate through slotmap values (const)
for (const auto& value : slotmap) { ... }

// Iterate through slotmap values (non-const)
for (auto& value : slotmap) { ... }

// Iterate through slotmap by index
for (int i = 0; i < slotmap.Size(); ++i)
{
    const auto& item = slotmap[i];
    ...
}

