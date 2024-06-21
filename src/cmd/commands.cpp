#include "cmd/commands.hpp"

#include <fstream>
#include <iostream>

#include "cmd/benchmark.hpp"
#include "cmd/boinc.hpp"
#include "cmd/test.hpp"
#include "form/formula_gen.hpp"
#include "form/pari.hpp"
#include "lang/analyzer.hpp"
#include "lang/comments.hpp"
#include "lang/evaluator.hpp"
#include "lang/evaluator_inc.hpp"
#include "lang/minimizer.hpp"
#include "lang/optimizer.hpp"
#include "lang/program_util.hpp"
#include "mine/iterator.hpp"
#include "mine/miner.hpp"
#include "mine/mutator.hpp"
#include "oeis/oeis_manager.hpp"
#include "oeis/oeis_program.hpp"
#include "sys/log.hpp"
#include "sys/setup.hpp"
#include "sys/util.hpp"

void Commands::initLog(bool silent) {
  if (silent && Log::get().level != Log::Level::DEBUG) {
    Log::get().silent = true;
  } else {
    Log::get().silent = false;
    Log::get().info("Starting " + Version::INFO +
                    ". See https://loda-lang.org/");
  }
}

void Commands::help() {
  initLog(true);
  Settings settings;
  std::cout << "Welcome to " << Version::INFO
            << ". More information at https://loda-lang.org/" << std::endl
            << std::endl;
  std::cout << "Usage: loda <command> <options>" << std::endl << std::endl;
  std::cout << "Core Commands:" << std::endl;
  std::cout << "  evaluate  <program>  Evaluate a program to an integer "
               "sequence (see "
               "-t,-b,-s)"
            << std::endl;
  std::cout << "  export    <program>  Export a program print result (see -o)"
            << std::endl;
  std::cout << "  optimize  <program>  Optimize a program and print it"
            << std::endl;
  std::cout << "  minimize  <program>  Minimize a program and print it (see -t)"
            << std::endl;
  std::cout << "  profile   <program>  Measure program evaluation time (see -t)"
            << std::endl;
  std::cout << "  fold <program> <id>  Fold a subprogram given by ID into a "
               "seq operation"
            << std::endl;
  std::cout << "  unfold    <program>  Unfold the first seq operation of a "
               "program"
            << std::endl;

  std::cout << std::endl << "OEIS Commands:" << std::endl;
  std::cout << "  mine                 Mine programs for OEIS sequences (see "
               "-i,-p,-P,-H)"
            << std::endl;
  std::cout << "  check  <program>     Check a program for an OEIS sequence "
               "(see -b)"
            << std::endl;
  std::cout
      << "  mutate <program>     Mutate a program and mine for OEIS sequences"
      << std::endl;
  std::cout << "  submit <file> [id]   Submit a program for an OEIS sequence"
            << std::endl;

  std::cout << std::endl << "Admin Commands:" << std::endl;
  std::cout << "  setup                Run interactive setup to configure LODA"
            << std::endl;
  std::cout << "  update               Run non-interactive update of LODA and "
               "its data"
            << std::endl;

  std::cout << std::endl << "Targets:" << std::endl;
  std::cout
      << "  <file>               Path to a LODA file (file extension: *.asm)"
      << std::endl;
  std::cout << "  <id>                 ID of an OEIS integer sequence "
               "(example: A000045)"
            << std::endl;
  std::cout << "  <program>            Either an <file> or an <id>"
            << std::endl;

  std::cout << std::endl << "Options:" << std::endl;
  std::cout << "  -t <number>          Number of sequence terms (default: "
            << settings.num_terms << ")" << std::endl;
  std::cout
      << "  -b                   Print result in b-file format from offset 0"
      << std::endl;
  std::cout << "  -B <number>          Print result in b-file format from a "
               "custom offset"
            << std::endl;
  std::cout << "  -o <string>          Export format (formula,loda,pari)"
            << std::endl;
  std::cout
      << "  -d                   Export with dependencies to other programs"
      << std::endl;
  std::cout << "  -s                   Evaluate program to number of "
               "execution steps"
            << std::endl;
  std::cout << "  -c <number>          Maximum number of interpreter cycles "
               "(no limit: -1)"
            << std::endl;
  std::cout << "  -m <number>          Maximum number of used memory cells "
               "(no limit: -1)"
            << std::endl;
  std::cout << "  -z <number>          Maximum evaluation time in seconds "
               "(no limit: -1)"
            << std::endl;
  std::cout << "  -l <string>          Log level (values: "
               "debug,info,warn,error,alert)"
            << std::endl;
  std::cout
      << "  -i <string>          Name of miner configuration from miners.json"
      << std::endl;
  std::cout << "  -p                   Parallel mining using default number of "
               "instances"
            << std::endl;
  std::cout << "  -P <number>          Parallel mining using custom number of "
               "instances"
            << std::endl;
  std::cout
      << "  -H <number>          Number of mining hours (default: unlimited)"
      << std::endl;
}

