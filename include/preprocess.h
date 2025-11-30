#ifndef PREPROCESS_H
#define PREPROCESS_H

#include <string>

struct Review {
    std::string reviewID;
    std::string reviewText;
};

// Dynamic array for reviews
struct ReviewArray {
    Review* reviews;
    size_t capacity;
    size_t size;
};

void load_reviews(const std::string& filename, ReviewArray& arr);
void init_review_array(ReviewArray& arr, size_t initial_capacity);
void free_review_array(ReviewArray& arr);

#endif
