#include "generator.hpp"

#include "generator_v1.hpp"
#include "generator_v2.hpp"

Generator::UPtr Generator::Factory::createGenerator( const Settings &settings, int64_t seed )
{
  Generator::UPtr generator;
  switch ( settings.generator_version )
  {
  case 1:
    generator.reset( new GeneratorV1( settings, seed ) );
    break;
  case 2:
    generator.reset( new GeneratorV2( settings, seed ) );
    break;
  default:
    Log::get().error( "Invalid generator version: " + std::to_string( settings.generator_version ), true );
    break;
  }
  return generator;
}

Generator::Generator( const Settings &settings, int64_t seed )
    :
    settings( settings )
{
  gen.seed( seed );
}

std::vector<number_t> Generator::fixCausality( Program &p )
{
  // fix causality of read operations
  std::vector<number_t> written_cells;
  written_cells.push_back( 0 );
  for ( size_t position = 0; position < p.ops.size(); position++ )
  {
    auto &op = p.ops[position];
    auto &meta = Operation::Metadata::get( op.type );

    // fix source operand in new operation
    if ( meta.num_operands == 2 && op.source.type == Operand::Type::DIRECT
        && std::find( written_cells.begin(), written_cells.end(), op.source.value ) == written_cells.end() )
    {
      op.source.value = written_cells[op.source.value % written_cells.size()];
    }

    // fix target operand in new operation
    if ( meta.num_operands > 0 && meta.is_reading_target && op.target.type == Operand::Type::DIRECT
        && std::find( written_cells.begin(), written_cells.end(), op.target.value ) == written_cells.end() )
    {
      auto new_cell = op.target.value % written_cells.size();
      if ( new_cell == op.source.value )
      {
        new_cell = written_cells.size() - new_cell - 1;
      }
      op.target.value = written_cells[new_cell];
    }

    // check if target cell not written yet
    if ( meta.is_writing_target && op.target.type == Operand::Type::DIRECT
        && std::find( written_cells.begin(), written_cells.end(), op.target.value ) == written_cells.end() )
    {
      // update written cells
      written_cells.push_back( op.target.value );
    }
  }
  return written_cells;
}

void Generator::ensureSourceNotOverwritten( Program &p )
{
  // make sure that the initial value does not get overridden immediately
  for ( auto it = p.ops.begin(); it < p.ops.end(); it++ )
  {
    if ( it->target.value == 0 )
    {
      if ( it->type == Operation::Type::MOV
          || ((it->type == Operation::Type::SUB || it->type == Operation::Type::TRN)
              && (it->source.type != Operand::Type::CONSTANT && it->source.value == 0)) )
      {
        it = p.ops.erase( it );
      }
    }
    else if ( it->source.type != Operand::Type::CONSTANT && it->source.value == 0 )
    {
      break;
    }
  }
}

void Generator::ensureTargetWritten( Program &p, const std::vector<number_t> &written_cells )
{
  // make sure that the target value gets written
  bool written = false;
  for ( auto &op : p.ops )
  {
    if ( op.type != Operation::Type::LPB && Operation::Metadata::get( op.type ).num_operands == 2
        && op.target.type == Operand::Type::DIRECT && op.target.value == 1 )
    {
      written = true;
      break;
    }
  }
  if ( !written )
  {
    number_t source = 0;
    if ( !written_cells.empty() )
    {
      source = written_cells.at( gen() % written_cells.size() );
    }
    p.ops.push_back(
        Operation( Operation::Type::MOV, Operand( Operand::Type::DIRECT, 1 ),
            Operand( Operand::Type::DIRECT, source ) ) );
  }
}

void Generator::ensureMeaningfulLoops( Program &p )
{
  // make sure loops do something
  number_t mem = 0;
  number_t num_ops = 0;
  bool can_descent = false;
  for ( size_t i = 0; i < p.ops.size(); i++ )
  {
    switch ( p.ops[i].type )
    {
    case Operation::Type::LPB:
    {
      mem = p.ops[i].target.value;
      can_descent = false;
      num_ops = 0;
      break;
    }
    case Operation::Type::ADD:
    case Operation::Type::MUL:
    case Operation::Type::POW:
    case Operation::Type::FAC:
      num_ops++;
      break;
    case Operation::Type::SUB:
    case Operation::Type::LOG:
    case Operation::Type::MOV:
    case Operation::Type::DIV:
    case Operation::Type::MOD:
    case Operation::Type::GCD:
    case Operation::Type::BIN:
    case Operation::Type::CMP:
      num_ops++;
      if ( p.ops[i].target.value == mem )
      {
        can_descent = true;
      }
      break;
    case Operation::Type::LPE:
    {
      if ( !can_descent )
      {
        Operation dec;
        dec.target = Operand( Operand::Type::DIRECT, mem );
        dec.source = Operand( Operand::Type::CONSTANT, (gen() % 4) + 1 );
        switch ( gen() % 3 )
        {
        case 0:
          dec.type = Operation::Type::SUB;
          break;
        case 1:
          dec.type = Operation::Type::DIV;
          dec.source.value++;
          break;
        case 2:
          dec.type = Operation::Type::MOD;
          dec.source.value++;
          break;
        }
        p.ops.insert( p.ops.begin() + i, dec );
        i++;
      }
      if ( num_ops < 2 )
      {
        for ( int64_t j = (gen() % 3) + 1; j > 0; j-- )
        {
          auto op = generateOperation();
          if ( op.first.type != Operation::Type::LPB && op.first.type != Operation::Type::LPE )
          {
            p.ops.insert( p.ops.begin() + i, op.first );
            i++;
          }
        }
      }
      break;
    }
    default:
      break;
    }
  }
}

void Generator::generateStateless( Program &p, size_t num_operations )
{
  // fill program with random operations
  size_t nops = 0;
  while ( p.ops.size() + nops < num_operations )
  {
    auto next_op = generateOperation();
    if ( next_op.first.type == Operation::Type::NOP || next_op.first.type == Operation::Type::LPE )
    {
      nops++;
      continue;
    }
    size_t position = (next_op.second * (p.ops.size() + 1));
    p.ops.emplace( p.ops.begin() + position, Operation( next_op.first ) );
    if ( next_op.first.type == Operation::Type::LPB )
    {
      position = ((position + p.ops.size()) / 2) + 1;
      p.ops.emplace( p.ops.begin() + position, Operation( Operation::Type::LPE ) );
    }
  }
}
