#pragma once

// 引入 Grammar 类。
// FirstFollow 需要依赖 Grammar 中保存的文法、终结符、非终结符和产生式信息。
#include "Grammar.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// FirstFollow 类：用于计算文法的 FIRST 集和 FOLLOW 集。
// 在 SLR(1) 语法分析中，FOLLOW 集会用于构造 ACTION/GOTO 分析表中的规约项。
class FirstFollow {
public:
    // 构造函数。
    // explicit 防止隐式类型转换。
    // 参数 grammar 是一个 Grammar 对象引用，表示要基于哪一个文法计算 FIRST/FOLLOW 集。
    explicit FirstFollow(const Grammar& grammar);

    // 计算所有符号的 FIRST 集。
    // FIRST(X) 表示从 X 开始推导时，第一个可能出现的终结符集合。
    void computeFirstSets();

    // 计算所有非终结符的 FOLLOW 集。
    // FOLLOW(A) 表示在某个句型中，非终结符 A 后面可能紧跟的终结符集合。
    void computeFollowSets();

    // 返回已经计算好的 FIRST 集。
    // 外层 map 的 key 是符号名，value 是该符号对应的 FIRST 集。
    // 返回 const 引用，避免外部修改内部数据，同时避免拷贝开销。
    const std::unordered_map<std::string, std::unordered_set<std::string>>& firstSets() const;

    // 返回已经计算好的 FOLLOW 集。
    // 外层 map 的 key 是非终结符名，value 是该非终结符对应的 FOLLOW 集。
    // 返回 const 引用，避免外部修改内部数据，同时避免拷贝开销。
    const std::unordered_map<std::string, std::unordered_set<std::string>>& followSets() const;

    // 打印 FIRST 集，主要用于调试和测试。
    void printFirstSets() const;

    // 打印 FOLLOW 集，主要用于调试和测试。
    void printFollowSets() const;

private:
    // 保存 Grammar 对象的引用。
    // FirstFollow 不自己存一份文法，而是直接使用外部传入的 Grammar。
    const Grammar& grammar_;

    // FIRST 集存储结构。
    // 例如 first_["Expr"] = { "IDN", "DEC", "SLP" }。
    std::unordered_map<std::string, std::unordered_set<std::string>> first_;

    // FOLLOW 集存储结构。
    // 例如 follow_["Expr"] = { "SEMI", "THEN", "DO", "SRP" }。
    std::unordered_map<std::string, std::unordered_set<std::string>> follow_;

    // 表示空串 ε。
    // 如果某个非终结符可以推出空串，就会在它的 FIRST 集中加入 EPSILON。
    static const std::string EPSILON;

    // 向集合 target 中加入一个元素 value。
    // 如果成功加入新元素，返回 true；
    // 如果元素本来已经存在，返回 false。
    bool addToSet(std::unordered_set<std::string>& target, const std::string& value);

    // 将 source 集合中的所有元素加入 target 集合。
    // 只要 target 发生了变化，就返回 true。
    bool addSet(
        std::unordered_set<std::string>& target,
        const std::unordered_set<std::string>& source
    );

    // 将 source 集合中除了 ε 以外的所有元素加入 target 集合。
    // 这个函数常用于 FIRST/FOLLOW 计算规则中需要排除空串的情况。
    bool addSetExceptEpsilon(
        std::unordered_set<std::string>& target,
        const std::unordered_set<std::string>& source
    );

    // 计算一个符号序列的 FIRST 集。
    // 例如对于 A -> B C D，需要计算 FIRST(B C D)。
    // 这个函数在 FOLLOW 集计算中尤其重要。
    std::unordered_set<std::string> firstOfSequence(
        const std::vector<std::string>& symbols
    ) const;
};