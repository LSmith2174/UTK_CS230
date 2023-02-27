//Laura Smith
//4/3/2022

//This part of the project uses what we made in the fetch and decode sections
//and builds the code. This is the execute portion, which uses the ALU to do 
//the operation needed for the given instruction. The following instructions 
//are supported in this stage:
//LUI, AUIPC, JAL, JALR, BEQ, BNE, BLT, BGE, LB, LH, LW, LD, LBU, LHU, LWU, SB, SH, SW, SD, ADDI, XORI, ORI, ANDI, SLLI, SRLI, SRAI, ADD, SUB, SLL, XOR, SRL, SRA, OR, AND, ECALL, MUL, DIV, REM.


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


enum OpcodeCategories {
   LOAD, STORE, BRANCH, JALR,
   JAL, OP_IMM, OP, AUIPC, LUI,
   OP_IMM_32, OP_32, SYSTEM,
   UNIMPL
    };

const OpcodeCategories OPCODE_MAP[4][8] = {
   // First row (inst[6:5] = 0b00)
   { LOAD, UNIMPL, UNIMPL, UNIMPL, OP_IMM, AUIPC, OP_IMM_32, UNIMPL }, 
   // Second row (inst[6:5] = 0b01)
   { STORE, UNIMPL, UNIMPL, UNIMPL, OP, LUI, OP_32, UNIMPL },
   // Third row (inst[6:5] = 0b10)
   { UNIMPL, UNIMPL, UNIMPL, UNIMPL, UNIMPL, UNIMPL, UNIMPL, UNIMPL },
   // Fourth row (inst[6:5] = 0b11)
   { BRANCH, JALR, UNIMPL, JAL, SYSTEM, UNIMPL, UNIMPL, UNIMPL }
};

    int64_t sign_extend(int64_t value, int8_t index) {
    if ((value >> index) & 1) {
        // Sign bit is 1
        return value | (-1UL << index);
    }
    else {
        // Sign bit is 0
        return value & ~(-1UL << index);
    }
}

struct DecodeOut {
   OpcodeCategories op;
   uint8_t rd;
   uint8_t funct3;
   uint8_t funct7;
   int64_t offset;    // Offsets for BRANCH and STORE
   int64_t left_val;  // typically the value of rs1
   int64_t right_val; // typically the value of rs2 or immediate

   friend ostream &operator<<(ostream &out, const DecodeOut &dec) {
       ostringstream sout;
        sout << "Operation: ";
        switch (dec.op) {
            case LUI:
                sout << "LUI";
                break;
            case AUIPC:
                sout << "AUIPC";
                break;
            case LOAD:
                sout << "LOAD";
                break;
            case STORE:
                sout << "STORE";
                break;
            case OP_IMM:
                sout << "OPIMM";
                break;
            case OP_IMM_32:
                sout << "OPIMM32";
                break;
            case OP:
                sout << "OP";
                break;
            case OP_32:
                sout << "OP32";
                break;
            case BRANCH:
                sout << "BRANCH";
                break;
            case JALR:
                sout << "JALR";
                break;
            case JAL:
                sout << "JAL";
                break;
            case SYSTEM:
                sout << "SYSTEM";
                break;
            case UNIMPL:
                sout << "NOT-IMPLEMENTED";
                break;
        }
        sout << '\n';
        sout << "RD       : " << (uint32_t)dec.rd << '\n';
        sout << "funct3   : " << (uint32_t)dec.funct3 << '\n';
        sout << "funct7   : " << (uint32_t)dec.funct7 << '\n';
        sout << "offset   : " << dec.offset << '\n';
        sout << "left     : " << dec.left_val << '\n';
        sout << "right    : " << dec.right_val;
        return out << sout.str();
   }
};

//List of ALU commands
enum AluCommands {
   ALU_ADD,
   ALU_SUB,
   ALU_MUL,
   ALU_DIV,
   ALU_REM,
   ALU_SLL,
   ALU_SRL,
   ALU_SRA,
   ALU_AND,
   ALU_OR,
   ALU_XOR,
   ALU_NOT
};

struct ExecuteOut {
    int64_t result;
    uint8_t n, z, c, v;


    friend ostream &operator<<(ostream &out, const ExecuteOut &eo) {
        ostringstream sout;
        sout << "Result: " << eo.result << " [NZCV]: " 
             << (uint32_t)eo.n 
             << (uint32_t)eo.z 
             << (uint32_t)eo.c
             << (uint32_t)eo.v;
        return out << sout.str();
    }
};

