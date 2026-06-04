#include "dragon/App.h"
#include "dragon/FightData.h"
#include "dragon/MugenData.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

struct LaunchOptions {
    bool console = false;
    bool verify = false;
    bool hasRoot = false;
    std::string verifyScenario;
    std::filesystem::path root;
};

std::filesystem::path normalizedAbsolute(const std::filesystem::path& path) {
    if (path.empty()) {
        return path;
    }

    std::error_code error;
    const auto absolute = path.is_absolute() ? path : std::filesystem::absolute(path, error);
    if (error) {
        return path.lexically_normal();
    }

    return absolute.lexically_normal();
}

bool isRuntimeRoot(const std::filesystem::path& root) {
    return dragon::hasMugenRuntimeRootFiles(root)
        && dragon::hasFightDefinition(root);
}

void addCandidate(std::vector<std::filesystem::path>& candidates, const std::filesystem::path& path) {
    const auto normalized = normalizedAbsolute(path);
    if (normalized.empty()) {
        return;
    }

    const auto key = normalized.generic_string();
    for (const auto& existing : candidates) {
        if (existing.generic_string() == key) {
            return;
        }
    }

    candidates.push_back(normalized);
}

LaunchOptions parseLaunchOptions(int argc, char** argv) {
    LaunchOptions options;

    for (int index = 1; index < argc; ++index) {
        const std::string_view arg(argv[index]);
        if (arg == "--console") {
            options.console = true;
            continue;
        }

        if (arg == "--verify") {
            if (options.verify) {
                throw std::runtime_error("Duplicate --verify option");
            }
            if (index + 1 >= argc) {
                throw std::runtime_error("--verify requires a scenario name");
            }
            options.verify = true;
            options.verifyScenario = argv[++index];
            if (options.verifyScenario.empty() || options.verifyScenario.starts_with("--")) {
                throw std::runtime_error("--verify requires a scenario name");
            }
            continue;
        }

        if (!options.hasRoot) {
            options.root = std::filesystem::path(argv[index]);
            options.hasRoot = true;
            continue;
        }

        throw std::runtime_error("Unexpected argument: " + std::string(arg));
    }

    return options;
}

std::filesystem::path executableDirectory(char** argv) {
    const std::filesystem::path executable = argv && argv[0] ? std::filesystem::path(argv[0]) : std::filesystem::path();
    if (executable.empty()) {
        std::error_code error;
        return std::filesystem::current_path(error);
    }

    const auto absolute = normalizedAbsolute(executable);
    if (!absolute.has_parent_path()) {
        std::error_code error;
        return std::filesystem::current_path(error);
    }

    return absolute.parent_path();
}

std::filesystem::path resolveRuntimeRoot(const LaunchOptions& options, const std::filesystem::path& exeDir) {
    std::vector<std::filesystem::path> candidates;

    if (options.hasRoot) {
        addCandidate(candidates, options.root);
    }

    std::error_code error;
    const auto cwd = std::filesystem::current_path(error);
    if (!error) {
        addCandidate(candidates, cwd / "game");
    }

    addCandidate(candidates, exeDir / "game");
    addCandidate(candidates, exeDir.parent_path() / "game");

    for (auto dir = exeDir; !dir.empty(); dir = dir.parent_path()) {
        addCandidate(candidates, dir / "game");
        if (dir == dir.parent_path()) {
            break;
        }
    }

    for (const auto& candidate : candidates) {
        if (isRuntimeRoot(candidate)) {
            return candidate;
        }
    }

    std::ostringstream message;
    message << "No valid Dragon MUGEN runtime root found.\n"
        << "A valid root must contain "
        << dragon::mugenRuntimeRootRequirementText()
        << ", and "
        << dragon::fightDefinitionRequirementText()
        << ".\n"
        << "Looked in:";
    for (const auto& candidate : candidates) {
        message << "\n  - " << candidate;
    }
    message << "\nLaunch from the repo root or pass the game folder, for example:\n"
        << "  dragon_mugen.exe game\n"
        << "  dragon_mugen.exe ..\\game";

    throw std::runtime_error(message.str());
}

} // namespace

int main(int argc, char** argv) {
    try {
        const LaunchOptions options = parseLaunchOptions(argc, argv);
        const std::filesystem::path root = resolveRuntimeRoot(options, executableDirectory(argv));

        if (options.verify) {
            if (options.console) {
                throw std::runtime_error("--verify and --console cannot be combined");
            }
            return dragon::runVerificationScenario(root, options.verifyScenario, std::cout);
        }

        if (!options.console) {
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
