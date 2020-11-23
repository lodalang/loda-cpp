#pragma once

#include "number.hpp"
#include "program.hpp"
#include "util.hpp"

class Generator
{
public:

  using UPtr = std::unique_ptr<Generator>;

  class Factory
  {
  public:
    static Generator::UPtr createGenerator( const Settings &settings, int64_t seed );
  };

  class Config
  {
  public:
    int64_t generator_version;
    size_t num_operations;
    size_t max_constant;
    size_t max_index;
    std::string operation_types;
    std::string operand_types;
    std::string program_template;

    static std::vector<Config> load( std::istream &in );

  };

  Generator( const Settings &settings, int64_t seed );

  virtual ~Generator()
  {
  }

  virtual Program generateProgram() = 0;

protected:

  virtual std::pair<Operation, double> generateOperation() = 0;

  void generateStateless( Program &p, size_t num_operations );

  void applyPostprocessing( Program &p );

  std::vector<number_t> fixCausality( Program &p );

  void ensureSourceNotOverwritten( Program &p );

  void ensureTargetWritten( Program &p, const std::vector<number_t> &written_cells );

  void ensureMeaningfulLoops( Program &p );

  const Settings &settings;
  std::mt19937 gen;

};
