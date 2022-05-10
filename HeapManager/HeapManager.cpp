#include <iostream>
#include <list>
#include <atomic>
#include <vector>
#include <string>
#include <mutex>
#include <thread>

#define _EXCEPTION_WRITE_BIGGER_THAN_BLOCK std::exception("Size to write is bigger than block size")

class HeapManager
{
private:
	std::list<std::atomic_uint8_t>* m_freeStore;  // free store of the heap manager implemented as a List so that is is really good performance on block allocation / deallocation
	std::list<std::atomic_uint8_t>::iterator m_freeHead; // allocated blocks stay to the left of the list and free ones to the right , free head tracks the first free block
	std::atomic_int m_freeStoreSize; // current size of the free store

public:
	class Block // block class to track the relevant block information, the block itself is a series of blocks inside the Heap freeStore.
	{
	private:
		friend class HeapManager;
		std::list<std::atomic_uint8_t>::iterator m_blockHead; // pointer to the head of the block
		std::atomic_int m_size; // size of the block 
		std::size_t m_integrityHash; // integrity hash of the last time block was written to, so that we can check block integrity
		std::mutex m_blockMutex; // block mutex for thread syncronization 

	public:
		Block(std::list<std::atomic_uint8_t>::iterator head, int iBytes) // initilizes block with head and size
		{
			m_blockHead = head;
			m_size = iBytes;
			std::vector<uint8_t> bytesToCheck;
			auto blockIterator = m_blockHead;
			for (int i = 0; i < m_size; i++) {
				bytesToCheck.push_back(*blockIterator);
				std::advance(blockIterator, 1);
			}
			m_integrityHash = std::hash<std::string>{}(std::string(bytesToCheck.begin(), bytesToCheck.end()));
		}

		std::atomic_int const getSize()
		{
			return m_size.load();
		}

		void writeBytes(std::vector<uint8_t> bytesToWrite) // write bytes to block. 
		{
			std::lock_guard<std::mutex> guard(m_blockMutex); // lock guard for thread synchronization , all synchronization is achieved through lock guards
			std::cout << "\n writing from thread: " << std::this_thread::get_id() << "\n";
			if (bytesToWrite.size() > m_size)
			{
				throw _EXCEPTION_WRITE_BIGGER_THAN_BLOCK;
			}
			auto writeIterator = m_blockHead;
			for (auto valueToWrite : bytesToWrite) {
				*writeIterator = valueToWrite;  // writes byte to block section
				std::advance(writeIterator, 1);
			}
			m_integrityHash = std::hash<std::string>{}(std::string(bytesToWrite.begin(), bytesToWrite.end())); // calculates and stores current integrity hash
		}

		void dumpBlockInfo() // dumps block information to cout
		{
			std::lock_guard<std::mutex> guard(m_blockMutex); // lock guard for thread synchronization , all synchronization is achieved through lock guards
			std::cout << "\nBlock size: " << m_size << " Block head: " << &m_blockHead;
			auto blockIterator = m_blockHead;
			std::cout << "\n";
			for (int i = 0; i < m_size; i++) {
				std::cout << "Value [" << std::to_string(i) << "]=" << std::to_string((uint8_t)*blockIterator) << ",";
				std::advance(blockIterator, 1);
			}
			std::cout << "\n";
		}

		bool checkBlockIntegrity() // method to check block integrity
		{
			std::lock_guard<std::mutex> guard(m_blockMutex); // lock guard for thread synchronization , all synchronization is achieved through lock guards
			std::cout << "\n checking block integrity from thread: " << std::this_thread::get_id() <<"\n";
			std::vector<uint8_t> bytesToCheck;
			auto blockIterator = m_blockHead;
			for (int i = 0; i < m_size; i++) {
				bytesToCheck.push_back(*blockIterator);
				std::advance(blockIterator, 1);
			}
			std::size_t checkHash = std::hash<std::string>{}(std::string(bytesToCheck.begin(), bytesToCheck.end())); // calculate current hash to compare against stored m_integrityHash
			if(checkHash== m_integrityHash)
			{
				std::cout << "\n Block Integrity is ok!";
				return true;
			}
			else {
				std::cout << "\n Error: Block Integrity is corrupted.";
				return false;
			}
		}

		std::thread writeInThread(std::vector<uint8_t> bytesToWrite) // threaded write bytes to block method.
		{
			return std::thread([this, bytesToWrite] { this->writeBytes(bytesToWrite); });
		}
	};

