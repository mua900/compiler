#pragma once

#include "stmt.hpp"
#include "log.hpp"

#include "expr.hpp"
#include "common.hpp"
#include "type.hpp"

#include "scope.hpp"

struct Environment {
  Environment() = default;
  Environment(int parent) : parent_index(parent) {}

  int parent_index = 0;    // parent environment

  DArray<String> variable_names;
  DArray<Variable> variables;
  int var_id = 1;

  DArray<String> procedure_names;
  DArray<Procedure> procedures;
  int proc_id = 1;

  // type_definitions
  DArray<String> type_names;
  DArray<Structure> types;
  int struct_id = 1;

  int bind_variable(String name, Variable var) {
    var.var_id = var_id;

    variable_names.add(name);
    variables.add(var);
    var_id++;
    return var_id - 1;
  }

  int bind_procedure(String name, Procedure proc) {
    proc.proc_id = proc_id;

    procedure_names.add(name);
    procedures.add(proc);
    proc_id++;
    return proc_id - 1;
  }

  // @xxx do we need to return pointers here?
  const Variable* get_variable(String name) const {
    int index = variable_names.find(name, compare_string);
    return (index == -1) ? NULL : &variables.data[index];
  }

  const Procedure* get_procedure(String name) const {
    int index = procedure_names.find(name, compare_string);
    return (index == -1) ? NULL : &procedures.data[index];
  }

  Procedure get_proc_from_id(int id) const {
    return procedures.data[id - 1];
  }

  Variable get_var_from_id(int id) const {
    return variables.data[id - 1];
  }

  void dump() {
    String_Builder sb(1024);

    if (variable_names.size != 0) {
      printf("variables: \n");
      for (auto var_name : variable_names) {
        sb.clear_and_append("\t");
        sb.append(var_name);
        printf("%s\n", sb.c_string());
      }
    }

    if (procedure_names.size != 0) {
      printf("procedures:\n");
      for (auto proc_name : procedure_names) {
        sb.clear_and_append("\t");
        sb.append(proc_name);
        printf("%s\n", sb.c_string());
      }
    }
  }
};
