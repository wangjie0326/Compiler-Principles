#pragma once

#include "Grammar.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class FirstFollow {
public:
    explicit FirstFollow(const Grammar& grammar);

    void computeFirstSets();
    void computeFollowSets();

    const std::unordered_map<std::string, std::unordered_set<std::string>>& firstSets() const;
    const std::unordered_map<std::string, std::unordered_set<std::string>>& followSets() const;

    void printFirstSets() const;
    void printFollowSets() const;

private:
    const Grammar& grammar_;

    std::unordered_map<std::string, std::unordered_set<std::string>> first_;
    std::unordered_map<std::string, std::unordered_set<std::string>> follow_;

    static const std::string EPSILON;

    bool addToSet(std::unordered_set<std::string>& target, const std::string& value);

    bool addSet(
        std::unordered_set<std::string>& target,
        const std::unordered_set<std::string>& source
    );

    bool addSetExceptEpsilon(
        std::unordered_set<std::string>& target,
        const std::unordered_set<std::string>& source
    );

    std::unordered_set<std::string> firstOfSequence(
        const std::vector<std::string>& symbols
    ) const;
};