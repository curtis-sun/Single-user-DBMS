%{
#include <cstdio>
#include <stdlib.h>
#include "parser.yy.cpp"
# define YYDEBUG 1

int yylex();
int yyerror(const char *);
%}

%union {
    int val_i;
    float val_f;
    char *val_s;
    Node *node;
    Select* select;

    FieldList *__fieldList;
    Field* __field;
    Type* __type;
    ValueLists* __valueLists;
    ValueList* __valueList;
    Expression* expression;
    WhereClause* whereClause;
    LogicalAnd* logicalAnd;
    Relational* relational;
    SetList* setList;
    IdentList* identList;
    Selector* selector;
    Agrg* aggregate;
}

/* keyword */
%token QUIT
%token SELECT DELETE UPDATE INSERT COPY
%token CREATE DROP USE SHOW ALTER
%token DATABASES DDATABASE TABLES TTABLE
%token FROM WHERE VALUES SET INTO ADD CHANGE KEY NOT ON TO RENAME UNIQUE WITH DELIMITER FORMAT PRIMARY FOREIGN REFERENCES CONSTRAINT DEFAULT DISTINCT

/* COLUMN DESCPRITION */
%token FFLOAT CCHAR VARCHAR DDATE IINT

/* number */
%token <val_i> INT_FORMAT
%token <val_f> FLOAT_FORMAT
%token <val_s> STRING_FORMAT IDENTIFIER DATE_FORMAT

/* operator */
%token EQ GT LT GE LE NE

/* aggretation */
%token AVG SUM MIN MAX COUNT

%token DESC LIKE INDEX CHECK IN NNULL IS AND OR

%type <node> stmt
%type <__fieldList> fieldList
%type <__field> field
%type <__type> type
%type <__valueLists> valueLists
%type <__valueList> valueList colList
%type <expression> expression multiplicative primary value col
%type <whereClause> whereClause
%type <logicalAnd> logicalAnd
%type <relational> relational
%type <setList> setClause
%type <identList> tableList columnList
%type <selector> selector
%type <aggregate> aggregate
%type <select> select

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
            YYACCEPT;
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
            YYACCEPT;
        }
    | COPY IDENTIFIER FROM STRING_FORMAT WITH '(' FORMAT STRING_FORMAT ',' DELIMITER STRING_FORMAT ')'
        {
            $$ = new Copy($2, $4, $8, $11);
            Node::setInstance($$);
            Node::execute();
            YYACCEPT;
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
    | select
        {
            $$ = $1;
            Node::setInstance($$);
            Node::execute();
        }


    | CREATE INDEX IDENTIFIER ON IDENTIFIER '(' columnList ')'
        {
            $$ = new AddIndex($3, $5, $7);
            Node::setInstance($$);
            Node::execute();
        }
    | CREATE UNIQUE INDEX IDENTIFIER ON IDENTIFIER '(' columnList ')'
        {
            $$ = new AddIndex($4, $6, $8, "unique");
            Node::setInstance($$);
            Node::execute();
        }
    | ALTER TTABLE IDENTIFIER ADD INDEX IDENTIFIER '(' columnList ')'
        {
            $$ = new AddIndex($3, $6, $8);
            Node::setInstance($$);
            Node::execute();
        }
    | ALTER TTABLE IDENTIFIER ADD UNIQUE INDEX IDENTIFIER '(' columnList ')'
        {
            $$ = new AddIndex($3, $7, $9, "unique");
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

select :
    SELECT selector FROM tableList WHERE whereClause
        {
            $$ = new Select($2, $4, $6);
        }
    | SELECT selector FROM tableList
        {
            $$ = new Select($2, $4);
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
    | CCHAR '(' INT_FORMAT ')'
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

colList:
    col
        {
            $$ = new ValueList();
            $$->addValue($1);
        }
    | colList ',' col
        {
            $$ = $1;
            $$->addValue($3);
        }
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
            $$ = new Relational(new Binary($1, CalcOp::EQ_OP, $3), nullptr);
        }
    | col GT expression
        {
            $$ = new Relational(new Binary($1, CalcOp::GT_OP, $3), nullptr);
        }
    | col LT expression
        {
            $$ = new Relational(new Binary($1, CalcOp::LT_OP, $3), nullptr);
        }
    | col GE expression
        {
            $$ = new Relational(new Binary($1, CalcOp::GE_OP, $3), nullptr);
        }
    | col LE expression
        {
            $$ = new Relational(new Binary($1, CalcOp::LE_OP, $3), nullptr);
        }
    | col NE expression
        {
            $$ = new Relational(new Binary($1, CalcOp::NE_OP, $3), nullptr);
        }
    | col IS NNULL
        {
            $$ = new Relational(nullptr, new Unary($1, CalcOp::IS_NULL));
        }
    | col IS NOT NNULL
        {
            $$ = new Relational(nullptr, new Unary($1, CalcOp::NOT_NULL));
        }
    | col LIKE expression
        {
            $$ = new Relational(new Binary($1, CalcOp::LIKE_OP, $3), nullptr);
        }
    | col IN '(' valueList ')'
        {
            $$ = new Relational(nullptr, new InExpr($1, $4));
        }
    | col IN '(' select ')'
        {
            $$ = new Relational(nullptr, new InExpr($1, $4));
        }
    ;

expression:
    multiplicative
        {
            $$ = $1;
        }
    | expression '+' multiplicative
        {
            $$ = new Binary($1, CalcOp::ADD_OP, $3);
        }
    | expression '-' multiplicative
        {
            $$ = new Binary($1, CalcOp::SUB_OP, $3);
        }
    ;

multiplicative:
    primary
        {
            $$ = $1;
        }
    | multiplicative '*' primary
        {
            $$ = new Binary($1, CalcOp::MUL_OP, $3);
        }
    | multiplicative '/' primary
        {
            $$ = new Binary($1, CalcOp::DIV_OP, $3);
        }
    ; 

primary:
    col
        {
            $$ = $1;
        }
    | value
        {
            $$ = $1;
        }
    | '(' expression ')'
        {
            $$ = $2;
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
    IDENTIFIER EQ expression
        {
            $$ = new SetList();
            $$->addSetClause($1, $3);
        }
    | setClause ',' IDENTIFIER EQ expression
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
    | expression
        {
            $$ = new Selector();
            $$->addValue($1);
        }
    | aggregate
        {
            $$ = new Selector();
            $$->addAgrg($1);
        }
    | selector ',' expression
        {
            $$ = $1;
            $$->addValue($3);
        }
    | selector ',' aggregate
        {
            $$ = $1;
            $$->addAgrg($3);
        }
    ;

aggregate:
    COUNT '(' '*' ')'
        {
            $$ = new AgrgCount(nullptr);
        }
    | COUNT '(' DISTINCT colList ')'
        {
            $$ = new AgrgCount($4);
        }
    | SUM '(' col ')'
        {
            $$ = new AgrgSum($3);
        }
    | AVG '(' col ')'
        {
            $$ = new AgrgAvg($3);
        }
    | MIN '(' col ')'
        {
            $$ = new AgrgMin($3);
        }
    | MAX '(' col ')'
        {
            $$ = new AgrgMax($3);
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
        yy_delete_buffer(my_string_buffer);
        yylex_destroy();
    }else{
        ret = yyparse();
    }
    return ret;
}

