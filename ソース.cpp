#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib>
//#include <nlohmann\\json.hpp>
#include "json.hpp"  // https://github.com/nlohmann/json
using json = nlohmann::json;

// ==== 外部ファイルからコマンドとURLの対応を読み込む ====
std::map<std::string, std::string> loadCommands(const std::string& filename) {
    std::map<std::string, std::string> commandMap;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "エラー: " << filename << " を開けませんでした。\n";
        return commandMap;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string command = line.substr(0, colonPos);
            std::string url = line.substr(colonPos + 1);
            commandMap[command] = url;
        }
    }
    return commandMap;
}

// ==== OSごとのURLオープンコマンド生成 ====
std::string getOpenCommand(const std::string& url) {
#ifdef _WIN32
    return "start \"\" \"" + url + "\"";
#elif __APPLE__
    return "open \"" + url + "\"";
#else
    return "xdg-open \"" + url + "\"";
#endif
}

// ==== JSONファイル出力 ====
void exportToJson(const std::map<std::string, std::string>& commands, const std::string& filename) {
    json j;
    for (const auto& [cmd, url] : commands) {
        j[cmd] = { {"url", url} };
    }

    std::ofstream ofs(filename);
    ofs << std::setw(4) << j;
    ofs.close();
    std::cout << "✅ JSONファイル '" << filename << "' に書き出しました。\n";
}

// ==== コマンド履歴を記録 ====
void saveHistory(const std::string& command) {
    std::ofstream hist("history.txt", std::ios::app);
    hist << command << "\n";
}

// ==== メイン ====
int main() {
    std::map<std::string, std::string> commands = loadCommands("command_list.txt");

    if (commands.empty()) {
        std::cerr << "エラー: 'command_list.txt' の読み込みに失敗したか空です。\n";
        return 1;
    }

    exportToJson(commands, "command_list.json");

    const std::string EXIT_COMMAND = "exit";
    std::cout << "利用可能なコマンド: ";
    for (const auto& [cmd, _] : commands) std::cout << cmd << " ";
    std::cout << "\n\n終了: " << EXIT_COMMAND << " / help: ヘルプ\n";

    while (true) {
        std::cout << "\n> コマンド入力: ";
        std::string inputCommand;
        if (!(std::cin >> inputCommand)) break;

        saveHistory(inputCommand);

        // === 終了 ===
        if (inputCommand == EXIT_COMMAND) {
            std::cout << "プログラムを終了します。\n";
            break;
        }

        // === help ===
        if (inputCommand == "help") {
            std::cout << "=== 使い方 ===\n"
                << "・コマンド名を入力すると対応するURLを開きます。\n"
                << "・list : 登録済みコマンド一覧\n"
                << "・search : コマンド名で検索\n"
                << "・add : 新しいコマンドを追加\n"
                << "・reload : command_list.txt を再読み込み\n"
                << "・exit : 終了\n";
            continue;
        }

        // === list ===
        if (inputCommand == "list") {
            std::cout << "=== コマンド一覧 ===\n";
            for (const auto& [cmd, url] : commands)
                std::cout << " " << cmd << " : " << url << "\n";
            continue;
        }

        // === reload ===
        if (inputCommand == "reload") {
            commands = loadCommands("command_list.txt");
            std::cout << "🔄 コマンドリストを再読み込みしました。\n";
            exportToJson(commands, "command_list.json");
            continue;
        }

        // === search ===
        if (inputCommand == "search") {
            std::cout << "検索ワード: ";
            std::string keyword;
            std::cin >> keyword;
            bool found = false;
            for (const auto& [cmd, url] : commands) {
                if (cmd.find(keyword) != std::string::npos) {
                    std::cout << " " << cmd << " : " << url << "\n";
                    found = true;
                }
            }
            if (!found)
                std::cout << "一致するコマンドが見つかりませんでした。\n";
            continue;
        }

        // === add ===
        if (inputCommand == "add") {
            std::string newCmd, newUrl;
            std::cout << "追加するコマンド名: ";
            std::cin >> newCmd;
            std::cout << "URL: ";
            std::cin >> newUrl;

            std::ofstream file("command_list.txt", std::ios::app);
            file << newCmd << ":" << newUrl << "\n";
            file.close();

            commands[newCmd] = newUrl;
            exportToJson(commands, "command_list.json");

            std::cout << "✅ 追加完了: " << newCmd << " → " << newUrl << "\n";
            continue;
        }

        // === 通常のURL実行 ===
        if (commands.count(inputCommand)) {
            const std::string& url = commands.at(inputCommand);
            std::cout << " -> URL: " << url << " を開きます...\n";
            std::string openCommand = getOpenCommand(url);
            int result = std::system(openCommand.c_str());

            if (result != 0)
                std::cerr << "⚠️ URLを開くコマンドの実行に失敗しました。\n";
            else
                std::cout << "✅ 実行成功。\n";
        }
        else {
            std::cout << "❌ 未登録のコマンド: " << inputCommand << "\n";
        }
    }

    return 0;
}
