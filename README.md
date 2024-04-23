# Expression grammar

```
primary ->
      STRING
    | NUMBER
    | IDENTIFIER
    | 'true'
    | 'false'
    | 'nil'
    | '(' expression ')'

unary ->
      ('-' | '+') unary
    | primary

multiply ->
      unary (('/' | '*') unary)*

add ->
      multiply (('-' | '+') multiply)*

compare ->
      add (('==' | '!=' | '<' | '>' | '<=' | '>=') add)?

expression ->
      compare
```