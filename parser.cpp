#include "parser.h"

namespace Http
{

Parser::Parser(const char *buffer, size_t size) :
  m_index(),
  m_buffer(buffer),
  m_buffer_size(size),
  m_state(State::METHOD)
{
  m_headers = std::make_shared<Headers>();
}

Parser::State Parser::parse()
{
  DEBUG("%s", m_buffer);
  
  while (m_index < m_buffer_size)
  {
    switch (m_state)
    {
      case State::METHOD:  parse_method();  break;
      case State::PATH:    parse_path();    break;
      case State::VERSION: parse_version(); break;
      case State::FIELD:   parse_field();   break;
      default: 
        DEBUG("parser stopped");
        return m_state;
        break;
    }
  }

  return m_state;
}

void Parser::parse_method()
{
  switch (curr())
  {
    case 'G': m_headers->set_method(Headers::Method::GET); m_index+=4; break;
    case 'H': m_headers->set_method(Headers::Method::HEAD); m_index+=5; break;
    case 'P': 
      switch (next())
      {
        case 'O': m_headers->set_method(Headers::Method::POST); m_index+=5; break;
        case 'U': m_headers->set_method(Headers::Method::PUT); m_index+=4; break;
        case 'A': m_headers->set_method(Headers::Method::PATCH); m_index+=6; break;
      }
      break;
    case 'D': m_headers->set_method(Headers::Method::DELETE); m_index+=7; break;
    case 'T': m_headers->set_method(Headers::Method::TRACE); m_index+=6; break;
    case 'O': m_headers->set_method(Headers::Method::OPTIONS); m_index+=8; break;
    case 'C': m_headers->set_method(Headers::Method::CONNECT); m_index+=8; break;
  }

  if (m_headers->get_method() == Headers::Method::NONE)
  {
    ERR("bad http method: %c", curr());
    m_state = State::BROKEN;
  }
  else
  {
    DEBUG("http method: %d", (int) m_headers->get_method());
    m_state = State::PATH;
  }
}

void Parser::parse_path()
{
  const char *c = m_buffer + m_index;
  const char *newline = strchr(c, '\n');

  if (newline == NULL)
  {
    m_state = State::BROKEN;
    return;
  }

  m_index = (newline - m_buffer) + 1;
  m_state = State::FIELD;
}

void Parser::parse_version()
{
  m_index++;
}

/**
 * Parse the remaining header fields
 *
 * 1. Find the first delimter (: for now)
 * 2. Mark the end of the field name
 * 3. Discard any whitespace
 * 4. Find the newline/end of field
 * 5. Mark the end of the value
 * 
 * BROKEN on null terminator
 */
void Parser::parse_field()
{
  const char *curr = m_buffer + m_index;
  const char *delim = strchr(curr, ':');

  if (delim == NULL)
  {
    m_state = State::BROKEN;
    return;
  }

  std::string field(curr, delim - curr);

  curr = delim + 1;

  while (*curr == ' ')
  {
    curr++;
  }

  if (*curr == '\0')
  {
    m_state = State::BROKEN;
    return;
  }

  char *newline = strchr(curr, '\n');

  if (newline == NULL)
  {
    m_state = State::BROKEN;
    return;
  }

  int cr_offset = *(newline - 1) == '\r' ? 1 : 0;
  std::string value(curr, newline - curr - cr_offset);

  DEBUG("field: %s", field.c_str());
  DEBUG("value: %s", value.c_str());

  m_headers->set_field(field, value);

  curr = newline + 1;
  m_index = curr - m_buffer;
}

inline const char & Parser::curr()
{
  return m_buffer[m_index];
}

inline const char & Parser::next()
{
  if (m_index+1 < m_buffer_size)
  {
    return m_buffer[m_index+1];
  }
  
  return m_buffer[m_buffer_size];
}

inline const char & Parser::prev()
{
  return m_buffer[m_index-1];
}

} // namespace
