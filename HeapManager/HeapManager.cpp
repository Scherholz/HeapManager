#include <iostream>
#include <list>
#include <atomic>
#include <vector>
#include <string>


class HeapManager
{
private:
	
	std::atomic_int m_freeStoreSize;

public:
	class Block
	{
	public:
		std::list<std::atomic_uint8_t>::iterator blockHead;
		std::atomic_int size;

		Block(std::list<std::atomic_uint8_t>::iterator head, int iBytes)
		{
			blockHead = head;
			size = iBytes;
		}
	};

	std::list<std::atomic_uint8_t>* m_freeStore;
	std::list<std::atomic_uint8_t>::iterator m_freeHead;
	HeapManager(int mBytes)
	{
		m_freeStore = new std::list<std::atomic_uint8_t>(1000000 * mBytes);
		m_freeHead = m_freeStore->begin();
		m_freeStoreSize = 1000000*mBytes;
	}

	~HeapManager() 
	{
		delete m_freeStore;
	}

	void* allocate(int iBytes)// -allocate a contiguous block of of size iBytes
	{
		Block* newBlock = new Block(m_freeHead, iBytes);
		std::advance(m_freeHead, iBytes);

		return reinterpret_cast<void*>(newBlock);
	}
	bool release(void *block)// -release a previously allocated block
	{
		Block *blockToBeDelete = reinterpret_cast<Block*>(block);
		int size = blockToBeDelete->size;

		m_freeStore->erase(blockToBeDelete->blockHead, std::next(blockToBeDelete->blockHead,size));
	
		delete blockToBeDelete;

		m_freeStore->resize(m_freeStoreSize);

		return true;
	}
	void* resize(void* pBlock, int iNewSizeBytes)
	{
		Block* blockToBeResized= reinterpret_cast<Block*>(pBlock);
		int size = blockToBeResized->size;
		std::list<std::atomic_uint8_t> tempBlock(0);
		std::list<std::atomic_uint8_t>::iterator blockHead = blockToBeResized->blockHead;		
		std::list<std::atomic_uint8_t>::iterator blockEnd = std::next(blockHead, size);
		
		tempBlock.splice(tempBlock.begin(), *m_freeStore, blockHead, blockEnd);
		
		m_freeStore->splice(m_freeHead, tempBlock);
		std::advance(m_freeHead, -size);
		blockToBeResized->blockHead = m_freeHead;
		blockToBeResized->size = iNewSizeBytes;
		std::advance(m_freeHead, iNewSizeBytes);

		return reinterpret_cast<void*>(true);
	}
};

int main(int argc, char* argv[])
{
	HeapManager* testHeap;
	if(argc==1)
	{
		testHeap = new HeapManager((int)argv[0]);
	}
	else {
		testHeap = new HeapManager(10);
	}
	
	auto it = testHeap->m_freeStore->begin();
	for (uint8_t i = 0; i < 100; i++)
	{
		*it = i;
		it++;
	}

	HeapManager::Block *testBlock1 = reinterpret_cast<HeapManager::Block*>(testHeap->allocate(2));

	std::cout << "\nTest block 1: " << &*testBlock1->blockHead << " " << testBlock1->size << " free Head: " << &*testHeap->m_freeHead;
	

	HeapManager::Block* testBlock2 = reinterpret_cast<HeapManager::Block*>(testHeap->allocate(3));
	auto tb2it = testBlock2->blockHead;
	for (uint8_t i = 0; i < testBlock2->size; i++) {
		*tb2it = i + 100;
		tb2it = std::next(tb2it, 1);
	}
	std::cout << "\n";
	tb2it = testBlock2->blockHead;
	for (uint8_t i = 0; i < testBlock2->size; i++) {
		std::cout << "Tb2 [" << std::to_string(i) << "]=" << std::to_string((uint8_t)*tb2it) << ",";
		tb2it = std::next(tb2it, 1);
	}
	std::cout << "\nTest block 2: " << &*testBlock2->blockHead << " " << testBlock2->size << " free Head: " << &*testHeap->m_freeHead;

	testHeap->release(testBlock1);

	std::cout << "\nTest block 2 after release of test block 1: " << &*testBlock2->blockHead << " " << testBlock2->size << " free Head: " << &*testHeap->m_freeHead;
	tb2it = testBlock2->blockHead;
	std::cout << "\n";
	for (uint8_t i = 0; i < testBlock2->size; i++) {
		std::cout << "Tb2 [" << std::to_string(i) << "]=" << std::to_string( (uint8_t) * tb2it )<< ",";
		tb2it = std::next(tb2it, 1);
	}
	HeapManager::Block* testBlock3 = reinterpret_cast<HeapManager::Block*>(testHeap->allocate(5));

	std::cout << "\nTest block 3: " << &*
		testBlock3->blockHead << " " << testBlock3->size << " free Head: " << &*testHeap->m_freeHead;

	testHeap->resize(testBlock2, 10);

	std::cout << "\nTest block 2: " << &*testBlock2->blockHead << " " << testBlock2->size << " free Head: " << &*testHeap->m_freeHead;
	tb2it = testBlock2->blockHead;
	std::cout << "\n";
	for (uint8_t i = 0; i < testBlock2->size; i++) {
		std::cout << "Tb2 [" << std::to_string(i) << "]=" << std::to_string((uint8_t)*tb2it) << ",";
		tb2it = std::next(tb2it, 1);
	}
	std::cout << "\nTest block 3: " << &*testBlock3->blockHead << " " << testBlock3->size << " free Head: " << &*testHeap->m_freeHead;

	delete testHeap;
	std::system("PAUSE");
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

/*Heap management is an important function of a run-time system and is often a performance bottleneck.

Submit your answer in the form of a well-documented single C++ file that will compile
to a command-line application that accepts user-specified arguments and generates its results as a textual output.
Each aspect of the problem should be demonstrable by compiled-in test code which can be invoked from the command line.

Use of the standard template library (C++11) should be demonstrated as part of the solution. Performance should be considered throughout the implementation.

For each part of the question below - provide a test that can be invoked from the command line to demonstrate correct behaviour.

	1. Create a heap class that manages an arbitrary reserved memory block whose size is specified in MB. The class should implement the following methods:
	- void *allocate(int iBytes); - allocate a contiguous block of of size iBytes

	- bool release(void *); - release a previously allocated block
		It should be an error to 'release' a block that has not been returned by 'allocate', indicated by 'release' returning 'false'.

	2. Add a 'resize' method with the following signature:
	void *resize(void *pBlock, int iNewSizeBytes);

	3. Modify your class so it is thread-safe.

	4. Modify your class to, under all circumstances be capable of allocating all of the reserved memory. Provide a description of how this requirement is met and any
	limitations that exist.

	5. Modify your class to detect data being written beyond the end of an allocated block and provide a suitable means to report this to the user of that block.

	6. Add a method to allow a block user to check if a block they are using may have been corrupted by an overwrite beyond the end of an adjacent block.
*/