#include "merkle_tree.h"
#include <functional>

// Recursive tree builder
MerkleNode* build_tree(MerkleNode** nodes, size_t count) {
    if (count == 0) return nullptr;
    if (count == 1) return nodes[0];

    size_t parentCount = (count + 1) / 2;
    MerkleNode** parents = new MerkleNode * [parentCount];

    for (size_t i = 0, j = 0; i < count; i += 2, j++) {
        MerkleNode* parent = new MerkleNode;
        parent->left = nodes[i];
        nodes[i]->parent = parent;

        if (i + 1 < count) {
            parent->right = nodes[i + 1];
            nodes[i + 1]->parent = parent;
            parent->hash = picosha2::hash256_hex_string(
                parent->left->hash + parent->right->hash
            );
        }
        else {
            parent->right = nullptr;
            parent->hash = parent->left->hash;
        }

        parents[j] = parent;
    }

    MerkleNode* root = build_tree(parents, parentCount);
    delete[] parents;
    return root;
}

// Initialize tree
void init_merkle_tree(MerkleTree& tree, string* reviewIDs, string* reviewTexts, size_t n) {
    tree.leafCount = n;
    tree.leaves = new MerkleNode * [n];

    for (size_t i = 0; i < n; i++) {
        tree.leaves[i] = new MerkleNode;
        tree.leaves[i]->hash = picosha2::hash256_hex_string(reviewIDs[i] + reviewTexts[i]);
    }

    tree.root = build_tree(tree.leaves, n);
}

// Free memory
void free_merkle_tree(MerkleTree& tree) {
    function<void(MerkleNode*)> free_node = [&](MerkleNode* node) {
        if (!node) return;
        free_node(node->left);
        free_node(node->right);
        delete node;
        };

    free_node(tree.root);

    delete[] tree.leaves;
    tree.leaves = nullptr;
    tree.leafCount = 0;
    tree.root = nullptr;
}

string get_merkle_root(MerkleTree& tree) {
    return tree.root ? tree.root->hash : "";
}

// Generate Merkle Proof
bool generate_proof(MerkleTree& tree, const string& leafHash, ProofStep proof[], size_t& proofLen) {
    MerkleNode* leaf = nullptr;

    for (size_t i = 0; i < tree.leafCount; i++) {
        if (tree.leaves[i]->hash == leafHash) {
            leaf = tree.leaves[i];
            break;
        }
    }

    if (!leaf) return false;

    proofLen = 0;
    MerkleNode* current = leaf;

    while (current->parent) {
        MerkleNode* parent = current->parent;

        if (parent->left == current && parent->right) {
            proof[proofLen].siblingHash = parent->right->hash;
            proof[proofLen].isLeft = false;
        }
        else if (parent->right == current && parent->left) {
            proof[proofLen].siblingHash = parent->left->hash;
            proof[proofLen].isLeft = true;
        }

        proofLen++;
        current = parent;
    }

    return true;
}

// Verify Merkle Proof
bool verify_proof(const string& leafHash, ProofStep proof[], size_t proofLen, const string& rootHash) {
    string hash = leafHash;

    for (size_t i = 0; i < proofLen; i++) {
        if (proof[i].isLeft)
            hash = picosha2::hash256_hex_string(proof[i].siblingHash + hash);
        else
            hash = picosha2::hash256_hex_string(hash + proof[i].siblingHash);
    }

    return hash == rootHash;
}
