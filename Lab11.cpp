#include "AutomatonStringMatcher.h"
#include <iostream>
#include <string>
#include <chrono>
#include <random>
#include <algorithm> 
#include <vector>
#include <functional>

// Генератор случайной строки
std::string generateRandomString(int length, char minChar = 'a', char maxChar = 'z') {
    // Используем thread_local, чтобы не переинициализировать генератор каждый раз, 
    // но сохраняем фиксированный seed для воспроизводимости, если нужно.
    // В вашем оригинале был rng(42), оставим так для консистентности тестов.
    static std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(minChar, maxChar);
    std::string str(length, ' ');
    for (int i = 0; i < length; i++)
    {
        str[i] = static_cast<char>(dist(rng));
    }
    return str;
}

// Структура для хранения результатов одного прогона
struct BenchmarkResult {
    int N;
    int M;
    double automatonBuildTime;
    double automatonSearchTime;
    long long automatonComparisons;

    double stdSearchTime;
    // std::search не возвращает количество сравнений напрямую.
    // Можно реализовать кастомный итератор или предикат для подсчета, 
    // но это сильно замедлит сам std::search. Оставим -1 или 0.
    long long stdSearchComparisons = -1;
};

int main() {
    AutomatonStringMatcher matcher;

    // Заголовки CSV
    std::cout << "TextLen, PatternLen, "
        << "AutoBuild_ms, AutoSearch_ms, AutoComparisons, "
        << "StdSearch_ms, StdSearchComparisons\n";

    for (int N = 1000000; N <= 10000000; N += 1000000) {
        // Генерируем текст один раз для каждого N
        std::string text = generateRandomString(N);

        for (int M = 10; M <= 1000; M += 330) {
            // Генерируем паттерн
            std::string pattern = generateRandomString(M);

            // --- Автоматный поиск (KMP/Ахо-Корасик и т.д.) ---

            auto startBuild = std::chrono::high_resolution_clock::now();
            matcher.compile(pattern);
            auto endBuild = std::chrono::high_resolution_clock::now();

            auto startSearchAuto = std::chrono::high_resolution_clock::now();
            auto resultAuto = matcher.search(text);
            auto endSearchAuto = std::chrono::high_resolution_clock::now();

            double buildTime = std::chrono::duration<double, std::milli>(endBuild - startBuild).count();
            double searchTimeAuto = std::chrono::duration<double, std::milli>(endSearchAuto - startSearchAuto).count();
            long long comparisonsAuto = resultAuto.comparisons;

            // --- Стандартный поиск std::search ---

            auto startSearchStd = std::chrono::high_resolution_clock::now();

            auto it = std::search(text.begin(), text.end(), pattern.begin(), pattern.end());

            auto endSearchStd = std::chrono::high_resolution_clock::now();

            double searchTimeStd = std::chrono::duration<double, std::milli>(endSearchStd - startSearchStd).count();

            std::cout << N << ", " << M << ", "
                << buildTime << ", " << searchTimeAuto << ", " << comparisonsAuto << ", "
                << searchTimeStd << ", " << -1 << "\n";
        }
    }

    return 0;
}
