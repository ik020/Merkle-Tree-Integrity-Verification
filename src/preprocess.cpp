#include "preprocess.h"
#include "json.hpp" 
#include <fstream>
#include <iostream>
#include <string>
using json = nlohmann::json;

// Initialize dynamic array
void init_review_array(ReviewArray& arr, size_t initial_capacity) {
    arr.reviews = new Review[initial_capacity];
    arr.capacity = initial_capacity;
    arr.size = 0;
}

// Free memory
void free_review_array(ReviewArray& arr) {
    delete[] arr.reviews;
    arr.reviews = nullptr;
    arr.size = 0;
    arr.capacity = 0;
}

// Helper: trim whitespace
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \n\r\t");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \n\r\t");
    return str.substr(first, (last - first + 1));
}

// Check if ID exists in array
bool id_exists(ReviewArray& arr, const std::string& id) {
    for (size_t i = 0; i < arr.size; i++) {
        if (arr.reviews[i].reviewID == id)
            return true;
    }
    return false;
}

// Add review to dynamic array (expand if needed)
void add_review(ReviewArray& arr, const Review& rev) {
    if (arr.size >= arr.capacity) {
        // double the capacity
        size_t new_cap = arr.capacity * 2;
        Review* new_arr = new Review[new_cap];
        for (size_t i = 0; i < arr.size; i++)
            new_arr[i] = arr.reviews[i];
        delete[] arr.reviews;
        arr.reviews = new_arr;
        arr.capacity = new_cap;
    }
    arr.reviews[arr.size++] = rev;
}

// Load JSON reviews into array
void load_reviews(const std::string& filename, ReviewArray& arr) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filename << "\n";
        return;
    }

    size_t counter = 1;
    std::string line;
    while (std::getline(file, line)) {
        try {
            auto j = json::parse(line);
            Review rev;

            if (j.contains("reviewID") && !j["reviewID"].is_null())
                rev.reviewID = j["reviewID"].get<std::string>();
            else
                rev.reviewID = "GENID_" + std::to_string(counter);

            // Ensure unique ID manually
            while (id_exists(arr, rev.reviewID)) {
                rev.reviewID = "GENID_" + std::to_string(counter++);
            }

            if (j.contains("reviewText") && !j["reviewText"].is_null())
                rev.reviewText = trim(j["reviewText"].get<std::string>());
            else
                rev.reviewText = "";

            add_review(arr, rev);
            counter++;
        }
        catch (...) {
            continue;
        }
    }

    std::cout << "Loaded " << arr.size << " reviews.\n";
}
