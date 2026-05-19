#include "Lexer.h"
#include "Parser.h"
#include "Grammar.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

// 匿名命名空间。
// 其中的函数只在当前 cpp 文件内部可见，避免和其他文件里的同名函数冲突。
namespace {

// 从标准输入读取完整源程序。
// 当用户运行 parser_main.exe 但不传入文件路径时，会使用这个函数读取输入。
std::string readAllFromStdin() {
    // inputBuffer 用来拼接所有输入行。
    std::ostringstream inputBuffer;

    // line 用来临时保存每一行输入内容。
    std::string line;

    // 持续从标准输入读取一行内容，直到输入结束。
    // Windows 命令行中通常用 Ctrl+Z 再回车表示输入结束。
    while (std::getline(std::cin, line)) {
        // 将当前行写入缓冲区，并补上换行符。
        inputBuffer << line << '\n';
    }

    // 返回完整输入内容。
    return inputBuffer.str();
}

// 从指定文件中读取完整源程序。
// filePath 是文件路径；
// content 用来保存读取到的文件内容；
// error 用来保存读取失败时的错误信息。
bool readAllFromFile(const std::string& filePath, std::string& content, std::string& error) {
    // 根据文件路径打开输入文件。
    std::ifstream inputFile(filePath);

    // 如果文件打开失败，记录错误信息并返回 false。
    if (!inputFile.is_open()) {
        error = "Cannot open source file: " + filePath;
        return false;
    }

    // buffer 用来暂存整个文件内容。
    std::ostringstream buffer;

    // 将文件流中的所有内容读入 buffer。
    buffer << inputFile.rdbuf();

    // 把读取到的字符串保存到 content 中。
    content = buffer.str();

    // 返回 true 表示文件读取成功。
    return true;
}

// 打印程序使用说明。
// 当命令行参数数量错误，或者文件读取失败时调用。
void printUsage(const char* programName) {
    std::cout << "Usage:\n";

    // 用法一：传入一个源程序文件路径。
    std::cout << "  " << programName << " <source-file>\n";

    // 用法二：不传入文件路径，从标准输入读取源程序。
    std::cout << "  " << programName << "    # read source program from standard input\n";
}
}

// 主函数。
// argc 表示命令行参数个数；
// argv 表示命令行参数数组。
int main(int argc, char* argv[]) {
    // 打印程序标题。
    std::cout << "============================================\n";
    std::cout << "Lab 2 SLR Parser\n";
    std::cout << "============================================\n";

    // source 用来保存待进行语法分析的源程序文本。
    std::string source;

    // 如果没有传入额外参数，说明从标准输入读取源程序。
    if (argc == 1) {
        std::cout << "Input source program, then press Ctrl+Z and Enter on Windows to finish:\n\n";

        // 从标准输入读取完整源程序。
        source = readAllFromStdin();
    } else if (argc == 2) {
        // 如果传入一个参数，说明该参数是源程序文件路径。

        // error 用来保存文件读取失败时的错误信息。
        std::string error;

        // 尝试从文件中读取源程序。
        if (!readAllFromFile(argv[1], source, error)) {
            // 如果读取失败，输出错误信息和使用说明。
            std::cerr << error << "\n";
            printUsage(argv[0]);
            return 1;
        }

        // 输出当前读取的源文件路径。
        std::cout << "Source file: " << argv[1] << "\n";
    } else {
        // 如果参数数量不符合要求，则打印使用说明并退出。
        printUsage(argv[0]);
        return 1;
    }

    // 如果没有读取到任何输入内容，则提示并正常结束。
    if (source.empty()) {
        std::cout << "No input provided.\n";
        return 0;
    }

    // 创建词法分析器对象。
    Lexer lexer;

    // 将源程序内容交给词法分析器。
    lexer.setInput(source);

    // 创建语法分析器对象。
    // Parser 会从 lexer 中获取 token，并进行 SLR 语法分析。
    Parser parser(lexer);

    // 执行语法分析。
    // accepted 为 true 表示语法分析成功，为 false 表示失败。
    const bool accepted = parser.parse();

    // 打印语法分析结果标题。
    std::cout << "\n============================================\n";
    std::cout << "Parse Result\n";
    std::cout << "============================================\n";

    // 根据 accepted 输出语法分析是否成功。
    if (accepted) {
        std::cout << "Syntax analysis succeeded.\n";
    } else {
        std::cout << "Syntax analysis failed.\n";
    }

    // 如果语法分析过程中记录了错误信息，则统一输出。
    if (!parser.errors().empty()) {
        std::cout << "\nErrors:\n";

        // 逐条打印错误信息。
        for (const auto& error : parser.errors()) {
            std::cout << "  " << error << "\n";
        }
    }

    // 打印移进/规约过程。
    // trace 中保存了语法分析过程中的每一步动作。
    std::cout << "\nShift/Reduce Trace:\n";
    for (const auto& step : parser.trace()) {
        std::cout << "  " << step << "\n";
    }

    // 打印规约产生式序列。
    // reductions 中保存了分析过程中使用过的产生式编号。
    std::cout << "\nReduction Production Sequence:\n";

    // 创建 Grammar 对象，用于根据产生式编号查找具体产生式内容。
    Grammar grammar;

    // 获取文法中的所有产生式。
    const auto& productions = grammar.productions();

    // 遍历语法分析过程中记录的规约产生式编号。
    for (const int productionId : parser.reductions()) {
        // 判断产生式编号是否合法。
        if (productionId >= 0 &&
            static_cast<std::size_t>(productionId) < productions.size()) {
            // 将对应产生式转换成字符串并输出。
            std::cout << "  " << grammar.productionToString(
                productions[static_cast<std::size_t>(productionId)]
            ) << "\n";
        }
    }

    // 如果语法分析成功，则打印语法分析树。
    if (accepted) {
        std::cout << "\nParse Tree:\n";

        // 将语法树输出到标准输出。
        parser.printParseTree(std::cout);
    }

    // 如果语法分析成功，程序返回 0；
    // 如果语法分析失败，程序返回 1。
    return accepted ? 0 : 1;
}