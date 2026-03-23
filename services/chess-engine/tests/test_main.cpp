#include <gtest/gtest.h>
#include <cstring>
#include <iostream>
#include "test_helpers.hpp"

namespace chess::test {
bool g_visualize = false;
}

int main(int argc, char** argv) {
    // Strip --visualize from argv before handing off to GTest
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--visualize") == 0) {
            chess::test::g_visualize = true;
            for (int j = i; j < argc - 1; ++j) {
                argv[j] = argv[j + 1];
            }
            --argc;
            --i;
        }
    }

    if (chess::test::g_visualize) {
        std::cout << "\033[1;32m[Visualization ON — boards printed after each move]\033[0m\n\n";
    } else {
        std::cout << "Tip: run with --visualize to see board states for sequence tests.\n\n";
    }

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