	HeapManager(int mBytes) // initializes HeapManager with size mBytes in MB example: 10 = 10MB
	{
		m_freeStore = new std::list<std::atomic_uint8_t>(1000000 * mBytes);
		m_freeHead = m_freeStore->begin();
		m_freeStoreSize = 1000000*mBytes;
	}

	~HeapManager() 
	{
		delete m_freeStore;
	}

	void* allocate(int iBytes)//allocate a contiguous block of of size iBytes
	{
		Block* newBlock = new Block(m_freeHead, iBytes);
		std::advance(m_freeHead, iBytes);

		return reinterpret_cast<void*>(newBlock);
	}
	bool release(void *block)//release a previously allocated block
	{
		Block *blockToBeDelete = reinterpret_cast<Block*>(block);
		int size = blockToBeDelete->m_size;

		m_freeStore->erase(blockToBeDelete->m_blockHead, std::next(blockToBeDelete->m_blockHead,size));
	
		delete blockToBeDelete;

		m_freeStore->resize(m_freeStoreSize);

		return true;
	}
	void* resize(void* pBlock, int iNewSizeBytes) // resizes a previously allocated block
	{
		Block* blockToBeResized= reinterpret_cast<Block*>(pBlock);
		int size = blockToBeResized->m_size;
		std::list<std::atomic_uint8_t> tempBlock(0);
		std::list<std::atomic_uint8_t>::iterator blockHead = blockToBeResized->m_blockHead;		
		std::list<std::atomic_uint8_t>::iterator blockEnd = std::next(blockHead, size);
		
		tempBlock.splice(tempBlock.begin(), *m_freeStore, blockHead, blockEnd);
		
		m_freeStore->splice(m_freeHead, tempBlock);
		std::advance(m_freeHead, -size);
		blockToBeResized->m_blockHead = m_freeHead;
		blockToBeResized->m_size = iNewSizeBytes;
		std::advance(m_freeHead, iNewSizeBytes);

		return reinterpret_cast<void*>(true);
	}
};

int main(int argc, char* argv[])
{
	HeapManager* testHeap;
	if(argc>1)
	{
		int heapSize = atoi(argv[1]);
		testHeap = new HeapManager(heapSize);
	}
	else {
		testHeap = new HeapManager(10);
	}

	HeapManager::Block *testBlock1 = reinterpret_cast<HeapManager::Block*>(testHeap->allocate(2));

	std::cout << "\nTest block 1: ";
	testBlock1->dumpBlockInfo();
	
	std::vector<uint8_t> testWrite{ 11,23 };
	std::vector<uint8_t> testWrite2{ 22,55 };

	std::thread t1 = testBlock1->writeInThread(testWrite);
	testBlock1->checkBlockIntegrity();
	std::thread t2 = testBlock1->writeInThread(testWrite2);
	testBlock1->checkBlockIntegrity();

	std::cout << "\nTest block 1 after write: ";
	testBlock1->dumpBlockInfo();

	std::vector<uint8_t> testWrite3{99,98,97};
	try
	{
		testBlock1->writeBytes(testWrite3);
	}
	catch (std::exception &err) {
		std::cout << "caught exception: " << err.what();
	}

	std::cout << "\nTest block 1 after exception: ";
	testBlock1->dumpBlockInfo();
	testBlock1->checkBlockIntegrity();

	HeapManager::Block* testBlock2 = reinterpret_cast<HeapManager::Block*>(testHeap->allocate(3));


	testHeap->release(testBlock1);

	std::cout << "\nTest block 2 after release of test block 1: ";
	testBlock2->dumpBlockInfo();


	HeapManager::Block* testBlock3 = reinterpret_cast<HeapManager::Block*>(testHeap->allocate(5));

	testHeap->resize(testBlock2, 10);


	std::cout << "\nTest block 3: ";
	testBlock3->dumpBlockInfo();

	t1.join();
	t2.join();
	delete testHeap;
	std::system("PAUSE");
}

//_______Visual Studio default instructions for project: 
	// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
	// Debug program: F5 or Debug > Start Debugging menu

	// Tips for Getting Started: 
	//   1. Use the Solution Explorer window to add/manage files
	//   2. Use the Team Explorer window to connect to source control
	//   3. Use the Output window to see build output and other messages
	//   4. Use the Error List window to view errors
	//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
	//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
//_______end of Visual Studio default instructions for project: 

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