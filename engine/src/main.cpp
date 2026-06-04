#include "dragon/App.h"
#include "dragon/MugenData.h"

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    try {
        const std::filesystem::path root = argc > 1
            ? std::filesystem::path(argv[1])
            : std::filesystem::path("game");

        if (argc <= 2 || std::string_view(argv[2]) != "--console") {
            return dragon::runApp(root);
        }

        std::cout << "Dragon MUGEN prototype\n";
        const auto characters = dragon::loadCharacters(root);
        if (characters.empty()) {
            std::cout << "No selectable characters resolved from M.U.G.E.N data\n";
            return 0;
        }

        std::cout << "Roster: MugenData\n";
        for (const auto& character : characters) {
            const auto files = dragon::resolveCharacterFiles(root, character);
            std::cout << "- " << character.id << " -> " << character.defPath;
            if (!character.preferredStagePath.empty()) {
                std::cout << " stage=" << character.preferredStagePath;
            }
            std::cout << "\n";
            std::cout << "  display: \"" << character.displayName << "\"\n";
            std::cout << "  author: \"" << character.author << "\"\n";
            std::cout << "  files:";
            if (!files.cmd.empty()) {
                std::cout << " cmd=" << files.cmd.filename();
            }
            if (!files.stateFiles.empty()) {
                std::cout << " states=" << files.stateFiles.size();
            }
            if (!files.sprite.empty()) {
                std::cout << " sprite=" << files.sprite.filename();
            }
            if (!files.anim.empty()) {
                std::cout << " anim=" << files.anim.filename();
            }
            if (!files.sound.empty()) {
                std::cout << " sound=" << files.sound.filename();
            }
            if (!files.palette.empty()) {
                std::cout << " palette=" << files.palette.filename();
            }
            std::cout << "\n";
        }

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n";
        return 1;
    }
}
