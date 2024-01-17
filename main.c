/*main.c*/

//
// Project 02: main program for execution of nuPython.
// 
// Solution by Prof. Joe Hummel
// Northwestern University
// CS 211
//

// to eliminate warnings about stdlib in Visual Studio
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>  // true, false
#include <string.h>   // strcspn
#include <assert.h>

#include "token.h"    // token defs
#include "scanner.h"  // scanner
#include "util.h"     // panic
#include "tokenqueue.h"  // program in token form
#include "parser.h"      // parser checks program syntax
#include "ast.h"         // builds AST representation
#include "ram.h"         // memory for program execution


//
// all_zeros
//
// Returns true if the given string contains all 0 digits,
// false otherwise.
//
bool all_zeros(char* s)    //helper #  1
{
  for (int i = 0; i < strlen(s); i++)
    if (s[i] != '0')
      return false;

  return true;
}

//
// execute_assignment_function_call
//
// Executes assignment statements that contain a 
// function call, returning true if execution is
// sucessful and false if not.
//
// Example: y = input("Enter an int>")
//          y = int(y)
//
static bool execute_assignment_function_call(struct AST_STMT* stmt, struct RAM* memory) //helper #2
{
  assert(stmt->stmt_type == AST_ASSIGNMENT);

  struct AST_STMT_ASSIGNMENT* assignment = stmt->types.assignment;
  char* variable_name = assignment->variable_name;

  assert(assignment->expr->expr_type == AST_FUNCTION_CALL_EXPR);

  char* function_name = assignment->expr->types.function_call->function_name;
  struct AST_EXPR* parameter = assignment->expr->types.function_call->param;

  //
  // functions have exactly one parameter that's a simple <element>
  // in the BNF, e.g. identifier or literal.
  //
  assert(parameter->expr_type == AST_UNARY_EXPR);

  struct AST_UNARY_EXPR* unary_expr = parameter->types.unary_expr;

  if (strcmp(function_name, "input") == 0)
  {
    //
    // x = input("...")
    //
    // 1. prompt the user:
    //
    if (unary_expr->unary_expr_type == AST_UNARY_EXPR_STR_LITERAL)
      printf("%s", unary_expr->types.literal_value);
    else
      panic("unsupported input() parameter unary expr type (execute_assignment_function_call)");

    //
    // 2. input from keyboard and assign to variable:
    //
    char line[256];
    fgets(line, sizeof(line), stdin);
    line[strcspn(line, "\r\n")] = '\0';  // delete EOL chars

    ram_write_str_by_id(memory, variable_name, line);
  }
  else if (strcmp(function_name, "int") == 0)
  {
    //
    // x = int(rhs)
    //
    if (unary_expr->unary_expr_type == AST_UNARY_EXPR_VARIABLE_ACCESS)
    {
      //
      // rhs => "right-hand-side", i.e. the variable on the rhs of =
      //
      char* rhs_var_name = parameter->types.unary_expr->types.variable_name;

      struct RAM_CELL* rhs_cell = ram_get_cell_by_id(memory, rhs_var_name);

      if (rhs_cell == NULL) {
        // no such variable:
        printf("ERROR: name '%s' is not defined\n", rhs_var_name);
        return false;
      }

      if (rhs_cell->ram_cell_type != RAM_TYPE_STR)
        panic("int() requires a string value (execute_assignment)");

      int value = atoi(rhs_cell->types.s);
      if (value == 0) {

        if (all_zeros(rhs_cell->types.s)) {
          // okay
        }
        else {
          printf("ERROR: invalid numeric string for int()\n");
          return false;
        }
      }

      // 
      // 3. write to memory
      //
      ram_write_int_by_id(memory, variable_name, value);
    }
      else if (unary_expr->unary_expr_type == AST_UNARY_EXPR_STR_LITERAL)
      {
          // Store the string literal in the memory.
          ram_write_str_by_id(memory, variable_name, unary_expr->types.literal_value);
      }

    else {
      panic("unsupported int(unary_expr_type) (execute_assignment_function_call)");
    }
  }

    ///###1
  else {
    panic("function not supported within assignment (execute_assignment_function_call)");
  }

  //
  // done, success:
  //
  return true;
}

//

