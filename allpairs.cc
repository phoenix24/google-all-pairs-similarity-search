// Copyright 2007 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ---
// A simple all-similar-pairs algorithm for binary vector input.
// ---
// Author: Roberto Bayardo

#include "allpairs.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>

#include <algorithm>
#include <string>
#include <vector>

#include <iostream>

#ifdef MICROSOFT
#define fseeko _fseeki64 
#define ftello _ftelli64 
#endif

namespace {

// A fudge factor so that we are conservative in dealing with floating
// point rounding issues.
const double kFudgeFactor = .00000001;

// If a vector is encountered with size larger than this constant,
// then we will barf. This is to protect against memory overflow from
// improperly formatted binary data.
const uint32_t kMaxVectorSize = 99999;

std::string ToString(uint32_t l) {
  char buf[30];
  sprintf(buf, "%lu", l);
  return std::string(buf);
}

// Intersect two vectors. Assumes features appear in both vectors in a
// consistent order. Also assumes that there are no duplicate
// features.
inline int CountSharedFeatures(
    const std::vector<uint32_t>& e1,
    const std::vector<uint32_t>& e2) {
  std::vector<uint32_t>::const_iterator it1 = e1.begin();
  std::vector<uint32_t>::const_iterator it2 = e2.begin();
  int return_me = 0;
  while (it1 != e1.end() && it2 != e2.end()) {
    if (*it1 == *it2) {
      ++return_me;
      ++it1;
      ++it2;
    } else if (*it1 < *it2) {
      ++it1;
    } else {
      ++it2;
    }
  }
  return return_me;
}

}  // namespace

bool AllPairs::FindAllSimilarPairs(
    double similarity_threshold,
    FILE* data,
    uint32_t max_vector_id,
    uint32_t max_feature_id,
    uint32_t max_features_in_ram) {
  Init(similarity_threshold);
  off_t resume_offset = 0;
  std::vector<uint32_t> current_vector;
  double longest_indexed_vector_size;
  do {
    InitScan(max_vector_id, max_feature_id);
    uint32_t features_in_ram = 0;
    if (fseeko(data, resume_offset, 0)) {
      error_ = "fseek failed: " +  std::string(strerror(errno));
      return false;
    }
    resume_offset = 0;
    int result;
    while ((result = NextVector(data, current_vector)) > 0) {
      FindMatches(vector_id_, current_vector);
      if (resume_offset == 0) {
        IndexVector(vector_id_, current_vector);
        features_in_ram += current_vector.size();
        if (features_in_ram > max_features_in_ram) {
          resume_offset = ftello(data);
          std::cerr << "; Halting indexing at line " << lines_processed_ <<
              " and vector id " << vector_id_ << std::endl;
          longest_indexed_vector_size =
              static_cast<double>(current_vector.size());
        }
      } else if (longest_indexed_vector_size / current_vector.size() <
                 t_squared_ - kFudgeFactor) {
        std::cerr <<
            "; Stopping line loop early, remaining vectors too long: " <<
            current_vector.size() << std::endl;
        result = 0;
        break;
      }
    }  // while
    if (result != 0) {
      return false;
    }
  } while (resume_offset != 0);
  std::cout << std::flush;
  InitScan(0,0);  // clear out the big data structures before returning.
  return true;
}

void AllPairs::InitScan(
    uint32_t max_vector_id,
    uint32_t max_feature_id) {
  last_vector_size_ = 0;
  inverted_lists_.clear();
  inverted_lists_.resize(max_feature_id);
  partial_vectors_.clear();
  partial_vectors_.resize(max_vector_id);
  lines_processed_ = 0;
}

void AllPairs::Init(double similarity_threshold) {
  t_ = similarity_threshold;
  t_squared_ = t_ * t_;
  vector_id_ = 0;
  similar_pairs_count_ = 0;
  error_.clear();
  candidates_considered_ = intersections_ = 0;
}

