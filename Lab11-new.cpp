#ifndef AHOCORASICK_H
#define AHOCORASICK_H

#include <string>
#include <vector>
#include <unordered_map>
#include <queue>
#include <set>
#include <utility>
#include <chrono>

class AhoCorasick {
private:
    struct Node {
        std::unordered_map<char, int> next;   // переходы по символам
        int fail;                              // fail-ссылка
        std::set<int> output;                  // индексы образцов, заканчивающихся здесь
        
        Node() : fail(0) {}
    };
    
    std::vector<Node> trie;
    int comparisons;                           // счётчик сравнений
    bool built;                                // построен ли автомат
    std::vector<std::string> patterns;         // хранение образцов (для отладки)

public:
    AhoCorasick() : comparisons(0), built(false) {
        trie.push_back(Node()); // корневой узел (индекс 0)
    }
    
    // Добавление образца (возвращает его индекс)
    int addPattern(const std::string& pattern) {
        built = false;
        int node = 0;
        for (char ch : pattern) {
            comparisons++; // сравнение при чтении символа образца
            if (trie[node].next.find(ch) == trie[node].next.end()) {
                trie[node].next[ch] = static_cast<int>(trie.size());
                trie.push_back(Node());
            }
            node = trie[node].next[ch];
        }
        int patternId = static_cast<int>(patterns.size());
        patterns.push_back(pattern);
        trie[node].output.insert(patternId);
        return patternId;
    }
    
    // Построение fail-ссылок (обязательно вызвать перед поиском)
    void build() {
        if (built) return;
        
        std::queue<int> q;
        
        // Инициализация fail-ссылок для первого уровня
        for (auto& p : trie[0].next) {
            char ch = p.first;
            int child = p.second;
            trie[child].fail = 0;
            q.push(child);
        }
        
        // BFS для построения остальных fail-ссылок
        while (!q.empty()) {
            int r = q.front();
            q.pop();
            
            for (auto& p : trie[r].next) {
                char ch = p.first;
                int u = p.second;
                q.push(u);
                
                // Поиск fail-ссылки для узла u
                int f = trie[r].fail;
                while (f != 0 && trie[f].next.find(ch) == trie[f].next.end()) {
                    comparisons++; // сравнение при проверке отсутствия перехода
                    f = trie[f].fail;
                }
                comparisons++; // последнее сравнение в условии цикла
                
                if (trie[f].next.find(ch) != trie[f].next.end()) {
                    comparisons++; // сравнение при нахождении перехода
                    trie[u].fail = trie[f].next[ch];
                } else {
                    trie[u].fail = 0;
                }
                
                // Объединение выходов из fail-ссылки
                for (int out : trie[trie[u].fail].output) {
                    trie[u].output.insert(out);
                }
            }
        }
        
        built = true;
    }
    
    // Поиск всех вхождений в тексте
    // возвращает вектор пар (индекс_образца, позиция_в_тексте)
    std::vector<std::pair<int, int>> search(const std::string& text) {
        if (!built) {
            build();
        }
        
        std::vector<std::pair<int, int>> matches;
        int node = 0;
        
        for (size_t pos = 0; pos < text.size(); ++pos) {
            char ch = text[pos];
            
            // Переход по fail-ссылкам пока не найдём переход по символу
            while (node != 0 && trie[node].next.find(ch) == trie[node].next.end()) {
                comparisons++; // сравнение при проверке отсутствия перехода
                node = trie[node].fail;
            }
            comparisons++; // последнее сравнение в условии цикла
            
            if (trie[node].next.find(ch) != trie[node].next.end()) {
                comparisons++; // сравнение при нахождении перехода
                node = trie[node].next[ch];
            }
            
            // Проверяем выходы из текущего узла
            for (int patternId : trie[node].output) {
                matches.emplace_back(patternId, static_cast<int>(pos - patterns[patternId].size() + 1));
            }
        }
        
        return matches;
    }
    
    // Получение числа сравнений
    int getComparisons() const {
        return comparisons;
    }
    
    // Сброс счётчика сравнений
    void resetComparisons() {
        comparisons = 0;
    }
    
    // Очистка автомата (для нового набора образцов)
    void clear() {
        trie.clear();
        trie.push_back(Node());
        comparisons = 0;
        built = false;
        patterns.clear();
    }
    
    // Проверка, построен ли автомат
    bool isBuilt() const {
        return built;
    }
    
    // Получить количество образцов
    size_t getPatternCount() const {
        return patterns.size();
    }
};

#endif // AHOCORASICK_H