//
// execute_assignment
//
// Executes assignment statements, returning true 
// if execution is sucessful and false if not.
// 
// Example: x = 123
//          y = input("Enter an int>")
//          y = int(y)
//          x = x + 1
//          z = y + 12
////helper #3
// Helper function that handles assignments involving unary expressions.
// It writes the value of the unary expression to the RAM (memory) with the given variable name.
// Returns true if the assignment is successful, false otherwise.
static bool handle_unary_assignment(struct AST_UNARY_EXPR* unary_expr, struct RAM* memory, char* variable_name) {
  
  // Handle integer literal assignments.

    if (unary_expr->unary_expr_type == AST_UNARY_EXPR_INT_LITERAL) {
        int value = atoi(unary_expr->types.literal_value);
        ram_write_int_by_id(memory, variable_name, value);
    }
    
    // Handle integer literal assignments.

    else if (unary_expr->unary_expr_type == AST_UNARY_EXPR_STR_LITERAL) {
        char* value = unary_expr->types.literal_value;
        ram_write_str_by_id(memory, variable_name, value);
    } else if (unary_expr->unary_expr_type == AST_UNARY_EXPR_REAL_LITERAL) {
        double value = atof(unary_expr->types.literal_value);
        ram_write_real_by_id(memory, variable_name, value);
    } else if (unary_expr->unary_expr_type == AST_UNARY_EXPR_VARIABLE_ACCESS) {
        char* var2 = unary_expr->types.variable_name;
        struct RAM_CELL* source_cell = ram_get_cell_by_id(memory, var2);
        if (source_cell == NULL) {
            printf("ERROR: name '%s' is not defined\n", var2);
            return false;
        }
      // Assign the accessed variable's value based on its type.

        if (source_cell->ram_cell_type == RAM_TYPE_INT) {
            ram_write_int_by_id(memory, variable_name, source_cell->types.i);
        } else if (source_cell->ram_cell_type == RAM_TYPE_REAL) {
            ram_write_real_by_id(memory, variable_name, source_cell->types.d);
        } else if (source_cell->ram_cell_type == RAM_TYPE_STR) {
            ram_write_str_by_id(memory, variable_name, source_cell->types.s);
        } else {
            return false;
        }
    } else {
        panic("assignment unary_expr_type not supported (handle_unary_assignment)");
        return false;
    }
  // If everything goes well, return true.

    return true;
}
//helper #4
// Helper function that handles assignments involving binary expressions.
// It computes the result of the binary expression and writes the value to the RAM (memory) with the given variable name.
// Returns true if the assignment is successful, false otherwise.
static bool handle_binary_assignment(struct AST_BINARY_EXPR* binary_expr, struct RAM* memory, char* variable_name) {
  // Extracting the left-hand side (LHS) and right-hand side (RHS) from the binary expression.

    struct AST_UNARY_EXPR* lhs_expr = binary_expr->lhs;
    struct AST_UNARY_EXPR* rhs_expr = binary_expr->rhs;

  if (binary_expr->op == AST_BINARY_EXPR_PLUS) {
      double lhs_value = 0.0, rhs_value = 0.0;
      bool is_lhs_real = false, is_rhs_real = false;
      char* lhs_str = NULL; // Placeholder for LHS string if present.
      char* rhs_str = NULL; // Placeholder for RHS string if present.

      // Determine LHS value and type:
      if (lhs_expr->unary_expr_type == AST_UNARY_EXPR_VARIABLE_ACCESS) {
          struct RAM_CELL* lhs_cell = ram_get_cell_by_id(memory, lhs_expr->types.variable_name);
          if (!lhs_cell) {
              printf("ERROR: name '%s' is not defined\n", lhs_expr->types.variable_name);
              return false;
          }
        // Switching based on the type of the LHS value.

          switch(lhs_cell->ram_cell_type) {
              case RAM_TYPE_INT:
                  lhs_value = lhs_cell->types.i;
                  break;
              case RAM_TYPE_REAL:
                  lhs_value = lhs_cell->types.d;
                  is_lhs_real = true;
                  break;
              case RAM_TYPE_STR:
                  lhs_str = lhs_cell->types.s;
                  break;
              default:
                  panic("Unsupported LHS type for addition.");
          }
      }
      else if (lhs_expr->unary_expr_type == AST_UNARY_EXPR_INT_LITERAL) {
          lhs_value = atoi(lhs_expr->types.literal_value);
      }
      else if (lhs_expr->unary_expr_type == AST_UNARY_EXPR_REAL_LITERAL) {
          lhs_value = atof(lhs_expr->types.literal_value);
          is_lhs_real = true;
      }
      else if (lhs_expr->unary_expr_type == AST_UNARY_EXPR_STR_LITERAL) {
          lhs_str = lhs_expr->types.literal_value;
      }

      // Determine RHS value and type:
      if (rhs_expr->unary_expr_type == AST_UNARY_EXPR_VARIABLE_ACCESS) {
          struct RAM_CELL* rhs_cell = ram_get_cell_by_id(memory, rhs_expr->types.variable_name);
          if (!rhs_cell) {
              printf("ERROR: name '%s' is not defined\n", rhs_expr->types.variable_name);
              return false;
          }

          switch(rhs_cell->ram_cell_type) {
              case RAM_TYPE_INT:
                  rhs_value = rhs_cell->types.i;
                  break;
              case RAM_TYPE_REAL:
                  rhs_value = rhs_cell->types.d;
                  is_rhs_real = true;
                  break;
              case RAM_TYPE_STR:
                  rhs_str = rhs_cell->types.s;
                  break;
              default:
                  panic("Unsupported RHS type for addition.");
          }
      }
      else if (rhs_expr->unary_expr_type == AST_UNARY_EXPR_INT_LITERAL) {
          rhs_value = atoi(rhs_expr->types.literal_value);
      }
      else if (rhs_expr->unary_expr_type == AST_UNARY_EXPR_REAL_LITERAL) {
          rhs_value = atof(rhs_expr->types.literal_value);
          is_rhs_real = true;
      }
      else if (rhs_expr->unary_expr_type == AST_UNARY_EXPR_STR_LITERAL) {
          rhs_str = rhs_expr->types.literal_value;
      }

      // Now, based on the types, execute the operation:
      if (lhs_str || rhs_str) {
          if (lhs_str && rhs_str) {
              char* result = malloc(strlen(lhs_str) + strlen(rhs_str) + 1);
              strcpy(result, lhs_str);
              strcat(result, rhs_str);
              ram_write_str_by_id(memory, variable_name, result);
              free(result);
          }
          else {
              printf("ERROR: unsupported operand type(s) for +\n");
              return false;
          }
      }         // Once LHS and RHS values are determined, execute the binary operation based on the type of values.

      else if (is_lhs_real || is_rhs_real) {
          double result = lhs_value + rhs_value;
          ram_write_real_by_id(memory, variable_name, result);
      }
      else {
          int result = (int)lhs_value + (int)rhs_value;
          ram_write_int_by_id(memory, variable_name, result);
      }
  }
 else {
        panic("unsupported binary expression (handle_binary_assignment)");
        return false;
    }
  // If everything goes well, return true.

    return true;
}

