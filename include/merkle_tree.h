#pragma once
#include <string>
#include "picosha2.h"
using namespace std;

struct MerkleNode {
    string hash;
    MerkleNode* left = nullptr;
    MerkleNode* right = nullptr;
    MerkleNode* parent = nullptr;
};

struct MerkleTree {
    MerkleNode** leaves = nullptr;
    size_t leafCount = 0;
    MerkleNode* root = nullptr;
};

struct ProofStep {
    string siblingHash;
    bool isLeft; // true = sibling on left, false = sibling on right
};

MerkleNode* build_tree(MerkleNode** nodes, size_t count);
void init_merkle_tree(MerkleTree& tree, string* reviewIDs, string* reviewTexts, size_t n);
void free_merkle_tree(MerkleTree& tree);
string get_merkle_root(MerkleTree& tree);

bool generate_proof(MerkleTree& tree, const string& leafHash, ProofStep proof[], size_t& proofLen);
bool verify_proof(const string& leafHash, ProofStep proof[], size_t proofLen, const string& rootHash);
