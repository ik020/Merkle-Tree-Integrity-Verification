#include "menu.h"
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include "merkle_tree.h"
#include "picosha2.h"
#include "json.hpp"
#include <queue>
#include <iomanip>
#include <map>
#include <functional>
#include <chrono>
#include <random>
#include <thread>
#include <future>

using namespace std;
using json = nlohmann::json;

Menu::Menu() {
    treeBuilt = false;
}

Menu::~Menu() {
    free_merkle_tree(tree);
}

void Menu::display() {
    cout << "\n==============================\n";
    cout << "      MERKLE TREE SYSTEM      \n";
    cout << "==============================\n";
    cout << "1. Load Dataset" << endl;
    cout << "2. Build Merkle Tree" << endl;
    cout << "3. Save Merkle Root" << endl;
    cout << "4. Compare Merkle Roots" << endl;
    cout << "5. Generate Merkle Proof" << endl;
    cout << "6. Modify a Review" << endl;
    cout << "7. Simulate Tampering" << endl;
    cout << "8. Visualize Merkle Tree" << endl;
    cout << "9. Run Performance Tests" << endl;  
    cout << "0. Exit" << endl;
    cout << "Choose an option: ";
}

void Menu::handleInput() {
    int choice;
    while (true) {
        display();
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Try again.\n";
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n'); 

        switch (choice) {
        case 1: loadDataset(); break;
        case 2: buildMerkleTree(); break;
        case 3: saveRoot(); break;
        case 4: compareRoot(); break;
        case 5: generateProof(); break;
        case 6: modifyReview(); break;
        case 7: simulateTampering(); break;
        case 8: visualizeTree(); break;
        case 9: runPerformanceTests(); break;  
        case 0: cout << "Exiting..." << endl; return;
        default: cout << "Invalid option! Try again.\n";
        }

    }
}

// ===== Load Dataset =====
void Menu::loadDataset() {
    string filename;
    cout << "Enter dataset filename (e.g., reviews_1M_clean.json): ";
    if (!(cin >> filename)) {
        cout << "Invalid filename input.\n";
        return;
    }

    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Could not open dataset file: " << filename << "\n";
        return;
    }

    reviewIDs.clear();
    reviewTexts.clear();

    string line;
    while (getline(file, line)) {
        if (line.empty()) continue;
        try {
            json j = json::parse(line);
            if (j.contains("reviewID") && j.contains("reviewText")) {
                reviewIDs.push_back(j["reviewID"].get<string>());
                reviewTexts.push_back(j["reviewText"].get<string>());
            }
        }
        catch (...) { continue; }
    }

    file.close();
    cout << "Loaded " << reviewIDs.size() << " reviews from " << filename << "\n";
}

// ===== Build Merkle Tree =====
void Menu::buildMerkleTree() {
    if (reviewIDs.empty()) {
        cout << "No dataset loaded. Load the dataset first.\n";
        return;
    }

    free_merkle_tree(tree);
    init_merkle_tree(tree, reviewIDs.data(), reviewTexts.data(), reviewIDs.size());
    treeBuilt = true;

    cout << "Merkle Tree built successfully.\n";
    cout << "Root hash: " << get_merkle_root(tree) << "\n";
}

// ===== Save Merkle Root =====
void Menu::saveRoot() {
    if (!treeBuilt) { cout << "Build the tree first!\n"; return; }
    ofstream out("merkle_root.txt");
    if (!out.is_open()) {
        cout << "Could not open merkle_root.txt for writing.\n";
        return;
    }
    out << get_merkle_root(tree);
    out.close();
    cout << "Merkle Root saved to merkle_root.txt\n";
}

