#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <unordered_map>
#include <algorithm>

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
    Cache(int capacity) : capacity(capacity) {}

    bool get(const string &key, double &population) {
        auto it = cacheMap.find(key);
        if (it == cacheMap.end())
            return false;
        cacheList.splice(cacheList.begin(), cacheList, it->second);
        population = it->second->population;
        return true;
    }

    void put(const string &key, const string &city, const string &country, double population) {
        auto it = cacheMap.find(key);

        if (it != cacheMap.end()) {
            it->second->population = population;
            cacheList.splice(cacheList.begin(), cacheList, it->second);
        } else {
            if (cacheList.size() == capacity) {
                auto lastIt = cacheList.end();
                --lastIt;
                cacheMap.erase(lastIt->key);
                cacheList.pop_back();
            }
            cacheList.push_front({key, city, country, population});
            cacheMap[key] = cacheList.begin();
        }
    }

    void printCache() const {
        cout << "\n--------- Current Cache ---------\n";
        for (const auto &entry : cacheList) {
            cout << "City: " << entry.city << ", Country: " << entry.country << ", Population: " << entry.population << "\n";
        }
        cout << "---------------------------------\n";
    }

private:
    int capacity;
    list<CacheEntry> cacheList;
    unordered_map<string, list<CacheEntry>::iterator> cacheMap;
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
    Cache cache(10);

    string city, country;
    while (true) {
        cout << "\nEnter city name (or type 'exit' to quit): ";
        getline(cin, city);
        if (toLower(city) == "exit") {
            break;
        }

        cout << "Enter country code: ";
        getline(cin, country);

        string lowerCity = toLower(city);
        string lowerCountry = toLower(country);
        string key = lowerCountry + "|" + lowerCity;
        double population;

        if (cache.get(key, population)) {
            cout << "\n(Cache hit) " << city << ", " << country << " has population: " << population << endl;
        } else {
            population = searchCSV(csvFile, city, country);
            if (population != -1) {
                cout << "\n" << city << ", " << country << " has population: " << population << endl;
                cache.put(key, city, country, population);
            } else {
                cout << "\nCity not found in the dataset.\n";
            }
        }

        cache.printCache();
    }
    return 0;
}