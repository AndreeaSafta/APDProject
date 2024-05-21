#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <mpi.h>
#include <chrono>

using namespace std;

// functia pentru tokenizarea unei linii de text
vector<string> tokenize(const string& line) {
    vector<string> tokens;
    stringstream ss(line);
    string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size); // obt numarul total de procese
    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank); // obt rangul procesului curent

    // masurarea timpului de inceput
    auto start_time = chrono::high_resolution_clock::now();

    string filename = "input.txt";
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Eroare la deschiderea fisierului!" << endl;
        MPI_Finalize();
        return 1;
    }


    stringstream buffer;
    buffer << file.rdbuf();
    string text = buffer.str();
    file.close();

    // tokenizarea textului
    vector<string> tokens = tokenize(text);

    int total_words = tokens.size();
    int wordsPerProcess = total_words / world_size;
    int extraWords = total_words % world_size;

    int start = world_rank * wordsPerProcess + min(world_rank, extraWords);
    int end = start + wordsPerProcess + (world_rank < extraWords ? 1 : 0);

    unordered_map<string, int> localWordCounts;
    for (int i = start; i < end; ++i) {
        localWordCounts[tokens[i]]++;
    }


    if (world_rank == 0) {
        unordered_map<string, int> globalWordCounts = localWordCounts;

        MPI_Status status;
        for (int source = 1; source < world_size; ++source) {
            int numEntries;
            MPI_Recv(&numEntries, 1, MPI_INT, source, 0, MPI_COMM_WORLD, &status);

            for (int i = 0; i < numEntries; ++i) {
                int wordLength;
                MPI_Recv(&wordLength, 1, MPI_INT, source, 0, MPI_COMM_WORLD, &status);

                char* word = new char[wordLength + 1];
                MPI_Recv(word, wordLength + 1, MPI_CHAR, source, 0, MPI_COMM_WORLD, &status);

                int count;
                MPI_Recv(&count, 1, MPI_INT, source, 0, MPI_COMM_WORLD, &status);

                globalWordCounts[string(word)] += count;
                delete[] word;
            }
        }

        // sortarea perechilor cuvant-frecventa dupa cuvant
        vector<pair<string, int>> sortedWordCounts(globalWordCounts.begin(), globalWordCounts.end());
        sort(sortedWordCounts.begin(), sortedWordCounts.end());

        // afisarea rezultatelor sortate
        for (const auto& pair : sortedWordCounts) {
            cout << "Frecventa cuvantului '" << pair.first << "': " << pair.second << endl;
        }

        // afisare timp de executie 
        auto end_time = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();
        cout << "Timpul total de rulare: " << duration << " milisecunde" << endl;
    }
    else {
        int numEntries = localWordCounts.size();
        MPI_Send(&numEntries, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

        for (const auto& pair : localWordCounts) {
            int wordLength = pair.first.size();
            MPI_Send(&wordLength, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Send(pair.first.c_str(), wordLength + 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&pair.second, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
