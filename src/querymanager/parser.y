%{
#include <cstdio>
#include <stdlib.h>
#include "parser.yy.cpp"

int yylex();
int yyerror(const char *);
%}

%union {
    int val_i;
    float val_f;
    char *val_s;
    Node *node;

    FieldList *__fieldList;
    Field* __field;
    Type* __type;
    ValueLists* __valueLists;
    ValueList* __valueList;
    Expression* expression;
    WhereClause* whereClause;
    LogicalAnd* logicalAnd;
    SetList* setList;
    IdentList* identList;
}

/* keyword */
%token QUIT
%token SELECT DELETE UPDATE INSERT
%token CREATE DROP USE SHOW ALTER
%token DATABASES DDATABASE TABLES TTABLE
%token FROM WHERE VALUES SET INTO ADD CHANGE KEY NOT ON TO RENAME

/* COLUMN DESCPRITION */
%token FFLOAT VARCHAR DDATE PRIMARY FOREIGN REFERENCES CONSTRAINT DEFAULT IINT

/* number */
%token <val_i> INT_FORMAT
%token <val_f> FLOAT_FORMAT
%token <val_s> STRING_FORMAT IDENTIFIER DATE_FORMAT

/* operator */
%token EQ GT LT GE LE NE

/* aggretation */
%token AVG SUM MIN MAX COUNT

%token DESC GROUP LIKE INDEX CHECK IN NNULL IS AND OR

%type <node> stmt
%type <__fieldList> fieldList
%type <__field> field
%type <__type> type
%type <__valueLists> valueLists
%type <__valueList> valueList selector
%type <expression> value relational expression col
%type <whereClause> whereClause
%type <logicalAnd> logicalAnd
%type <setList> setClause
%type <identList> tableList columnList

%start program

%%
program:
    stmt ';'
    | program stmt ';'
    ;

