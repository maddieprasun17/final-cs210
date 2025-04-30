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

using namespace std;

string toLower(const string &s) {
    string result = s;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

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

double searchCSV(const string &fileName, const string &city, const string &country) {
    string lowerCity = toLower(city);
    string lowerCountry = toLower(country);

    ifstream file(fileName);
    if (!file.is_open()) {
        cout << "Error opening file " << fileName << endl;
        return -1;
    }

    string line;
    getline(file, line);

    while (getline(file, line)) {
        stringstream ss(line);
        string countryCode, cityName, populationStr;

        getline(ss, countryCode, ',');
        getline(ss, cityName, ',');
        getline(ss, populationStr, ',');

        if (toLower(countryCode) == lowerCountry && toLower(cityName) == lowerCity) {
            return stod(populationStr);
        }
    }
    return -1;
}

int main() {
    const string csvFile = "C:\\Users\\maddi\\Downloads\\world_cities.csv";

    return 0;
}