%{
#include <cstdio>
#include <string.h>
#include "SqlParser.h"
#include "QueryManager.h"
#include "../utils/pagedef.h"
# include "parser.tab.hpp"

int yyerror(const char *);
%}

DATE_FORMAT     '[0-9]{4}-[0-9]{1,2}-[0-9]{1,2}'
IDENTIFIER       [a-zA-Z][a-zA-Z0-9_]*
STRING_FORMAT   '[^']*'
INT_FORMAT      [0-9]+
FLOAT_FORMAT    (-?[0-9]+)(\.[0-9]+)?

%%

databases|DATABASES                 { return DATABASES; }
database|DATABASE                   { return DDATABASE; }
tables|TABLES                       { return TABLES; }
table|TABLE                         { return TTABLE; }
show|SHOW                           { return SHOW; }
create|CREATE                       { return CREATE; }
drop|DROP                           { return DROP; }
use|USE                             { return USE; }
primary|PRIMARY                     { return PRIMARY; }
key|KEY                             { return KEY; }
not|NOT                             { return NOT; }
null|NULL                           { return NNULL; }
insert|INSERT                       { return INSERT; }
into|INTO                           { return INTO; }
values|VALUES                       { return VALUES; }
delete|DELETE                       { return DELETE; }
from|FROM                           { return FROM; }
where|WHERE                         { return WHERE; }
update|UPDATE                       { return UPDATE; }
set|SET                             { return SET; }
select|SELECT                       { return SELECT; }
is|IS                               { return IS; }
int|INT                             { return IINT; }
varchar|VARCHAR                     { return VARCHAR; }
char|CHAR                           { return CCHAR; }
default|DEFAULT                     { return DEFAULT; }
constraint|CONSTRAINT               { return CONSTRAINT; }
change|CHANGE                       { return CHANGE; }
alter|ALTER                         { return ALTER; }
add|ADD                             { return ADD; }
rename|RENAME                       { return RENAME; }
desc|DESC                           { return DESC; }
index|INDEX                         { return INDEX; }
and|AND                             { return AND; }
date|DATE                           { return DDATE; }
float|FLOAT                         { return FFLOAT; }
foreign|FOREIGN                     { return FOREIGN; }
references|REFERENCES               { return REFERENCES; }
on|ON                               { return ON; }
to|TO                               { return TO; }
"="                                 { return EQ; }
">"                                 { return GT; }
"<"                                 { return LT; }
"!="                                { return NE; }
"<>"                                { return NE; }
">="                                { return GE; }
"<="                                { return LE; }
quit|QUIT                           { return QUIT; }
decimal|DECIMAL                     { return FFLOAT; }
integer|INTEGER                     { return IINT; }

unique|UNIQUE                       { return UNIQUE; }
copy|COPY                           { return COPY; }
with|WITH                           { return WITH; }
delimiter|DELIMITER                 { return DELIMITER; }
format|FORMAT                       { return FORMAT; }
distinct|DISTINCT                   { return DISTINCT; }
check|CHECK                         { return CHECK; }
or|OR                               { return OR; }
in|IN                               { return IN; }
like|LIKE                           { return LIKE; }
count|COUNT                         { return COUNT; }
sum|SUM                             { return SUM; }
min|MIN                             { return MIN; }
max|MAX                             { return MAX; }
avg|AVG                             { return AVG; }

{IDENTIFIER}                        { yylval.val_s = strdup(yytext); 
    for (int i = 0; i < strlen(yylval.val_s); i ++){
        if (yylval.val_s[i] >= 'A' && yylval.val_s[i] <= 'Z'){
            yylval.val_s[i] += 'a' - 'A';
        }
    } return IDENTIFIER; }
{DATE_FORMAT}                       { yylval.val_s = strndup(yytext + 1, strlen(yytext) - 2); return DATE_FORMAT; }
{STRING_FORMAT}                     { yylval.val_s = strndup(yytext + 1, strlen(yytext) - 2); return STRING_FORMAT; }
{INT_FORMAT}                        { yylval.val_i = atoi(yytext); return INT_FORMAT; }
{FLOAT_FORMAT}                      { yylval.val_f = atof(yytext); return FLOAT_FORMAT; }
[ \t\r\n]+                          { /* ignore */ }
.                                   { return(yytext[0]); }
%%

int yywrap() {
    return 1;
}