//The ALU Commands taking a left and right value and producing a result
ExecuteOut alu(AluCommands cmd, int64_t left, int64_t right) {
    ExecuteOut ret;
    switch (cmd) {
        case ALU_ADD:
            ret.result = left + right;
        break;
        case ALU_SUB:
            ret.result = left - right;
        break;
        case ALU_MUL:
            ret.result = left * right;
        break;
        case ALU_DIV:
            ret.result = left / right;
        break;
        case ALU_REM:
            ret.result = left % right;
        break;
        case ALU_SLL:
             ret.result = left << right;
        break;
        case ALU_SRL:
            ret.result = static_cast<uint64_t>(left) >> right;
        break;
        case ALU_SRA:
            ret.result = left >> right;
        break;
        case ALU_AND:
            ret.result = left & right;
        break;
        case ALU_OR:
            ret.result = left | right;
        break;
        case ALU_XOR:
            ret.result = left ^ right;
        break;
        case ALU_NOT:
            ret.result = ~left;
        break;
    }
    // Now that we have the result, determine the flags.
    uint8_t sign_left = (left >> 63) & 1;
    uint8_t sign_right = (right >> 63) & 1;
    uint8_t sign_result = (ret.result >> 63) & 1;
    ret.z = !ret.result;
    ret.n = sign_result;
    ret.v = ~sign_left & ~sign_right & sign_result |
             sign_left & sign_right & ~sign_result;
    ret.c = (ret.result > left) || (ret.result > right);
    return ret;
}



const int MEM_SIZE = 1 << 18;
const int NUM_REGS = 32;
class Machine {
   char *mMemory;   // The memory.
   int mMemorySize; // The size of the memory (should be MEM_SIZE)
   int64_t mPC;     // The program counter
   int64_t mRegs[NUM_REGS]; // The register file

   FetchOut mFO;    // Result of the fetch method.

   DecodeOut mDO;   // Result of the decode method.

   ExecuteOut mEO;  // Result of the Execute method.

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

    //All of the decoders, using the table provided, the types are broken down
    //into rd, funct3, funct7, rs1, rs2, and the immediate
    void decode_r() {
    mDO.rd        = (mFO.instruction >> 7) & 0x1f;
    mDO.funct3    = (mFO.instruction >> 12) & 7;
    mDO.left_val  = get_xreg(mFO.instruction >> 15); // get_xreg truncates for us
    mDO.right_val = get_xreg(mFO.instruction >> 20);
    mDO.funct7    = (mFO.instruction >> 25) & 0x7f;
    }

    void decode_i() {
    mDO.rd        = (mFO.instruction >> 7) & 0x1f;
    mDO.funct3    = (mFO.instruction >> 12) & 7;
    mDO.left_val  = get_xreg(mFO.instruction >> 15); // get_xreg truncates for us
    mDO.right_val = sign_extend((((mFO.instruction >> 20) & 0x7ff) << 0), 11);
    }

    void decode_s() {
    mDO.rd        = (mFO.instruction >> 7) & 0x1f;
    mDO.funct3    = (mFO.instruction >> 12) & 7;
    mDO.left_val  = get_xreg(mFO.instruction >> 15); // get_xreg truncates for us
    mDO.right_val = get_xreg(mFO.instruction >> 20);
    mDO.offset    = sign_extend((((mFO.instruction >> 7) & 0x1f) << 0) |
                                (((mFO.instruction >> 25) & 0x7f) << 5), 11);
    }

    
    void decode_b() {
    mDO.rd        = (mFO.instruction >> 7) & 0x1f;
    mDO.funct3    = (mFO.instruction >> 12) & 7;
    mDO.left_val  = get_xreg(mFO.instruction >> 15); // get_xreg truncates for us
    mDO.right_val = get_xreg(mFO.instruction >> 20);
    mDO.offset    = sign_extend((((mFO.instruction >> 31) & 1) << 12) |
                                (((mFO.instruction >> 25) & 0x3f) << 5) |
                                (((mFO.instruction >> 8) & 0xf) << 1) | 
                                (((mFO.instruction >> 7) & 1) << 11), 12);
    }

    void decode_u() {
    mDO.rd        = (mFO.instruction >> 7) & 0x1f;
    mDO.right_val    = sign_extend((((mFO.instruction >> 12) & 0xfffff) << 12), 31);
    }

