/*
  ECE 487 Laboratory 7
  Memory Simulator
  A simulator for a two-level momeory heirarchy using set-associative cache mapping. Parameters are prompted
  from the user and memory access operations are given in a file provided by the user. The simulator
  calculates various operational and performance characteristics of the given memory system including
  cache size, bits required for each field, hit rate, operating status, and final status.
  
  Compile:	g++ -std=c++11 -Wall Lab7.cpp -o Lab7.out
  Run:		./Lab7.out
  
  Jonathan Platt
  11807130
*/

#include <fstream>									// Imported for working with files (ifstream)
#include <iostream>									// Imported for interacting with user (cout, cin)
#include <iomanip>									// Imported for improving printing (setw, setprecision)
#include <string>									// Imported for strings
#include <vector>									// Imported for vectors

using namespace std;								// Use standard namespace for brevity and convenience

// Simple enumerated type for whether the replacement policy is LRU or FIFO
enum ReplacementPolicy {LRU,FIFO};					// LRU is Least Recently Used
													// FIFO is First In First Out
// Simple enumerated type for whether an operation is read or write
enum ReadWrite {READ, WRITE};

// Simple enumerated type for whether a memory access results in a hit or miss
enum HitMiss {HIT, MISS};

/*****************************************************************************
Struct name:      MemoryReference
Purpose:          Stores a single memory reference operation (read / write)
                  and the address being operated on
******************************************************************************/
struct MemoryReference {
	ReadWrite operation;							// Whether the operation is read or write 
	int memoryAddress;								// The memory address being read / written
};

/*****************************************************************************
Function name:    log2
Purpose:          Calculates and return the base 2 logarithm of the input
Input parameters: value - int - the number to take the logarithm of
Return value:     int - the base 2 logarithm of the input rounded down to an
                        integer. If the input is less than 1, 0 is returned.
******************************************************************************/
int log2(int value) {
	int power = 0;									// Initialize power (answer) to 0
	while(value > 1) {								// While the input value is > 1 (log2(value) > 0)
		value = value >> 1;							// Right shift value 1 bit (divide by 2)
		power++;									// Increment the power value
	}
	return power;									// Return power once value <= 1
}

