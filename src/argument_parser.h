#ifndef ARGUMENT_PARSER_H
#define ARGUMENT_PARSER_H

#include <iostream>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <stdio.h>
#include <vector>

class parser {
/*
 * This parser will take the argument of a command line program and
 * will parse it inside a string vector. If a argument value is present
 * and of the form argument=value, it will populate the argument and
 * the value. The value is stored inside another vector.
 * By defaults, the all argument and values are strings.
 */
public:
    parser(int argc, char *argv[]);
    std::string name_of_program;
    void print(void);
    bool isarg(std::string arg);
    std::string get(std::string arg);
    void helper();//print the help
private:
    int nb_of_args;
    std::vector<std::string> args;
    std::vector<std::string> arg_vals;
    void split(std::string txt,std::string *arg,std::string *val);
    bool value(std::string arg);
};
#endif // ARGUMENT_PARSER_H