stmt:
        
    | QUIT
        {
            $$ = new Quit();
            Node::setInstance($$);
            Node::execute();
        }



    | SHOW DATABASES
        {
            $$ = new ShowDatabases();
            Node::setInstance($$);
            Node::execute();
        }



    | CREATE DDATABASE IDENTIFIER
        {
            $$ = new CreateDatabase($3);
            Node::setInstance($$);
            Node::execute();
        }
    | DROP DDATABASE IDENTIFIER
        {
            $$ = new DropDatabase($3);
            Node::setInstance($$);
            Node::execute();
        }
    | USE IDENTIFIER
        {
            $$ = new UseDatabase($2);
            Node::setInstance($$);
            Node::execute();
        }
    | SHOW TABLES
        {
            $$ = new ShowTables();
            Node::setInstance($$);
            Node::execute();
        } 



    | CREATE TTABLE IDENTIFIER '(' fieldList ')'
        {
            $$ = new CreateTable($3, $5);
            Node::setInstance($$);
            Node::execute();
        }
    | DROP TTABLE IDENTIFIER
        {
            $$ = new DropTable($3);
            Node::setInstance($$);
            Node::execute();
        }
    | DESC IDENTIFIER
        {
            $$ = new DescTable($2);
            Node::setInstance($$);
            Node::execute();
        }
    | INSERT INTO IDENTIFIER VALUES valueLists
        {
            $$ = new Insert($3, $5);
            Node::setInstance($$);
            Node::execute();
        }
    | DELETE FROM IDENTIFIER WHERE whereClause
        {
            $$ = new Delete($3, $5);
            Node::setInstance($$);
            Node::execute();
        }
    | UPDATE IDENTIFIER SET setClause WHERE whereClause
        {
            $$ = new Update($2, $4, $6);
            Node::setInstance($$);
            Node::execute();
        }
    | SELECT selector FROM tableList WHERE whereClause
        {
            $$ = new Select($2, $4, $6);
            Node::setInstance($$);
            Node::execute();
        }
    | SELECT selector FROM tableList
        {
            $$ = new Select($2, $4, nullptr);
            Node::setInstance($$);
            Node::execute();
        }

    

    | CREATE INDEX IDENTIFIER ON IDENTIFIER '(' columnList ')'
        {
            $$ = new AddIndex($3, $5, $7);
            Node::setInstance($$);
            Node::execute();
        }
    | ALTER TTABLE IDENTIFIER ADD INDEX IDENTIFIER '(' columnList ')'
        {
            $$ = new AddIndex($3, $6, $8);
            Node::setInstance($$);
            Node::execute();
        }
    | ALTER TTABLE IDENTIFIER DROP INDEX IDENTIFIER
        {
            $$ = new DropIndex($3, $6);
            Node::setInstance($$);
            Node::execute();
        }
    | ALTER TTABLE IDENTIFIER ADD field
        {
            $$ = new AddColumn($3, $5);
            Node::setInstance($$);
            Node::execute();
        }
	| ALTER TTABLE IDENTIFIER DROP IDENTIFIER
        {
            $$ = new DropColumn($3, $5);
            Node::setInstance($$);
            Node::execute();
        }
	| ALTER TTABLE IDENTIFIER CHANGE IDENTIFIER field
        {
            $$ = new ChangeColumn($3, $5, $6);
            Node::setInstance($$);
            Node::execute();
        }
	| ALTER TTABLE IDENTIFIER RENAME TO IDENTIFIER
        {
            $$ = new RenameTable($3, $6);
            Node::setInstance($$);
            Node::execute();
        }
	| ALTER TTABLE IDENTIFIER DROP PRIMARY KEY
        {
            $$ = new DropPri($3);
            Node::setInstance($$);
            Node::execute();
        }
	| ALTER TTABLE IDENTIFIER ADD CONSTRAINT IDENTIFIER PRIMARY KEY '(' columnList ')'
        {
            $$ = new AddPri($3, $6, $10);
            Node::setInstance($$);
            Node::execute();
        }
    | ALTER TTABLE IDENTIFIER DROP PRIMARY KEY IDENTIFIER
        {
            $$ = new DropPri($3);
            Node::setInstance($$);
            Node::execute();
        }
	| ALTER TTABLE IDENTIFIER ADD CONSTRAINT IDENTIFIER FOREIGN KEY '(' columnList ')' REFERENCES IDENTIFIER '(' columnList ')'
        {
            $$ = new AddForeign($3, $6, $10, $13, $15);
            Node::setInstance($$);
            Node::execute();
        }
	| ALTER TTABLE IDENTIFIER DROP FOREIGN KEY IDENTIFIER
        {
            $$ = new DropIndex($3, $7);
            Node::setInstance($$);
            Node::execute();
        }
    ;

fieldList : 
    field
        {
            $$ = new FieldList();
            $$->addField($1);
        }
    | fieldList ',' field
        {
            $$ = $1;
            $$->addField($3);
        }
    ;

field  : 
    IDENTIFIER type
        {
            $$ = new ColField($1, $2);
        }
    | IDENTIFIER type NOT NNULL
        {
            $$ = new ColField($1, $2, true);
        }
    | IDENTIFIER type DEFAULT value
        {
            $$ = new ColField($1, $2, false, $4);
        }
    | IDENTIFIER type NOT NNULL DEFAULT value
        {
            $$ = new ColField($1, $2, true, $6);
        }
    | PRIMARY KEY '(' columnList ')'
        {
            $$ = new PrimaryField($4);
        }
    | FOREIGN KEY '(' IDENTIFIER ')' REFERENCES IDENTIFIER '(' IDENTIFIER ')'
        {
            $$ = new ForeignField($4, $7, $9);
        }
    ;

type  : 
    IINT
        {
            $$ = new Type(AttrType::INTEGER);
        }
    | IINT '(' INT_FORMAT ')'
        {
            $$ = new Type(AttrType::INTEGER, $3);
        }
    | VARCHAR '(' INT_FORMAT ')'
        {
            $$ = new Type(AttrType::STRING, $3);
        }
    | DDATE
        {
            $$ = new Type(AttrType::DATE);
        }
    | FFLOAT
        {
            $$ = new Type(AttrType::FLOAT);
        }
    ;