/*****************************************************************************
******************************************************************************
Class name:       UserInterface
Purpose:          Prompt the user for input of memory simulator parameters
******************************************************************************/
class UserInterface {
	public:
/*****************************************************************************
Function name:    valuePrompt
Purpose:          Prompt the user for the value of a particular memory feature
Input parameters: prompt - string - the prompt to print to the user
                  minimumVal - int - the minimum value of the feature
                  maximumVal - int - the maximum value of the feature
Return value:     int - the recieved numerical user input
******************************************************************************/
		int valuePrompt(string prompt, int minimumVal, int maximumVal) {
			int size = -1;							// Stores the size recieved from the user
			string buffer;							// String buffer in case of non-numeric input
			while(true) {							// Repeat until we recieve valid input
				cout << prompt;						// Prompt the user using the functions 'prompt' input
				cin >> size;						// Store user input in int 'size'
				// If size is a power of two within the limits, return it as an int
				if(size >= minimumVal && size <= maximumVal && isPowerOfTwo(size)) return size;
				// Otherwise, inform the use their input is invalud
				cout << "Error: Value must be an integer power of two between "
				     << minimumVal << " and " << maximumVal << "." << endl;
				if (cin.fail()) {					// If 'cin' fails (non-integer input)
					cin.clear();					// Clear the error
					cin >> buffer;					// Read the erroneous input into the string buffer
				}
			}
		}
/*****************************************************************************
Function name:    replacementPolicyPrompt
Purpose:          Prompt the user for the memory's replacement policy
Input parameters: None
Return value:     ReplacementPolicy - the user selected replacement policy
******************************************************************************/
		ReplacementPolicy replacementPolicyPrompt() {
			string text;							// Stores the text input recieved from the user
			while(true) {							// Repeat until we recieve valid input
				cout << "Enter the replacement policy (L = LRU, F = FIFO): ";
				cin >> text;
				if(text == "L" || text == "l") return LRU;		// If the input is 'L', return LRU enum
				else if(text == "F" || text == "f") return FIFO;// If the input is 'F', return FIFO enum
				// If neither an L or F is detected, print and error and reprompt the user
				cout << "Error: Character must be an 'L' or an 'F'" << endl;
			}
		}
/*****************************************************************************
Function name:    memoryReferenceFilePrompt
Purpose:          Prompt the user for the memory reference file and read it
Input parameters: memorySize - int - the memorySize size in bytes
Return value:     vector<MemoryReference> - vector containing the memory
                                            operations from reference file
******************************************************************************/
		vector<MemoryReference> memoryReferenceFilePrompt(int memorySize) {
			// Stores the memory references read from the text file as a vector of MemoryReference structs
			vector<MemoryReference> memoryReferences;
			string file;							// Stores the input file handle
			string buffer;							// Stores read from the input file
			int numReferences;						// The number of references from file's first line
			int memoryAddress;						// Stores a memory address read from the file
			bool repeat = true;						// Stores whether to repeat (reprompt user if error)
			while(repeat) {
				repeat = false;						// Do not repeat unless an error is encountered
				cout << "Enter the name of the input file containing the list of "
						"memory references generated by the CPU: ";
				cin >> file;
				ifstream inputFile;					// Create file handler
				inputFile.open(file);				// Open the file recieved from the user input
				if(!inputFile) {					// If the file cannot be opened, print an error
					cout << "Error: Input file: \"" << file << "\" not found" << endl;
					repeat = true;					// And set the repeat parameter
					continue;						// Restart the loop
				}
				inputFile >> numReferences;			// Read the first line (containing number of refs)
				// If the number of references is not an integer greater than 0, print an error
				if (inputFile.fail() || numReferences < 1) {
					cout << "Error: File must contain at least 1 memory reference." << endl;
					repeat = true;					// And set the repeat parameter
					continue;						// Restart the loop
				}
				// Set the vector size to the same as the number of references
				memoryReferences.resize(numReferences);
				// For every reference in the specified file
				for(int i = 0; i < numReferences; i++) {
					inputFile >> buffer;			// Read the operation (read / write)
					// Set the appropriate memoryReference at the current vector index to read / write
					if(buffer == "R") memoryReferences[i].operation = READ;
					else if(buffer == "W") memoryReferences[i].operation = WRITE;
					else {							// If neither read nor write, print an error
						cout << "Error: Input file contains an invalid operation on line "
							 << 3 + i << "." << endl;
						repeat = true;				// Set the repeat parameter
						break;						// And exist the for loop
					}
					inputFile >> memoryAddress;		// Read the memory address from the file
					// If the address is an int between 0 and the memory size
					if(memoryAddress >= 0 && memoryAddress < memorySize && !inputFile.fail()) {
						// Set the address of the current vector index to the read address
						memoryReferences[i].memoryAddress = memoryAddress;
					}
					else {							// If not an int within the bounds, print error
						cout << "Error: Input file contains an invalid memory address on line "
							 << 3 + i << "." << endl;
						repeat = true;				// Set the repeat parameter
						break;						// And exist the for loop
					}
				}
				inputFile.close();
			}
			return memoryReferences;
		}
/*****************************************************************************
Function name:    repeatPrompt
Purpose:          Prompt the user if they would like to continue / repeat
Input parameters: None
Return value:     bool - The user's response (true to repeat, false to end)
******************************************************************************/
		bool repeatPrompt() {
			string text;							// Stores the text input recieved from the user
			while(true) {							// Repeat until we recieve valid input
				cout << "Continue? (y = yes, n = no): ";
				cin >> text;
				if(text == "Y" || text == "y") return true;			// If 'y' (yes), return true
				else if(text == "N" || text == "n") return false;	// If 'n' (no), return false
				// If neither a Y or N is detected, print and error and reprompt the user
				cout << "Error: Character must be a 'y' or an 'n'" << endl;
			}
		}
	private:
/*****************************************************************************
Function name:    powerOfTwo
Purpose:          Returns if the input is a nonnegative integer power of two
Input parameters: value - int - the input to check if it is a power of two
Return value:     bool - true is the input is a power of two, false if not
******************************************************************************/
		bool isPowerOfTwo(int value) {
			if(value < 1) return false;				// Numbers below 1 are not nonnegative powers of two
			for(int i = 1; i <= value; i*=2) {		// Double i until it is greater than the input value
				if(i == value) return true;			// If i is exactly the value, return true
			}
			return false;							// If i never equaled value, it is not a power of two
		}
};

