#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <string>
#include <filesystem>

int n = 3; // Change this to the number of processes
int p, vs, ds;

std::string format_plaintext(const std::set<int> & p)
{
    std::stringstream ss;
    for (auto it = p.begin(); it != p.end(); ++it) {
        if (it != p.begin()) {
            ss << " ";
        }
        ss << *it;
    }
    return ss.str();
}

bool directoryExists(const std::string& path) {
    return std::filesystem::exists(path) && std::filesystem::is_directory(path);
}

bool isSubset(const std::set<int>& a, const std::set<int>& b) {
    return std::includes(b.begin(), b.end(), a.begin(), a.end());
}

bool verifySubsetValidity(const std::vector<std::vector<std::set<int>>>& proposals, const std::vector<std::vector<std::set<int>>>& decisions) {
    bool res = true;
    for (size_t i = 0; i < p; ++i) {
        for (size_t j = 0; j < n; ++j) {
            if (!isSubset(proposals[j][i], decisions[j][i])) {
                std::cerr << "In line " << i + 1 << ", proc " << j + 1 << " (Subset Validity Failed)\n";

                std::cout << format_plaintext(proposals[j][i]) << std::endl;
                std::cout << format_plaintext(decisions[j][i]) << std::endl;
                res = false;
            }
        }
    }
    return res;
}

bool verifyJointProposals(const std::vector<std::vector<std::set<int>>>& proposals, const std::vector<std::vector<std::set<int>>>& decisions) {
    bool res = true;
    for (size_t i = 0; i < p; ++i) {
        std::set<int> jointProposals;
        // Combine all proposals from different processes into jointProposals
        for (size_t j = 0; j < n; ++j) {
            jointProposals.insert(proposals[j][i].begin(), proposals[j][i].end());
        }

        // Verify if the decision O_i is a subset of jointProposals
        for (size_t j = 0; j < n; ++j) {
            if (!isSubset(decisions[j][i], jointProposals)) {
                 std::cerr << "In line " << i + 1 << ", proc " << j + 1 << " (Joint Proposals Failed)\n";

                std::cout << format_plaintext(decisions[j][i]) << std::endl;
                std::cout << format_plaintext(jointProposals) << std::endl;
                res = false;
            }
        }       
    }
    return res;
}

bool verifyConsistency(const std::vector<std::vector<std::set<int>>>& decisions) {
    bool res = true;
    for (size_t k = 0; k < p; ++k) {
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                if (!(isSubset(decisions[j][k], decisions[i][k]) || isSubset(decisions[i][k], decisions[j][k]))) {
                    std::cerr << "In line " << k + 1 << ", proc " << i + 1 << " & " << j + 1 << " (Consistency verification failed)\n";

                    std::cout << format_plaintext(decisions[i][k]) << std::endl;
                    std::cout << format_plaintext(decisions[j][k]) << std::endl;
                    res = false;
                }
            }
        }
    }
    return res;
}

bool readConfigAndOutput(const std::string& configPath, const std::string& outputPath, std::vector<std::set<int>>& proposals, std::vector<std::set<int>>& decisions) {
    std::ifstream config(configPath);
    std::ifstream output(outputPath);

    if (!config.is_open()) {
        std::cerr << "Failed to open files " << configPath << ".\n";
        return false;
    }
    
    if (!output.is_open()) {
        std::cerr << "Failed to open files " << outputPath << ".\n";
        return false;
    }

    config >> p >> vs >> ds;

    proposals.resize(p);
    decisions.resize(p);

    // Read proposals from config file
    for (int i = 0; i < p; ++i) {
        std::string line;
        std::getline(config, line); // Consume newline
        std::getline(config, line);

        std::stringstream ss(line);
        int num;
        while (ss >> num) {
            proposals[i].insert(num);
        }
    }

    // Read decisions from output file
    for (int i = 0; i < p; ++i) {
        std::string line;
        std::getline(output, line);

        std::stringstream ss(line);
        int num;
        while (ss >> num) {
            decisions[i].insert(num);
        }
    }

    return true;
}

std::string formatFileName(const std::string& dir, int processNumber, const std::string& str) {
    std::ostringstream oss;
    oss << dir << "proc" << std::setw(2) << std::setfill('0') << processNumber << str;
    return oss.str();
}

int main(int argc, char* argv[]) {
    std::string DIR_TO_FILE;
    if (argc > 1) {
        DIR_TO_FILE = argv[1];
         if (!directoryExists(DIR_TO_FILE)) {
            std::cerr << "Directory does not exist: " << DIR_TO_FILE << std::endl;
            return 1;
        }
        DIR_TO_FILE = DIR_TO_FILE + "/";
    }
    
    std::vector<std::vector<std::set<int>>> allProposals(n);
    std::vector<std::vector<std::set<int>>> allDecisions(n);

    for (int i = 0; i < n; ++i) {
        std::string configPath = formatFileName(DIR_TO_FILE, i + 1, ".config");
        std::string outputPath = formatFileName(DIR_TO_FILE, i + 1, ".output");

        if (!readConfigAndOutput(configPath, outputPath, allProposals[i], allDecisions[i])) {
            std::cerr << "Failed to read. Abort.\n";
            return 1;
        }
    }

    std::cout << "number of proposals: " << p << std::endl;

    bool verify = true;
    if (verifySubsetValidity(allProposals, allDecisions)) {
        std::cout << "Verification successful for subset validity.\n";
    } else verify = false;

    if (verifyJointProposals(allProposals, allDecisions)) {
        std::cout << "Verification successful for joint proposal.\n";
    } else verify = false;

    if (verifyConsistency(allDecisions)) {
        std::cout << "Verification successful for consistency.\n";
    } else verify = false;

    if (verify) {
        std::cout << "Verification successful for all conditions.\n";
    }

    return 0;
}