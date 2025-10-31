#define _CRT_SECURE_NO_WARNINGS // localtimeのセキュリティ警告を抑制

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cstdlib>
#include <iomanip> 
#include <chrono>   
#include <ctime>    
#include <vector>   
#include <random>   
#include "json.hpp" // nlohmann::json のパスは環境に合わせてください
using json = nlohmann::json;

// =========================================================
// ユーティリティ関数
// =========================================================

// ==== 外部ファイルからコマンドとURLの対応を読み込む ====
std::map<std::string, std::string> loadCommands(const std::string& filename) {
    std::map<std::string, std::string> commandMap;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "エラー: '" << filename << "' を開けませんでした。\n";
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

// ==== 外部ファイルにコマンドとURLの対応を書き込む ====
void saveCommands(const std::map<std::string, std::string>& commandMap, const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "エラー: '" << filename << "' への書き込みに失敗しました。\n";
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

// ==== 外部ファイルから格言リストを読み込む (新規追加) ====
std::vector<std::string> loadAphorisms(const std::string& filename) {
    std::vector<std::string> aphorisms;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "警告: 格言ファイル '" << filename << "' を開けませんでした。デフォルトの格言を使用します。\n";
        return { "格言ファイルがありません", "油断大敵", "継続は力なり" };
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.find_first_not_of(' ') != std::string::npos) {
            aphorisms.push_back(line);
        }
    }

    if (aphorisms.empty()) {
        std::cerr << "警告: 格言ファイル '" << filename << "' は空です。デフォルトの格言を使用します。\n";
        return { "格言ファイルが空です", "思い立ったが吉日" };
    }

    return aphorisms;
}

// ==== today コマンドの実装 (新規追加) ====
void displayTodayInfo() {
    // 1. 現在時刻の取得
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm_struct = *std::localtime(&now); // _CRT_SECURE_NO_WARNINGSで警告を抑制

    // 2. 令和の年数を計算 (令和元年=2019年)
    int rewa_year = tm_struct.tm_year + 1900 - 2018;

    // 3. 曜日の日本語表記
    const char* weekdays[] = { "日", "月", "火", "水", "木", "金", "土" };
    const char* current_weekday = weekdays[tm_struct.tm_wday];

    // 4. 時刻と日付のフォーマット
    std::stringstream ss_time;
    // put_timeで時間をフォーマット
    ss_time << std::put_time(&tm_struct, "%H時%M分");

    // 5. 格言リストを外部ファイルから読み込む (静的変数で一度だけ読み込む)
    static std::vector<std::string> aphorisms = loadAphorisms("aphorisms.txt");

    // 6. ランダムな格言の選択 (日付をシードとして使用し、毎日同じ格言を出す)
    std::string today_aphorism = "格言なし";
    if (!aphorisms.empty()) {
        // 現在時刻を日単位に丸めてシードとして使用
        static std::mt19937 generator(static_cast<unsigned int>(now / (60 * 60 * 24)));
        std::uniform_int_distribution<int> distribution(0, aphorisms.size() - 1);
        today_aphorism = aphorisms[distribution(generator)];
    }

    // 7. 出力フォーマット
    std::cout << "\n=== Today's Info ===\n";
    std::cout << "R" << rewa_year << "年 "
        << (tm_struct.tm_year + 1900) << "/"
        << std::setfill('0') << std::setw(2) << tm_struct.tm_mon + 1 << "/"
        << std::setfill('0') << std::setw(2) << tm_struct.tm_mday
        << "(" << current_weekday << ")"
        << " " << ss_time.str()
        << " : " << today_aphorism << "\n";
    std::cout << "====================\n";
}

// =========================================================
// メイン
// =========================================================

int main() {
#ifdef _WIN32
    // Windows環境でのみ実行
    // コマンドプロンプトの文字コードをUTF-8 (65001) に変更
    system("chcp 65001 > nul");
#endif
    std::map<std::string, std::string> commands = loadCommands("command_list.txt");

    if (commands.empty()) {
        std::cerr << "エラー: 'command_list.txt' の読み込みに失敗したか空です。\n";
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

        if (!(std::getline(std::cin >> std::ws, inputCommand))) break;
        if (inputCommand.empty()) continue;

        saveHistory(inputCommand);

        // === 終了 ===
        if (inputCommand == EXIT_COMMAND) {
            std::cout << "プログラムを終了します。\n";
            break;
        }

        // === today ===
        if (inputCommand == "today") {
            displayTodayInfo();
            continue;
        }

        // === help ===
        if (inputCommand == "help") {
            std::cout << "=== 使い方 ===\n"
                << "・コマンド名を入力すると対応するURLまたは実行ファイルを開きます。\n"
                << "・list : 登録済みコマンド一覧\n"
                << "・search : コマンド名で検索\n"
                << "・add : 新しいコマンドを追加\n"
                << "・delete : 既存のコマンドを削除\n"
                << "・today : 現在の日時と今日の格言を表示\n"
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
            saveCommands(commands, "command_list.txt");
            exportToJson(commands, "command_list.json");

            std::cout << "✅ 追加完了: " << newCmd << " → " << newUrl << "\n";
            continue;
        }

        // === delete ===
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

                saveCommands(commands, "command_list.txt");
                exportToJson(commands, "command_list.json");

                std::cout << "✅ 削除完了: コマンド '" << delCmd << "' をリストから削除しました。\n";
            }
            else {
                std::cout << "❌ エラー: コマンド '" << delCmd << "' は登録されていません。\n";
            }
            continue;
        }

        // === URLまたは実行ファイルの実行 ===
        if (commands.count(inputCommand)) {
            const std::string& target = commands.at(inputCommand);
            std::string openCommand;

            bool isUrl = (target.find("http://") == 0 || target.find("https://") == 0 || target.find("file://") == 0);

            if (isUrl) {
                std::cout << " -> URL: " << target << " を開きます...\n";
                openCommand = getOpenCommand(target);
            }
            else {
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