// official commands

void Commands::setup() {
  initLog(true);
  Setup::runWizard();
}

void Commands::update() {
  initLog(false);
  OeisManager manager(settings);
  manager.update(true);
  manager.getStats();
  manager.generateLists();
}

void Commands::evaluate(const std::string& path) {
  initLog(true);
  Program program = OeisProgram::getProgramAndSeqId(path).first;
  Evaluator evaluator(settings);
  Sequence seq;
  evaluator.eval(program, seq);
  if (!settings.print_as_b_file) {
    std::cout << seq << std::endl;
  }
}

void Commands::check(const std::string& path) {
  initLog(true);
  auto program_and_id = OeisProgram::getProgramAndSeqId(path);
  auto program = program_and_id.first;
  OeisSequence seq(program_and_id.second);
  if (seq.id == 0) {
    auto id_str = Comments::getSequenceIdFromProgram(program);
    seq = OeisSequence(id_str);
  }
  Evaluator evaluator(settings);
  auto terms = seq.getTerms(OeisSequence::FULL_SEQ_LENGTH);
  auto num_required = OeisProgram::getNumRequiredTerms(program);
  auto result = evaluator.check(program, terms, num_required, seq.id);
  switch (result.first) {
    case status_t::OK:
      std::cout << "ok" << std::endl;
      break;
    case status_t::WARNING:
      std::cout << "warning" << std::endl;
      break;
    case status_t::ERROR:
      std::cout << "error" << std::endl;
      break;
  }
}

void Commands::optimize(const std::string& path) {
  initLog(true);
  Program program = OeisProgram::getProgramAndSeqId(path).first;
  Optimizer optimizer(settings);
  optimizer.optimize(program);
  ProgramUtil::print(program, std::cout);
}

void Commands::minimize(const std::string& path) {
  initLog(true);
  Program program = OeisProgram::getProgramAndSeqId(path).first;
  Minimizer minimizer(settings);
  minimizer.optimizeAndMinimize(program, settings.num_terms);
  ProgramUtil::print(program, std::cout);
}

void throwConversionError(const std::string& format) {
  throw std::runtime_error("program cannot be converted to " + format);
}

void Commands::export_(const std::string& path) {
  initLog(true);
  Program program = OeisProgram::getProgramAndSeqId(path).first;
  const auto& format = settings.export_format;
  Formula formula;
  PariFormula pari_formula;
  FormulaGenerator generator;
  if (format.empty() || format == "formula") {
    if (!generator.generate(program, -1, formula, settings.with_deps)) {
      throwConversionError(format);
    }
    std::cout << formula.toString() << std::endl;
  } else if (format == "pari") {
    if (!generator.generate(program, -1, formula, settings.with_deps) ||
        !PariFormula::convert(formula, false, pari_formula)) {
      throwConversionError(format);
    }
    std::cout << pari_formula.toString() << std::endl;
  } else if (format == "pari-vector") {
    if (!generator.generate(program, -1, formula, settings.with_deps) ||
        !PariFormula::convert(formula, true, pari_formula)) {
      throwConversionError(format);
    }
    std::cout << pari_formula.toString() << std::endl;
  } else if (format == "loda") {
    ProgramUtil::print(program, std::cout);
  } else {
    throw std::runtime_error("unknown format");
  }
}