valueLists:
    '(' valueList ')'
        {
            $$ = new ValueLists();
            $$->addValueList($2);
        }
    | valueLists ',' '(' valueList ')'
        {
            $$ = $1;
            $$->addValueList($4);
        }
    ;

valueList:
    value
        {
            $$ = new ValueList();
            $$->addValue($1);
        }
    | valueList ',' value
        {
            $$ = $1;
            $$->addValue($3);
        }
    ;

value:
    INT_FORMAT  { $$ = new Primary($1); }
    | FLOAT_FORMAT  { $$ = new Primary($1); }
    | STRING_FORMAT  { $$ = new Primary($1, AttrType::STRING); }
    | DATE_FORMAT  { $$ = new Primary($1, AttrType::DATE); }
    | NNULL { $$ = new Primary(nullptr, AttrType::NO_TYPE); }
    ;

whereClause:
    logicalAnd
        {
            $$ = new WhereClause($1);
        }
    | whereClause OR logicalAnd
        {
            $$ = $1;
            $$->addLogicalAnd($3);
        }
    ;

logicalAnd:
    relational
        {
            $$ = new LogicalAnd($1);
        }
    | logicalAnd AND relational
        {
            $$ = $1;
            $$->addRelational($3);
        }
    ;

relational:
    col EQ expression
        {
            $$ = new Binary($1, CompOp::EQ_OP, $3);
        }
    | col GT expression
        {
            $$ = new Binary($1, CompOp::GT_OP, $3);
        }
    | col LT expression
        {
            $$ = new Binary($1, CompOp::LT_OP, $3);
        }
    | col GE expression
        {
            $$ = new Binary($1, CompOp::GE_OP, $3);
        }
    | col LE expression
        {
            $$ = new Binary($1, CompOp::LE_OP, $3);
        }
    | col NE expression
        {
            $$ = new Binary($1, CompOp::NE_OP, $3);
        }
    | col IS NNULL
        {
            $$ = new Unary($1, CompOp::IS_NULL);
        }
    | col IS NOT NNULL
        {
            $$ = new Unary($1, CompOp::NOT_NULL);
        }
    ;

expression: 
    col
        {
            $$ = $1;
        }
    | value
        {
            $$ = $1;
        }
    ;

col:
    IDENTIFIER
        {
            $$ = new Col($1);
        }
    | IDENTIFIER '.' IDENTIFIER
        {
            $$ = new Col($1, $3);
        }
    ;

setClause:
    IDENTIFIER EQ value
        {
            $$ = new SetList();
            $$->addSetClause($1, $3);
        }
    | setClause ',' IDENTIFIER EQ value
        {
            $$ = $1;
            $$->addSetClause($3, $5);
        }
    ;

selector:
    '*'
        {
            $$ = nullptr;
        }
    | col
        {
            $$ = new ValueList();
            $$->addValue($1);
        }
    | selector ',' col
        {
            $$ = $1;
            $$->addValue($3);
        }
    ;

tableList:
    IDENTIFIER
            {
                $$ = new IdentList();
                $$->addIdent($1);
            }
    | tableList ',' IDENTIFIER
            {
                $$ = $1;
                $$->addIdent($3);
            }
    ;

columnList:
    IDENTIFIER
            {
                $$ = new IdentList();
                $$->addIdent($1);
            }
    | columnList ',' IDENTIFIER
            {
                $$ = $1;
                $$->addIdent($3);
            }
    ;
%%

int yyerror(const char *msg) {
    fprintf(stderr, "error: %s\n", msg);
    return 1;
}

char start_parse(const char *expr_input)
{
    char ret;
    if(expr_input){
        YY_BUFFER_STATE my_string_buffer = yy_scan_string(expr_input);
        yy_switch_to_buffer( my_string_buffer ); // switch flex to the buffer we just created
        ret = yyparse();
        yy_delete_buffer(my_string_buffer );
    }else{
        ret = yyparse();
    }
    return ret;
}

