import ast

with open("print_ast.py", "r") as f:
    print(ast.dump(ast.parse(f.read()), indent=2))