    void decode_j() {
    mDO.rd        = (mFO.instruction >> 7) & 0x1f; 
    mDO.right_val    = sign_extend((((mFO.instruction >> 12) & 0xff) << 12) |
                                (((mFO.instruction >> 20) & 1) << 11) |
                                (((mFO.instruction >> 21) & 0x3ff) << 1) | 
                                (((mFO.instruction >> 31) & 1) << 20), 20);
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

    //truncates the value for us
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

    void decode() {
    uint8_t opcode_map_row = (mFO.instruction >> 5) & 3;
    uint8_t opcode_map_col = (mFO.instruction >> 2) & 7;
    uint8_t inst_size      = mFO.instruction & 3;
    if (inst_size != 3) {
        cerr << "[DECODE] Invalid instruction (not a 32-bit instruction).\n";
        return;
    }
    mDO.op = OPCODE_MAP[opcode_map_row][opcode_map_col];
    // Decode the rest of mDO based on the instruction type
    switch (mDO.op) {
    case LOAD:
    case JALR:
    case OP_IMM:
    case OP_IMM_32:
    case SYSTEM:
        decode_i();
    break;
    case STORE:
        decode_s();
    break;
    case BRANCH:
        decode_b();
    break;
    case JAL:
        decode_j();
    break;
    case AUIPC:
    case LUI:
        decode_u();
    break;
    case OP:
    case OP_32:
        decode_r();
    break;
    default:
        cerr << "Invalid op type: " << mDO.op << '\n';
    break;
        }
    }

    DecodeOut &debug_decode_out() { 
      return mDO; 
   }

    // telling the ALU what to do for each instruction
   void execute() {
   AluCommands cmd;
   // Most instructions will follow left/right
   // but some won't, so we need these:
   int64_t op_left = mDO.left_val;
   int64_t op_right = mDO.right_val; 
   
   if (mDO.op == BRANCH) {
      // A branch needs to subtract the operands
      cmd = ALU_SUB;
   }
   else if (mDO.op == LOAD || mDO.op == STORE) {
      // For loads and stores, we need to add the
      // offset with the base register.
      cmd = ALU_ADD;
   }
   else if (mDO.op == OP || mDO.op == OP_32) {
      // We can't tell which ALU command to use until
      // we read the funct3 and funct7
      if (mDO.op == OP_32) {
            op_left = sign_extend(op_left, 31);
            op_right = sign_extend(op_right, 31);
      }
      switch (mDO.funct3) {
         case 0b000: // ADD or SUB
             if (mDO.funct7 == 0) {
                cmd = ALU_ADD;
             }
             else if (mDO.funct7 == 32) {
                cmd = ALU_SUB;
             }
             //MUL
             else if (mDO.funct7 == 1){
                cmd = ALU_MUL;
             }
         break;
         // Finish the rest of the OP functions here.
         case 0b001:
         //SLL
            cmd = ALU_SLL;
         break;

         case 0b100:
         //XOR
            if (mDO.funct7 == 0){
                cmd = ALU_XOR;
            }
         //DIV
            else if (mDO.funct7 == 1){
                cmd = ALU_DIV;
            }
         break;

         case 0b101:
         //SRL
            if (mDO.funct7 == 0) {
                cmd = ALU_SRL;
            }
         //SRA
            else if (mDO.funct7 == 32) {
                cmd = ALU_SRA;
            }
         break;

         case 0b110:
         //OR
            if (mDO.funct7 == 0){
                cmd = ALU_OR;
            }
         //REM
            else if (mDO.funct7 == 1){
                cmd = ALU_REM;
            }
            break;
        
         case 0b111:
         //AND
            cmd = ALU_AND;
            break; 
      }

   }

   else if (mDO.op == OP_IMM || mDO.op == OP_IMM_32) {
       if (mDO.op == OP_IMM_32){
            op_left = sign_extend(op_left, 31);
            op_right = sign_extend(op_right, 31);
       }
       switch (mDO.funct3) {
         case 0b000: 
         //ADDI
            cmd = ALU_ADD;
            break;
         
         case 0b100:
         //XORI
            cmd = ALU_XOR;
            break;

         case 0b110:
         //ORI
            cmd = ALU_OR;
            break;
        
         case 0b111:
         //ANDI
            cmd = ALU_AND;
            break; 
        
         case 0b001:
         //SLLI
            cmd = ALU_SLL;
         break;
         
         case 0b101:
         //SRLI
         if (mDO.funct7 == 0){
            cmd = ALU_SRL;
         }
         //SRAI
         else if (mDO.funct7 == 32){
            cmd = ALU_SRA;
         }
         
         break;
      }
   }
   else if (mDO.op == JALR) {
       // JALR has an offset and a register value that need to be added together.
       cmd = ALU_ADD;
   }
   else if (mDO.op == JAL || mDO.op == SYSTEM){
       // JAL and ECALL effectively do nothing, but ALU has to do something
        int64_t op_left = 0;
        int64_t op_right = 0; 
       cmd = ALU_ADD;
   }
   else if (mDO.op == AUIPC){
       //AUIPC adds program counter to imm
        int64_t op_left = mPC; 
       cmd = ALU_ADD;
   }
   else if (mDO.op == LUI){
       //LUI sets result to upper imm by adding 0 to it in the ALU
        int64_t op_left = 0; 
       cmd = ALU_ADD;
   }
   
   mEO = alu(cmd, op_left, op_right);
   }

ExecuteOut &debug_execute_out() { 
      return mEO; 
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

    Machine mach (arr, MEM_SIZE);
    //Loop through instructions
    //Run fetch, decode, and execute then move the program counter to the next 4 bytes
    while (mach.get_pc() != size){
        mach.fetch();
        cout << mach.debug_fetch_out() << '\n';
        mach.decode();
        cout << mach.debug_decode_out() << '\n';
        mach.execute();
        cout << mach.debug_execute_out() << '\n';
        mach.set_pc(mach.get_pc() + 4);
    }


return 0; 
}