#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>

#include "codegen/cpp_file.hpp"
#include "yang_parser.hpp"

int main(int argc, char **argv)
{
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <output_dir> <yang_search_dir> <yang_file1> [yang_file2...]" << std::endl;
        return 1;
    }

    try {
        std::string output_dir = argv[1];
        std::string yang_dir = argv[2];

        YangParser parser{yang_dir};

        for (int i = 3; i < argc; ++i) {
            std::string yang_file = argv[i];

            std::string stem = std::filesystem::path(yang_file).stem().string();
            std::replace(stem.begin(), stem.end(), '@', '_');
            std::replace(stem.begin(), stem.end(), '-', '_');

            CppFile file_res = parser.parse_module(yang_file);

            std::string out_path = (std::filesystem::path(output_dir) / (stem + ".hpp")).string();
            file_res.save_to_disk(out_path);
        }
    } catch (const std::exception &ex) {
        std::cerr << "[CRIT]: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