//helper #5
static bool handle_function_call_assignment(struct AST_STMT* stmt, struct RAM* memory) {
    bool success = execute_assignment_function_call(stmt, memory);
    if (!success) return false;
    return true;
}

// This function is responsible for executing an assignment statement.
// It reads the type of expression on the right-hand side of the assignment
// and dispatches the handling to the appropriate helper function.
// Returns true if the assignment is successful, false otherwise.
//helper #6
static bool execute_assignment(struct AST_STMT* stmt, struct RAM* memory) {
  // Ensure the provided statement is of type 'assignment'.
    assert(stmt->stmt_type == AST_ASSIGNMENT);
    struct AST_STMT_ASSIGNMENT* assignment = stmt->types.assignment;
    char* variable_name = assignment->variable_name;


  // If the right-hand side of the assignment is a unary expression,
  // delegate the execution to the `handle_unary_assignment` function.
    if (assignment->expr->expr_type == AST_UNARY_EXPR) {
        return handle_unary_assignment(assignment->expr->types.unary_expr, memory, variable_name);
    } else if (assignment->expr->expr_type == AST_BINARY_EXPR) {
        return handle_binary_assignment(assignment->expr->types.binary_expr, memory, variable_name);
    } 
      
      // If the right-hand side of the assignment is a function call expression,
      // delegate the execution to the `handle_function_call_assignment` function.
    else if (assignment->expr->expr_type == AST_FUNCTION_CALL_EXPR) {
        return handle_function_call_assignment(stmt, memory);
    } else {
      // If the type of the right-hand side expression isn't recognized,
        // raise an error.
        panic("assignment expr_type not supported (execute_assignment)");
        return false;
    }
    return true;
}


