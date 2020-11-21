# include "systemmanager/Table.h"
# include <cstring>
# include <iostream>

int main(){
    char name[MAX_NAME_LEN];
    memcpy(name, "table", 6);
    Table* table = new Table("db", name);
    AttrType types[2];
    types[0] = INTEGER;
    types[1] = STRING;
    int colLens[2];
    colLens[0] = 5;
    colLens[1] = 10;
    char names[2][MAX_NAME_LEN];
    memcpy(names[0], "act", 4);
    memcpy(names[1], "movie", 6);

    table ->createTable(types, colLens, names, 2);
    table ->addColumn(INTEGER, 5, "oh");
    table -> closeTable();
    table->openTable();
    return 0;
}