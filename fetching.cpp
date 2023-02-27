//Laura Smith
//3/24/2022

//This part of the project takes a command line argument including a file
//name, but if there is no file name it will return an error. The code takes
//the binary file and stores the instructions into an array through reading
//four bytes at a time then moving the program counter to the next four byte
//instruction. This code emulates the 'fetch' stage of the pipeline, which 
//is the first step of the pipline.

#include <iostream>
#include <cstdint>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <iomanip>
using namespace std;

struct FetchOut {
    uint32_t instruction;

    // The code below allows us to cout a FetchOut structure.
    // We can use this to debug our code.
    // FetchOut fo = { 0xdeadbeef };
    // cout << fo << '\n';
    friend ostream &operator<<(ostream &out, const FetchOut &fo) {
        ostringstream sout;
        sout << "0x" << hex << setfill('0') << right << setw(8) << fo.instruction;
        return out << sout.str();
    }
};

const int MEM_SIZE = 1 << 18;
const int NUM_REGS = 32;
class Machine {
   char *mMemory;   // The memory.
   int mMemorySize; // The size of the memory (should be MEM_SIZE)
   int64_t mPC;     // The program counter
   int64_t mRegs[NUM_REGS]; // The register file

   FetchOut mFO;    // Result of the fetch() method.

   // Read from the internal memory
   // Usage:
   // int myintval = memory_read<int>(0); // Read the first 4 bytes
   // char mycharval = memory_read<char>(8); // Read byte index 8
   template<typename T>
   T memory_read(int64_t address) const {
       return *reinterpret_cast<T*>(mMemory + address);
   }

   // Write to the internal memory
   // Usage:
   // memory_write<int>(0, 0xdeadbeef); // Set bytes 0, 1, 2, 3 to 0xdeadbeef
   // memory_write<char>(8, 0xff);      // Set byte index 8 to 0xff
   template<typename T>
   void memory_write(int64_t address, T value) {
       *reinterpret_cast<T*>(mMemory + address) = value;
   }
public:
   Machine(char *mem, int size) {
      mMemory = mem;
      mMemorySize = size;
      mPC = 0;
      set_xreg(2, mMemorySize);
   }

   int64_t get_pc() const {
      return mPC;
   }
   void set_pc(int64_t to) {
      mPC = to;
   }

   int64_t get_xreg(int which) const {
      which &= 0x1f; // Make sure the register number is 0 - 31
      return mRegs[which];
   }
   void set_xreg(int which, int64_t value) {
      which &= 0x1f;
      mRegs[which] = value;
   }

   void fetch() {
      //read 4 bytes at a time
    mFO.instruction = memory_read<uint32_t>(mPC);
   }
   FetchOut &debug_fetch_out() { 
      return mFO; 
   }
};

int main (int argc, char *argv[]) {

    // If a filename isn't entered then print error and exit
    if (argc != 2){
        cout << "Error: No File Name Provided";
        return 0;
    }

    // Open binary file and return error and exit if unable to open
    ifstream fin (argv[1], ios::binary);
    if (!fin.is_open()) {
        cout << "File could not be opened.";
        return 0;
    }

    // Calculate file size
    fin.seekg(0, ios::end);
    int size = fin.tellg();

    // Return pointer to beginning
    fin.seekg(0, ios::beg);

    // If the size isn't a multiple of 4 then print error and exit
    if (size % 4 != 0) {
        cout << "Incorrect File Length";
        return 0;
    }

    // New array char pointer 262KB
    char *arr = new char[262*1024];

    // Read from file and assign to array
    fin.read (arr, size);

    // Close file
    fin.close();

    Machine MachineFetch (arr, MEM_SIZE);
    //Loop through instructions
    //Run fetch then move the program counter to the next 4 bytes
    while (MachineFetch.get_pc() != size){
        MachineFetch.fetch();
        cout << MachineFetch.debug_fetch_out() << '\n';
        MachineFetch.set_pc(MachineFetch.get_pc() + 4);
    }
return 0; 
}