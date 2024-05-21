#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <locale>
#include <cctype>
#include <algorithm>
#include <thread>
#include <mutex>
#include <chrono> 

std::string normalizeText(const std::string& text) {
    std::string result = text;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(text);

    while (tokenStream >> token) {
        // eiminare semne de punctuatie la inceput si sf
        token.erase(std::remove_if(token.begin(), token.end(), [](unsigned char c) {
            return std::ispunct(c);
            }), token.end());

        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

void countWordFrequenciesThread(const std::string& text) {
    std::unordered_map<std::string, int> wordFreq;
    std::string normalizedText = normalizeText(text);
    std::vector<std::string> tokens = tokenize(normalizedText);

    std::mutex mtx; // Mutex pentru protejarea accesului la wordFreq

    // functie lambda pentru numararea frecventei cuvintelor intr-un interval
    auto countWordsInRange = [&](int start, int end) {
        std::unordered_map<std::string, int> localFreq;
        for (int i = start; i < end; ++i) {
            const std::string& word = tokens[i];
            if (!word.empty()) {
                localFreq[word]++;
            }
        }
        std::lock_guard<std::mutex> lock(mtx);
        for (const auto& entry : localFreq) {
            wordFreq[entry.first] += entry.second;
        }
        };

    int wordsPerThread = tokens.size() / std::thread::hardware_concurrency();
    std::vector<std::thread> threads;

    for (int i = 0; i < tokens.size(); i += wordsPerThread) {
        threads.emplace_back(countWordsInRange, i, std::min(i + wordsPerThread, (int)tokens.size()));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::vector<std::pair<std::string, int>> sortedWords(wordFreq.begin(), wordFreq.end());
    std::sort(sortedWords.begin(), sortedWords.end());

    std::cout << "Frecventa cuvintelor este:" << std::endl;
    for (const auto& entry : sortedWords) {
        std::cout << entry.first << ": " << entry.second << std::endl;
    }
}

int main() {
    std::ifstream inputFile("input.txt");
    if (!inputFile) {
        std::cerr << "Nu s-a putut deschide fisierul input.txt" << std::endl;
        return 1;
    }

    char bom[3];
    inputFile.read(bom, 3);
    if (!(bom[0] == '\xEF' && bom[1] == '\xBB' && bom[2] == '\xBF')) {
        inputFile.seekg(0); // daca nu exista BOM, revenim la inceputul fisierului
    }

    std::stringstream buffer;
    buffer << inputFile.rdbuf();
    std::string text = buffer.str();

    auto start = std::chrono::high_resolution_clock::now();
    countWordFrequenciesThread(text);
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Timpul de executie: " << duration.count() << " milisecunde" << std::endl;

    return 0;
}