void Commands::profile(const std::string& path) {
  initLog(true);
  Program program = OeisProgram::getProgramAndSeqId(path).first;
  Sequence res;
  Evaluator evaluator(settings);
  auto start_time = std::chrono::steady_clock::now();
  evaluator.eval(program, res);
  auto cur_time = std::chrono::steady_clock::now();
  auto micro_secs = std::chrono::duration_cast<std::chrono::microseconds>(
                        cur_time - start_time)
                        .count();
  std::cout.setf(std::ios::fixed);
  std::cout.precision(3);
  if (micro_secs < 1000) {
    std::cout << micro_secs << "µs" << std::endl;
  } else if (micro_secs < 1000000) {
    std::cout << static_cast<double>(micro_secs) / 1000.0 << "ms" << std::endl;
  } else {
    std::cout << static_cast<double>(micro_secs) / 1000000.0 << "s"
              << std::endl;
  }
}

void Commands::fold(const std::string& main_path, const std::string& sub_id) {
  initLog(true);
  auto main = OeisProgram::getProgramAndSeqId(main_path).first;
  auto sub = OeisProgram::getProgramAndSeqId(sub_id);
  if (sub.second == 0) {
    throw std::runtime_error("subprogram must be given by ID");
  }
  std::map<int64_t, int64_t> cell_map;
  if (!OeisProgram::fold(main, sub.first, sub.second, cell_map)) {
    throw std::runtime_error("cannot fold program");
  }
  ProgramUtil::print(main, std::cout);
}

void Commands::unfold(const std::string& path) {
  initLog(true);
  auto p = OeisProgram::getProgramAndSeqId(path).first;
  if (!OeisProgram::unfold(p)) {
    throw std::runtime_error("cannot unfold program");
  }
  ProgramUtil::print(p, std::cout);
}

void Commands::autoFold() {
  initLog(false);
  OeisManager manager(settings);
  const auto programs = manager.loadAllPrograms();
  const auto num_ids = manager.getStats().all_program_ids.size();
  Log::get().info("Folding programs");
  bool folded;
  Program main;
  std::map<int64_t, int64_t> cell_map;
  size_t main_id, sub_id, main_loops, sub_loops;
  for (main_id = 0; main_id < num_ids; main_id++) {
    if (programs[main_id].ops.empty() ||
        !OeisProgram::isTooComplex(programs[main_id])) {
      continue;
    }
    folded = false;
    main = programs[main_id];
    main_loops = ProgramUtil::numOps(programs[main_id], Operation::Type::LPB);
    for (sub_id = 0; sub_id < num_ids; sub_id++) {
      sub_loops = ProgramUtil::numOps(programs[sub_id], Operation::Type::LPB);
      if (programs[sub_id].ops.empty() || sub_id == main_id ||
          main_loops == 0 || sub_loops == 0 || main_loops == sub_loops) {
        continue;
      }
      cell_map.clear();
      if (OeisProgram::fold(main, programs[sub_id], sub_id, cell_map)) {
        folded = true;
        break;
      }
    }
    if (folded) {
      Log::get().info("Folded " + OeisSequence(main_id).id_str() + " using " +
                      OeisSequence(sub_id).id_str());
    }
  }
}

std::unique_ptr<ProgressMonitor> makeProgressMonitor(const Settings& settings) {
  std::unique_ptr<ProgressMonitor> progress_monitor;
  if (settings.num_mine_hours > 0) {
    const int64_t target_seconds = settings.num_mine_hours * 3600;
    progress_monitor.reset(new ProgressMonitor(target_seconds, "", "", 0));
  }
  return progress_monitor;
}

void Commands::mine() {
  initLog(false);
  auto progress_monitor = makeProgressMonitor(settings);
  Miner miner(settings, progress_monitor.get());
  miner.mine();
}

void Commands::mutate(const std::string& path) {
  initLog(false);
  Program base_program = OeisProgram::getProgramAndSeqId(path).first;
  auto progress_monitor = makeProgressMonitor(settings);
  Miner miner(settings, progress_monitor.get());
  miner.setBaseProgram(base_program);
  miner.mine();
}

void Commands::submit(const std::string& path, const std::string& id) {
  initLog(false);
  Miner miner(settings);
  miner.submit(path, id);
}

// hidden commands

void Commands::boinc() {
  initLog(false);
  Boinc boinc(settings);
  boinc.run();
}

void Commands::test() {
  initLog(false);
  Test test;
  test.all();
}

