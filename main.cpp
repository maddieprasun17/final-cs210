#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <random>

using namespace std;
using namespace std::chrono;

string toLower(const string &s) {
    string result = s;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

struct TrieNode {
    bool isEndOfWord;
    unordered_map<string, double> countryPopulation;
    unordered_map<char, TrieNode*> children;
    TrieNode() : isEndOfWord(false) {}
};

class NameTrie {
private:
    TrieNode* root;

public:
    NameTrie() {
        root = new TrieNode();
    }

    void insert(const string& cityName, const string& countryCode, double population) {
        TrieNode* node = root;
        string lowerCity = toLower(cityName);
        for (char c : lowerCity) {
            if (node->children.find(c) == node->children.end()) {
                node->children[c] = new TrieNode();
            }
            node = node->children[c];
        }
        node->isEndOfWord = true;
        string lowerCountry = toLower(countryCode);
        node->countryPopulation[lowerCountry] = population;
    }

    double search(const string& cityName, const string& countryCode) {
        TrieNode* node = root;
        string lowerCity = toLower(cityName);
        string lowerCountry = toLower(countryCode);
        for (char c : lowerCity) {
            if (node->children.find(c) == node->children.end()) {
                return -1.0;
            }
            node = node->children[c];
        }
        if (!node->isEndOfWord) {
            return -1.0;
        }
        auto it = node->countryPopulation.find(lowerCountry);
        if (it == node->countryPopulation.end()) {
            return -1.0;
        }
        return it->second;
    }
};

struct CacheEntry {
    string key;
    string city;
    string country;
    double population;
};

class Cache {
public:
    virtual ~Cache() = default;
    virtual bool get(const string &key, double &population) = 0;
    virtual void put(const string &key, const string &city, const string &country, double population) = 0;
    virtual void printCache() const = 0;
};

class LFUCache : public Cache {
private:
    struct Node {
        string key;
        string city;
        string country;
        double population;
        int freq;
        list<string>::iterator freq_it;
    };

    int capacity;
    int min_freq;
    unordered_map<string, Node> key_map;
    unordered_map<int, list<string>> freq_map;

public:
    LFUCache(int cap) : capacity(cap), min_freq(0) {}

    bool get(const string &key, double &population) override {
        auto it = key_map.find(key);
        if (it == key_map.end()) {
            return false;
        }

        Node &node = it->second;
        int old_freq = node.freq;
        node.freq++;
        population = node.population;

        freq_map[old_freq].erase(node.freq_it);
        if (freq_map[old_freq].empty()) {
            freq_map.erase(old_freq);
            if (old_freq == min_freq) {
                min_freq++;
            }
        }

        freq_map[node.freq].push_front(key);
        node.freq_it = freq_map[node.freq].begin();
        return true;
    }

    void put(const string &key, const string &city, const string &country, double population) override {
        if (capacity <= 0) return;

        auto it = key_map.find(key);
        if (it != key_map.end()) {
            Node &node = it->second;
            node.population = population;
            node.city = city;
            node.country = country;
            double dummy;
            get(key, dummy);
            return;
        }

        if (key_map.size() >= capacity) {
            string evict_key = freq_map[min_freq].back();
            freq_map[min_freq].pop_back();
            key_map.erase(evict_key);

            if (freq_map[min_freq].empty()) {
                freq_map.erase(min_freq);
            }
        }

        min_freq = 1;
        freq_map[min_freq].push_front(key);
        key_map[key] = {key, city, country, population, 1, freq_map[min_freq].begin()};
    }

    void printCache() const override {
        cout << "\n--------- Current LFU Cache ---------\n";
        for (const auto &pair : key_map) {
            const Node &node = pair.second;
            cout << "City: " << node.city << ", Country: " << node.country << ", Population: " << node.population << ", Freq: " << node.freq << "\n";
        }
        cout << "-------------------------------------\n";
    }
};

class FIFOCache : public Cache {
private:
    list<CacheEntry> entries;
    unordered_map<string, list<CacheEntry>::iterator> cacheMap;
    int capacity;

public:
    FIFOCache(int cap) : capacity(cap) {}

    bool get(const string &key, double &population) override {
        auto it = cacheMap.find(key);
        if (it == cacheMap.end()) {
            return false;
        }
        population = it->second->population;
        return true;
    }

    void put(const string &key, const string &city, const string &country, double population) override {
        auto it = cacheMap.find(key);
        if (it != cacheMap.end()) {
            it->second->population = population;
            return;
        }

        if (entries.size() >= capacity) {
            CacheEntry oldest = entries.front();
            cacheMap.erase(oldest.key);
            entries.pop_front();
        }

        entries.push_back({key, city, country, population});
        cacheMap[key] = --entries.end();
    }

    void printCache() const override {
        cout << "\n--------- Current FIFO Cache ---------\n";
        for (const CacheEntry &entry : entries) {
            cout << "City: " << entry.city << ", Country: " << entry.country << ", Population: " << entry.population << "\n";
        }
        cout << "--------------------------------------\n";
    }
};

class RandomCache : public Cache {
private:
    vector<CacheEntry> entries;
    unordered_map<string, size_t> keyMap;
    int capacity;

public:
    RandomCache(int cap) : capacity(cap) { srand(time(0)); }

    bool get(const string &key, double &population) override {
        auto it = keyMap.find(key);
        if (it == keyMap.end())
            return false;
        population = entries[it->second].population;
        return true;
    }

    void put(const string &key, const string &city, const string &country, double population) override {
        auto it = keyMap.find(key);
        if (it != keyMap.end()) {
            entries[it->second] = {key, city, country, population};
            return;
        }

        if (entries.size() >= capacity) {
            int index = rand() % entries.size();
            string evictKey = entries[index].key;

            if (index != entries.size() - 1) {
                entries[index] = entries.back();
                keyMap[entries[index].key] = index;
            }

            entries.pop_back();
            keyMap.erase(evictKey);
        }

        entries.push_back({key, city, country, population});
        keyMap[key] = entries.size() - 1;
    }

    void printCache() const override {
        cout << "\n------- Current Random Cache --------\n";
        for (const CacheEntry &entry : entries) {
            cout << "City: " << entry.city << ", Country: " << entry.country
                 << ", Population: " << entry.population << "\n";
        }
        cout << "-------------------------------------\n";
    }
};

int main() {
    const string csvFile = "C:\\Users\\maddi\\Downloads\\world_cities.csv";
    NameTrie trie;

    ifstream file(csvFile);
    if (!file.is_open()) {
        cerr << "Error opening file " << csvFile << endl;
        return 1;
    }

    vector<pair<string, string>> allCities;
    string line;
    getline(file, line);

    while (getline(file, line)) {
        stringstream ss(line);
        string countryCode, cityName, populationStr;
        getline(ss, cityName, ',');
        getline(ss, countryCode, ',');
        getline(ss, populationStr, ',');

        try {
            double population = stod(populationStr);
            trie.insert(cityName, countryCode, population);
            allCities.emplace_back(cityName, countryCode);
        } catch (const exception &e) {
            cerr << "Error parsing line: " << line << " - " << e.what() << endl;
        }
    }
    file.close();

    if (allCities.empty()) {
        cerr << "No cities loaded. Exiting!" << endl;
        return 1;
    }

    const int numQueries = 750;
    const int sampleSize = 250;
    vector<pair<string, string>> testQueries;

    shuffle(allCities.begin(), allCities.end(), mt19937{random_device{}()});
    for (int i = 0; i < sampleSize && i < allCities.size(); i++) {
        testQueries.push_back(allCities[i]);
    }

    while (testQueries.size() < numQueries) {
        testQueries.push_back(testQueries[rand() % testQueries.size()]);
    }

    shuffle(testQueries.begin(), testQueries.end(), mt19937{random_device{}()});

    ofstream outFile("C:\\Users\\maddi\\Downloads\\load_results.csv");
    outFile << "CacheType,QueryNumber,Country,City,Hit,TimeMicroSeconds\n";

    vector<string> cacheTypes = {"LFU", "FIFO", "Random"};
    for (const string& type : cacheTypes) {
        Cache* cache = nullptr;
        if (type == "LFU") {
            cache = new LFUCache(10);
        } else if (type == "FIFO") {
            cache = new FIFOCache(10);
        } else {
            cache = new RandomCache(10);
        }

        for (int i = 0; i < numQueries; ++i) {
            string city = testQueries[i].first;
            string country = testQueries[i].second;
            string key = toLower(country) + "|" + toLower(city);
            double population;
            bool hit;

            auto start = high_resolution_clock::now();
            hit = cache->get(key, population);
            if (!hit) {
                population = trie.search(city, country);
                if (population != -1.0) {
                    cache->put(key, city, country, population);
                }
            }

            auto end = high_resolution_clock::now();
            duration<double, micro> duration = end - start;
            outFile << type << "," << i+1 << "," << city << "," << country << "," << (hit ? "1" : "0") << "," << fixed << setprecision(3) << duration.count() << "\n";
        }
        delete cache;
    }
    outFile.close();
    return 0;
}