/*****************************************************************************
Struct name:      CacheBlock
Purpose:          Stores the components of a simple cache block including
                  dirty bit, valid bit, tag, and data, where data stores the
                  number of the memory block stored in the cache block
******************************************************************************/
struct CacheBlock {
	int dirtyBit = 0;								// 1 if the cache has had a 'write', 0 if not
	int validBit = 0;								// 1 if the cache contains valid data, 0 if not
	int tag = -1;									// The cache's tag value, or -1 if unknown
	int data = -1;									// The number of the memory block in the cache
};													// or -1 is unknown or not set

/*****************************************************************************
******************************************************************************
Class name:       CacheSet
Purpose:          Models a cache set, which may contain multiple cache blocks,
                  including keeping track a cache's priority (via FIFO or LRU)
                  to faithfully model the cache's replacement policy
******************************************************************************/
class CacheSet {
	public:
/*****************************************************************************
Function name:    CacheSet (constructor)
Purpose:          Creates a CacheSet object with the specified parameters
Input parameters: associativity - int - the associativity of the cache memory
                                        (the number of cache blocks per set)
                  policy - ReplacementPolicy - whether to follow FIFO or LRU
Return value:     none
******************************************************************************/
		CacheSet(int associativity, ReplacementPolicy policy) {
			this->associativity = associativity;	// Set the CacheSet object's associativity
			this->policy = policy;					// Set the CacheSet object's replacement policy
			// Set the size of the cache block and cache block priority vectors to the
			// associativity, since that is the number of cache blocks per set
			cacheBlocks.resize(associativity);
			cacheReplacementPriority.resize(associativity);
			for(int i = 0; i < associativity; i++) {// For every element in the priority queue
				cacheReplacementPriority[i] = i;	// set the element to contain its position index
			}										// meaning that lower indicies are first in the queue
			return;									// to be replaced (ie lower slots in cache filled 1st)
		}
		
/*****************************************************************************
Function name:    getCacheBlock
Purpose:          Gets the cache block at the specified offset within the set
Input parameters: blockNumber - int - the offset of the block within the set
Return value:     CacheBlock - the CacheBlock object with the given offset
******************************************************************************/
		CacheBlock getCacheBlock(int blockNumber) {
			return cacheBlocks[blockNumber];		// Return the block at the given index
		}
		
/*****************************************************************************
Function name:    getCacheBlock
Purpose:          Gets the cache block at the specified offset within the set
Input parameters: blockNumber - int - the offset of the block within the set
Return value:     CacheBlock - the CacheBlock object with the given offset
******************************************************************************/
		HitMiss access(int tag, int data, ReadWrite operation) {
			// Find the index of the cache block with the given tag
			int cacheBlockIndex = findCacheBlock(tag);
			// Update the priority of the given index, returning the updated index
			int updatedIndex = updatePriority(cacheBlockIndex);
			// If the operation is a WRITE, set the dirty bit
			if(operation == WRITE) cacheBlocks[updatedIndex].dirtyBit = 1;
			// If the previous and updated index are the same, then the data was already
			// in the cache and the access was a HIT
			if(cacheBlockIndex == updatedIndex) return HIT;
			// Otherwise, update the cache blocks parameters, including the dirty bit on a READ
			cacheBlocks[updatedIndex].tag = tag;
			cacheBlocks[updatedIndex].data = data;
			if(operation == READ) cacheBlocks[updatedIndex].dirtyBit = 0;
			cacheBlocks[updatedIndex].validBit = 1;
			return MISS;							// Return a MISS (only reached if not a HIT)
		}
	private:
		int associativity;							// Stores the associativity (blocks per set)
		ReplacementPolicy policy;					// Stores the replacement policy (FIFO or LRU)
		vector<CacheBlock> cacheBlocks;				// Vector of cache blocks. Size set by associativity
		vector<int> cacheReplacementPriority;		// Stores block IDs in priority order with lower
													// priorities (first to be replaced) at lower indicies
/*****************************************************************************
Function name:    findCacheBlock
Purpose:          Find the index of the cache block with the given tag value
Input parameters: tag - int - the tag of the cache block to search for
Return value:     int - index of the cache block with the provided tag
                        or -1 if the tag is not found
******************************************************************************/
		int findCacheBlock(int tag) {
			for(int i = 0; i < associativity; i++) {// for every block in the set
				// Return the index if the block tag matches the provided tag
				if(cacheBlocks[i].tag == tag) return i;
			}
			return -1;								// Otherwise return -1 (block with tag not found)
		}

/*****************************************************************************
Function name:    updatePriority
Purpose:          Update the priority of the block that is being accessed
Input parameters: tag - int - the tag of the cache block to search for
Return value:     int - index of the cache block with the provided tag
                        or -1 if the tag is not found
******************************************************************************/
		int updatePriority(int cacheBlockID) {
			if(cacheBlockID != -1) {				// If the block is already in cache (index not -1, hit)
				// Return block id if we're a FIFO policy, since FIFO priorities only change on misses
				if(policy == FIFO) return cacheBlockID;
				else if(policy == LRU) {			// Otherwise (if policy is LRU)
					// Loop through the cache priority queue
					for(int i = 0; i < associativity; i++) {
						// and when the cache block id is found in the priority queue
						if(cacheReplacementPriority[i] == cacheBlockID) {
							// Remove the block id from its current position and put it at the end
							cacheReplacementPriority.erase(cacheReplacementPriority.begin()+i);
							cacheReplacementPriority.push_back(cacheBlockID);
							return cacheBlockID;	// Return the block id (unchanged)
						}
					}
					// If the block id isn't found in the priority queue there is an error
					cerr << "Cache Set Priority Error" << endl;
					throw "Cache Set Priority Error";
				}
			}
			else {									// Otherwise, if the block was not in cache (miss)
				// Find the lowest priority block id in the queue (index 0), and move it to the back
				cacheBlockID = cacheReplacementPriority[0];
				cacheReplacementPriority.erase(cacheReplacementPriority.begin());
				cacheReplacementPriority.push_back(cacheBlockID);
			}
			return cacheBlockID;					// Return the updated block id from the queue
		}
};