void Commands::testIncEval(const std::string& test_id) {
  initLog(false);
  Settings settings;
  OeisManager manager(settings);
  auto& stats = manager.getStats();
  size_t target_id = 0;
  if (!test_id.empty()) {
    target_id = OeisSequence(test_id).id;
  }
  int64_t count = 0;
  for (size_t id = 0; id < stats.all_program_ids.size(); id++) {
    if (!stats.all_program_ids[id] || (target_id > 0 && id != target_id)) {
      continue;
    }
    if (Test::checkIncEval(settings, id, false)) {
      count++;
    }
  }
  Log::get().info("Passed incremental evaluation check for " +
                  std::to_string(count) + " programs");
}

void Commands::testAnalyzer() {
  initLog(false);
  Log::get().info("Testing analyzer");
  Parser parser;
  Program program;
  OeisManager manager(settings);
  auto& stats = manager.getStats();
  int64_t log_count = 0, exp_count = 0;
  for (size_t id = 0; id < stats.all_program_ids.size(); id++) {
    if (!stats.all_program_ids[id]) {
      continue;
    }
    OeisSequence seq(id);
    auto id_str = seq.id_str();
    std::ifstream in(seq.getProgramPath());
    if (!in) {
      continue;
    }
    try {
      program = parser.parse(in);
    } catch (std::exception& e) {
      Log::get().warn("Skipping " + id_str + ": " + e.what());
      continue;
    }
    bool is_log = Analyzer::hasLogarithmicComplexity(program);
    bool is_exp = Analyzer::hasExponentialComplexity(program);
    if (is_log) {
      Log::get().info(id_str + " has logarithmic complexity");
      log_count++;
    }
    if (is_exp) {
      Log::get().info(id_str + " has exponential complexity");
      exp_count++;
    }
    if (is_log && is_exp) {
      Log::get().error(
          id_str + " has both logarithmic and exponential complexity", true);
    }
  }
  Log::get().info(std::to_string(log_count) +
                  " programs have logarithmic complexity");
  Log::get().info(std::to_string(exp_count) +
                  " programs have exponential complexity");
}

void Commands::testPari(const std::string& test_id) {
  initLog(false);
  Parser parser;
  Interpreter interpreter(settings);
  Evaluator evaluator(settings);
  IncrementalEvaluator inceval(interpreter);
  OeisManager manager(settings);
  Memory tmp_memory;
  manager.load();
  auto& stats = manager.getStats();
  int64_t good = 0, bad = 0;
  size_t target_id = 0;
  if (!test_id.empty()) {
    target_id = OeisSequence(test_id).id;
  }
  for (size_t id = 0; id < stats.all_program_ids.size(); id++) {
    if (!stats.all_program_ids[id] || (target_id > 0 && id != target_id)) {
      continue;
    }
    auto seq = manager.getSequences().at(id);
    Program program;
    try {
      program = parser.parse(seq.getProgramPath());
    } catch (std::exception& e) {
      Log::get().warn(std::string(e.what()));
      continue;
    }

    // generate PARI code
    FormulaGenerator generator;
    Formula formula;
    PariFormula pari_formula;
    const bool as_vector = false;  // TODO: switch to true
    Sequence expSeq;
    try {
      if (!generator.generate(program, id, formula, true) ||
          !PariFormula::convert(formula, as_vector, pari_formula)) {
        continue;
      }
    } catch (const std::exception& e) {
      // error during formula generation => check evaluation
      bool hasEvalError;
      try {
        evaluator.eval(program, expSeq, 10);
        hasEvalError = false;
      } catch (const std::exception&) {
        hasEvalError = true;
      }
      if (!hasEvalError) {
        Log::get().error(
            "Expected evaluation error for " + seq.id_str() + ": " + e.what(),
            true);
      }
    }

    // determine number of terms for testing
    size_t numTerms = seq.existingNumTerms();
    if (inceval.init(program)) {
      const int64_t targetTerms = 15 * inceval.getLoopCounterDecrement();
      numTerms = std::min<size_t>(numTerms, targetTerms);
      while (numTerms > 0) {
        tmp_memory.clear();
        tmp_memory.set(Program::INPUT_CELL, numTerms - 1);
        interpreter.run(inceval.getSimpleLoop().pre_loop, tmp_memory);
        int64_t tmpTerms =
            tmp_memory.get(inceval.getSimpleLoop().counter).asInt();
        if (tmpTerms <= targetTerms) {
          break;
        }
        numTerms--;
      }
    }
    for (const auto& op : program.ops) {
      if (op.type == Operation::Type::SEQ) {
        numTerms = std::min<size_t>(numTerms, 5);
      }
      if ((op.type == Operation::Type::POW ||
           op.type == Operation::Type::BIN) &&
          op.source.type == Operand::Type::DIRECT) {
        numTerms = std::min<size_t>(numTerms, 5);
      }
    }
    Log::get().info("Checking " + std::to_string(numTerms) + " terms of " +
                    seq.id_str() + ": " + pari_formula.toString());

    if (numTerms == 0) {
      Log::get().warn("Skipping " + seq.id_str());
      continue;
    }

    // evaluate LODA program
    try {
      evaluator.eval(program, expSeq, numTerms);
    } catch (const std::exception&) {
      Log::get().warn("Cannot evaluate " + seq.id_str());
      continue;
    }
    if (expSeq.empty()) {
      Log::get().error("Evaluation error", true);
    }

    // evaluate PARI program
    auto genSeq = pari_formula.eval(numTerms);

    // compare results
    if (genSeq != expSeq) {
      Log::get().info("Generated sequence: " + genSeq.to_string());
      Log::get().info("Expected sequence:  " + expSeq.to_string());
      Log::get().error("Unexpected PARI sequence", true);
      bad++;
    } else {
      good++;
    }
  }
  Log::get().info(std::to_string(good) + " passed, " + std::to_string(bad) +
                  " failed PARI check");
}

