#include "miner.hpp"

#include "generator.hpp"
#include "interpreter.hpp"
#include "oeis.hpp"
#include "optimizer.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "synthesizer.hpp"

#include <chrono>
#include <fstream>
#include <random>
#include <sstream>
#include <unordered_set>

Miner::Miner( const Settings &settings )
    :
    settings( settings ),
    oeis( settings ),
    interpreter( settings )
{
  oeis.load();
}

bool Miner::updateCollatz( const Program &p, const Sequence &seq ) const
{
  if ( isCollatzValuation( seq ) )
  {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch() );
    std::string file_name = "programs/oeis/collatz_" + std::to_string( ms.count() % 1000000 ) + ".asm";
    std::ofstream out( file_name );
    out << "; " << seq << std::endl;
    out << std::endl;
    Printer r;
    r.print( p, out );
    out.close();
    Log::get().alert( "Found possible Collatz valuation: " + seq.to_string() );
    return true;
  }
  return false;
}

bool Miner::isCollatzValuation( const Sequence &seq )
{
  if ( seq.size() < 10 )
  {
    return false;
  }
  for ( size_t i = 3; i < seq.size(); i++ )
  {
    if ( i % 2 == 0 ) // even
    {
      if ( seq[i / 2] >= seq[i] )
      {
        return false;
      }
    }
    else // odd
    {
      size_t j = ((3 * i) + 1) / 2;
      if ( j < seq.size() && seq[j] >= seq[i] )
      {
        return false;
      }
    }
  }
  return true;
}

void Miner::mine( volatile sig_atomic_t &exit_flag )
{
  Log::get().info( "Mining programs for OEIS sequences" );

  // metrics labels
  std::map<std::string, std::string> labels = { { "num_operations", std::to_string( settings.num_operations ) }, {
      "max_constant", std::to_string( settings.max_constant ) }, { "max_index", std::to_string( settings.max_index ) },
      { "operation_types", settings.operation_types }, { "operand_types", settings.operand_types }, };
  if ( !settings.program_template.empty() )
  {
    labels.emplace( "program_template", settings.program_template );
  }

  Generator generator( settings, std::random_device()() );
  Sequence norm_seq;
  size_t generated = 0;
  size_t fresh = 0;
  size_t updated = 0;
  auto time = std::chrono::steady_clock::now();

  while ( !exit_flag )
  {
    auto program = generator.generateProgram();
    auto seq_programs = oeis.findSequence( program, norm_seq );
    for ( auto s : seq_programs )
    {
      auto r = oeis.updateProgram( s.first, s.second );
      if ( r.first )
      {
        if ( r.second )
        {
          fresh++;
        }
        else
        {
          updated++;
        }
      }
    }
    if ( updateCollatz( program, norm_seq ) )
    {
      fresh++;
    }
    generated++;
    auto time2 = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>( time2 - time );
    if ( duration.count() >= 60 )
    {
      time = time2;
      Log::get().info( "Generated " + std::to_string( generated ) + " programs" );
      Metrics::get().write( "generated", labels, generated );
      Metrics::get().write( "fresh", labels, fresh );
      Metrics::get().write( "updated", labels, updated );
      generated = 0;
      fresh = 0;
      updated = 0;
    }
  }
}

void Miner::synthesize( volatile sig_atomic_t &exit_flag )
{
  Log::get().info( "Start synthesizing programs for OEIS sequences" );
  bool tweet_alerts = Log::get().tweet_alerts;
  Log::get().tweet_alerts = false;
  std::vector<std::unique_ptr<Synthesizer>> synthesizers;
  synthesizers.resize( 2 );
  synthesizers[0].reset( new LinearSynthesizer() );
  synthesizers[1].reset( new PeriodicSynthesizer() );
  Program program;
  size_t found = 0;
  for ( auto &synthesizer : synthesizers )
  {
    for ( auto &seq : oeis.getSequences() )
    {
      if ( exit_flag )
      {
        break;
      }
      if ( seq.empty() )
      {
        continue;
      }
      if ( synthesizer->synthesize( seq.full, program ) )
      {
        Log::get().debug( "Synthesized program for " + seq.to_string() );
        auto r = oeis.updateProgram( seq.id, program );
        if ( r.first )
        {
          found++;
        }
      }
    }
  }
  Log::get().tweet_alerts = tweet_alerts;
  if ( found > 0 )
  {
    Log::get().alert( "Synthesized " + std::to_string( found ) + " new or shorter programs" );
  }
  else
  {
    Log::get().info( "Finished synthesis without new results" );
  }
}
