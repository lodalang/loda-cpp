#include "parser.hpp"

#include "common.hpp"
#include "program.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>

Program Parser::Parse( const std::string& file )
{
  file_in.reset( new std::ifstream( file ) );
  if ( !file_in )
  {
    throw std::runtime_error( "error opening file" );
  }
  auto p = Parse( *file_in );
  file_in->close();
  return p;
}

Program Parser::Parse( std::istream& in_ )
{
  in = &in_;
  Program p;
  Operation o;
  std::string l;
  while ( true )
  {
    *in >> std::ws;
    auto c = in->peek();
    if ( c == EOF )
    {
      break;
    }
    o = Operation();
    if ( c != ';' )
    {
      // read normal operation
      o.type = ReadOperationType();
      *in >> std::ws;
      if ( o.type == Operation::Type::MOV || o.type == Operation::Type::ADD || o.type == Operation::Type::SUB
          || o.type == Operation::Type::LPB )
      {
        o.target = ReadOperand( p );
        ReadSeparator( ',' );
        o.source = ReadOperand( p );
      }
    }

    // read comment
    c = in->peek();
    while ( c == ' ' || c == '\t' )
    {
      in->get();
      c = in->peek();
    }
    if ( c == ';' )
    {
      in->get();
      c = in->peek();
      while ( c == ' ' || c == '\t' )
      {
        in->get();
        c = in->peek();
      }
      std::getline( *in, l );
      o.comment = l;
    }

    // add operation to program
    if ( o.type != Operation::Type::NOP || !o.comment.empty() )
    {
      p.ops.push_back( o );
    }
  }
  return p;
}

void Parser::ReadSeparator( char separator )
{
  *in >> std::ws;
  if ( in->get() != separator )
  {
    throw std::runtime_error( "expected separator" );
  }
}

number_t Parser::ReadValue()
{
  number_t v;
  int c;
  *in >> std::ws;
  c = in->peek();
  if ( !std::isdigit( c ) )
  {
    throw std::runtime_error( "invalid value" );
  }
  *in >> v;
  return v;
}

std::string Parser::ReadIdentifier()
{
  std::string s;
  int c;
  *in >> std::ws;
  c = in->get();
  if ( c == '_' || std::isalpha( c ) )
  {
    s += (char) c;
    while ( true )
    {
      c = in->peek();
      if ( c == '_' || std::isalnum( c ) )
      {
        s += (char) c;
        in->get();
      }
      else
      {
        break;
      }
    }
    std::transform( s.begin(), s.end(), s.begin(), ::tolower );
    return s;
  }
  else
  {
    throw std::runtime_error( "invalid identifier" );
  }
}

Operand Parser::ReadOperand( Program& p )
{
  *in >> std::ws;
  int c = in->peek();
  if ( c == '$' )
  {
    in->get();
    c = in->peek();
    if ( c == '$' )
    {
      in->get();
      return Operand( Operand::Type::MEM_ACCESS_INDIRECT, ReadValue() );
    }
    else
    {
      return Operand( Operand::Type::MEM_ACCESS_DIRECT, ReadValue() );
    }

  }
  else
  {
    return Operand( Operand::Type::CONSTANT, ReadValue() );
  }
}

Operation::Type Parser::ReadOperationType()
{
  auto t = ReadIdentifier();
  if ( t == "nop" )
  {
    return Operation::Type::NOP;
  }
  else if ( t == "mov" )
  {
    return Operation::Type::MOV;
  }
  else if ( t == "add" )
  {
    return Operation::Type::ADD;
  }
  else if ( t == "sub" )
  {
    return Operation::Type::SUB;
  }
  else if ( t == "lpb" )
  {
    return Operation::Type::LPB;
  }
  else if ( t == "lpe" )
  {
    return Operation::Type::LPE;
  }
  else if ( t == "dbg" )
  {
    return Operation::Type::DBG;
  }
  else if ( t == "end" )
  {
    return Operation::Type::END;
  }
  throw std::runtime_error( "invalid operation: " + t );
}

void Parser::SetWorkingDir( const std::string& dir )
{
  working_dir = dir;
}