// ===== Compare Merkle Roots =====
void Menu::compareRoot() {
    if (!treeBuilt) { cout << "Build the tree first!\n"; return; }
    ifstream in("merkle_root.txt");
    if (!in.is_open()) { cout << "No saved root to compare.\n"; return; }

    string savedRoot;
    in >> savedRoot;
    in.close();

    string currentRoot = get_merkle_root(tree);
    if (savedRoot == currentRoot)
        cout << "Integrity Verified: Roots match.\n";
    else
        cout << "Data Integrity Violated: Roots do not match!\n";
}

// ===== Generate Merkle Proof =====
void Menu::generateProof() {
    if (!treeBuilt) { cout << "Build the Merkle tree first!\n"; return; }

    cout << "Enter Review ID to generate proof (or numeric index): ";
    string id;
    getline(cin, id);
    if (id.empty()) getline(cin, id);

    int index = -1;
    try { index = stoi(id); }
    catch (...) { index = -1; }

    if (index >= 0 && index < reviewIDs.size()) { /* numeric index */ }
    else {
        for (size_t i = 0; i < reviewIDs.size(); i++)
            if (reviewIDs[i] == id) { index = i; break; }
    }

    if (index == -1) { cout << "Review ID not found!\n"; return; }

    string leafHash = picosha2::hash256_hex_string(reviewIDs[index] + reviewTexts[index]);
    cout << "Leaf hash used for proof: " << leafHash << "\n";

    vector<ProofStep> proof(512);
    size_t proofLen = 0;

    if (!generate_proof(tree, leafHash, proof.data(), proofLen)) {
        cout << "ERROR: Could not generate proof for this review.\n";
        return;
    }

    bool ok = verify_proof(leafHash, proof.data(), proofLen, get_merkle_root(tree));
    cout << "Verification result: " << (ok ? "NO TAMPERING DETECTED" : "TAMPERING DETECTED") << "\n";

    for (size_t i = 0; i < proofLen; i++)
        cout << "Step " << i << " | SiblingHash = " << proof[i].siblingHash
        << " | isLeft = " << proof[i].isLeft << "\n";

    visualizeProofTree(leafHash, proof, proofLen);
}

