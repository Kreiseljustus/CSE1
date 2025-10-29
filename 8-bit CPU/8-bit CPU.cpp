#include <iostream>
#include <vector>

#include <filesystem>
#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <fstream>

#define MEMORY_SIZE 512
#define PROGRAM_MEM_SIZE 256

#define VALID_W_MEM_ADDR(addr) (true/*(addr > PROGRAM_MEM_SIZE - 1) && (addr < MEMORY_SIZE)*/)
#define VALID_R_MEM_ADDR(addr) ((addr >= 0) && (addr <= MEMORY_SIZE))

enum OpCode {
    LDA = 0x01, //Load a value into register A
    LDB = 0x02, //Load a value into register B
    ADD = 0x03, //Adds A + B registers and stores in A
    SUB = 0x04, //Subtracts A - B registers and stores in A
    OUTV = 0x05, //Output a value. Arg1: memory adress to value
    STA = 0x06, //Store A into memory. Arg1: memory adress to store
    LDM = 0x07, //Load A from memory. Arg1: memory adress to load
    JEZ = 0x08, //Jump when zero flag is set to 1. Arg1: memory adress to jump to
    JNZ = 0x09, //Jump when zero flag is set to 0. Arg1: memory adress to jump to
    MUL = 0x0A, //Multiply A * B and stores in A
    DIV = 0x0B, //Divide A / B and stores in A
    INP = 0x0C, //Takes input and stores in memory adress. Arg1: memory adress to store
    LD = 0x0D, //Load a register with a value from a memory adress. Arg1: register (0x01 = A, 0x02 = B), Arg2: memory adress to load

    HLT = 0xFF //Stops the cpu
};

struct TestCPU {
    //Registers
    uint8_t A = 0;
    uint8_t B = 0;
    //Program Counter
    uint8_t PC = 0;

    //First bit = Invalid adress provided
    //Second bit = Previous math operation is 0
    //Third bit = Carry flag (unsigned overflow)
    //Fourth bit = Divide by Zero
    //Fith bit = Unused
    //Sixth bit = Unused
    //Seventh bit = Unused
    //Eighth bit = Unused
    uint8_t Flags = 0; 

    //MEMORY_SIZE Bytes memory
    //First PROGRAM_MEM_SIZE Bytes are program memory and read-only
    uint8_t memory[MEMORY_SIZE]{};
    bool running = true;

    void loadProgram(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            std::cerr << "Failed to open file: " << filepath << std::endl;
            return;
        }

        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        if (fileSize > PROGRAM_MEM_SIZE) {
            std::cout << "The program is too big to load into the available memory. Increase PROGRAM_MEM_SIZE to fix" << std::endl;
            return;
        }

        file.read(reinterpret_cast<char*>(memory), fileSize);
    }


    void tick() {
        OpCode opcode = static_cast<OpCode>(memory[PC++]); //Get opcode at program counter and increament by 1 (which would be the next instruction or an argument)

        switch (opcode) {
        case LDA: {
            A = memory[PC++];
            break;
        }
        case LDB: {
            B = memory[PC++];
            break;
        }
        case ADD: {
            uint16_t result = A + B;
            uint8_t newValue = result & 0xFF;

            bool carry = result > 0xFF;

            if (A == 0) {
                Flags |= 0b00000010;
            }
            else {
                Flags &= ~0b00000010;
            }

            if (carry) {
                Flags |= 0b00000100;
            }
            else {
                Flags &= ~0b00000100;
            }

            A = newValue;

            break;
        }
        case SUB: {
            bool underflow = A < B; //Not sure if its called underflow

            A = A - B;

            if (A == 0) {
                Flags |= 0b00000010;
            }
            else {
                Flags &= ~0b00000010;
            }

            //Might need another flag for underflow
            if (underflow) {
                Flags |= 0b00000100;
            }
            else {
                Flags &= ~0b00000100;
            }

            break;
        }
        case OUTV: {
            uint8_t addr = memory[PC++];
            uint8_t value = memory[addr];
            std::cout << static_cast<char>(value);
            break;
        }
        case STA: {
            uint8_t addr = memory[PC++];

            if (VALID_W_MEM_ADDR(addr)) {
                memory[addr] = A;
            }
            else {
                Flags |= 0b00000001;
            }

            break;
        }
        case LDM: {
            uint8_t addr = memory[PC++];

            if (VALID_R_MEM_ADDR(addr)) {
                A = memory[addr];
            }
            else {
                Flags |= 0b00000001;
            }

            break;
        }
        case JEZ: {
            uint8_t addr = memory[PC++];

            if (Flags & (1 << 1)) {
                PC = addr;
            }
            break;
        }
        case JNZ: {
            uint8_t addr = memory[PC++];

            if (!(Flags & (1 << 1))) {
                PC = addr;
            }
            break;
        }
        case MUL: {
            uint16_t result = A * B;
            A = result & 0xFF;

            if (A == 0) Flags |= 0b00000010;
            else Flags &= ~0b00000010;

            if (result > 0xFF) Flags |= 0b00000100;
            else Flags &= ~0b00000100;

            break;
        }
        case DIV: {

            if (B == 0) {
                Flags |= 0b00001000;
            }
            else {
                Flags &= ~0b00001000;
                uint8_t result = A / B;

                if (result == 0) {
                    Flags |= 0b00000010;
                }
                else {
                    Flags &= ~0b00000010;
                }

                A = result;
            }
            break;
        }
        case INP: {
            uint8_t addr = memory[PC++];
            uint8_t value;
            std::cin >> value;

            memory[addr] = static_cast<uint8_t>(value);

            break;
        }
        case LD: {
            uint8_t reg = memory[PC++];
            uint8_t addr = memory[PC++];

            if (reg == 1) {
                A = memory[addr];
            }
            else if (reg == 2) {
                B = memory[addr];
            }
            else {
                std::cerr << "Invalid register provided for LD" << std::endl;
            }
            break;
        }
        case HLT:
            running = false;
            break;
        }
    }

    void run(int tickSpeedMs = 0) {
        using namespace std::chrono_literals;

        while (running) {
            tick();

            if (tickSpeedMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(tickSpeedMs));
            }
        }
    }
};

int main(int argc, char** argv)
{
    std::string inputFile;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-I" && i + 1 < argc) {
            inputFile = argv[i + 1];
            i++;
        }
    }

    if (inputFile.empty()) {
        std::cerr << "No input file provided. Use -I <filepath>" << std::endl;
        return 1;
    }

    TestCPU cpu;
    cpu.loadProgram(inputFile);
    cpu.run(0);
}
