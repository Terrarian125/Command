#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib>
#include <iomanip> // ★ 追加: std::setw のために必要
// nlohmann::json のパスは環境に合わせてください
#include "json.hpp" // https://github.com/nlohmann/json
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

// ==== 外部ファイルにコマンドとURLの対応を書き込む (新規追加) ====
void saveCommands(const std::map<std::string, std::string>& commandMap, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "エラー: " << filename << " への書き込みに失敗しました。\n";
        return;
    }
    for (const auto& [command, url] : commandMap) {
        file << command << ":" << url << "\n";
    }
    file.close();
    std::cout << "✅ コマンドリストをファイル '" << filename << "' に保存しました。\n";
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
    if (!ofs.is_open()) {
        std::cerr << "エラー: JSONファイル '" << filename << "' を開けませんでした。\n";
        return;
    }
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
        // 続行するためにファイルを作成します
        saveCommands(commands, "command_list.txt");
    }

    exportToJson(commands, "command_list.json");

    const std::string EXIT_COMMAND = "exit";
    std::cout << "利用可能なコマンド: ";
    for (const auto& [cmd, _] : commands) std::cout << cmd << " ";
    std::cout << "\n\n終了: " << EXIT_COMMAND << " / help: ヘルプ\n";

    while (true) {
        std::cout << "\n> コマンド入力: ";
        std::string inputCommand;

        // 堅牢な入力のため、改行文字を無視し、一行全体を受け取ります
        if (!(std::getline(std::cin >> std::ws, inputCommand))) break;
        if (inputCommand.empty()) continue;

        saveHistory(inputCommand);

        // === 終了 ===
        if (inputCommand == EXIT_COMMAND) {
            std::cout << "プログラムを終了します。\n";
            break;
        }

        // === help ===
        if (inputCommand == "help") {
            std::cout << "=== 使い方 ===\n"
                << "・コマンド名を入力すると対応するURLまたは実行ファイルを開きます。\n"
                << "・list : 登録済みコマンド一覧\n"
                << "・search : コマンド名で検索\n"
                << "・add : 新しいコマンドを追加\n"
                << "・delete : 既存のコマンドを削除 (★New)\n"
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
            if (!(std::cin >> keyword)) continue;
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
            if (!(std::cin >> newCmd)) continue;
            std::cout << "URL/実行ファイル名: ";
            if (!(std::cin >> newUrl)) continue;

            commands[newCmd] = newUrl;
            saveCommands(commands, "command_list.txt"); // ファイルに書き込み
            exportToJson(commands, "command_list.json"); // JSONを更新

            std::cout << "✅ 追加完了: " << newCmd << " → " << newUrl << "\n";
            continue;
        }

        // === delete (追加機能) ===
        if (inputCommand == "delete") {
            std::string delCmd;
            std::cout << "削除するコマンド名: ";
            if (!(std::cin >> delCmd)) continue;

            if (commands.count(delCmd)) {
                std::cout << "コマンド '" << delCmd << "' (" << commands.at(delCmd) << ") を削除しますか？ (y/n): ";
                char confirm;
                if (!(std::cin >> confirm) || (confirm != 'y' && confirm != 'Y')) {
                    std::cout << "操作をキャンセルしました。\n";
                    continue;
                }

                commands.erase(delCmd);

                // ファイルとJSONを更新
                saveCommands(commands, "command_list.txt");
                exportToJson(commands, "command_list.json");

                std::cout << "✅ 削除完了: コマンド '" << delCmd << "' をリストから削除しました。\n";
            }
            else {
                std::cout << "❌ エラー: コマンド '" << delCmd << "' は登録されていません。\n";
            }
            continue;
        }

        // === URLまたは実行ファイルの実行 (修正済み) ===
        if (commands.count(inputCommand)) {
            const std::string& target = commands.at(inputCommand);
            std::string openCommand;

            // ターゲットがURLであるかどうかの簡単なチェック
            bool isUrl = (target.find("http://") == 0 || target.find("https://") == 0 || target.find("file://") == 0);

            if (isUrl) {
                // URLの場合: getOpenCommand 関数を使ってOSに応じたブラウザオープンコマンドを生成
                std::cout << " -> URL: " << target << " を開きます...\n";
                openCommand = getOpenCommand(target);
            }
            else {
                // 実行ファイル名/パスの場合: そのままシステムコマンドとして実行 (例: game1.exe, A.exe)
                std::cout << " -> 実行ファイル/コマンド: " << target << " を実行します...\n";
                openCommand = target;
            }

            int result = std::system(openCommand.c_str());

            if (result != 0)
                std::cerr << "⚠️ 実行に失敗しました。コマンドが見つからないか、エラーが発生しました。\n";
            else
                std::cout << "✅ 実行成功。\n";
        }
        else {
            std::cout << "❌ 未登録のコマンド: " << inputCommand << "\n";
        }
    }

    return 0;
}