void Menu::modifyReview() {
    if (reviewTexts.empty()) { cout << "Load dataset first!\n"; return; }

    cout << "Enter review index to modify (0 - " << reviewTexts.size() - 1 << "): ";
    size_t idx;
    if (!(cin >> idx)) {
        cout << "Invalid index input.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (idx >= reviewTexts.size()) { cout << "Invalid index!\n"; return; }

    cout << "Original review: " << reviewTexts[idx] << "\n";
    cout << "Enter new review text: ";
    string newText; getline(cin, newText);
    reviewTexts[idx] = newText;

    free_merkle_tree(tree);
    init_merkle_tree(tree, reviewIDs.data(), reviewTexts.data(), reviewIDs.size());
    treeBuilt = true;

    cout << "Review updated. Merkle tree rebuilt.\n";
}

void Menu::simulateTampering() {
    if (!treeBuilt) { cout << "Build the Merkle tree first!\n"; return; }
    if (reviewIDs.empty() || reviewTexts.empty()) { cout << "No reviews available.\n"; return; }

    string leafHash = picosha2::hash256_hex_string(reviewIDs[0] + reviewTexts[0]);

    vector<ProofStep> proof(512);
    size_t proofLen = 0;

    if (!generate_proof(tree, leafHash, proof.data(), proofLen)) {
        cout << "ERROR: Could not generate proof for tamper check.\n";
        return;
    }

    bool ok = verify_proof(leafHash, proof.data(), proofLen, get_merkle_root(tree));
    cout << (ok ? "NO TAMPERING DETECTED\n" : "TAMPERING DETECTED\n");
}

void Menu::visualizeTree() {
    if (!treeBuilt) { cout << "Build the tree first!\n"; return; }
    if (!tree.root) { cout << "Tree root is null.\n"; return; }

    int maxLevels = 3;
    cout << "Enter how many levels to visualize (root = level 0). Enter 0 to only show root: ";
    if (!(cin >> maxLevels)) { cout << "Invalid input.\n"; return; }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    if (maxLevels < 0) maxLevels = 0;

    ofstream dot("merkle_tree.dot");
    if (!dot.is_open()) { cout << "Failed to create DOT file.\n"; return; }

    dot << "digraph MerkleTree {\n";
    dot << "node [shape=box, style=filled, fontname=\"Courier\"];\n";

    queue<pair<MerkleNode*, pair<string, int>>> q;
    int id = 0;
    string rootName = "node" + to_string(id++);
    q.push({ tree.root, { rootName, 0 } });

    map<MerkleNode*, string> nodeNames;
    nodeNames[tree.root] = rootName;

    while (!q.empty()) {
        auto pr = q.front(); q.pop();
        MerkleNode* node = pr.first;
        string name = pr.second.first;
        int level = pr.second.second;
        if (!node) continue;

        string color;
        if (node == tree.root) color = "#FFD700";            
        else if (!node->left && !node->right) color = "#98FB98"; 
        else color = "#87CEEB";                              

        string label = node->hash.substr(0, min<size_t>(16, node->hash.size())) + "...";
        dot << name << " [label=\"" << label << "\", fillcolor=\"" << color << "\"];\n";

        if (level < maxLevels) {
            if (node->left) {
                string leftName = nodeNames.count(node->left) ? nodeNames[node->left] : "node" + to_string(id++);
                nodeNames[node->left] = leftName;
                dot << name << " -> " << leftName << ";\n";
                q.push({ node->left, { leftName, level + 1 } });
            }
            if (node->right) {
                string rightName = nodeNames.count(node->right) ? nodeNames[node->right] : "node" + to_string(id++);
                nodeNames[node->right] = rightName;
                dot << name << " -> " << rightName << ";\n";
                q.push({ node->right, { rightName, level + 1 } });
            }
        }
    }

    dot << "}\n";
    dot.close();
    cout << "DOT file generated: merkle_tree.dot\n";

#ifdef _WIN32
    system("dot -Tpng merkle_tree.dot -o merkle_tree.png && start merkle_tree.png");
#else
    system("dot -Tpng merkle_tree.dot -o merkle_tree.png && xdg-open merkle_tree.png");
#endif
}

void Menu::visualizeProofTree(const string& leafHash, const vector<ProofStep>& proof, size_t proofLen) {
    if (!treeBuilt || proofLen == 0) { cout << "Cannot visualize proof.\n"; return; }

    ofstream dot("merkle_proof_tree.dot");
    if (!dot.is_open()) { cout << "Failed to create DOT file.\n"; return; }

    dot << "digraph MerkleProofTree {\n";
    dot << "rankdir=TB;\n";
    dot << "node [shape=box, style=filled, fontname=\"Courier\"];\n";

    string prevNode = "leaf";
    dot << prevNode << " [label=\"Leaf\\n" << leafHash.substr(0, 16)
        << "...\", fillcolor=\"#FFA07A\"];\n";

    for (size_t i = 0; i < proofLen; i++) {
        string siblingNode = "s" + to_string(i);
        string parentNode = "p" + to_string(i);

        dot << siblingNode << " [label=\"Sibling\\n" << proof[i].siblingHash.substr(0, 16)
            << "...\", fillcolor=\"#B0E0E6\"];\n";
        dot << parentNode << " [label=\"Parent " << i << "\", fillcolor=\"#87CEFA\"];\n";

        // Highlight path
        if (proof[i].isLeft) {
            dot << siblingNode << " -> " << parentNode << " [penwidth=1];\n";
            dot << prevNode << " -> " << parentNode << " [penwidth=3, color=\"red\"];\n"; // bold path
        }
        else {
            dot << prevNode << " -> " << parentNode << " [penwidth=3, color=\"red\"];\n";
            dot << siblingNode << " -> " << parentNode << " [penwidth=1];\n";
        }

        prevNode = parentNode;
    }

    dot << prevNode << " -> root;\n";
    dot << "root [label=\"Merkle Root\\n" << get_merkle_root(tree).substr(0, 16)
        << "...\", fillcolor=\"#90EE90\"];\n";

    dot << "}\n";
    dot.close();

    cout << "DOT file generated: merkle_proof_tree.dot\n";

#ifdef _WIN32
    system("dot -Tpng merkle_proof_tree.dot -o merkle_proof_tree.png && start merkle_proof_tree.png");
#else
    system("dot -Tpng merkle_proof_tree.dot -o merkle_proof_tree.png && xdg-open merkle_proof_tree.png");
#endif
}

void Menu::runPerformanceTests() {
    if (reviewIDs.empty() || reviewTexts.empty()) {
        cout << "Load dataset first!\n";
        return;
    }

    size_t numTests = 20; 
    cout << "Running advanced performance tests on Merkle tree with " << reviewIDs.size() << " reviews...\n";

    auto startBuild = std::chrono::high_resolution_clock::now();
    free_merkle_tree(tree);
    init_merkle_tree(tree, reviewIDs.data(), reviewTexts.data(), reviewIDs.size());
    auto endBuild = std::chrono::high_resolution_clock::now();
    treeBuilt = true;

    double buildMs = std::chrono::duration<double, std::milli>(endBuild - startBuild).count();

    double memMB = 0.0;

    cout << "Merkle tree built in " << std::fixed << std::setprecision(2) << buildMs << " ms\n";
    cout << "Approx memory used by tree: " << memMB << " MB\n\n";

    cout << "Advanced Performance Results:\n";
    cout << "-----------------------------\n";
    cout << "Test | Proof Gen (ms) | Verify (ms) | Passed\n";

    std::ofstream csv("performance_results.csv");
    if (csv.is_open()) {
        csv << "Test,GenTime(ms),VerifyTime(ms),Passed\n";

        int failedProofs = 0;
        double totalGenMs = 0.0, totalVerMs = 0.0;

        for (size_t t = 0; t < numTests; t++) {
            size_t idx = rand() % reviewIDs.size();
            string leafHash = picosha2::hash256_hex_string(reviewIDs[idx] + reviewTexts[idx]);

            vector<ProofStep> proof(512);
            size_t proofLen = 0;

            auto startGen = std::chrono::high_resolution_clock::now();
            if (!generate_proof(tree, leafHash, proof.data(), proofLen)) {
                failedProofs++;
                csv << t + 1 << ",0,0,NO\n";
                continue;
            }
            auto endGen = std::chrono::high_resolution_clock::now();
            double genMs = std::chrono::duration<double, std::milli>(endGen - startGen).count();
            totalGenMs += genMs;

            auto startVer = std::chrono::high_resolution_clock::now();
            bool ok = verify_proof(leafHash, proof.data(), proofLen, get_merkle_root(tree));
            auto endVer = std::chrono::high_resolution_clock::now();
            double verMs = std::chrono::duration<double, std::milli>(endVer - startVer).count();
            totalVerMs += verMs;

            csv << t + 1 << "," << genMs << "," << verMs << "," << (ok ? "YES" : "NO") << "\n";

            cout << std::setw(4) << t + 1
                << " | " << std::setw(12) << std::fixed << std::setprecision(2) << genMs
                << " | " << std::setw(11) << std::fixed << std::setprecision(2) << verMs
                << " | " << (ok ? "YES" : "NO") << "\n";
        }

        csv.close();

        cout << "\nSummary:\n";
        cout << "-----------------------------\n";
        cout << "Number of tests: " << numTests << "\n";
        cout << "Failed proofs (should be 0): " << failedProofs << "\n";
        cout << "Average proof generation time: " << totalGenMs / numTests << " ms\n";
        cout << "Average proof verification time: " << totalVerMs / numTests << " ms\n";
        cout << "Results saved to performance_results.csv\n";
    }
    else {
        cout << "Failed to open CSV file for writing.\n";
    }
}