//
// execute_function_call
//
// Executes the stmt denoting a function call, such as print(...).
// Returns true if successful, false if not.
// 
// Example: print("a string")
//          print(123)
//          print(x)
////helper #7
static bool execute_function_call(struct AST_STMT* stmt, struct RAM* memory)
{
  assert(stmt->stmt_type == AST_FUNCTION_CALL);

  char* function_name = stmt->types.function_call->function_name;
  struct AST_EXPR* parameter = stmt->types.function_call->param;

  //
  // functions have exactly one parameter that's a simple <element>
  // in the BNF, e.g. identifier or literal.
  //
  assert(parameter->expr_type == AST_UNARY_EXPR);

  struct AST_UNARY_EXPR* unary_expr = parameter->types.unary_expr;

  if (strcmp(function_name, "print") == 0)
  {
    //
    if (unary_expr->unary_expr_type == AST_UNARY_EXPR_INT_LITERAL ||
      unary_expr->unary_expr_type == AST_UNARY_EXPR_REAL_LITERAL ||
      unary_expr->unary_expr_type == AST_UNARY_EXPR_STR_LITERAL)
    {  printf("%s\n", unary_expr->types.literal_value);
    }
    else if (unary_expr->unary_expr_type == AST_UNARY_EXPR_VARIABLE_ACCESS)
    { char* identifier = unary_expr->types.variable_name;

      struct RAM_CELL* memory_cell = ram_get_cell_by_id(memory, identifier);
      if (memory_cell == NULL)
      {
        printf("ERROR: name '%s' is not defined\n", identifier);
        return false;
      }
      else if (memory_cell->ram_cell_type == RAM_TYPE_INT)
        printf("%d\n", memory_cell->types.i);
      else if (memory_cell->ram_cell_type == RAM_TYPE_STR)
        printf("%s\n", memory_cell->types.s);
////#####2 adding ability to handle real liiteals
        else if (memory_cell->ram_cell_type == RAM_TYPE_REAL) // <-- New branch
          printf("%f\n", memory_cell->types.d);
          ////#####2
          
      else
        panic("unsupported variable type (execute_function_call)");
    }
    else {
      panic("unsupported print(unary_expr_type) (execute_function_call)");
    }
  }
  else {
    panic("unsupported function (execute_function_call)");
  }

  //
  // done, success:
  //
  return true;
}


// This function evaluates the condition of a while loop based on two unary expressions
// (usually the left-hand side (lhs) and right-hand side (rhs) of a comparison).
// It fetches the value of the lhs variable from memory and compares it against the rhs value.
// Currently, the function only supports the '!=' operator for comparison.
// Returns true if the condition is met, and false otherwise
////////helper #8
bool evaluate_while_condition(struct AST_UNARY_EXPR* lhs, struct AST_UNARY_EXPR* rhs, struct RAM* memory) {
    struct RAM_CELL* lhs_cell = ram_get_cell_by_id(memory, lhs->types.variable_name);
    if (!lhs_cell) {
        printf("Error: variable '%s' not found\n", lhs->types.variable_name);
        return false;
    }
  // If the rhs is an integer literal, compare the lhs variable's value with the rhs value.

    if (rhs->unary_expr_type == AST_UNARY_EXPR_INT_LITERAL) {
        int rhs_value = atoi(rhs->types.literal_value);
        if (lhs_cell->ram_cell_type != RAM_TYPE_INT) {
            printf("Error: unsupported types for operator !=\n");
            return false;
        }
        return lhs_cell->types.i != rhs_value;
    }
  // If the rhs is a real number literal, compare the lhs variable's value with the rhs value.

    if (rhs->unary_expr_type == AST_UNARY_EXPR_REAL_LITERAL) {
        double rhs_value = atof(rhs->types.literal_value);
        if (lhs_cell->ram_cell_type == RAM_TYPE_INT) {
            return (double)lhs_cell->types.i != rhs_value;
        } else if (lhs_cell->ram_cell_type == RAM_TYPE_REAL) {
            return lhs_cell->types.d != rhs_value;
        } else {
            printf("Error: unsupported types for operator !=\n");
            return false;
        }
    }

    if (rhs->unary_expr_type == AST_UNARY_EXPR_STR_LITERAL) {
        if (lhs_cell->ram_cell_type != RAM_TYPE_STR) {
            printf("Error: unsupported types for operator !=\n");
            return false;
        }
        return strcmp(lhs_cell->types.s, rhs->types.literal_value) != 0;
    }
  // If the type of the rhs isn't recognized, print an error and return false.

    printf("Error: unsupported condition in while loop\n");
    return false;
}



/////####7

