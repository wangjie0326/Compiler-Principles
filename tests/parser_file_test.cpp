#include "Lexer.h"
#include "Parser.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// 匿名命名空间。
// 这里面的辅助函数只在当前测试文件中可见，避免和其他文件中的同名函数冲突。
namespace {

// 将文本内容写入指定文件。
// filePath 表示文件路径，content 表示要写入的文本内容。
// 写入成功返回 true，失败返回 false。
bool writeTextFile(const std::string& filePath, const std::string& content) {
    // 创建输出文件流。
    std::ofstream output(filePath);

    // 如果文件打开失败，返回 false。
    if (!output.is_open()) {
        return false;
    }

    // 将内容写入文件。
    output << content;

    // 返回 true 表示写入成功。
    return true;
}

// 从指定文件中读取全部文本内容。
// filePath 表示文件路径，content 用来保存读取到的内容。
// 读取成功返回 true，失败返回 false。
bool readTextFile(const std::string& filePath, std::string& content) {
    // 创建输入文件流。
    std::ifstream input(filePath);

    // 如果文件打开失败，返回 false。
    if (!input.is_open()) {
        return false;
    }

    // buffer 用于暂存整个文件内容。
    std::ostringstream buffer;

    // 将文件中的全部内容读入 buffer。
    buffer << input.rdbuf();

    // 将读取结果保存到 content 中。
    content = buffer.str();

    // 返回 true 表示读取成功。
    return true;
}
}

// FileParserCase 表示一个文件输入测试用例。
// 每个测试用例包含测试名称、文件路径、源程序内容以及期望的分析结果。
struct FileParserCase {
    // 测试用例名称。
    std::string name;

    // 测试文件路径。
    std::string filePath;

    // 要写入测试文件的源程序内容。
    std::string source;

    // 期望语法分析是否通过。
    // true 表示期望接受，false 表示期望拒绝。
    bool shouldAccept;
};

// Parser 文件输入测试程序入口。
// 该测试用于验证 Parser 能否从文件内容中读取源程序并完成语法分析。
int main() {
    // 构造文件输入测试用例。
    // 第一个是合法程序，期望语法分析成功；
    // 第二个是缺少 end 的非法程序，期望语法分析失败。
    std::vector<FileParserCase> cases = {
        {
            "valid source file",
            "parser_valid_input.txt",
            "begin a = 1; b = 2; end;",
            true
        },
        {
            "invalid source file",
            "parser_invalid_input.txt",
            "begin a = 1; b = 2;",
            false
        }
    };

    // allOk 用来记录所有测试用例是否都通过。
    bool allOk = true;

    // 逐个执行测试用例。
    for (const auto& testCase : cases) {
        // 先把测试源程序写入对应的测试文件。
        if (!writeTextFile(testCase.filePath, testCase.source)) {
            std::cerr << "[FAIL] Cannot create test file: "
                      << testCase.filePath << "\n";
            return 1;
        }

        // sourceFromFile 用来保存从文件中重新读取出的源程序内容。
        std::string sourceFromFile;

        // 从测试文件中读取源程序。
        if (!readTextFile(testCase.filePath, sourceFromFile)) {
            std::cerr << "[FAIL] Cannot read test file: "
                      << testCase.filePath << "\n";
            return 1;
        }

        // 创建词法分析器。
        Lexer lexer;

        // 将从文件读取到的源程序交给词法分析器。
        lexer.setInput(sourceFromFile);

        // 创建语法分析器。
        // Parser 会从 lexer 中读取 token，并根据 SLR 分析表进行语法分析。
        Parser parser(lexer);

        // 执行语法分析。
        const bool accepted = parser.parse();

        // 打印当前测试用例的基本信息。
        std::cout << "============================================\n";
        std::cout << "[CASE] " << testCase.name << "\n";
        std::cout << "File: " << testCase.filePath << "\n";
        std::cout << "Source: " << sourceFromFile << "\n";
        std::cout << "Expected: " << (testCase.shouldAccept ? "accept" : "reject") << "\n";
        std::cout << "Accepted: " << (accepted ? "yes" : "no") << "\n";

        // 如果语法分析过程中产生错误信息，则输出错误列表。
        if (!parser.errors().empty()) {
            std::cout << "Errors:\n";

            // 逐条打印错误信息。
            for (const auto& error : parser.errors()) {
                std::cout << "  " << error << "\n";
            }
        }

        // 如果语法分析成功，则检查语法树是否正常生成。
        if (accepted) {
            // 对于成功接受的输入，语法树根节点应该有效，语法树节点列表也不应为空。
            if (parser.parseTreeRoot() < 0 || parser.parseTreeNodes().empty()) {
                std::cerr << "[FAIL] Parse tree should not be empty for accepted file case: "
                          << testCase.name << "\n";
                allOk = false;
            }

            // 打印语法树。
            std::cout << "Parse Tree:\n";
            parser.printParseTree(std::cout);
        }

        // 判断实际分析结果是否和期望一致。
        if (accepted != testCase.shouldAccept) {
            std::cerr << "[FAIL] Unexpected parser result for file case: "
                      << testCase.name << "\n";
            allOk = false;
        } else {
            // 当前测试用例通过。
            std::cout << "[PASS] " << testCase.name << "\n";
        }
    }

    // 如果存在任意测试用例失败，返回 1。
    if (!allOk) {
        return 1;
    }

    // 所有文件输入测试均通过。
    std::cout << "============================================\n";
    std::cout << "[PASS] Parser file input tests passed.\n";

    // 返回 0 表示测试成功。
    return 0;
}