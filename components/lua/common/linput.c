#include "platform.h"
#include "linput.h"
#include "lua.h"
#include "lauxlib.h"
#include <stdio.h>

static struct input_state {
  char       *data;
  int         line_pos;
  size_t      len;
  const char *prompt;
  char last_nl_char;
} ins = {0};

#define NUL '\0'
#define BS  '\010'
#define CR  '\r'
#define LF  '\n'
#define DEL  0x7f
#define BS_OVER "\010 \010"

bool input_echo = true;
bool run_input = true;

/*
** The input state (ins) is private, so input_setup() exposes the necessary
** access to public properties and is called in user_init() before the Lua
** enviroment is initialised.
*/
void input_setup(int bufsize, const char *prompt) {
  // Initialise non-zero elements
  ins.data      = malloc(bufsize);
  ins.len       = bufsize;
  ins.prompt    = prompt;
}

void input_setprompt (const char *prompt) {
  ins.prompt = prompt;
}


size_t feed_lua_input(const char *buf, size_t n)
{
  for (size_t i = 0; i < n; ++i) {
    char ch = buf[i];
    if (!run_input) // We're no longer interested in the remaining bytes
      return i;

    /* handle CR & LF characters and aggregate \n\r and \r\n pairs */
    char tmp_last_nl_char = ins.last_nl_char;
    /* handle CR & LF characters
       filters second char of LF&CR (\n\r) or CR&LF (\r\n) sequences */
    if ((ch == CR && tmp_last_nl_char == LF) || // \n\r sequence -> skip \r
        (ch == LF && tmp_last_nl_char == CR))   // \r\n sequence -> skip \n
    {
      continue;
    }

    /* backspace key */
    if (ch == DEL || ch == BS) {
      if (ins.line_pos > 0) {
        if(input_echo) printf(BS_OVER);
        ins.line_pos--;
      }
      ins.data[ins.line_pos] = 0;
      continue;
    }

    /* end of data */
    if (ch == CR || ch == LF) {
      ins.last_nl_char = ch;

      if (input_echo) putchar(LF);
      if (ins.line_pos == 0) {
        /* Get a empty line, then go to get a new line */
        printf(ins.prompt);
        fflush(stdout);
      } else {
        ins.data[ins.line_pos++] = LF;
        lua_input_string(ins.data, ins.line_pos);
        ins.line_pos = 0;
      }
      continue;
    }
    else
      ins.last_nl_char = NUL;

    if(input_echo) putchar(ch);

    /* it's a large line, discard it */
    if ( ins.line_pos + 1 >= ins.len ){
      ins.line_pos = 0;
    }

    ins.data[ins.line_pos++] = ch;
  }

  return n; // we consumed/buffered all the provided data
}