/*****************************************************************************
******************************************************************************
Class name:       MemorySimulator
Purpose:          Class for simulating memory supporting generation of the
                  structure and simulation of reference operations
******************************************************************************/
class MemorySimulator {
	public:
/*****************************************************************************
Function name:    MemorySimulator (constructor)
Purpose:          Create the memory simulator object and initialize attributes
Input parameters: memorySize - int - The size of main memory in bytes
                  cacheSize - int - The size of the cache memory in bytes
                  cacheBlockSize - The size of the cache blocks in bytes
                  associativity - The degree of associativity, 
                                  1 meaning direct mapped
                  policy - ReplacementPolicy - The memory's replacement policy
                                               LRU or FIFO
Return value:     none
******************************************************************************/
		MemorySimulator(int memorySize, int cacheSize, int cacheBlockSize,
						int associativity, ReplacementPolicy policy) {
			// Set the objects internal attributes
			this->cacheSize = cacheSize;
			this->cacheBlockSize = cacheBlockSize;
			this->associativity = associativity;
			// Calculate number of cache blocks, cache sets, and memory blocks total
			cacheBlocks = cacheSize/cacheBlockSize;
			cacheSets = cacheBlocks/associativity;
			memoryBlocks = memorySize/cacheBlockSize;
			// Calculate the number of address bits and the sizes of the offset, index and tag fields
			totalAddressBits = log2(memorySize);
			offsetBits = log2(cacheBlockSize);
			indexBits = log2(cacheSets);
			tagBits = log2(memoryBlocks/cacheSets);
			// Initialize the cache memory, which is made up of the previously calculated number of
			// cache sets, each of which has the same associativity and replacement policy
			cacheMemory = vector<CacheSet>(cacheSets,CacheSet(associativity,policy));
			return;
		}
		
/*****************************************************************************
Function name:    printMemoryInfo
Purpose:          Prints various pieces of information about the memory module
                  including address size in bits, offset, index, and tag sizes
                  and the total cache size accounting for overhead bits
Input parameters: none (values come from MemorySimulator object)
Return value:     none (Output directly printed to console)
******************************************************************************/
		void printMemoryInfo() {
			// Print the number of address lines requires and the size of the 3 fields
			cout << "Total address lines required = " << totalAddressBits << endl;
			cout << "Number of bits for offset = " << offsetBits << endl;
			cout << "Number of bits for index = " << indexBits << endl;
			cout << "Number of bits for tag = " << tagBits << endl;
			// Calculate the total cache size (2 accounts for the dirty and valid bits)
			// divide by 8 to convert the dirty, valid, and tag bit sum to bytes
			float totalCacheSize = (2 + tagBits)*cacheBlocks/8. + cacheSize;
			// Print toal cache size, displaying a maximum of 8 significant figures (then revert)
			cout << "Total cache size required = " << setprecision(8) << totalCacheSize
				 << setprecision(6) << " bytes" << endl << endl;
		}
		
/*****************************************************************************
Function name:    runSimulation
Purpose:          Runs the memory simulation using the given vector of memory
                  references and prints summary statistics on the program
Input parameters: memoryReferences - vector<MemoryReference>& - Vector of
                                   memory references to simulate, passed by
                                   reference to avoid needless data copying
Return value:     none (Output directly printed to console)
******************************************************************************/
		void runSimulation(vector<MemoryReference>& memoryReferences) {
			// Print the memory references header for the simulation
			cout << "main memory address" << setw(12) << "mm blk #" << setw(12) << "cm set #" 
				 << setw(12) << "cm blk #" << setw(12) << "hit/miss" << endl;
			cout << string(67,'-') << endl;			// Print line to separate header from data
			int hitCount = 0;						// Counts the number of cache hits while simulating
			// For each memory reference item in the vector
			for(unsigned int i = 0; i < memoryReferences.size(); i++) {
				// Run the simulation one step
				HitMiss status = simulationStep(memoryReferences[i]);
				// Print the result of the current simulation step
				printSimulationStep(memoryReferences[i].memoryAddress,status);
				if(status == HIT) hitCount++;		// If the operation is a hit, increase the hit count
			}
			// Calculate the ideal hit count for the memory reference file
			int idealHitCount = calculateIdealHitCount(memoryReferences);
			// Print ideal hit count and calculated ideal hit rate
			cout << "\nHighest possible hit rate = " << idealHitCount << "/" << memoryReferences.size()
				 << " = " << (float)idealHitCount/memoryReferences.size()*100 << "%" << endl;
			// Print actual hit count and calculated actual hit rate
			cout << "Actual hit rate = " << hitCount << "/" << memoryReferences.size()
				 << " = " << (float)hitCount/memoryReferences.size()*100 << "%" << endl << endl;
			return;
		}
		
/*****************************************************************************
Function name:    printCache
Purpose:          Prints the state of the cache of the MemorySimulator object
                  at the current simulation step
Input parameters: none (values come from MemorySimulator object)
Return value:     none (Output directly printed to console)
******************************************************************************/
		void printCache() {
			// Print cache contents header
			cout << "Cache blk #" << setw(12) << "dirty bit" << setw(12) << "valid bit" 
				 << setw(10) << "tag" << setw(16) << "Data" << endl;
			cout << string(67,'-') << endl;			// Print line to separate header from data
			for(int i = 0; i < cacheBlocks; i++) {	// For each cache block in the memory simulator
				printCacheBlock(i);					// print the cache block
			}
			cout << endl;							// Print blank line for spacing
			return;
		}
	private:
		int cacheSize;								// Stores the size of cache in bytes
		int cacheBlockSize;							// Stores the size of cache blocks in bytes
		int associativity;							// Stores the memory's degree of associativity 
		int cacheBlocks;							// Stores the number of cache blocks
		int cacheSets;								// Stores the number of cache sets
		int memoryBlocks;							// Stores the number of memory blocks
		int totalAddressBits;						// Stores the number of address bits for memory
		int offsetBits;								// Stores the number of offset bits in the address
		int indexBits;								// Stores the number of index bits in the address
		int tagBits;								// Stores the number of tag bits in the address
		vector<CacheSet> cacheMemory;				// Emulated cache memory device made from a vector
													// of cache sets
/*****************************************************************************
Function name:    simulationStep
Purpose:          Runs the memory simulator one step, performing the operation
                  specified in the memory reference parameter
Input parameters: memoryReference - MemoryReference& - the memory access
                                    operation to perform, contains an address
                                    and a read/write flag
Return value:     HitMiss - whether the access results in a hit or a miss in
                            the cache
******************************************************************************/
		HitMiss simulationStep(MemoryReference& memoryReference) {
			// Extract the parameters from the MemoryReference struct
			int memoryAddress = memoryReference.memoryAddress;
			ReadWrite operation = memoryReference.operation;
			// Calculate the memory block and cache set the operation would access
			int memoryBlockNumber = memoryAddress / cacheBlockSize;
			int cacheSetNumber = memoryBlockNumber % cacheSets;
			// Calculate the tag value for the memory address being referenced
			int tag = memoryBlockNumber / cacheSets;
			// Perform the cache access operation and store the return status (hit or miss)
			HitMiss status = cacheMemory[cacheSetNumber].access(tag,memoryBlockNumber,operation);
			return status;
		}
		
/*****************************************************************************
Function name:    calculateIdealHitCount
Purpose:          Calculates and returns the ideal hit rate of a sequence of
                  memory references
Input parameters: memoryReferences - vector<MemoryReference>& - the memory 
                                     access operations to consider
Return value:     int - the ideal hit count of the sequence
******************************************************************************/
		int calculateIdealHitCount(vector<MemoryReference>& memoryReferences) {
			// Create a zero initialized array of ints to count references to memory blocks
			int memoryBlockReferences[memoryBlocks] = {0};
			// For every memory reference operation in the vector input
			for(unsigned int i = 0; i < memoryReferences.size(); i++) {
				// Calculate the memory block number under reference
				int memoryBlockNumber = memoryReferences[i].memoryAddress/cacheBlockSize;
				// Increment the count at the index given by the memory block number
				memoryBlockReferences[memoryBlockNumber]++;
			}
			int idealHitCount = 0;					// Stores the ideal number of memory hits
			for(int i = 0; i < memoryBlocks; i++) {	// For every memory block
				// If the number of references to that block is greater than one, increase idealHitCount
				// by the number of references minus 1, since the first access must always be a miss 
				if(memoryBlockReferences[i] > 1) idealHitCount += memoryBlockReferences[i] - 1;
			}
			return idealHitCount;					// Return the ideal hit count
		}

/*****************************************************************************
Function name:    printCacheBlock
Purpose:          Prints the contents of a single cache block
Input parameters: cacheBlockNumber - int - the number of the block to print 
Return value:     none (Output directly printed to console)
******************************************************************************/
		void printCacheBlock(int cacheBlockNumber) {
			// Calculate the cache set and set offset of the specified block
			int cacheSetNumber = cacheBlockNumber / associativity;
			int cacheSetOffset = cacheBlockNumber % associativity;
			// Get the specified cache block from the calculated cache set in the cache memory vector
			CacheBlock cacheBlock = cacheMemory[cacheSetNumber].getCacheBlock(cacheSetOffset);
			string tagString;						// Strings to store tag and data for printing
			string dataString;
			if(cacheBlock.validBit == 0) {			// If the cache block is considered invalid
				tagString = string(tagBits,'x');	// Set the tag string to contain the same number of
				dataString = "?";					// 'X's as there are tag bits. Also set data to '?'
			} else {								// Otherwise
				// Calculate the binary equivalent of the tag and format the data string
				tagString = toBinary(cacheBlock.tag,tagBits);
				dataString = "mm blk # " + to_string(cacheBlock.data);
			}
			// Print the parameterized contents of the cache block to the console
			cout << setw(7) << cacheBlockNumber << setw(12) << cacheBlock.dirtyBit << setw(12)
				 << cacheBlock.validBit << setw(15) << tagString << string(8,' ') << dataString << endl;
			return;
		}

/*****************************************************************************
Function name:    printSimulationStep
Purpose:          Prints the a memory reference and its result (a single
                  simulation step)
Input parameters: memoryAddress - int - the memory address being accessed
                  status - HitMiss - the result of the memory access operation
Return value:     none (Output directly printed to console)
******************************************************************************/
		void printSimulationStep(int memoryAddress, HitMiss status) {
			// Calculate the memory block and cache set the operation would access
			int memoryBlockNumber = memoryAddress / cacheBlockSize;
			int cacheSetNumber = memoryBlockNumber % cacheSets;
			// Generate string for cacheBlockNumber, since it could represent a range
			string cacheBlockNumber;
			// If the associativity is 1, there can only be one possible cache block number
			if(associativity == 1) cacheBlockNumber = to_string(cacheSetNumber);
			// If the associativity is 2, there is a binary choice of cache block number
			else if(associativity == 2) cacheBlockNumber = to_string(cacheSetNumber*2)
									+ " or " + to_string(cacheSetNumber*2 + 1);
			// If the associativity is > 2, there is a consecutive range of possible cache block numbers
			else cacheBlockNumber = to_string(cacheSetNumber*associativity)
									+ " - " + to_string((cacheSetNumber+1)*associativity - 1);
			// Convert the returned HitMiss status into a string
			string statusString = (status == HIT) ? "hit" : "miss";
			
			// Print the result of the memory simulator step
			cout << setw(11) << memoryAddress << setw(17) << memoryBlockNumber << setw(12) << cacheSetNumber 
				 << setw(15) << cacheBlockNumber << setw(10) << statusString << endl;
			return;
		}

/*****************************************************************************
Function name:    toBinary
Purpose:          Converts an integer to a binary number in the specified
                  number of bits
Input parameters: value - int - the number to convert to binary
                  numberOfBits - int - the desired number of bits to output 
Return value:     string - the binary representation of the input value
******************************************************************************/
		string toBinary(int value, int numberOfBits) {
			string binary = "";						// Create an empty string
			for(int i = 0; i < numberOfBits; i++) {	// For every desired bit
				int currentBit = (value >> i) % 2;	// Calculate the bit at the current bit offset
				// Convert the bit to a string and append it to the front of the string
				binary = to_string(currentBit) + binary;
			}
			return binary;							// Return the string
		}
};