//
// execute
//
// Executes the given nuPython program.
//
static void execute(struct AST_STMT* program, struct RAM* memory)
{
  //
  // loop through the program stmt by stmt:
  //
  struct AST_STMT* cur = program;

  while (cur != NULL)
  {
    bool success = false;

    // printf("stmt type %d\n", cur->stmt_type);

    if (cur->stmt_type == AST_ASSIGNMENT)
    {
      success = execute_assignment(cur, memory);
    }
    else if (cur->stmt_type == AST_DEREF_PTR_ASSIGNMENT)
    {
      panic("deref ptr assignment not yet supported (execute)");
    }
    else if (cur->stmt_type == AST_FUNCTION_CALL)
    {
      success = execute_function_call(cur, memory);
    }
    else if (cur->stmt_type == AST_IF_THEN_ELSE)
    {
      panic("if-then-else not yet supported (execute)");
    }
      ////
    else if (cur->stmt_type == AST_WHILE_LOOP) {
                struct AST_STMT_WHILE_LOOP* while_loop = cur->types.while_loop;

                if (while_loop->condition->expr_type != AST_BINARY_EXPR || 
                    while_loop->condition->types.binary_expr->op != AST_BINARY_EXPR_NOT_EQUAL) {
                    panic("Unsupported condition in while loop (execute)");
                }

                struct AST_BINARY_EXPR* bin_expr = while_loop->condition->types.binary_expr;
      

                while (evaluate_while_condition(bin_expr->lhs, bin_expr->rhs, memory)) {
                 execute(while_loop->body, memory);  // Execute the body of the loop
                }

                // Ensure we advance to the next statement after the while loop
                cur = cur->next;
                continue;  // Jump to the next iteration to avoid advancing `cur` twice
            }

            

      //####




      //####1 end
    else if (cur->stmt_type == AST_PASS)
    {
      //
      // pass => do nothing!
      //
    }
    else
    {
      panic("unknown statement type?! (execute)");
    }

    //
    // did execution fail?
    //
    if (!success)  // yes, end execution:
      break;

    //
    // if execution was successful, advance to 
    // next stmt:
    //
    cur = cur->next;
  }

  //
  // done:
  //
}


//
// main
//
int main(int argc, char* argv[])
{
  //
  // Ask the user for a filename, if they don't enter one
  // then we'll take input from the keyboard:
  //
  char filename[64];

  printf("Enter nuPython file (press ENTER to input from keyboard)>\n");

  fgets(filename, 64, stdin);  // safely read at most 64 chars
  filename[strcspn(filename, "\r\n")] = '\0';  // delete EOL chars e.g. \n

  FILE* input = NULL;
  bool  keyboardInput = false;

  if (strlen(filename) == 0) {
    //
    // input from the keyboard, aka stdin:
    //
    input = stdin;
    keyboardInput = true;
  }
  else {
    //
    // can we open the file?
    //
    input = fopen(filename, "r");

    if (input == NULL) // unable to open:
    {
      printf("**ERROR: unable to open input file '%s' for input.\n", filename);
      return 0;
    }

    keyboardInput = false;
  }

  //
  // input the tokens, either from keyboard or the given nuPython 
  // file; the "input" variable controls the source. the scanner will
  // stop and return EOS when the user enters $ or we reach EOF on
  // the nuPython file:
  //

  //
  // setup for parsing and execution:
  //
  if (keyboardInput)  // prompt the user if appropriate:
  {
    printf("nuPython input (enter $ when you're done)>\n");
  }

  parser_init();

  //
  // call parser to check program syntax:
  //
  struct TokenQueue* tokens = parser_parse(input);

  if (tokens == NULL)
  {
    // 
    // program has a syntax error, error msg already output:
    //
    printf("**parsing failed, cannot execute\n");
  }
  else
  {
    printf("**parsing successful\n");
    printf("**building AST...\n");

    struct AST_STMT* program = ast_build(tokens);

    printf("**starting execution...\n");

    //
    // if program is coming from the keyboard, consume
    // the rest of the input after the $ before we
    // start executing the python:
    //
    if (keyboardInput) {
      int c = fgetc(stdin);
      while (c != '\n')
        c = fgetc(stdin);
    }

    //
    // now execute the nuPython program:
    //
    struct RAM* memory = ram_init();

    execute(program, memory);

    ram_print(memory);
    // 1. Free the token queue:
    tokenqueue_destroy(tokens);

    // 2. Free the AST:
    ast_destroy(program);

    // 3. Free the RAM memory:
    ram_free(memory);
  }



  //
  // done:
  //
  if (!keyboardInput)
    fclose(input);

  return 0;
}
