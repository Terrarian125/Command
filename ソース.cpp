#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib> // std::system のために必要

// 外部ファイルからコマンドとURLの対応を読み込む関数 (変更なし)
std::map<std::string, std::string> loadCommands(const std::string& filename) {
    // ... loadCommands の実装 ...
    std::map<std::string, std::string> commandMap;
    std::ifstream file(filename);
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

// OSに応じてURLを開くコマンド文字列を生成する関数 (変更なし)
std::string getOpenCommand(const std::string& url) {
#ifdef _WIN32
    return "start \"\" \"" + url + "\"";
#elif __APPLE__
    return "open \"" + url + "\"";
#else
    return "xdg-open \"" + url + "\"";
#endif
}

int main() {
    // 1. 外部ファイルの読み込み
    std::map<std::string, std::string> commands = loadCommands("command_list.txt");

    if (commands.empty()) {
        std::cerr << "エラー: 'command_list.txt'の読み込みに失敗したか、ファイルが空です。" << std::endl;
        return 1;
    }

    // 💡 終了コマンドの定義 (ループから抜ける手段を提供)
    const std::string EXIT_COMMAND = "exit";

    std::cout << "利用可能なコマンド:\n";
    std::cout << "  ";
    for (const auto& pair : commands) {
        std::cout << pair.first << " ";
    }
    std::cout << "\n終了するには '" << EXIT_COMMAND << "' と入力してください。\n";

    // 🛠️ ここから無限ループ
    while (true) {
        std::cout << "\n> コマンド入力: ";
        std::string inputCommand;

        // ユーザー入力を受け付け
        if (!(std::cin >> inputCommand)) {
            // EOF (Ctrl+Z や Ctrl+D) が入力された場合もループを抜ける
            break;
        }

        // 終了コマンドのチェック
        if (inputCommand == EXIT_COMMAND) {
            std::cout << "プログラムを終了します。\n";
            break; // ループを抜ける
        }

        // 2. URLの検索
        if (commands.count(inputCommand)) {
            const std::string& url = commands.at(inputCommand);
            std::cout << " -> URL: " << url << " を開きます...\n";

            // 3. URLのオープン
            std::string openCommand = getOpenCommand(url);
            int result = std::system(openCommand.c_str());

            if (result != 0) {
                // 開くのに失敗した場合も、プログラムは終了せずループの先頭に戻る
                std::cerr << " -> ⚠️ エラー: URLを開くコマンドの実行に失敗しました。OSのコマンドを確認してください。\n";
            }
            else {
                std::cout << " -> ✅ 実行成功。\n";
            }
        }
        else {
            // コマンドが見つからなかった場合も、プログラムは終了せずループの先頭に戻る
            std::cout << " -> ❌ エラー: コマンド '" << inputCommand << "' は見つかりませんでした。再入力してください。\n";
        }
    }

    return 0;
}