#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <string>

std::unordered_map<std::string, uint8_t> opcodes = {
    {"LDA", 0x01},
    {"LDB", 0x02},
    {"ADD", 0x03},
    {"SUB", 0x04},
    {"OUTV", 0x05},
    {"STA", 0x06},
    {"LDM", 0x07},
    {"JEZ", 0x08},
    {"JNZ", 0x09},
    {"MUL", 0x0A},
    {"DIV", 0x0B},
    {"INP", 0x0C},
    {"LD",  0x0D},
    {"HLT", 0xFF}
};

int main(int argc, char* argv[])
{
    std::string inputFile, outputFile;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-I" && i + 1 < argc) inputFile = argv[++i];
        else if (arg == "-O" && i + 1 < argc) outputFile = argv[++i];
    }

    if (inputFile.empty()) inputFile = "program.asm";
    if (outputFile.empty()) outputFile = "program.bin";

    std::ifstream file(inputFile);
    std::vector<std::string> lines;
    std::string line;

    while (std::getline(file, line)) {
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos) line = line.substr(0, commentPos);
        lines.push_back(line);
    }

    std::unordered_map<std::string, size_t> labels;
    size_t pc = 0;

    for (auto& line : lines) {
        std::stringstream ss(line);
        std::string word;
        ss >> word;

        if (word.empty() || word[0] == ';') continue;

        if (word.back() == ':') labels[word.substr(0, word.size() - 1)] = pc;
        else {
            pc++;
            std::string arg;
            while (ss >> arg) pc++;
        }
    }

    std::vector<uint8_t> program;
    for (auto& line : lines) {
        std::stringstream ss(line);
        std::string word;
        ss >> word;

        if (word.empty() || word[0] == ';') continue;
        if (word.back() == ':') continue;

        auto it = opcodes.find(word);
        if (it == opcodes.end()) continue;

        program.push_back(it->second);

        std::string arg;
        while (ss >> arg) {
            if (labels.find(arg) != labels.end()) program.push_back(static_cast<uint8_t>(labels[arg]));
            else program.push_back(static_cast<uint8_t>(std::stoi(arg, nullptr, 0)));
        }
    }

    if (program.size() == 0) { std::cerr << "No source file provided!" << std::endl; return -1; }

    std::ofstream out(outputFile, std::ios::binary);
    out.write(reinterpret_cast<const char*>(program.data()), program.size());

    std::cout << "Assembled " << program.size() << " bytes." << std::endl;
}
