#include "cartridge.hpp"
#include <iostream>

void printDetailedInfo(const Cartridge& cart) {
    const auto& header = cart.getHeader();
    
    std::cout << "\nDetailed ROM Information:" << std::endl;
    std::cout << "ROM Size: " << cart.getROMSize() << " bytes" << std::endl;
    // Add more detailed information as needed
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file.gb>" << std::endl;
        return 1;
    }

    try {
        Cartridge cart(argv[1]);
        printDetailedInfo(cart);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 