/*****************************************************************************
Function name:    main
Purpose:          Main program loop, get user input and outputs results
Input parameters: None
Return value:     int - returns 0 if program terminates with no errors
******************************************************************************/
int main() {
	UserInterface interface;						// Instantiate interface object
	do {											// Repeat until user terminates
		// Prompt user for integer size / associativity inputs
		int memorySize = interface.valuePrompt("Enter the size of main memory in bytes: ",4,32768);
		int cacheSize = interface.valuePrompt("Enter the size of the cache in bytes: ",2,memorySize);
		int cacheBlockSize = interface.valuePrompt("Enter the cache block/line size: ",2,cacheSize);
		int cacheBlocks = cacheSize/cacheBlockSize;	// Number of cache blocks is size over block size 
		cout << endl;								// Print blank line for spacing
		int associativity = interface.valuePrompt("Enter the degree of set-associativity "
									"(input n for an n-way set-associative mapping): ",1,cacheBlocks);
		// Prompt user for replacement policy
		ReplacementPolicy policy = interface.replacementPolicyPrompt();
		// Prompt user for memory reference file and read the file of memory references
		vector<MemoryReference> memoryReferences = interface.memoryReferenceFilePrompt(memorySize);
		
		cout << endl << "Simulator Output:" << endl;
		// Create simulator with user provided cache and main memory sizes and layout
		MemorySimulator simulator(memorySize,cacheSize,cacheBlockSize,associativity,policy);
		simulator.printMemoryInfo();				// Print information about the memory devices buses
		simulator.runSimulation(memoryReferences);	// Run simulation with memory references from file
		simulator.printCache();						// Print the final state of the cache device
	} while(interface.repeatPrompt());				// Prompt user and conditionally restart program
	return 0;
}