void Commands::dot(const std::string& path) {
  initLog(true);
  Program program = OeisProgram::getProgramAndSeqId(path).first;
  ProgramUtil::exportToDot(program, std::cout);
}

void Commands::generate() {
  initLog(true);
  OeisManager manager(settings);
  MultiGenerator multi_generator(settings, manager.getStats(), false);
  auto program = multi_generator.generateProgram();
  ProgramUtil::print(program, std::cout);
}

void Commands::migrate() {
  initLog(false);
  OeisManager manager(settings);
  manager.migrate();
}

void Commands::maintain(const std::string& id) {
  initLog(false);
  OeisManager manager(settings);
  manager.load();
  size_t start = 0;
  size_t end = manager.getTotalCount() + 1;
  bool check = false;
  if (!id.empty()) {
    OeisSequence seq(id);
    start = seq.id;
    end = seq.id + 1;
    check = true;
  }
  for (size_t id = start; id < end; id++) {
    manager.maintainProgram(id, check);
  }
}

void Commands::iterate(const std::string& count) {
  initLog(true);
  int64_t c = stoll(count);
  Iterator it;
  Program p;
  while (c-- > 0) {
    p = it.next();
    //        std::cout << "\x1B[2J\x1B[H";
    ProgramUtil::print(p, std::cout);
    std::cout << std::endl;
    //        std::cin.ignore();
  }
}

void Commands::benchmark() {
  initLog(true);
  Benchmark benchmark;
  benchmark.smokeTest();
}

void Commands::findSlow(int64_t num_terms, const std::string& type) {
  initLog(false);
  auto t = Operation::Type::NOP;
  if (!type.empty()) {
    t = Operation::Metadata::get(type).type;
  }
  Benchmark benchmark;
  benchmark.findSlow(num_terms, t);
}

void Commands::lists() {
  initLog(false);
  OeisManager manager(settings);
  manager.load();
  manager.generateLists();
}

void Commands::compare(const std::string& path1, const std::string& path2) {
  initLog(true);
  Program p1 = OeisProgram::getProgramAndSeqId(path1).first;
  Program p2 = OeisProgram::getProgramAndSeqId(path2).first;
  auto id_str = Comments::getSequenceIdFromProgram(p1);
  OeisSequence seq(id_str);
  OeisManager manager(settings);
  manager.load();
  size_t num_usages = 0;
  if (seq.id < manager.getStats().program_usages.size()) {
    num_usages = manager.getStats().program_usages[seq.id];
  }
  auto result = manager.getFinder().isOptimizedBetter(
      p1, p2, seq, OeisSequence::EXTENDED_SEQ_LENGTH, num_usages);
  if (result.empty()) {
    result = "Worse or Equal";
  }
  std::cout << result << std::endl;
}
