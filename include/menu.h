#pragma once
#include <vector>
#include <string>
#include "merkle_tree.h"
#include "picosha2.h"
#include "json.hpp"

using namespace std;

struct ProofStep;

class Menu {
public:
    Menu();
    ~Menu();

    void display();
    void handleInput();
    void loadDataset();
    void buildMerkleTree();
    void saveRoot();
    void compareRoot();
    void generateProof();
    void modifyReview();
    void simulateTampering();
    void visualizeTree();
    void runPerformanceTests();

private:
    vector<string> reviewIDs;
    vector<string> reviewTexts;

    MerkleTree tree;
    bool treeBuilt;

    void visualizeProofTree(const string& leafHash, const vector<ProofStep>& proof, size_t proofLen);
};