void AllPairs::FindMatches(
    uint32_t vector_id,
    const std::vector<uint32_t>& vec) {
#ifdef MICROSOFT
  candidates_.clear();
#else
  candidates_.clear_no_resize();
#endif
  double vector_size = static_cast<double>(vec.size());
  int min_previous_vector_length =
      static_cast<int>((vector_size * t_squared_) - kFudgeFactor);
  int new_candidates_possible_end_index =
      vec.size() - min_previous_vector_length + 1;
  for (int j = 0; j < vec.size(); ++j) {
    if (vec[j] >= inverted_lists_.size())
      continue;
    InvertedList& il = inverted_lists_[vec[j]];
    // We first advance the starting point.
    while (il.start < il.vector_ids.size() &&
           partial_vectors_[il.vector_ids[il.start]].original_size <
           min_previous_vector_length) {
      ++il.start;
    }
    // Now that we've determined the starting point, we scan the list
    // of vectors to generate the set of candidates with their
    // partially accumulated counts.
    std::vector<uint32_t>::iterator k = il.vector_ids.begin() + il.start;
    const std::vector<uint32_t>::iterator end_k = il.vector_ids.end();
    if (j < new_candidates_possible_end_index) {
      for (; k < end_k; ++k) {
        assert(*k != vector_id);
        candidates_[*k].increment();
      }
    } else {
      // At this point any "new" candidates cannot possibly meet the
      // threshold, so we only increment the counters for elements
      // that are already in the candidate set in order to obtain
      // their partial counts.
      hashmap_iterator_t candidate;
      for (; k != end_k; ++k) {
        assert(*k != vector_id);
        candidate = candidates_.find(*k);
        if (candidate != candidates_.end()) {
          candidate->second.increment();
        }
      }
    }
  }
  candidates_considered_ += candidates_.size();
  // Given the set of candidates with the partially accumulated
  // counts, we determine which candidates can potentially meet the
  // threshold, and for those than can, we perform a list intersection
  // to compute the unaccumulated portion of the score.
  for (hashmap_iterator_t it = candidates_.begin();
       it != candidates_.end();
       ++it) {
    PartialVector& il = partial_vectors_[it->first];
    // Compute an upperbound on the # of shared terms
    int shared_terms = it->second.count;
    shared_terms += il.feature_ids.size();
    // Compute an upperbound on the square of the score
    double denominator = vector_size * static_cast<double>(il.original_size);
    double score_squared =
      static_cast<double>(shared_terms * shared_terms) / denominator;
    if (score_squared >= t_squared_ - kFudgeFactor) {
      if (il.feature_ids.size() == 0) {
        // For this case, the upperbound is precise
        FoundSimilarPair(vector_id, it->first, sqrt(score_squared));
      } else {
        // Need to compute the exact # of shared terms to get the precise score
        ++intersections_;
        shared_terms = CountSharedFeatures(vec, il.feature_ids) +
            it->second.count;
        score_squared = static_cast<double>(
            shared_terms * shared_terms) / denominator;
        if (score_squared >= t_squared_ - kFudgeFactor) {
          FoundSimilarPair(vector_id, it->first, sqrt(score_squared));
        }
      }
    }
  }
}

void AllPairs::FoundSimilarPair(
    uint32_t id1,
    uint32_t id2,
    double similarity_score) {
  // Right now we simply dump similar pairs to stdout.
  std::cout << id1 << ',' << id2 << ',' << similarity_score << '\n';
  ++similar_pairs_count_;
}

void AllPairs::IndexVector(
    uint32_t vector_id,
    const std::vector<uint32_t>& current_vector) {
  // Find the number of features we do *not* want to index.
  int size = current_vector.size();
  int not_indexed_count = static_cast<int>(
      (static_cast<double>(size) *
       static_cast<double>(t_)) -
      kFudgeFactor);
  // Create the partial vector consisting of the unindexed features.
  if (vector_id >= partial_vectors_.size())
    partial_vectors_.resize(vector_id + 1);
  if (not_indexed_count > 0) {
    partial_vectors_[vector_id].feature_ids.assign(
        current_vector.begin() + size - not_indexed_count,
        current_vector.end());
  }
  partial_vectors_[vector_id].original_size = size;
  // Put all other features in the inverted index.
  size -= not_indexed_count;
  for (int i = 0; i < size; ++i) {
    if (current_vector[i] >= inverted_lists_.size())
      inverted_lists_.resize(current_vector[i] + 1);
    inverted_lists_[current_vector[i]].vector_ids.push_back(vector_id);
  }
}

int AllPairs::NextVector(
    FILE* data,
    std::vector<uint32_t>& vec) {
  size_t bytes_read;
  uint32_t vector_size;

  while ((bytes_read = fread(&vector_id_, 1, 4, data)) == 4) {
    bytes_read = fread(&vector_size, 1, 4, data);
    if (bytes_read != 4) {
      if (ferror(data))
        break;
      error_ = "Dataset format error. Partial vector length encountered "
          "for vector id " + ToString(vector_id_);
      return -1;
    }
    if (vector_size > kMaxVectorSize) {
      error_ = "Dataset format error. Size of vector id " +
          ToString(vector_id_) +
          " exceeds maximum: " +
          ToString(vector_size);
      return -1;
    }
    if (vector_size < last_vector_size_) {
      error_ = "Dataset format error. Size of vector id " +
          ToString(vector_id_) +
          " is smaller than that of the preceeding vector: " +
          ToString(vector_size);
      return -1;
    }
    last_vector_size_ = vector_size;
    vec.resize(vector_size);
    bytes_read = fread(&(vec[0]), 1, 4 * vector_size, data);
    if (bytes_read != 4 * vector_size) {
      if (ferror(data))
        break;
      error_ = "Dataset format error. Dataset truncated while reading "
          "features from vector id " +
          ToString(vector_id_);
      return -1;
    }
    lines_processed_++;
    return 1;
  }
  if (ferror(data)) {
    error_ = "Dataset read error, ferror code=" + ToString(ferror(data));
    return -1;
  }
  if (bytes_read != 0) {
    error_ = "Dataset format error. Partial vector id encountered.";
    return -1;
  }
  return 0;
}
