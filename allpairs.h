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
// This class implements an algorithm that identifies all similar
// pairs of vectors in a given input file.
//
// The input file must have the following properties for the algorithm
// to behave correctly and/or efficiently:
//
// 1) It must consist of a list of vectors in sparse vector
// representation. The currently supported format is "apriori binary".
//
// 2) Vectors in the file are assumed to appear in increasing order of
// vector size. That is, a vector of size $i$ will always appear after
// a vector of size less than $i$.
//
// 3) Features within a vector must always appear from least to most
// frequent in a consistent order.  That is if feature $x$ appears
// less frequently than feature $y$ within the dataset, then $x$
// should always appear before $y$ within any vector containing both
// features. Furthermore if two features $x$ and $y$ have the same
// frequency, then one must be chosen to consistently appear before
// the other should they both appear in a given vector.
//
// 4) A vector must not contain duplicate features.
// ---
// Author: Roberto Bayardo
//
#ifndef _ALLPAIRS_H_
#define _ALLPAIRS_H_

#include <vector>

#ifdef MICROSOFT   // MS VC++ specific code
typedef unsigned __int32  uint32_t;
#include <hash_map>   // NOTE we don't use sparsehash with MS VC++!
#else
#include <stdint.h>
#include <google/dense_hash_map>
#endif

class DataSourceIterator;

class AllPairs {
 public:

  AllPairs() {
#ifndef MICROSOFT
    candidates_.set_empty_key(0);
#endif
  }
  ~AllPairs() {
    InitScan(0);
  }

  // Finds all pairs of vectors in the "data" stream with cosine
  // similarity meeting the specified threshold. Does not assume
  // ownership of the data stream. Returns false if the computation
  // could not complete successfully due to data stream failure. A
  // call to GetErrorMessage() on the data stream object will return a
  // human-readable description of the problem.
  //
  // The caller can provide an estimate of the max_feature_id which
  // will be used to preallocate buffers.  The correct output will be
  // produced even if the estimate is inaccurate (provided there is
  // sufficient memory for the buffers to be allocated to the
  // specified size.)
  //
  // The caller can also provide a maximum on the number of features
  // that will be stored in ram. This will bound the memory used, at
  // the expense of extra scans of the dataset should it contain more
  // features than the maximum.
  //
  // This method may output status & progress messages to stderr.
  bool FindAllSimilarPairs(
      double similarity_threshold,
      DataSourceIterator* data,
      uint32_t max_feature_id,
      uint32_t max_features_in_ram);

  // Returns the number of similar pairs found by the last call to
  // FindAllSimilarPairs.
  long SimilarPairsCount() const { return similar_pairs_count_; }

  // Returns the number of pair candidates considered by the last call
  // to FindAllSimilarPairs.
  long long CandidatesConsidered() const { return candidates_considered_; }

  // Returns the number of vector intersections performed by the last
  // call to FindSimilarPairs.
  long long IntersectionsPerformed() const { return intersections_; }

 private:
  // First method called by FindSimilarPairs for rudimentary variable
  // initialization.
  void Init(double similarity_threshold);

  // Called by FindSimilarPairs before beginning any dataset scan to
  // reset all relevant datastructures.
  void InitScan(uint32_t max_feature_id);

  // Finds all vectors in the inverted index that are similar to the
  // given vector.
  void FindMatches(
      uint32_t vector_id,
      const std::vector<uint32_t>& input_vector);

  // Put the given vector into the partial inverted index.
  void IndexVector(
      uint32_t vector_id,
      const std::vector<uint32_t>& input_vector);

  // Called for each pair of similar vectors found. Current
  // implementation simply outputs the similar pairs to stdout.
  void FoundSimilarPair(
      uint32_t vector_id_1,
      uint32_t vector_id_2,
      double similarity_score);

  double t_;  // stores the similarity threshold
  double t_squared_;

  // Stats variables.
  long similar_pairs_count_;
  int lines_processed_;
  long long candidates_considered_;
  long long intersections_;

  struct PartialVector {
    // Do not use this constructor directly. Use MakePartialVector().
    PartialVector(
        uint32_t vector_id,
        int original_size,
        int partial_size,
        const uint32_t* begin);

    uint32_t id;
    // We store the "actual" length of the vector from which this
    // partial vector was derived.
    int original_size;
    int size;
    uint32_t feature[];
  };

  // Factory method for constructing a partial vector object.
  static PartialVector* MakePartialVector(
      uint32_t vector_id, int orig_size, int size, const uint32_t* features);

  // Releases memory allocated by the MakePartialVector factory.
  static void FreePartialVector(PartialVector* free_me);

  struct InvertedList {
    InvertedList() : start(0) {}
    int start;
    std::vector<PartialVector*> vectors;
  };

  std::vector<InvertedList> inverted_lists_;

  // Stores a list of partial vectors so they can be easily deleted.
  std::vector<PartialVector*> partial_vectors_;

  struct Counter {
    Counter() : count(0) {}
    void increment() { ++count; }
    int count;
  };

#ifdef MICROSOFT
  typedef stdext::hash_map<PartialVector*, Counter> hashmap_t;
  typedef stdext::hash_map<PartialVector*, Counter>::iterator hashmap_iterator_t;
#else
  typedef google::dense_hash_map<PartialVector*, Counter> hashmap_t;
  typedef google::dense_hash_map<PartialVector*, Counter>::iterator
  hashmap_iterator_t;
#endif
  hashmap_t candidates_;
};

#endif
