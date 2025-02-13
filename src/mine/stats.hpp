#pragma once

#include <map>
#include <set>
#include <unordered_set>

#include "eval/evaluator.hpp"
#include "mine/blocks.hpp"

class OpPos {
 public:
  Operation op;
  size_t pos;
  size_t len;

  inline bool operator==(const OpPos &o) const {
    return op == o.op && pos == o.pos && len == o.len;
  }

  inline bool operator!=(const OpPos &o) const { return !((*this) == o); }

  inline bool operator<(const OpPos &o) const {
    if (pos != o.pos) return pos < o.pos;
    if (len != o.len) return len < o.len;
    if (op != o.op) return op < o.op;
    return false;  // equal
  }
};

class Stats {
 public:
  static const std::string CALL_GRAPH_HEADER;
  static const std::string PROGRAMS_HEADER;
  static const std::string STEPS_HEADER;
  static const std::string SUMMARY_HEADER;

  Stats();

  void load(std::string path);

  void save(std::string path);

  std::string getMainStatsFile(std::string path) const;

  void updateProgramStats(size_t id, const Program &program);

  void updateSequenceStats(size_t id, bool program_found, bool formula_found);

  void finalize();

  int64_t getTransitiveLength(size_t id) const;

  int64_t num_programs;
  int64_t num_sequences;
  int64_t num_formulas;
  steps_t steps;
  std::map<Number, int64_t> num_constants;
  std::map<Operation, int64_t> num_operations;
  std::map<OpPos, int64_t> num_operation_positions;
  std::multimap<int64_t, int64_t> call_graph;
  std::vector<int64_t> num_programs_per_length;
  std::vector<int64_t> num_ops_per_type;
  std::vector<int64_t> program_lengths;
  std::vector<int64_t> program_usages;
  std::vector<bool> all_program_ids;
  std::vector<bool> latest_program_ids;
  std::vector<bool> supports_inceval;
  std::vector<bool> supports_logeval;
  Blocks blocks;

 private:
  mutable std::set<size_t> visited_programs;  // used for getTransitiveLength()
  mutable std::set<size_t>
      printed_recursion_warning;  // used for getTransitiveLength()
  Blocks::Collector blocks_collector;

  void resizeProgramLists(size_t id);
};

class RandomProgramIds {
 public:
  explicit RandomProgramIds(const std::vector<bool> &flags);

  bool empty() const;
  bool exists(int64_t id) const;
  int64_t get() const;

 private:
  std::vector<int64_t> ids_vector;
  std::unordered_set<int64_t> ids_set;
};

class RandomProgramIds2 {
 public:
  explicit RandomProgramIds2(const Stats &stats);

  bool exists(int64_t id) const;
  int64_t get() const;
  int64_t getFromAll() const;

 private:
  RandomProgramIds all_program_ids;
  RandomProgramIds latest_program_ids;
};
